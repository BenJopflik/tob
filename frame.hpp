#pragma once

#include <memory>

struct Frame
{
    enum Type
    {
        CONTINUATION = 0,
        TEXT,
        BINARY,
//      0x3 - 0x7 reserved
        CONNECTION_CLOSE = 8,
        PING,
        PONG,
//      0xB - 0xF reserved
    };

public:
    Frame();
    ~Frame();

    std::shared_ptr<uint8_t> serialize(const uint8_t * payload, uint64_t & size);
    uint8_t * deserialize(uint8_t * data, uint64_t size);

public:
    uint8_t * payload {nullptr};
    uint64_t payload_length {0};
    uint32_t mask_key {0};
    uint8_t  type {CONTINUATION};
    uint8_t fin {false};
    uint8_t masked {false};

};
