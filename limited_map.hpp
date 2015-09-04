#pragma once
#include <utility>
#include <functional>
#include <map>
#include <iostream>

template <class Key, class Value, template<class T> class Comparator>
class LimitedMap
{
    struct Payload
    {
    public:
        explicit Payload(const Value & value) : value(value) {}

    public:
        Value value      {0};
        Value prev_value {0};

    };

    using Map  = std::map<Key, Payload, Comparator<Key>>;
    using Iter = decltype(std::declval<Map>().end());

public:
    LimitedMap(uint64_t depth);
   ~LimitedMap();

    void update(const Key & key, const Value & value);
    void erase (const Key & key);

    Iter begin();
    Iter end();

    void clear();
    bool empty() const;

private:
    Map  m_map;
    Iter m_end {m_map.end()};

    uint64_t m_depth {0};

};

#include "limited_map.tpp"
