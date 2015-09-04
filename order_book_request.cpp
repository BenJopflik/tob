#include <iostream>
#include <cstring>
#include <string>

#include "socket/socket.hpp"
#include "diff_logic.hpp"
#include "frame.hpp"
#include "orders.hpp"
#include "order_book_request.hpp"
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

uint64_t OrderBookRequest::process_header(const char * header, uint64_t size)
{
    auto content_length_ptr = strcasestr(header, "content-length: ");
    if (!content_length_ptr)
        throw std::runtime_error("no content-length");

    return std::stoi(content_length_ptr + 16);
}

void OrderBookRequest::on_read(Socket *socket)
{
    if (!m_ssl_connected)
    {
        ssl_connect();
        return;
    }

    bool eof = false;
    int64_t readed = 0;
    char * buffer = (m_full_json.size() ? &m_full_json[0] : m_buffer);
    uint64_t buffer_size = m_full_json.size() ? m_full_json.size() - 1 : 50000;
    while ((readed = SSL_read(m_ssl, buffer + m_offset, buffer_size - m_offset)) > 0)
    {
        m_size += readed;
        m_offset += readed;

        if (m_full_json.empty())
        {
            auto delim = strstr(m_buffer, "\r\n\r\n");
            if (!delim || delim - m_buffer > m_offset)
                continue;

            *delim = 0;

            uint64_t body_size = process_header(m_buffer, delim - m_buffer);
            delim += 4;
            m_full_json.resize(body_size + 1);
            memset(&m_full_json[0], 0, body_size + 1);
            buffer = &m_full_json[0];
            m_offset = m_size - (delim - m_buffer);
            memcpy(buffer, delim, m_offset);
            buffer_size = body_size;
        }

        if (m_offset == buffer_size)
            break;
    }

    if (m_offset == buffer_size)
    {
        Json::Value root;
        Json::Reader reader;
        bool parsingSuccessful = reader.parse(&m_full_json[0], &m_full_json[0] + buffer_size, root );
        if (!parsingSuccessful)
        {
            std::cerr << "invalid json" << std::endl;
            assert(false);
        }

        auto json_timestamp = root["timestamp"];
        if (!json_timestamp)
            throw std::runtime_error("no timestamp in json");

        auto timestamp = std::stoll(json_timestamp.asString());
        auto first_timestamp = m_orders.get_first_timestamp();
        std::cerr << "TIMESTAMP " << timestamp << " / " << first_timestamp << std::endl;
        if (timestamp < m_orders.get_first_timestamp())
            throw std::runtime_error("order_book is too old");

        m_orders.set_order_book_timestamp(timestamp);

        auto store_orders = [&](uint64_t type, Json::Value & orders)
        {
            uint64_t price = 0;
            uint64_t amount = 0;
            for (const auto level : orders)
            {
                price = std::stod(level[0].asString()) * 1000000; // XXX double to uint64_t
                amount = std::stod(level[1].asString()) * 1000000; // TODO rm
                m_order_book.add(Order(type, timestamp, price, amount));
            }
        };

        auto bids = root["bids"];
        store_orders(Order::BUY, bids);
        auto asks = root["asks"];
        store_orders(Order::SELL, asks);
    }
}

bool OrderBookRequest::ssl_connect()
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
                throw std::runtime_error("unexpected ssl error");
        }
        return false;
    }
    m_ssl_connected = true;
    m_wait_for_read = false;
    return true;
}

void OrderBookRequest::on_write(Socket *socket)
{
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

    std::string request;
    request = "GET " + m_url.url() + " HTTP/1.1\r\n";
    request += "Host: " + m_url.host() + "\r\n\r\n";

    if (SSL_write(m_ssl, request.c_str(), request.size()) <= 0)
        ERR_print_errors_fp(stderr);
    m_wait_for_read = true;
}

void OrderBookRequest::on_error(Socket * socket)
{
}

void OrderBookRequest::on_close(Socket *socket, int64_t fd)
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

void OrderBookRequest::on_connected(Socket * socket)
{
}

void OrderBookRequest::on_rearm(Socket * socket)
{
    socket->add_to_poller(m_wait_for_read ? Action::READ : Action::WRITE);
}
