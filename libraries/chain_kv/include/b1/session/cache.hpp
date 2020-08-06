#ifndef cache_data_store_h
#define cache_data_store_h

#include <map>
#include <unordered_set>
#include <vector>

#include <b1/session/bytes.hpp>
#include <b1/session/cache_fwd_decl.hpp>
#include <b1/session/key_value.hpp>

namespace b1::session
{

// An in memory caching data store for storing key_value types.
//
// \tparam allocator The memory allocator to managed the memory used by the key_value instances stored in the cache.
// \remarks This type implements the "data store" concept.
template <typename allocator>
class cache final
{
private:
    // Currently there are some constraints that require the use of a std::map.
    // Namely, we need to be able to iterator over the entries in lexigraphical ordering on the keys.
    using cache_type = std::map<bytes, key_value>;

public:
    using allocator_type = allocator;
    
    class iterator;
    using const_iterator = const iterator;
    
    class iterator  final: public std::iterator<std::bidirectional_iterator_tag, key_value>
    {
    public:
        iterator() = default;
        iterator(const iterator& it) = default;
        iterator(iterator&&) = default;
        iterator(cache_type::iterator it);
        
        auto operator=(const iterator& it) -> iterator& = default;
        auto operator=(iterator&&) -> iterator& = default;
        
        auto operator++() -> iterator&;
        auto operator++() const -> const_iterator&;
        auto operator++(int) -> iterator;
        auto operator++(int) const -> const_iterator;
        auto operator--() -> iterator&;
        auto operator--() const -> const_iterator&;
        auto operator--(int) -> iterator;
        auto operator--(int) const -> const_iterator;
        auto operator*() const -> const key_value&;
        auto operator==(const_iterator& other) const -> bool;
        auto operator!=(const_iterator& other) const -> bool;
        
    private:
        cache_type::iterator m_it;
    };
    
public:
    cache() = default;
    cache(const cache&) = default;
    cache(cache&&) = default;
    cache(std::shared_ptr<allocator> memory_allocator);
    
    auto operator=(const cache&) -> cache& = default;
    auto operator=(cache&&) -> cache& = default;
    
    auto read(const bytes& key) const -> const key_value&;
    auto write(key_value kv) -> void;
    auto contains(const bytes& key) const -> bool;
    auto erase(const bytes& key) -> void;

    template <typename iterable>
    auto read(const iterable& keys) const -> const std::pair<std::vector<key_value>, std::unordered_set<bytes>>;

    template <typename iterable>
    auto write(const iterable& key_values) -> void;

    template <typename iterable>
    auto erase(const iterable& keys) -> void;

    template <typename data_store, typename iterable>
    auto write_to(data_store& ds, const iterable& keys) const -> void;

    template <typename data_store, typename iterable>
    auto read_from(const data_store& ds, const iterable& keys) -> void;
    
    auto find(const bytes& key) -> iterator;
    auto find(const bytes& key) const -> const_iterator;
    auto begin() -> iterator;
    auto begin() const -> const_iterator;
    auto end() -> iterator;
    auto end() const -> const_iterator;
    auto cbegin() const -> const_iterator;
    auto cend() const -> const_iterator;
    auto lower(const bytes& key) -> iterator;
    auto lower(const bytes& key) const -> const_iterator;
    auto upper(const bytes& key) -> iterator;
    auto upper(const bytes& key) const -> const_iterator;
    auto clower(const bytes& key) const -> const_iterator;
    auto cupper(const bytes& key) const -> const_iterator;
    
