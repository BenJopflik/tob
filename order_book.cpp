#include "order_book.hpp"
#include "order.hpp"
#include <iostream>
#include "common/logger.hpp"

OrderBook::OrderBook(uint64_t depth) : m_bid(depth), m_ask(depth), m_depth(depth)
{

}

OrderBook::operator bool () const
{
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
    std::lock_guard<std::mutex> lock(m_mutex);
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

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!*this)
    {
        std::cerr << "Waiting for data" << std::endl;
        return;
    }

    std::cerr << "\n\n=========================================" << std::endl;

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
    std::cerr << "BIDS: " << std::endl;

    for (auto level = m_bid.begin(); level != m_bid.end(); ++level)
        print_level(level, BID_HIGHLIGHT);
}

