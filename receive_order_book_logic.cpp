#include <iostream>
#include <cstring>
#include <string>

#include "socket/socket.hpp"
#include "diff_logic.hpp"
#include "frame.hpp"
#include "orders.hpp"
#include "receive_order_book_logic.hpp"
#include "order_book.hpp"
#include "json/json.h"

//------

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

//------

#define JSON_BUF_INIT_VALUE 5000
#define HTTP_DELIMITER "\r\n\r\n"

ReceiveOrderBookLogic::ReceiveOrderBookLogic(const Url & url,
                                             OrderBook & order_book) : m_order_book(order_book)
{
    m_request  = "GET "   + url.url()  + " HTTP/1.1\r\n";
    m_request += "Host: " + url.host() + "\r\n\r\n";

    m_buffer.resize(JSON_BUF_INIT_VALUE);
}

static uint64_t get_body_size(const char * header)
{
    static const std::string CONTENT_LENGTH("content-length: ");
    static const size_t CONTENT_LENGTH_SIZE = CONTENT_LENGTH.size();

    auto content_length_ptr = strcasestr(header, CONTENT_LENGTH.c_str());
    if (!content_length_ptr)
        throw std::runtime_error("no content-length");

    return std::stoi(content_length_ptr + CONTENT_LENGTH_SIZE);
}

bool ReceiveOrderBookLogic::process_header()
{
    auto delim = strstr(&m_buffer[0], HTTP_DELIMITER);
    if (!delim)
    {
        if (m_read_offset == m_buffer.size())
        {
            decltype(m_buffer) tmp;
            tmp.resize(m_buffer.size() * 2);
            ::memcpy(&tmp[0], &m_buffer[0], m_read_offset);
            tmp.swap(m_buffer);
        }

        return false;
    }

    *delim = 0;

    uint64_t body_size = get_body_size(&m_buffer[0]);
    if (!body_size)
        throw std::runtime_error("invalid content_length value");

    m_json_full_size = body_size;

    delim += strlen(HTTP_DELIMITER);

    m_read_offset -= delim - &m_buffer[0];

    if (m_buffer.size() < body_size)
    {
        decltype(m_buffer) tmp;
        tmp.resize(body_size);
        ::memcpy(&tmp[0], delim, m_read_offset);
        tmp.swap(m_buffer);
    }
    else
    {
        ::memmove(&m_buffer[0], delim, m_read_offset);
    }

    m_ready_to_receive_json = true;

    return true;
}

void ReceiveOrderBookLogic::process_json()
{
    Json::Value root;
    Json::Reader reader;
    bool success = reader.parse(&m_buffer[0], &m_buffer[0] + m_json_full_size, root);
    if (!success)
    {
        std::cerr << "invalid json " << &m_buffer[0] << std::endl;
        assert(false);
    }

    auto json_timestamp = root["timestamp"];
    if (!json_timestamp)
        throw std::runtime_error("no timestamp in json");

    auto timestamp = std::stoll(json_timestamp.asString());

    auto store_orders = [&](uint64_t type, Json::Value & orders)
    {
        uint64_t price  = 0;
        uint64_t amount = 0;
        for (const auto level : orders)
        {
            price  = std::stod(level[0].asString()) * 1000000; // XXX double to uint64_t
            amount = std::stod(level[1].asString()) * 1000000; // TODO rm
            m_order_book.add(Order(type, timestamp, price, amount));
        }
    };

    auto bids = root["bids"];
    store_orders(Order::BUY, bids);
    auto asks = root["asks"];
    store_orders(Order::SELL, asks);
}

void ReceiveOrderBookLogic::on_read(Socket *socket)
{
    if (!m_wait_for_read)
        return;

    if (!m_ssl_connected)
    {
        ssl_connect();
        return;
    }

    int64_t  readed = 0;
    uint64_t buffer_size = m_buffer.size();

    while ((readed = SSL_read(m_ssl,
                              &m_buffer[0] + m_read_offset,
                              buffer_size  - m_read_offset)) > 0)
    {
        m_read_offset += readed;

        if (!m_ready_to_receive_json && !process_header())
            continue;

        if (m_read_offset == m_json_full_size)
            break;
    }

    if (m_read_offset == m_buffer.size())
    {
        process_json();
        socket->close();
    }
}

bool ReceiveOrderBookLogic::ssl_connect()
{
    int ret = 0;
    if ((ret = SSL_connect(m_ssl)) == -1)
    {
        switch (SSL_get_error(m_ssl, ret))
        {
            case SSL_ERROR_WANT_READ:
                m_wait_for_read = true;
                break;

            case SSL_ERROR_WANT_WRITE:
                m_wait_for_read = false;
                break;

            default:
                assert(false);
        }

       return false;
    }

    std::cerr << "SSL CONNECTED" << std::endl;

    m_ssl_connected = true;
    m_wait_for_read = false;
    return true;
}

void ReceiveOrderBookLogic::on_write(Socket *socket)
{
    if (m_wait_for_read)
        return;

    if (!m_ssl)
    {
        auto method = SSLv23_client_method();
        m_ssl_ctx = SSL_CTX_new(method);
        if ( m_ssl_ctx == NULL )
        {
            ERR_print_errors_fp(stderr);
            assert(false);
        }

        m_ssl = SSL_new(m_ssl_ctx);
        SSL_set_fd(m_ssl, socket->get_fd());
    }

    if (!m_ssl_connected && !ssl_connect())
        return;

    int64_t writed = 0;
    while ((writed = SSL_write(m_ssl, m_request.c_str() + m_write_offset, m_request.size() - m_write_offset)))
    {
        if (writed < 0)
        {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("write error");
        }

        m_write_offset += writed;
    }

    if (m_write_offset >= m_request.size())
        m_wait_for_read = true;
}

void ReceiveOrderBookLogic::on_close(Socket *socket, const uint64_t &)
{
    if (m_ssl)
    {
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }

    if (m_ssl_ctx)
    {
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
    }

    CRYPTO_cleanup_all_ex_data();
}
