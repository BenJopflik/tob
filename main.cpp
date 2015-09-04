#include <utility>
#include <deque>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "socket/tcp/client.hpp"
#include "poller/poller.hpp"
#include "common/url.hpp"
#include "common/to_string.hpp"
#include "memory/pool.hpp"
#include "orders.hpp"

#include "diff_logic.hpp"
#include "receive_order_book_logic.hpp"
#include "order_book.hpp"
#include "common/timer.hpp"
#include "common/ip_addr.hpp"

const uint64_t NUMBER_OF_WORKERS = 1;


static void ssl_init()
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

static void ssl_destroy()
{
    ERR_free_strings();
    EVP_cleanup();
}

std::shared_ptr<Socket> create_tcp_socket(const std::string & host, const uint16_t & port)
{
    Tcp::Params client_params;
    IpAddr ip_addr(host);

    client_params.ip    = ip_addr.ip;
    client_params.port  = port;
    client_params.flags = Tcp::Params::NON_BLOCK | Tcp::Params::REUSE_ADDR | Tcp::Params::REUSE_PORT;
    client_params.read_buffer  = 65000;
    client_params.write_buffer = 65000;

    auto socket = TcpClient::create(client_params);

    return socket;
}

int main(int argc, char *argv[])
{
    ssl_init();
    OrderBook order_book(3);
    Orders<1000> orders;
    auto poller = Poller::create();

    Url diff_url("wss://ws.pusherapp.com/app/de504dc5763aeef9ff52?protocol=5");
    auto diff_socket = create_tcp_socket(diff_url.host(), 80);
    diff_socket->set_callbacks<DiffLogic<1000>>(std::ref(diff_url), std::ref(orders));
    poller->update_socket(diff_socket, EPOLLIN | EPOLLOUT);

    Url order_book_url("https://www.bitstamp.net/api/order_book");
    auto order_book_socket = create_tcp_socket(order_book_url.host(), 443);
    order_book_socket->set_callbacks<ReceiveOrderBookLogic>(std::ref(order_book_url), std::ref(order_book));
    poller->update_socket(order_book_socket, EPOLLOUT | EPOLLIN);

    //----------------------

    std::shared_ptr<Order> order;
    Timer reconect_timer;
    Timer update_timer;
    Timer stop_timer;
    stop_timer.reset();
    uint64_t last_time = 0;
    const uint64_t RECONECT_INTERVAL = 20;

    bool order_book_ready = false;

    for (;;)
    {
        poller->poll();

        for (;;)
        {
            auto event = poller->get_next_event();
            if (!event.action)
                break;

            try
            {
                if (event.action & PollerEvent::READ)
                    event.socket->read();

                if (event.action & PollerEvent::WRITE)
                    event.socket->write();

                if (event.action & PollerEvent::CLOSE)
                    event.socket->close(event.close_reason);
            }
            catch (const std::exception & e)
            {
                std::cerr << "Exception in callback: " << e.what() << std::endl << "Closing socket." << std::endl;
                event.socket->close();
            }
        }

        if (!order_book)
        {
            if (!order_book_socket.get() && reconect_timer.elapsed_seconds() > RECONECT_INTERVAL)
            {
                std::cerr << "creating new order_book_socket" << std::endl;
                order_book_socket = create_tcp_socket(order_book_url.host(), 443);
                order_book_socket->set_callbacks<ReceiveOrderBookLogic>(std::ref(order_book_url), std::ref(order_book));
                poller->update_socket(order_book_socket, EPOLLOUT | EPOLLIN);
            }

            continue;
        }

        if (!order_book_ready)
        {
            uint64_t first_diff_timestamp = orders.get_first_timestamp();
            uint64_t last_book_timestamp  = order_book.get_last_timestamp();

            std::cerr << first_diff_timestamp << "/" << last_book_timestamp << std::endl;

            if (last_book_timestamp < first_diff_timestamp)
            {
                order_book.clear();
                reconect_timer.reset();
                order_book_socket.reset();
                continue;
            }

            order_book_ready = true;
        }

        while (order = orders.get_order())
            order_book.add(*order);

        if (update_timer.elapsed_seconds() != last_time)
        {
            last_time = update_timer.elapsed_seconds();
            order_book.print();
        }
    }

    ssl_destroy();
    return 0;
}
