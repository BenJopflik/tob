#pragma once

#include <queue>
#include <stdexcept>
#include <mutex>

#include "memory/pool.hpp"
#include "thread/spinlock.hpp"
#include "order.hpp"

template <uint64_t NUMBER_OF_ORDERS>
class Orders : private Pool<Order, NUMBER_OF_ORDERS>
{
public:
    Orders();

    template <typename ...Args>
    void add_order(Args... args);

    uint64_t get_first_timestamp() const;
    std::shared_ptr<Order> get_order();
    void clear();

private:
    std::queue<std::shared_ptr<Order>> m_orders; // TODO change to circular buffer

    mutable SpinlockYield m_lock;
};

#include "orders.tpp"
