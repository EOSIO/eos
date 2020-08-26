#pragma once

#include <map>
#include <unordered_set>
#include <vector>

#include <b1/session/bytes.hpp>
#include <b1/session/cache_fwd_decl.hpp>
#include <b1/session/key_value.hpp>

namespace eosio::session {

// An in memory caching data store for storing key_value types.
//
// \tparam allocator The memory allocator to managed the memory used by the key_value instances stored in the cache.
// \remarks This type implements the "data store" concept.
template <typename allocator>
class cache final {
private:
    // Currently there are some constraints that require the use of a std::map.
    // Namely, we need to be able to iterator over the entries in lexigraphical ordering on the keys.
    // It also provides us with lower bound and upper bound out of the box.
    using cache_type = std::map<bytes, key_value>;

public:
    using allocator_type = allocator;
    
    template <typename iterator_type, typename iterator_traits>
    class cache_iterator final {
    public:
        using difference_type = typename iterator_traits::difference_type;
        using value_type = typename iterator_traits::value_type;
        using pointer = typename iterator_traits::pointer;
        using reference = typename iterator_traits::reference;
        using iterator_category = typename iterator_traits::iterator_category;

        cache_iterator() = default;
        cache_iterator(const cache_iterator& it) = default;
        cache_iterator(cache_iterator&&) = default;
        cache_iterator(iterator_type it);
        
        cache_iterator&  operator=(const cache_iterator& it) = default;
        cache_iterator&  operator=(cache_iterator&&) = default;
        
        cache_iterator& operator++();
        cache_iterator operator++(int);
        cache_iterator& operator--();
        cache_iterator operator--(int);
        reference operator*() const;
        reference operator->() const;
        bool operator==(const cache_iterator& other) const;
        bool operator!=(const cache_iterator& other) const;
        
    private:
        iterator_type m_it;
    };

    struct iterator_traits {
        using difference_type = long;
        using value_type = key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
    };
    using iterator = cache_iterator<cache_type::iterator, iterator_traits>;

    struct const_iterator_traits {
        using difference_type = long;
        using value_type = const key_value;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
    };
    using const_iterator = cache_iterator<cache_type::const_iterator, const_iterator_traits>;

public:
    cache() = default;
    cache(const cache&) = default;
    cache(cache&&) = default;
    cache(std::shared_ptr<allocator> memory_allocator);
    
    auto operator=(const cache&) -> cache& = default;
    auto operator=(cache&&) -> cache& = default;
    
    const key_value& read(const bytes& key) const;
    void write(key_value kv);
    bool contains(const bytes& key) const;
    void erase(const bytes& key);
    void clear();

    template <typename iterable>
    const std::pair<std::vector<key_value>, std::unordered_set<bytes>> read(const iterable& keys) const;

    template <typename iterable>
    void write(const iterable& key_values);

    template <typename iterable>
    void erase(const iterable& keys);

    template <typename data_store, typename iterable>
    void write_to(data_store& ds, const iterable& keys) const;

    template <typename data_store, typename iterable>
    void read_from(const data_store& ds, const iterable& keys);
    
    iterator find(const bytes& key);
    const_iterator find(const bytes& key) const;
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    iterator lower_bound(const bytes& key);
    const_iterator lower_bound(const bytes& key) const;
    iterator upper_bound(const bytes& key);
    const_iterator upper_bound(const bytes& key) const;
    
