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
// \tparam Allocator The memory allocator to managed the memory used by the key_value instances stored in the cache.
// \remarks This type implements the "data store" concept.
template <typename Allocator>
class cache final {
private:
    // Currently there are some constraints that require the use of a std::map.
    // Namely, we need to be able to iterator over the entries in lexigraphical ordering on the keys.
    // It also provides us with lower bound and upper bound out of the box.
    using cache_type = std::map<bytes, key_value>;

public:
    using allocator_type = Allocator;
    
    template <typename Iterator_type, typename Iterator_traits>
    class cache_iterator final {
    public:
        using difference_type = typename Iterator_traits::difference_type;
        using value_type = typename Iterator_traits::value_type;
        using pointer = typename Iterator_traits::pointer;
        using reference = typename Iterator_traits::reference;
        using iterator_category = typename Iterator_traits::iterator_category;

        cache_iterator() = default;
        cache_iterator(const cache_iterator& it) = default;
        cache_iterator(cache_iterator&&) = default;
        cache_iterator(Iterator_type it);
        
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
        Iterator_type m_it;
    };

    struct iterator_traits final {
        using difference_type = long;
        using value_type = key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
    };
    using iterator = cache_iterator<cache_type::iterator, iterator_traits>;

    struct const_iterator_traits final {
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
    cache(std::shared_ptr<Allocator> memory_allocator);
    
    auto operator=(const cache&) -> cache& = default;
    auto operator=(cache&&) -> cache& = default;
    
    const key_value& read(const bytes& key) const;
    void write(key_value kv);
    bool contains(const bytes& key) const;
    void erase(const bytes& key);
    void clear();

    template <typename Iterable>
    const std::pair<std::vector<key_value>, std::unordered_set<bytes>> read(const Iterable& keys) const;

    template <typename Iterable>
    void write(const Iterable& key_values);

    template <typename Iterable>
    void erase(const Iterable& keys);

    template <typename Data_store, typename Iterable>
    void write_to(Data_store& ds, const Iterable& keys) const;

    template <typename Data_store, typename Iterable>
    void read_from(const Data_store& ds, const Iterable& keys);
    
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
    
