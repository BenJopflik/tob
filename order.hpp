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
    Order(const uint64_t & type,
          const uint64_t & timestamp,
          const uint64_t & price,
          const uint64_t & amount) : type(type),
                                     timestamp(timestamp),
                                     price(price),
                                     amount(amount) {}

    bool operator < (const Order & right) {return timestamp < right.timestamp;}

    // XXX DEBUG
    std::string to_string() const
    {
        std::string ret;
        ret = (type == BUY ? "BID" : "ASK");
        ret += " ";
        ret += std::to_string(price);
        ret += " ";
        ret += std::to_string(amount);
        return ret;
    }

public:
    uint64_t type      {INVALID};

    uint64_t timestamp {0};
    uint64_t price     {0};
    uint64_t amount    {0};
};

