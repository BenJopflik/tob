
template <uint64_t NUMBER_OF_ORDERS>
Orders<NUMBER_OF_ORDERS>::Orders()
{
}

template <uint64_t NUMBER_OF_ORDERS>
template <typename ...Args>
void Orders<NUMBER_OF_ORDERS>::add_order(Args... args)
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);

    auto order = Pool<Order, NUMBER_OF_ORDERS>::get_shared(args...);
    if (!order)
        throw std::runtime_error("order queue is full");

    m_orders.push(order);
}

template <uint64_t NUMBER_OF_ORDERS>
uint64_t Orders<NUMBER_OF_ORDERS>::get_first_timestamp() const
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);

    if (m_orders.empty())
        return (uint64_t)-1;
    return m_orders.front()->timestamp;
}

template <uint64_t NUMBER_OF_ORDERS>
std::shared_ptr<Order> Orders<NUMBER_OF_ORDERS>::get_order()
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);

    if (m_orders.empty())
        return std::shared_ptr<Order>();

    auto order = m_orders.front();
    m_orders.pop();
    return order;
}

template <uint64_t NUMBER_OF_ORDERS>
void Orders<NUMBER_OF_ORDERS>::clear()
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);

    while (!m_orders.empty())
        m_orders.pop();
}
