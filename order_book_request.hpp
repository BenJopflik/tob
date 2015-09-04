#pragma once
#include <deque>
#include <vector>

#include "socket/socket_callbacks.hpp"
#include "common/url.hpp"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

class Orders;
class OrderBook;

class OrderBookRequest: public SocketCallbacks
{
public:
    OrderBookRequest(const Url & url,
                     Orders & orders,
                     OrderBook & order_book) : m_url(url),
                                               m_orders(orders),
                                               m_order_book(order_book) {}
private:

    virtual void on_read(Socket *) override;
    virtual void on_write(Socket *) override;
    virtual void on_error(Socket *) override;

    virtual void on_close(Socket *, int64_t fd) override;
    virtual void on_connected(Socket *) override;
    virtual void on_rearm(Socket *) override;
    uint64_t process_header(const char * header, uint64_t size);
    bool ssl_connect();

private:
    Url      m_url;
    char     m_buffer[50000];
    uint64_t m_offset {0};
    uint64_t m_size {0};
    Orders & m_orders;
    OrderBook & m_order_book;

    std::vector<char> m_full_json;

//    SSL
    SSL_CTX * m_ssl_ctx {nullptr};
    SSL * m_ssl {nullptr};
    bool m_ssl_connected {false};
    bool m_wait_for_read {true};
};
