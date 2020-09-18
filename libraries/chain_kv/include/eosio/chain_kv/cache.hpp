#pragma once

#include <map>
#include <unordered_set>
#include <vector>

#include <chain_kv/shared_bytes.hpp>

namespace eosio::chain_kv {

// An in memory caching data store for storing key_value types.
//
// \remarks This type implements the "data store" concept.
class cache {
 private:
   // Currently there are some constraints that require the use of a std::map.
   // Namely, we need to be able to iterator over the entries in lexigraphical ordering on the keys.
   // It also provides us with lower bound and upper bound out of the box.
   using cache_type = std::map<shared_bytes, shared_bytes>;

 public:
   template <typename Iterator_type, typename Iterator_traits>
   class cache_iterator final {
    public:
      using difference_type   = typename Iterator_traits::difference_type;
      using value_type        = typename Iterator_traits::value_type;
      using pointer           = typename Iterator_traits::pointer;
      using reference         = typename Iterator_traits::reference;
      using iterator_category = typename Iterator_traits::iterator_category;

      cache_iterator()                         = default;
      cache_iterator(const cache_iterator& it) = default;
      cache_iterator(cache_iterator&&)         = default;
      cache_iterator(Iterator_type it);

      cache_iterator& operator=(const cache_iterator& it) = default;
      cache_iterator& operator=(cache_iterator&&) = default;

      cache_iterator& operator++();
      cache_iterator  operator++(int);
      cache_iterator& operator--();
      cache_iterator  operator--(int);
      value_type      operator*() const;
      value_type      operator->() const;
      bool            operator==(const cache_iterator& other) const;
      bool            operator!=(const cache_iterator& other) const;

    private:
      Iterator_type m_it;
   };

   struct iterator_traits final {
      using difference_type   = long;
      using value_type        = std::pair<shared_bytes, shared_bytes>;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::bidirectional_iterator_tag;
   };
   using iterator = cache_iterator<cache_type::iterator, iterator_traits>;

   struct const_iterator_traits final {
      using difference_type   = long;
      using value_type        = const std::pair<shared_bytes, shared_bytes>;
      using pointer           = const value_type*;
      using reference         = const value_type&;
      using iterator_category = std::bidirectional_iterator_tag;
   };
   using const_iterator = cache_iterator<cache_type::const_iterator, const_iterator_traits>;

 public:
   cache()             = default;
   cache(const cache&) = default;
   cache(cache&&)      = default;

   cache& operator=(const cache&) = default;
   cache& operator=(cache&&) = default;

   const shared_bytes& read(const shared_bytes& key) const;
   void                write(const shared_bytes& key, const shared_bytes& value);
   bool                contains(const shared_bytes& key) const;
   void                erase(const shared_bytes& key);
   void                clear();

   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read(const Iterable& keys) const;

   template <typename Iterable>
   void write(const Iterable& key_values);

   template <typename Iterable>
   void erase(const Iterable& keys);

   template <typename Data_store, typename Iterable>
   void write_to(Data_store& ds, const Iterable& keys) const;

   template <typename Data_store, typename Iterable>
   void read_from(const Data_store& ds, const Iterable& keys);

   iterator       find(const shared_bytes& key);
   const_iterator find(const shared_bytes& key) const;
   iterator       begin();
   const_iterator begin() const;
   iterator       end();
   const_iterator end() const;
   iterator       lower_bound(const shared_bytes& key);
   const_iterator lower_bound(const shared_bytes& key) const;
   iterator       upper_bound(const shared_bytes& key);
   const_iterator upper_bound(const shared_bytes& key) const;

 private:
   template <typename On_cache_hit, typename On_cache_miss>
   const shared_bytes& find_(const shared_bytes& key, const On_cache_hit& cache_hit,
                             const On_cache_miss& cache_miss) const;

