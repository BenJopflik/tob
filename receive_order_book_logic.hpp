#pragma once
#include <deque>
#include <vector>

#include "socket/socket_callbacks.hpp"
#include "common/url.hpp"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

class OrderBook;

class ReceiveOrderBookLogic: public SocketCallbacks
{
public:
    ReceiveOrderBookLogic(const Url & url,
                          OrderBook & order_book);

private:
    virtual void on_read (Socket *) override;
    virtual void on_write(Socket *) override;
    virtual void on_close(Socket *, const uint64_t &) override;

    bool process_header();
    void process_json();

    bool ssl_connect();

private:
    std::vector<char> m_buffer;
    uint64_t m_buffer_size    {0};
    uint64_t m_read_offset    {0};
    uint64_t m_json_full_size {0};

    bool m_ready_to_receive_json {false};

    std::string m_request;
    uint64_t m_write_offset {0};

    OrderBook & m_order_book;

    bool m_wait_for_read {false};

//    SSL
    SSL_CTX * m_ssl_ctx {nullptr};
    SSL     * m_ssl     {nullptr};

    bool m_ssl_connected {false};
};
