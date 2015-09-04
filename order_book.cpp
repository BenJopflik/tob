#include "order_book.hpp"
#include "order.hpp"
#include <iostream>
#include "common/logger.hpp"

OrderBook::OrderBook(uint64_t depth) : m_bid(depth), m_ask(depth), m_depth(depth)
{

}

OrderBook::operator bool () const
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);

    return !m_bid.empty() || !m_ask.empty();
}

template <class Destination>
static void update(Destination & dest, const Order & order)
{
    if (!order.amount)
        return dest.erase(order.price);

    dest.update(order.price, order.amount);
}

void OrderBook::add(const Order & order)
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);


    if (order.timestamp < m_last_timestamp)
        return;

    m_last_timestamp = order.timestamp;

    if (order.type == Order::BUY)
        update(m_bid, order);
    else if (order.type == Order::SELL)
        update(m_ask, order);
}

template <class T>
static void print_level(T & level, const std::string & color)
{
    if (level->second.value != level->second.prev_value)
    {
        std::cerr << color;
        level->second.prev_value = level->second.value;
    }

    std::cerr << level->first << " " << level->second.value << "\e[0m" << std::endl;
}

void OrderBook::print()
{
    static const std::string ASK_HIGHLIGHT = "\e[41m";
    static const std::string BID_HIGHLIGHT = "\e[42m";

    std::lock_guard<decltype(m_lock)> lock(m_lock);

    std::cerr << std::string(100, '\n');

//    std::cerr << std::endl << "=================" << std::endl;
    std::cerr << "ASKS: " << std::endl;

    {
        auto level = m_ask.end();
        do
        {
            --level;
            print_level(level, ASK_HIGHLIGHT);
        }
        while (level != m_ask.begin());
    }

    std::cerr << std::endl;

    std::cerr << "BIDS: " << std::endl;

    for (auto level = m_bid.begin(); level != m_bid.end(); ++level)
        print_level(level, BID_HIGHLIGHT);
}

void OrderBook::clear()
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);

    m_bid.clear();
    m_ask.clear();
    m_last_timestamp = 0;
}

uint64_t OrderBook::get_last_timestamp() const
{
    std::lock_guard<decltype(m_lock)> lock(m_lock);

    return m_last_timestamp;
}
