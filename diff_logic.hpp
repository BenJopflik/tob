#pragma once
#include <deque>

#include "socket/socket_callbacks.hpp"
#include "common/url.hpp"

class Orders;

class DiffLogic: public SocketCallbacks
{
private:
    enum State
    {
        HANDSHAKE = 0,
        IN_WORK,

    };

    struct OutputFrame
    {
    public:
        OutputFrame(const std::shared_ptr<uint8_t> & frame, uint64_t size) : frame(frame), size(size) {}

    public:
        std::shared_ptr<uint8_t> frame {nullptr};
        uint64_t size {0};
        uint64_t offset {0};
    };

public:
    DiffLogic(const Url & url, Orders & orders) : m_url(url), m_orders(orders) {}

private:

    virtual void on_read(Socket *) override;
    virtual void on_write(Socket *) override;
    virtual void on_error(Socket *) override;

    virtual void on_close(Socket *, int64_t fd) override;
    virtual void on_connected(Socket *) override;
    virtual void on_rearm(Socket *) override;
    void generate_frame(uint64_t type, const std::string & message, uint32_t mask_key = 0);
    void write_handshake(Socket *socket);

private:
    Url      m_url;
    uint64_t m_state {HANDSHAKE};
    uint8_t  m_buffer[50000];
    uint64_t m_offset {0};
    uint64_t m_size {0};
    std::deque<OutputFrame> m_output_queue;
    Orders & m_orders;

};
