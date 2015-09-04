#include <queue>
#include <stdexcept>
#include <mutex>

#include "memory/pool.hpp"
#include "thread/spinlock.hpp"
#include "order.hpp"

#define MAX_NUMBER_OF_ORDERS 1000
class Orders
{
public:
    // TODO add check
    Orders(uint64_t max_number_of_orders) : m_max_number_of_orders(max_number_of_orders) {}

    template <typename ...Args>
    void add_order(Args... args)
    {
        std::lock_guard<decltype(m_lock)> lock(m_lock);
        if (m_number_of_orders + 1 == MAX_NUMBER_OF_ORDERS)
            throw std::runtime_error("order queue is full");
        ++m_number_of_orders;
        auto order = m_pool.get_shared(args...);
        if (order && order->timestamp > m_order_book_timestamp)
            m_orders.push(order);
    }

    void set_order_book_timestamp(uint64_t order_book_timestamp)
    {
        std::lock_guard<decltype(m_lock)> lock(m_lock);
        m_order_book_timestamp = order_book_timestamp;
        while (m_number_of_orders && m_orders.front()->timestamp <= m_order_book_timestamp)
        {
            m_orders.pop();
            --m_number_of_orders;
        }
    }

    uint64_t get_first_timestamp() const
    {
        std::lock_guard<decltype(m_lock)> lock(m_lock);
        if (m_orders.empty())
            return (uint64_t)-1;
        return m_orders.front()->timestamp;
    }

    std::shared_ptr<Order> get_order()
    {
        std::lock_guard<decltype(m_lock)> lock(m_lock);
        if (!m_number_of_orders)
            return std::shared_ptr<Order>();
        --m_number_of_orders;
        auto order = m_orders.front();
        m_orders.pop();
        return order;
    }

    void clear()
    {
        std::lock_guard<decltype(m_lock)> lock(m_lock);
        m_number_of_orders = 0;
        while (!m_orders.empty())
            m_orders.pop();
    }

private:
    uint64_t m_max_number_of_orders {0};
    uint64_t m_order_book_timestamp {0};
    mutable SpinlockYield m_lock;
    uint64_t m_number_of_orders {0};
    Pool<Order, MAX_NUMBER_OF_ORDERS> m_pool;
    std::queue<std::shared_ptr<Order>> m_orders;

};
