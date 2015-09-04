#pragma once
#include <deque>
#include <iostream>
#include <cstring>

#include "socket/socket_callbacks.hpp"
#include "common/url.hpp"
#include "orders.hpp"

#include "json/json.h"
#include "socket/socket.hpp"
#include "frame.hpp"
#include "orders.hpp"
#include "order_book.hpp"


template <uint64_t T>
class DiffLogic: public SocketCallbacks
{
    static const uint64_t BUFFER_SIZE = 50000;

private:
    enum State
    {
        HANDSHAKE = 0,
        HANDSHAKE_RESPONSE,
        IN_WORK,

    };

    struct OutputFrame
    {
    public:
        OutputFrame(const std::shared_ptr<uint8_t> & frame, uint64_t size) : frame(frame), size(size) {}

    public:
        std::shared_ptr<uint8_t> frame {nullptr};

        uint64_t size   {0};
        uint64_t offset {0};
    };

public:
    DiffLogic(const Url & url, Orders<T> & orders) : m_url(url), m_orders(orders) {}

private:
    virtual void on_read (Socket *) override;
    virtual void on_write(Socket *) override;
    virtual void on_close(Socket *, const uint64_t &) override;

    void generate_frame(uint64_t type, const std::string & message, uint32_t mask_key = 0);
    void write_handshake(Socket *socket);

private:
    Url      m_url;
    uint64_t m_offset {0};
    uint8_t  m_buffer[BUFFER_SIZE];

    uint64_t m_state {HANDSHAKE};

    std::deque<OutputFrame> m_output_queue;
    Orders<T> & m_orders;
};

#include "diff_logic.tpp"