    const std::shared_ptr<allocator>& memory_allocator();
    const std::shared_ptr<const allocator> memory_allocator() const;

private:
    template <typename on_cache_hit, typename on_cache_miss>
    const key_value& find_(const bytes& key, const on_cache_hit& cache_hit, const on_cache_miss& cache_miss) const;
    
private:
    std::shared_ptr<allocator> m_allocator;
    cache_type m_cache;
};

template <typename allocator>
cache<allocator> make_cache(std::shared_ptr<allocator> a) {
    return {std::move(a)};
}

template <typename allocator>
cache<allocator>::cache(std::shared_ptr<allocator> memory_allocator)
: m_allocator{std::move(memory_allocator)} {
}

template <typename allocator>
const std::shared_ptr<allocator>& cache<allocator>::memory_allocator() {
    return m_allocator;
}

template <typename allocator>
const std::shared_ptr<const allocator> cache<allocator>::memory_allocator() const {
    return m_allocator;
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
const key_value& cache<allocator>::find_(const bytes& key, const on_cache_hit& cache_hit, const on_cache_miss& cache_miss) const {
    auto it = m_cache.find(key);
    
    if (it == std::end(m_cache)) {
        cache_miss();
        return key_value::invalid;
    }
    
    if (cache_hit(it)) {
        return it->second;
    }
    
    return key_value::invalid;
}

template <typename allocator>
const key_value& cache<allocator>::read(const bytes& key) const {
    return find_(key, [](auto& it){ return true; }, [](){});
}

template <typename allocator>
void cache<allocator>::write(key_value kv) {
    auto it = m_cache.find(kv.key());
    if (it == std::end(m_cache)) {
        auto key = kv.key();
        m_cache.emplace(std::move(key), std::move(kv));
        return;
    }
    it->second = std::move(kv);
}

template <typename allocator>
bool cache<allocator>::contains(const bytes& key) const {
    return find_(key, [](auto& it){ return true; }, [](){}) != key_value::invalid;
}

template <typename allocator>
void cache<allocator>::erase(const bytes& key) {
    find_(key, [&](auto& it){ m_cache.erase(it); return false; }, [](){});
}

template <typename allocator>
void cache<allocator>::clear() {
    m_cache.clear();
}

// Reads a batch of keys from the cache.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
// \returns An std::pair where the first item is list of the found key_values and the second item is a set of the keys not found.
template <typename allocator>
template <typename iterable>
const std::pair<std::vector<key_value>, std::unordered_set<bytes>> cache<allocator>::read(const iterable& keys) const {
    auto not_found = std::unordered_set<bytes>{};
    auto kvs = std::vector<key_value>{};
    
    for (const auto& key : keys) {
        auto& kv = find_(key, [](auto& it){ return true; }, [](){});
        if (kv == key_value::invalid) {
            not_found.emplace(key);
            continue;
        }
        kvs.emplace_back(kv);
    }
    
    return {std::move(kvs), std::move(not_found)};
}

// Writes a batch of key_values to the cache.
//
// \tparam iterable Any type that can be used within a range based for loop and returns key_value instances in its iterator.
// \param key_values An iterable instance that returns key_value instances in its iterator.
template <typename allocator>
template <typename iterable>
void cache<allocator>::write(const iterable& key_values) {
    for (const auto& kv : key_values) {
        write(kv);
    }
}

// Erases a batch of key_values from the cache.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename allocator>
template <typename iterable>
void cache<allocator>::erase(const iterable& keys) {
    for (const auto& key : keys) {
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
void cache<allocator>::write_to(data_store& ds, const iterable& keys) const {
    auto kvs = std::vector<key_value>{};
    
    for (const auto& key : keys) {
        auto& kv = find_(key, [](auto& it){ return true; }, [](){});
        if (kv == key_value::invalid) {
            continue;
        }

        if (!ds.memory_allocator()->equals(*memory_allocator())) {
            kvs.emplace_back(make_kv(kv.key().data(), kv.key().length(), kv.value().data(), kv.value().length(), ds.memory_allocator()));
        } else {
            kvs.emplace_back(kv);
        }
        
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
void cache<allocator>::read_from(const data_store& ds, const iterable& keys) {
    auto kvs = std::vector<key_value>{};
    
    for (const auto& key : keys) {
        auto& kv = ds.read(key);
        if (kv == key_value::invalid) {
            continue;
        }
        
        if (m_allocator->equals(*ds.memory_allocator())) {
          kvs.emplace_back(kv);
        } else {
          kvs.emplace_back(make_kv(kv.key().data(), kv.key().length(), kv.value().data(), kv.value().length(), m_allocator));
        }
    }
    
    write(kvs);
    
    return;
}

template <typename allocator>
typename cache<allocator>::iterator cache<allocator>::find(const bytes& key) {
    return {m_cache.find(key)};
}

template <typename allocator>
typename cache<allocator>::const_iterator cache<allocator>::find(const bytes& key) const {
    return {m_cache.find(key)};
}

template <typename allocator>
typename cache<allocator>::iterator cache<allocator>::begin() {
    return {std::begin(m_cache)};
}

template <typename allocator>
typename cache<allocator>::const_iterator cache<allocator>::begin() const {
    return {std::begin(m_cache)};
}

template <typename allocator>
typename cache<allocator>::iterator cache<allocator>::end() {
    return {std::end(m_cache)};
}

template <typename allocator>
typename cache<allocator>::const_iterator cache<allocator>::end() const {
    return {std::end(m_cache)};
}

template <typename allocator>
typename cache<allocator>::iterator cache<allocator>::lower_bound(const bytes& key) {
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
typename cache<allocator>::const_iterator cache<allocator>::lower_bound(const bytes& key) const {
    return {m_cache.lower_bound(key)};
}

template <typename allocator>
typename cache<allocator>::iterator cache<allocator>::upper_bound(const bytes& key) {
    return {m_cache.upper_bound(key)};
}

template <typename allocator>
typename cache<allocator>::const_iterator cache<allocator>::upper_bound(const bytes& key) const {
    return {m_cache.upper_bound(key)};
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
cache<allocator>::cache_iterator<iterator_type, iterator_traits>::cache_iterator(iterator_type it)
: m_it{std::move(it)} {
}

template <typename allocator, typename iterator_type, typename iterator_traits>
using cache_iterator_alias = typename cache<allocator>::template cache_iterator<iterator_type, iterator_traits>;

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
cache_iterator_alias<allocator, iterator_type, iterator_traits>& cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator++() {
    ++m_it;
    return *this;
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
cache_iterator_alias<allocator, iterator_type, iterator_traits> cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator++(int) {
    return {m_it++};
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
cache_iterator_alias<allocator, iterator_type, iterator_traits>& cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator--() {
    --m_it;
    return *this;
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
cache_iterator_alias<allocator, iterator_type, iterator_traits> cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator--(int) {
    return {m_it--};
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
typename cache_iterator_alias<allocator, iterator_type, iterator_traits>::reference cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator*() const {
    return m_it->second;
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
typename cache_iterator_alias<allocator, iterator_type, iterator_traits>::reference cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator->() const {
    return m_it->second;
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
bool cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator==(const cache_iterator& other) const {
    return m_it == other.m_it;
}

template <typename allocator>
template <typename iterator_type, typename iterator_traits>
bool cache<allocator>::cache_iterator<iterator_type, iterator_traits>::operator!=(const cache_iterator& other) const {
    return !(*this == other);
}

}