 private:
   cache_type m_cache;
};

// Searches the cache for the given key.
//
// \tparam on_cache_hit A functor to invoke when the key is found.  It has the following signature
// void(cache_type::iterator& it) \tparam on_cache_miss A functor to invoke when the key is not found.  It has the
// following singature void(void). \param key The key to search the cache for. \param cache_hit The functor to invoke
// when the key is found. \param cache_miss The functor to invoke when the key is not found. \return A reference to the
// value shared_bytes instance if found, shared_bytes::invalid otherwise.
template <typename on_cache_hit, typename on_cache_miss>
const shared_bytes& cache::find_(const shared_bytes& key, const on_cache_hit& cache_hit,
                                 const on_cache_miss& cache_miss) const {
   auto it = m_cache.find(key);

   if (it == std::end(m_cache)) {
      cache_miss();
      return shared_bytes::invalid();
   }

   if (cache_hit(it)) {
      return it->second;
   }

   return shared_bytes::invalid();
}

inline const shared_bytes& cache::read(const shared_bytes& key) const {
   return find_(
         key, [](auto& it) { return true; }, []() {});
}

inline void cache::write(const shared_bytes& key, const shared_bytes& value) {
   auto it = m_cache.find(key);
   if (it == std::end(m_cache)) {
      m_cache.emplace(std::move(key), std::move(value));
      return;
   }
   it->second = std::move(value);
}

inline bool cache::contains(const shared_bytes& key) const {
   return find_(
                key, [](auto& it) { return true; }, []() {}) != shared_bytes::invalid();
}

inline void cache::erase(const shared_bytes& key) {
   find_(
         key,
         [&](auto& it) {
            m_cache.erase(it);
            return false;
         },
         []() {});
}

inline void cache::clear() { m_cache.clear(); }

// Reads a batch of keys from the cache.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An Iterable instance that returns shared_bytes instances in its iterator. \returns An std::pair
// where the first item is collection of the found key/valuess and the second item is a set of the keys not found.
template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
cache::read(const Iterable& keys) const {
   auto not_found = std::unordered_set<shared_bytes>{};
   auto kvs       = std::vector<std::pair<shared_bytes, shared_bytes>>{};

   for (const auto& key : keys) {
      auto& value = find_(
            key, [](auto& it) { return true; }, []() {});
      if (value == shared_bytes::invalid()) {
         not_found.emplace(key);
         continue;
      }
      kvs.emplace_back(key, value);
   }

   return { std::move(kvs), std::move(not_found) };
}

// Writes a batch of key/value pairs to the cache.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns an std::pair holding a key and a
// value in its iterator. \param key_values An Iterable instance that returns std::pair holding a key and a value in its
// iterator.
template <typename Iterable>
void cache::write(const Iterable& key_values) {
   for (const auto& kv : key_values) { write(kv.first, kv.second); }
}

// Erases a batch of key/value pairs from the cache.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An Iterable instance that returns shared_bytes instances in its iterator.
template <typename Iterable>
void cache::erase(const Iterable& keys) {
   for (const auto& key : keys) {
      find_(
            key,
            [&](auto& it) {
               m_cache.erase(it);
               return false;
            },
            []() {});
   }
}

// Writes a batch of key/value pairs from this cache into the given data_store instance.
//
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that
// concept. \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances
// in its iterator. \param ds A data store instance. \param keys An Iterable instance that returns shared_bytes
// instances in its iterator.
template <typename Data_store, typename Iterable>
void cache::write_to(Data_store& ds, const Iterable& keys) const {
   auto kvs = std::vector<std::pair<shared_bytes, shared_bytes>>{};

   for (const auto& key : keys) {
      auto& value = find_(
            key, [](auto& it) { return true; }, []() {});
      if (value == shared_bytes::invalid()) {
         continue;
      }
      kvs.emplace_back(std::pair{ key, value });
   }

   ds.write(kvs);
}

// Reads a batch of key/value pairs from the given data store into the cache.
//
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that
// concept. \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances
// in its iterator. \param ds A data store instance. \param keys An Iterable instance that returns shared_bytes
// instances in its iterator.
template <typename Data_store, typename Iterable>
void cache::read_from(const Data_store& ds, const Iterable& keys) {
   auto kvs = std::vector<std::pair<shared_bytes, shared_bytes>>{};

   for (const auto& key : keys) {
      auto& value = ds.read(key);
      if (value == shared_bytes::invalid()) {
         continue;
      }
      kvs.emplace_back(std::pair{ key, value });
   }

   write(kvs);

   return;
}

inline typename cache::iterator cache::find(const shared_bytes& key) { return { m_cache.find(key) }; }

inline typename cache::const_iterator cache::find(const shared_bytes& key) const { return { m_cache.find(key) }; }

inline typename cache::iterator cache::begin() { return { std::begin(m_cache) }; }

inline typename cache::const_iterator cache::begin() const { return { std::begin(m_cache) }; }

inline typename cache::iterator cache::end() { return { std::end(m_cache) }; }

inline typename cache::const_iterator cache::end() const { return { std::end(m_cache) }; }

inline typename cache::iterator cache::lower_bound(const shared_bytes& key) { return { m_cache.lower_bound(key) }; }

inline typename cache::const_iterator cache::lower_bound(const shared_bytes& key) const {
   return { m_cache.lower_bound(key) };
}

inline typename cache::iterator cache::upper_bound(const shared_bytes& key) { return { m_cache.upper_bound(key) }; }

inline typename cache::const_iterator cache::upper_bound(const shared_bytes& key) const {
   return { m_cache.upper_bound(key) };
}

template <typename Iterator_type, typename Iterator_traits>
cache::cache_iterator<Iterator_type, Iterator_traits>::cache_iterator(Iterator_type it) : m_it{ std::move(it) } {}

template <typename Iterator_type, typename Iterator_traits>
using cache_iterator_alias = typename cache::template cache_iterator<Iterator_type, Iterator_traits>;

template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Iterator_type, Iterator_traits>&
cache::cache_iterator<Iterator_type, Iterator_traits>::operator++() {
   ++m_it;
   return *this;
}

template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Iterator_type, Iterator_traits>
cache::cache_iterator<Iterator_type, Iterator_traits>::operator++(int) {
   return { m_it++ };
}

template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Iterator_type, Iterator_traits>&
cache::cache_iterator<Iterator_type, Iterator_traits>::operator--() {
   --m_it;
   return *this;
}

template <typename Iterator_type, typename Iterator_traits>
cache_iterator_alias<Iterator_type, Iterator_traits>
cache::cache_iterator<Iterator_type, Iterator_traits>::operator--(int) {
   return { m_it-- };
}

template <typename Iterator_type, typename Iterator_traits>
typename cache_iterator_alias<Iterator_type, Iterator_traits>::value_type
cache::cache_iterator<Iterator_type, Iterator_traits>::operator*() const {
   return std::pair{ m_it->first, m_it->second };
}

template <typename Iterator_type, typename Iterator_traits>
typename cache_iterator_alias<Iterator_type, Iterator_traits>::value_type
cache::cache_iterator<Iterator_type, Iterator_traits>::operator->() const {
   return std::pair{ m_it->first, m_it->second };
}

template <typename Iterator_type, typename Iterator_traits>
bool cache::cache_iterator<Iterator_type, Iterator_traits>::operator==(const cache_iterator& other) const {
   return m_it == other.m_it;
}

template <typename Iterator_type, typename Iterator_traits>
bool cache::cache_iterator<Iterator_type, Iterator_traits>::operator!=(const cache_iterator& other) const {
   return !(*this == other);
}

} // namespace eosio::session
