#pragma once

#include <map>
#include <cstdint>
#include "limited_map.hpp"
#include <mutex>

class LimitedQueue;

class Order;

class OrderBook
{
public:
    OrderBook(uint64_t depth);

    operator bool () const;
    void add(const Order & order);
    void print();
    void clear();

    uint64_t get_last_timestamp() const;

private:
    LimitedMap<uint64_t, uint64_t, std::greater> m_bid;
    LimitedMap<uint64_t, uint64_t, std::less>    m_ask;

    uint64_t m_last_timestamp {0};
    uint64_t m_depth {0};

    mutable std::mutex m_lock; // TODO change to spinlock

};
