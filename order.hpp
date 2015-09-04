#pragma once
#include "limited_map.hpp"

struct Order
{
public:
    enum Type
    {
        INVALID = 0,
        BUY,
        SELL,

    };

public:
    Order() {}
    Order(uint64_t type,
          uint64_t timestamp,
          uint64_t price,
          uint64_t amount) : type(type),
                             timestamp(timestamp),
                             price(price),
                             amount(amount) {}

    bool operator < (const Order & right) {return timestamp < right.timestamp;}

public:
    uint64_t type {INVALID};
    uint64_t timestamp {0};

    uint64_t price {0};
    uint64_t amount {0};
};

