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
        Value value {0};
        Value prev_value {0};

    };

    using Map = std::map<Key, Payload, Comparator<Key>>;
    using Iter = decltype(std::declval<Map>().end());

public:
    LimitedMap(uint64_t depth) : m_depth(depth) {}
   ~LimitedMap() {}

    void update(const Key & key, const Value & value)
    {
        auto iter = m_map.insert({key, Payload(value)});
        if (!iter.second)
        {
            iter.first->second.prev_value = iter.first->second.value;
            iter.first->second.value = value;
            return;
        }

        uint64_t size = m_map.size();
        if (size <= m_depth)
        {
            m_end = m_map.end();
        }
        else if (size > m_depth)
        {
            bool moved = false;
            if (m_end == m_map.end())
            {
                --m_end;
                if (m_end->first == key)
                    return;
                moved = true;
            }

            if (Comparator<Key>()(key, m_end->first))
            {
                if (!moved)
                    --m_end;
            }
            else if (moved)
            {
                ++m_end;
            }

            m_end->second.prev_value = 0;
        }
    }

    void erase(const Key & key)
    {
        auto iter = m_map.find(key);
        if (iter == m_map.end())
            return;

        uint64_t size = m_map.size() - 1;
        if (size <= m_depth)
        {
            m_end = m_map.end();
        }
        else if (size > m_depth)
        {
            if (Comparator<Key>()(key, m_end->first) || key == m_end->first)
                ++m_end;
        }

        m_map.erase(iter);
    }

    Iter begin()
    {
        return m_map.begin();
    }

    Iter end()
    {
        return m_end;
    }

    bool empty() const
    {
        return m_map.empty();
    }

private:
    Map m_map;
    Iter m_end {m_map.end()};

    uint64_t m_depth {0};

};
