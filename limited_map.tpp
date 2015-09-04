template <class Key, class Value, template<class T> class Comparator>
LimitedMap<Key, Value, Comparator>::LimitedMap(uint64_t depth) : m_depth(depth)
{

}

template <class Key, class Value, template<class T> class Comparator>
LimitedMap<Key, Value, Comparator>::~LimitedMap()
{

}

template <class Key, class Value, template<class T> class Comparator>
void LimitedMap<Key, Value, Comparator>::update(const Key & key, const Value & value)
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

        if (Comparator<Key>()(key, m_end->first) && !moved)
            --m_end;

        m_end->second.prev_value = 0;
    }
}

template <class Key, class Value, template<class T> class Comparator>
void LimitedMap<Key, Value, Comparator>::erase(const Key & key)
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

template <class Key, class Value, template<class T> class Comparator>
inline auto LimitedMap<Key, Value, Comparator>::begin() -> Iter
{
    return m_map.begin();
}

template <class Key, class Value, template<class T> class Comparator>
inline auto LimitedMap<Key, Value, Comparator>::end() -> Iter
{
    return m_end;
}

template <class Key, class Value, template<class T> class Comparator>
inline void LimitedMap<Key, Value, Comparator>::clear()
{
    m_map.clear();
    m_end = m_map.end();
}

template <class Key, class Value, template<class T> class Comparator>
inline bool LimitedMap<Key, Value, Comparator>::empty() const
{
    return m_map.empty();
}