    auto memory_allocator() -> const std::shared_ptr<allocator>&;

private:
    template <typename on_cache_hit, typename on_cache_miss>
    auto find_(const bytes& key, const on_cache_hit& cache_hit, const on_cache_miss& cache_miss) const -> const key_value&;
    
private:
    cache_type m_cache;
    std::shared_ptr<allocator> m_allocator;
};

template <typename allocator>
auto make_cache(std::shared_ptr<allocator> a) -> cache<allocator>
{
    return {std::move(a)};
}

template <typename allocator>
cache<allocator>::cache(std::shared_ptr<allocator> memory_allocator)
: m_allocator{std::move(memory_allocator)}
{
}

// Searches the cache for the given key.
//
// \tparam on_cache_hit A functor to invoke when the key is found.  It has the following signature void(cache_type::iterator& it)
// \tparam on_cache_miss A functor to invoke when the key is not found.  It has the following singature void(void).
// \param key The key to search the cache for.
// \param cache_hit The functor to invoke when the key is found.
// \param cache_miss The functor to invoke when the key is not found.
// \return A reference to the key_value instance if found, key_value::invalid otherwise.
template <typename allocator>
template <typename on_cache_hit, typename on_cache_miss>
auto cache<allocator>::find_(const bytes& key, const on_cache_hit& cache_hit, const on_cache_miss& cache_miss) const -> const key_value&
{
    auto it = m_cache.find(key);
    
    if (it == std::end(m_cache))
    {
        cache_miss();
        return key_value::invalid;
    }
    
    if (cache_hit(it))
    {
        return it->second;
    }
    
    return key_value::invalid;
}

template <typename allocator>
auto cache<allocator>::read(const bytes& key) const -> const key_value&
{
    return find_(key, [](auto& it){ return true; }, [](){});
}

template <typename allocator>
auto cache<allocator>::write(key_value kv) -> void
{
    auto it = m_cache.find(kv.key());
    if (it == std::end(m_cache))
    {
        m_cache.emplace(kv.key(), std::move(kv));
        return;
    }
    it->second = std::move(kv);
}

template <typename allocator>
auto cache<allocator>::contains(const bytes& key) const -> bool
{
    return find_(key, [](auto& it){ return true; }, [](){}) != key_value::invalid;
}

template <typename allocator>
auto cache<allocator>::erase(const bytes& key) -> void
{
    find_(key, [&](auto& it){ m_cache.erase(it); return false; }, [](){});
}

// Reads a batch of keys from the cache.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
// \returns An std::pair where the first item is list of the found key_values and the second item is a set of the keys not found.
template <typename allocator>
template <typename iterable>
auto cache<allocator>::read(const iterable& keys) const -> const std::pair<std::vector<key_value>, std::unordered_set<bytes>>
{
    auto not_found = std::unordered_set<bytes>{};
    auto kvs = std::vector<key_value>{};
    
    for (const auto& key : keys)
    {
        auto& kv = find_(key, [](auto& it){ return true; }, [](){});
        if (kv == key_value::invalid)
        {
            not_found.emplace(key);
            continue;
        }
        kvs.emplace_back(kv);
    }
    
    return std::make_pair(std::move(kvs), std::move(not_found));
}

// Writes a batch of key_values to the cache.
//
// \tparam iterable Any type that can be used within a range based for loop and returns key_value instances in its iterator.
// \param key_values An iterable instance that returns key_value instances in its iterator.
template <typename allocator>
template <typename iterable>
auto cache<allocator>::write(const iterable& key_values) -> void
{
    for (const auto& kv : key_values)
    {
        write(std::move(kv));
    }
}

// Erases a batch of key_values from the cache.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename allocator>
template <typename iterable>
auto cache<allocator>::erase(const iterable& keys) -> void
{
    for (const auto& key : keys)
    {
        find_(key, [&](auto& it){ m_cache.erase(it); return false; }, [](){});
    }
}

// Writes a batch of key_values from this cache into the given data_store instance.
//
// \tparam data_store A type that implements the "data store" concept.  cache is an example of an implementation of that concept.
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param ds A data store instance.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename allocator>
template <typename data_store, typename iterable>
auto cache<allocator>::write_to(data_store& ds, const iterable& keys) const -> void
{
    auto kvs = std::vector<key_value>{};
    
    for (const auto& key : keys)
    {
        auto& kv = find_(key, [](auto& it){ return true; }, [](){});
        if (kv == key_value::invalid)
        {
            continue;
        }
        kvs.emplace_back(make_kv(kv.key().data(), kv.key().length(), kv.value().data(), kv.value().length(), ds.memory_allocator()));
    }
    
    ds.write(kvs);
}

// Reads a batch of key_values from the given data store into the cache.
//
// \tparam data_store A type that implements the "data store" concept.  cache is an example of an implementation of that concept.
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param ds A data store instance.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename allocator>
template <typename data_store, typename iterable>
auto cache<allocator>::read_from(const data_store& ds, const iterable& keys) -> void
{
    auto kvs = std::vector<key_value>{};
    
    for (const auto& key : keys)
    {
        auto& kv = ds.read(key);
        if (kv == key_value::invalid)
        {
            continue;
        }
        
        kvs.emplace(make_kv(kv.key().data(), kv.key().size(), kv.value().data(), kv.value().size(), m_allocator));
    }
    
    write(kvs);
    
    return;
}

template <typename allocator>
auto cache<allocator>::find(const bytes& key) -> typename cache<allocator>::iterator
{
    return {m_cache.find(key)};
}

template <typename allocator>
auto cache<allocator>::find(const bytes& key) const -> typename cache<allocator>::const_iterator
{
    return {m_cache.find(key)};
}

template <typename allocator>
auto cache<allocator>::begin() -> typename cache<allocator>::iterator
{
    return {std::begin(m_cache)};
}

template <typename allocator>
auto cache<allocator>::begin() const -> typename cache<allocator>::const_iterator
{
    return {std::begin(m_cache)};
}

template <typename allocator>
auto cache<allocator>::end() -> typename cache<allocator>::iterator
{
    return {std::end(m_cache)};
}

template <typename allocator>
auto cache<allocator>::end() const -> typename cache<allocator>::const_iterator
{
    return {std::end(m_cache)};
}

template <typename allocator>
auto cache<allocator>::cbegin() const -> typename cache<allocator>::const_iterator
{
    return {std::begin(m_cache)};
}

template <typename allocator>
auto cache<allocator>::cend() const -> typename cache<allocator>::const_iterator
{
    return {std::end(m_cache)};
}

template <typename allocator>
auto cache<allocator>::lower(const bytes& key) -> typename cache<allocator>::iterator
{
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
auto cache<allocator>::lower(const bytes& key) const -> typename cache<allocator>::const_iterator
{
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
auto cache<allocator>::upper(const bytes& key) -> typename cache<allocator>::iterator
{
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
auto cache<allocator>::upper(const bytes& key) const -> typename cache<allocator>::const_iterator
{
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
auto cache<allocator>::clower(const bytes& key) const -> typename cache<allocator>::const_iterator
{
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
auto cache<allocator>::cupper(const bytes& key) const -> typename cache<allocator>::const_iterator
{
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
cache<allocator>::iterator::iterator(cache_type::iterator it)
: m_it{std::move(it)}
{
}

template <typename allocator>
auto cache<allocator>::iterator::operator++() -> typename cache<allocator>::iterator&
{
    ++m_it;
    return *this;
}

template <typename allocator>
auto cache<allocator>::iterator::operator++() const -> typename cache<allocator>::const_iterator&
{
    ++m_it;
    return *this;
}

template <typename allocator>
auto cache<allocator>::iterator::operator++(int) -> typename cache<allocator>::iterator
{
    return iterator{m_it++};
}

template <typename allocator>
auto cache<allocator>::iterator::operator++(int) const -> typename cache<allocator>::const_iterator
{
    return iterator{m_it++};
}

template <typename allocator>
auto cache<allocator>::iterator::operator--() -> typename cache<allocator>::iterator&
{
    --m_it;
    return *this;
}

template <typename allocator>
auto cache<allocator>::iterator::operator--() const -> typename cache<allocator>::const_iterator&
{
    --m_it;
    return *this;
}

template <typename allocator>
auto cache<allocator>::iterator::operator--(int) -> typename cache<allocator>::iterator
{
    return iterator{m_it--};
}

template <typename allocator>
typename cache<allocator>::const_iterator cache<allocator>::iterator::operator--(int) const
{
    return iterator{m_it--};
}

template <typename allocator>
auto cache<allocator>::iterator::operator*() const -> const key_value&
{
    return m_it->second;
}

template <typename allocator>
auto cache<allocator>::iterator::operator==(const_iterator& other) const -> bool
{
    return m_it == other.m_it;
}

template <typename allocator>
auto cache<allocator>::iterator::operator!=(const_iterator& other) const -> bool
{
    return !(*this == other);
}

}

#endif /* cache_data_store_h */