    const std::shared_ptr<Allocator>& memory_allocator();
    const std::shared_ptr<const Allocator> memory_allocator() const;

private:
    template <typename On_cache_hit, typename On_cache_miss>
    const key_value& find_(const bytes& key, const On_cache_hit& cache_hit, const On_cache_miss& cache_miss) const;
    
private:
    std::shared_ptr<Allocator> m_allocator;
    cache_type m_cache;
};

template <typename Allocator>
cache<Allocator> make_cache(std::shared_ptr<Allocator> a) {
    return {std::move(a)};
}

template <typename Allocator>
cache<Allocator>::cache(std::shared_ptr<Allocator> memory_allocator)
: m_allocator{std::move(memory_allocator)} {
}

template <typename Allocator>
const std::shared_ptr<Allocator>& cache<Allocator>::memory_allocator() {
    return m_allocator;
}

template <typename Allocator>
const std::shared_ptr<const Allocator> cache<Allocator>::memory_allocator() const {
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
template <typename Allocator>
template <typename on_cache_hit, typename on_cache_miss>
const key_value& cache<Allocator>::find_(const bytes& key, const on_cache_hit& cache_hit, const on_cache_miss& cache_miss) const {
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

template <typename Allocator>
const key_value& cache<Allocator>::read(const bytes& key) const {
    return find_(key, [](auto& it){ return true; }, [](){});
}

template <typename Allocator>
void cache<Allocator>::write(key_value kv) {
    auto it = m_cache.find(kv.key());
    if (it == std::end(m_cache)) {
        auto key = kv.key();
        m_cache.emplace(std::move(key), std::move(kv));
        return;
    }
    it->second = std::move(kv);
}

template <typename Allocator>
bool cache<Allocator>::contains(const bytes& key) const {
    return find_(key, [](auto& it){ return true; }, [](){}) != key_value::invalid;
}

template <typename Allocator>
void cache<Allocator>::erase(const bytes& key) {
    find_(key, [&](auto& it){ m_cache.erase(it); return false; }, [](){});
}

template <typename Allocator>
void cache<Allocator>::clear() {
    m_cache.clear();
}

// Reads a batch of keys from the cache.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An Iterable instance that returns bytes instances in its iterator.
// \returns An std::pair where the first item is list of the found key_values and the second item is a set of the keys not found.
template <typename Allocator>
template <typename Iterable>
const std::pair<std::vector<key_value>, std::unordered_set<bytes>> cache<Allocator>::read(const Iterable& keys) const {
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
// \tparam Iterable Any type that can be used within a range based for loop and returns key_value instances in its iterator.
// \param key_values An Iterable instance that returns key_value instances in its iterator.
template <typename Allocator>
template <typename Iterable>
void cache<Allocator>::write(const Iterable& key_values) {
    for (const auto& kv : key_values) {
        write(kv);
    }
}

// Erases a batch of key_values from the cache.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An Iterable instance that returns bytes instances in its iterator.
template <typename Allocator>
template <typename Iterable>
void cache<Allocator>::erase(const Iterable& keys) {
    for (const auto& key : keys) {
        find_(key, [&](auto& it){ m_cache.erase(it); return false; }, [](){});
    }
}

// Writes a batch of key_values from this cache into the given data_store instance.
//
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that concept.
// \tparam Iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param ds A data store instance.
// \param keys An Iterable instance that returns bytes instances in its iterator.
template <typename Allocator>
template <typename Data_store, typename Iterable>
void cache<Allocator>::write_to(Data_store& ds, const Iterable& keys) const {
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
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that concept.
// \tparam Iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param ds A data store instance.
// \param keys An Iterable instance that returns bytes instances in its iterator.
template <typename Allocator>
template <typename Data_store, typename Iterable>
void cache<Allocator>::read_from(const Data_store& ds, const Iterable& keys) {
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

template <typename Allocator>
typename cache<Allocator>::iterator cache<Allocator>::find(const bytes& key) {
    return {m_cache.find(key)};
}

template <typename Allocator>
typename cache<Allocator>::const_iterator cache<Allocator>::find(const bytes& key) const {
    return {m_cache.find(key)};
}

template <typename Allocator>
typename cache<Allocator>::iterator cache<Allocator>::begin() {
    return {std::begin(m_cache)};
}

template <typename Allocator>
typename cache<Allocator>::const_iterator cache<Allocator>::begin() const {
    return {std::begin(m_cache)};
}

template <typename Allocator>
typename cache<Allocator>::iterator cache<Allocator>::end() {
    return {std::end(m_cache)};
}

template <typename Allocator>
typename cache<Allocator>::const_iterator cache<Allocator>::end() const {
    return {std::end(m_cache)};
}

template <typename Allocator>
typename cache<Allocator>::iterator cache<Allocator>::lower_bound(const bytes& key) {
    return {m_cache.lower_bound(key)};
}

template <typename Allocator>
typename cache<Allocator>::const_iterator cache<Allocator>::lower_bound(const bytes& key) const {
    return {m_cache.lower_bound(key)};
}

template <typename Allocator>
typename cache<Allocator>::iterator cache<Allocator>::upper_bound(const bytes& key) {
    return {m_cache.upper_bound(key)};
}

template <typename Allocator>
typename cache<Allocator>::const_iterator cache<Allocator>::upper_bound(const bytes& key) const {
    return {m_cache.upper_bound(key)};
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::cache_iterator(Iterator_type it)
: m_it{std::move(it)} {
}

template <typename Allocator, typename Iterator_type, typename Iterator_traits>
using cache_iterator_alias = typename cache<Allocator>::template cache_iterator<Iterator_type, Iterator_traits>;

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Allocator, Iterator_type, Iterator_traits>& cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator++() {
    ++m_it;
    return *this;
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Allocator, Iterator_type, Iterator_traits> cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator++(int) {
    return {m_it++};
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Allocator, Iterator_type, Iterator_traits>& cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator--() {
    --m_it;
    return *this;
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Allocator, Iterator_type, Iterator_traits> cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator--(int) {
    return {m_it--};
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
typename cache_iterator_alias<Allocator, Iterator_type, Iterator_traits>::reference cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator*() const {
    return m_it->second;
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
typename cache_iterator_alias<Allocator, Iterator_type, Iterator_traits>::reference cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator->() const {
    return m_it->second;
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
bool cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator==(const cache_iterator& other) const {
    return m_it == other.m_it;
}

template <typename Allocator>
template <typename Iterator_type, typename Iterator_traits>
bool cache<Allocator>::cache_iterator<Iterator_type, Iterator_traits>::operator!=(const cache_iterator& other) const {
    return !(*this == other);
}

}
