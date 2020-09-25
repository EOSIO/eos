#pragma once

#include <queue>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <b1/session/cache.hpp>
#include <b1/session/shared_bytes.hpp>

namespace eosio::session {

template <class... Ts>
struct overloaded : Ts... {
   using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Defines a session for reading/write data to a cache and persistent data store.
//
// \tparam Parent The parent of this session
template <typename Parent>
class session {
 private:
   struct session_impl;

 public:
   struct iterator_state {
      bool next_in_cache{ false };
      bool previous_in_cache{ false };
      bool deleted{ false };
   };

   using parent_type           = std::variant<session*, Parent*>;
   using cache_data_store_type = cache;
   using iterator_cache_type   = std::map<shared_bytes, iterator_state>;

   friend Parent;

   // Defines a key ordered, cyclical iterator for traversing a session (which includes parents and children of each
   // session). Basically we are iterating over a list of sessions and each session has its own cache of key/value
   // pairs, along with iterating over a persistent data store all the while maintaining key order with the iterator.
   template <typename Iterator_traits>
   class session_iterator {
    public:
      using difference_type   = typename Iterator_traits::difference_type;
      using value_type        = typename Iterator_traits::value_type;
      using pointer           = typename Iterator_traits::pointer;
      using reference         = typename Iterator_traits::reference;
      using iterator_category = typename Iterator_traits::iterator_category;
      friend session;

    public:
      session_iterator()                           = default;
      session_iterator(const session_iterator& it) = default;
      session_iterator(session_iterator&&)         = default;

      session_iterator& operator=(const session_iterator& it) = default;
      session_iterator& operator=(session_iterator&&) = default;

      session_iterator& operator++();
      session_iterator& operator--();
      value_type        operator*() const;
      value_type        operator->() const;
      bool              operator==(const session_iterator& other) const;
      bool              operator!=(const session_iterator& other) const;

      bool deleted() const;

    protected:
      template <typename Test_predicate, typename Move_predicate>
      void move_(const Test_predicate& test, const Move_predicate& move);
      void move_next_();
      void move_previous_();

    private:
      typename Iterator_traits::iterator_cache_iterator m_active_iterator;
      session*                                          m_active_session{ nullptr };
   };

   struct iterator_traits {
      using difference_type         = std::ptrdiff_t;
      using value_type              = std::pair<shared_bytes, shared_bytes>;
      using pointer                 = value_type*;
      using reference               = value_type&;
      using iterator_category       = std::bidirectional_iterator_tag;
      using iterator_cache_iterator = typename iterator_cache_type::const_iterator;
   };
   using iterator = session_iterator<iterator_traits>;

   struct const_iterator_traits {
      using difference_type         = std::ptrdiff_t;
      using value_type              = const std::pair<shared_bytes, shared_bytes>;
      using pointer                 = value_type*;
      using reference               = value_type&;
      using iterator_category       = std::bidirectional_iterator_tag;
      using iterator_cache_iterator = typename iterator_cache_type::const_iterator;
   };
   using const_iterator = session_iterator<const_iterator_traits>;

 public:
   session() = default;
   session(Parent& parent);
   session(session& parent);
   session(const session&) = delete;
   session(session&& other);
   ~session();

   session& operator=(const session&) = delete;
   session& operator                  =(session&& other);

   parent_type parent() const;

   const std::unordered_set<shared_bytes>& updated_keys() const;
   const std::unordered_set<shared_bytes>& deleted_keys() const;

   template <typename Other_parent>
   void attach(Other_parent& parent);
   void detach();

   // undo stack operations
   void undo();
   void commit();

   // this part also identifies a concept.  don't know what to call it yet
   shared_bytes read(const shared_bytes& key) const;
   void         write(const shared_bytes& key, const shared_bytes& value);
   bool         contains(const shared_bytes& key) const;
   void         erase(const shared_bytes& key);
   void         clear();
   bool         is_deleted(const shared_bytes& key) const;

   // returns a pair containing the key/value pairs found and the keys not found.
   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read(const Iterable& keys) const;

   template <typename Iterable>
   void write(const Iterable& key_values);

   // returns a list of the keys not found.
   template <typename Iterable>
   void erase(const Iterable& keys);

   template <typename Other_data_store, typename Iterable>
   void write_to(Other_data_store& ds, const Iterable& keys) const;

   template <typename Other_data_store, typename Iterable>
   void read_from(const Other_data_store& ds, const Iterable& keys);

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
   template <typename Iterator_type, typename Predicate, typename Comparator, typename Move>
   Iterator_type make_iterator_(const Predicate& predicate, const Comparator& comparator, const Move& move,
                                bool prime_cache_only = false) const;

   void                                  prime_cache_();
   std::pair<shared_bytes, shared_bytes> bounds_(const shared_bytes& key) const;

   struct iterator_cache_params {
      bool prime_only{ false };
      bool mark_deleted{ false };
      bool overwrite{ true };
   };
   void update_iterator_cache_(const shared_bytes& key, const iterator_cache_params& params) const;

 private:
   parent_type m_parent{ nullptr };

   // The cache used by this session instance.  This will include all new/update key/value pairs
   // and may include values read from the persistent data store.
   mutable eosio::session::cache m_cache;

   // Indicates if the next/previous key in lexicographical order for a given key exists in the cache.
   // The first value in the pair indicates if the previous key in lexicographical order exists in the cache.
   // The second value in the pair indicates if the next key lexicographical order exists in the cache.
   mutable iterator_cache_type m_iterator_cache;

   // keys that have been updated during this session.
   mutable std::unordered_set<shared_bytes> m_updated_keys;

   // keys that have been deleted during this session.
   mutable std::unordered_set<shared_bytes> m_deleted_keys;
};

template <typename Parent>
typename session<Parent>::parent_type session<Parent>::parent() const {
   return m_parent;
}

template <typename Parent>
void session<Parent>::prime_cache_() {
   m_iterator_cache.clear();

   auto keys_to_remove = std::set<shared_bytes>{};
   for (const auto& kv : m_cache) {
      auto key     = kv.first;
      auto updated = m_updated_keys.find(key);
      if (updated == std::end(m_updated_keys)) {
         keys_to_remove.emplace(key);
      }
   }
   if (!keys_to_remove.empty()) {
      m_cache.erase(keys_to_remove);
   }

   auto key = shared_bytes::invalid();
   auto end = [&](auto& ds) {
      auto end = std::end(ds);
      auto it  = end;
      if (it == std::begin(ds)) {
         return it;
      }
      if ((*(--it)).first > key) {
         key = (*it).first;
      }
      return end;
   };
   make_iterator_<iterator>(
         end, std::greater<shared_bytes>{}, [](auto& it, const auto& end) { ++it; }, true);
   if (key != shared_bytes::invalid()) {
      m_iterator_cache.try_emplace(key, iterator_state{});
   }
   key        = shared_bytes::invalid();
   auto begin = [&](auto& ds) {
      auto end = std::end(ds);
      auto it  = std::begin(ds);
      if (it == end) {
         return it;
      }
      if ((*it).first < key) {
         key = (*it).first;
      }
      return end;
   };
   make_iterator_<iterator>(
         begin, std::less<shared_bytes>{}, [](auto& it, const auto& end) { ++it; }, true);
   if (key != shared_bytes::invalid()) {
      m_iterator_cache.try_emplace(key, iterator_state{});
   }
}

template <typename Parent>
void session<Parent>::clear() {
   m_deleted_keys.clear();
   m_updated_keys.clear();
   m_cache.clear();
   m_iterator_cache.clear();
}

template <typename Parent>
session<Parent>::session(Parent& parent) : m_parent{ &parent } {
   attach(parent);
}

template <typename Parent>
session<Parent>::session(session& parent) : m_parent{ &parent } {
   attach(parent);
}

template <typename Parent>
session<Parent>::session(session&& other)
    : m_parent{ std::move(other.m_parent) }, m_cache{ std::move(other.m_cache) }, m_iterator_cache{ std::move(
                                                                                        other.m_iterator_cache) },
      m_updated_keys{ std::move(other.m_updated_keys) }, m_deleted_keys{ std::move(other.m_deleted_keys) } {
   session* null_parent = nullptr;
   other.m_parent       = null_parent;
}

template <typename Parent>
session<Parent>& session<Parent>::operator=(session&& other) {
   if (this == &other) {
      return *this;
   }

   m_parent         = std::move(other.m_parent);
   m_cache          = std::move(other.m_cache);
   m_iterator_cache = std::move(other.m_iterator_cache);
   m_updated_keys   = std::move(other.m_updated_keys);
   m_deleted_keys   = std::move(other.m_deleted_keys);

   session* null_parent = nullptr;
   other.m_parent       = null_parent;

   return *this;
}

template <typename Parent>
session<Parent>::~session() {
   commit();
   undo();
}

template <typename Parent>
void session<Parent>::undo() {
   detach();
   clear();
}

template <typename Parent>
std::pair<shared_bytes, shared_bytes> session<Parent>::bounds_(const shared_bytes& key) const {
   auto lower_bound_key = shared_bytes::invalid();
   auto upper_bound_key = shared_bytes::invalid();

   // We need to be careful about requesting iterators on the session since it will be rentrant into this
   // method.  Ask for the iterator in a way that isn't reentrant.  We can do this by invoking the make_iterator_
   // method directly.

   auto lower = [&](auto& ds) {
      auto it    = ds.lower_bound(key);
      auto begin = std::begin(ds);
      auto end   = std::end(ds);
      if (it == begin && it == end) {
         // No bounds for this key;
         return end;
      }

      auto test_set = [](const auto& pending, auto& current, const auto& comparator) {
         if (!pending) {
            return;
         }
         if (!current || comparator(pending, current)) {
            current = pending;
         }
      };

      if (it == begin) {
         // There is no lower bound for this key, but there might be an upper bound.
         if ((*it).first == key) {
            ++it;
            end = std::end(ds);
            if (it == std::begin(ds) || it == end) {
               // We wrapped around.  These are circular iterators
               return end;
            }
         }
         test_set((*it).first, upper_bound_key, std::less<shared_bytes>{});
      } else if (it == end) {
         // There are no keys greater than or equal to the given key, but there might be a lower bound.
         --it;
         test_set((*it).first, lower_bound_key, std::greater<shared_bytes>{});
      } else if ((*it).first == key) {
         test_set((*(--it)).first, lower_bound_key, std::greater<shared_bytes>{});
         ++(++it);
         end = std::end(ds);
         if (it == std::begin(ds) || it == end) {
            // We wrapped around. These are circular iterators.
            return end;
         }
         test_set((*it).first, upper_bound_key, std::less<shared_bytes>{});
      } else {
         test_set((*it).first, upper_bound_key, std::less<shared_bytes>{});
         test_set((*(--it)).first, lower_bound_key, std::greater<shared_bytes>{});
      }

      return std::end(ds);
   };
   make_iterator_<iterator>(
         lower, std::less<shared_bytes>{}, [](auto& it, const auto& end) { ++it; }, true);

   return std::pair{ lower_bound_key, upper_bound_key };
}

template <typename Parent>
void session<Parent>::update_iterator_cache_(const shared_bytes& key, const iterator_cache_params& params) const {
   auto result = m_iterator_cache.try_emplace(key, iterator_state{});

   auto& it = result.first;

   if (params.overwrite) {
      // Force an overwrite
      it->second.deleted = params.mark_deleted;
   }

   if (params.prime_only) {
      // We only want to make sure the key exists in the iterator cache.
      return;
   }

   auto [lower_bound, upper_bound] = bounds_(key);

   if (lower_bound != shared_bytes::invalid()) {
      auto lower_it                        = m_iterator_cache.try_emplace(lower_bound, iterator_state{});
      lower_it.first->second.next_in_cache = true;
      it->second.previous_in_cache         = true;
   }

   if (upper_bound != shared_bytes::invalid()) {
      auto upper_it                            = m_iterator_cache.try_emplace(upper_bound, iterator_state{});
      upper_it.first->second.previous_in_cache = true;
      it->second.next_in_cache                 = true;
   }
}

template <typename Parent>
const typename std::unordered_set<shared_bytes>& session<Parent>::updated_keys() const {
   return m_updated_keys;
}

template <typename Parent>
const typename std::unordered_set<shared_bytes>& session<Parent>::deleted_keys() const {
   return m_deleted_keys;
}

template <typename Parent>
template <typename Other_parent>
void session<Parent>::attach(Other_parent& parent) {
   m_parent = &parent;
   prime_cache_();
}

template <typename Parent>
void session<Parent>::detach() {
   session* null_parent = nullptr;
   m_parent             = null_parent;
}

template <typename Parent>
void session<Parent>::commit() {
   if (m_updated_keys.empty() && m_deleted_keys.empty()) {
      // Nothing to commit.
      return;
   }

   auto write_through = [&](auto& ds) {
      ds.erase(m_deleted_keys);
      m_cache.write_to(ds, m_updated_keys);
      clear();
   };

   std::visit(
         [&](auto* p) {
            if (!p) {
               return;
            }
            write_through(*p);
         },
         m_parent);
}

template <typename Parent>
shared_bytes session<Parent>::read(const shared_bytes& key) const {
   // Find the key within the session.
   // Check this level first and then traverse up to the parent to see if this key/value
   // has been read and/or update.
   if (m_deleted_keys.find(key) != std::end(m_deleted_keys)) {
      // key has been deleted at this level.
      return shared_bytes::invalid();
   }

   auto value = m_cache.read(key);
   if (value != shared_bytes::invalid()) {
      return value;
   }

   std::visit(
         [&](auto* p) {
            if (p) {
               value = p->read(key);
            }
         },
         m_parent);

   if (value != shared_bytes::invalid()) {
      m_cache.write(key, value);
      update_iterator_cache_(key, { .prime_only = true, .mark_deleted = false, .overwrite = false });
   }

   return value;
}

template <typename Parent>
void session<Parent>::write(const shared_bytes& key, const shared_bytes& value) {
   m_updated_keys.emplace(key);
   m_deleted_keys.erase(key);
   m_cache.write(key, value);
   update_iterator_cache_(key, { .prime_only = true, .mark_deleted = false, .overwrite = true });
}

template <typename Parent>
bool session<Parent>::contains(const shared_bytes& key) const {
   // Traverse the heirarchy to see if this session (and its parent session)
   // has already read the key into memory.

   if (m_deleted_keys.find(key) != std::end(m_deleted_keys)) {
      return false;
   }
   if (m_cache.find(key) != std::end(m_cache)) {
      return true;
   }

   return std::visit(
         [&](auto* p) {
            if (p && p->contains(key)) {
               update_iterator_cache_(key, { .prime_only = true, .mark_deleted = false, .overwrite = false });
               return true;
            }
            return false;
         },
         m_parent);
}

template <typename Parent>
void session<Parent>::erase(const shared_bytes& key) {
   m_deleted_keys.emplace(key);
   m_updated_keys.erase(key);
   m_cache.erase(key);
   update_iterator_cache_(key, { .prime_only = true, .mark_deleted = true, .overwrite = true });
}

// Reads a batch of keys from the session.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An iterable instance that returns shared_bytes instances in its iterator. \returns An std::pair
// where the first item is list of the found key/value pairs and the second item is a set of the keys not found.
template <typename Parent>
template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
session<Parent>::read(const Iterable& keys) const {
   auto not_found = std::unordered_set<shared_bytes>{};
   auto kvs       = std::vector<std::pair<shared_bytes, shared_bytes>>{};

   for (const auto& key : keys) {
      auto value = read(key);
      if (value != shared_bytes::invalid()) {
         kvs.emplace_back(key, value);
      } else {
         not_found.emplace(key);
      }
   }

   return { std::move(kvs), std::move(not_found) };
}

// Writes a batch of key/value pairs to the session.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns key/value pairs in its
// iterator. \param key_values An iterable instance that returns key/value pairs in its iterator.
template <typename Parent>
template <typename Iterable>
void session<Parent>::write(const Iterable& key_values) {
   // Currently the batch write will just iteratively call the non batch write
   for (const auto& kv : key_values) { write(kv.first, kv.second); }
}

// Erases a batch of key_values from the session.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An iterable instance that returns shared_bytes instances in its iterator.
template <typename Parent>
template <typename Iterable>
void session<Parent>::erase(const Iterable& keys) {
   // Currently the batch erase will just iteratively call the non batch erase
   for (const auto& key : keys) { erase(key); }
}

template <typename Parent>
template <typename Other_data_store, typename Iterable>
void session<Parent>::write_to(Other_data_store& ds, const Iterable& keys) const {
   auto results = std::vector<std::pair<shared_bytes, shared_bytes>>{};
   for (const auto& key : keys) {
      auto value = read(key);
      if (value != shared_bytes::invalid()) {
         results.emplace_back(make_shared_bytes(key.data(), key.size()), make_shared_bytes(value.data(), value.size()));
      }
   }
   ds.write(results);
}

template <typename Parent>
template <typename Other_data_store, typename Iterable>
void session<Parent>::read_from(const Other_data_store& ds, const Iterable& keys) {
   ds.write_to(*this, keys);
}

template <typename Parent>
bool session<Parent>::is_deleted(const shared_bytes& key) const {
   if (m_deleted_keys.find(key) != std::end(m_deleted_keys)) {
      return true;
   } else if (m_updated_keys.find(key) != std::end(m_updated_keys)) {
      return false;
   }

   return std::visit(
         [&](auto* p) {
            if (p) {
               return p->is_deleted(key);
            }
            return false;
         },
         m_parent);
}

// Factor for creating the initial session iterator.
//
// \tparam Predicate A functor used for getting the starting iterating in the cache list and the persistent data store.
// \tparam Comparator A functor for determining if the "current" iterator points to the cache list or the persistent
// data store.
template <typename Parent>
template <typename Iterator_type, typename Predicate, typename Comparator, typename Move>
Iterator_type session<Parent>::make_iterator_(const Predicate& predicate, const Comparator& comparator,
                                              const Move& move, bool prime_cache_only) const {
   // This method is the workhorse of the iterator and iterator cache updating.
   // It will search through all the session levels to find the best key that matches
   // the requested iterator.  Once that key has been identified, it will force an update
   // on the iterator cache by inserting that key and (possibly) the previous key from that key
   // and the next key from that key.

   auto new_iterator              = Iterator_type{};
   new_iterator.m_active_session  = const_cast<session*>(this);
   new_iterator.m_active_iterator = std::end(m_iterator_cache);

   auto deleted = [&](auto& key) { return is_deleted(key); };

   auto find_first_not = [&](auto& session, auto& it, const auto& predicate) {
      // Return the first key that passes the given predicate.
      if (it == std::end(session)) {
         return shared_bytes::invalid();
      }
      auto beginning_key = (*it).first;
      auto pending_key   = (*it).first;
      while (true) {
         if (!predicate(pending_key)) {
            return pending_key;
         }
         move(it, std::end(session));
         if (it == std::end(session) || it == std::begin(session)) {
            // These are circular iterators.  We might have wrapped around.
            return shared_bytes::invalid();
         }
         pending_key = (*it).first;
         if (pending_key < beginning_key) {
            // We've looped around.
            return shared_bytes::invalid();
         }
      }
   };

   auto current_key = shared_bytes::invalid();

   std::visit(
         [&](auto* p) {
            if (p) {
               // Get the iterator from the parent
               auto it = predicate(*p);
               if (it != std::end(*p)) {
                  current_key = (*it).first;
                  if (m_deleted_keys.find(current_key) != std::end(m_deleted_keys)) {
                     current_key = find_first_not(*p, it, deleted);
                  }
               }
            }
         },
         m_parent);

   // Get the iterator from this
   auto pending     = predicate(m_cache);
   auto pending_key = shared_bytes::invalid();
   if (pending != std::end(m_cache)) {
      pending_key = (*pending).first;
   }

   if (!current_key || (pending_key && comparator(pending_key, current_key))) {
      current_key = pending_key;
   }

   if (current_key) {
      // Update the iterator cache with this key.  It has to exist in the cache before
      // we can get an iterator to it.  In some cases, we will only be inserting the new
      // key and bypassing the search for its previous and next key.
      update_iterator_cache_(current_key,
                             { .prime_only = prime_cache_only, .mark_deleted = false, .overwrite = false });
      new_iterator.m_active_iterator = m_iterator_cache.find(current_key);

      if (new_iterator.m_active_iterator->second.deleted) {
         new_iterator.m_active_iterator = std::end(m_iterator_cache);
      }
   }

   return new_iterator;
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::find(const shared_bytes& key) {
   // This comparator just chooses the one that isn't invalid.
   static auto comparator = [](const auto& left, const auto& right) {
      if (!left && !right) {
         return true;
      }

      if (!left) {
         return false;
      }

      return true;
   };

   return make_iterator_<iterator>([&](auto& ds) { return ds.find(key); }, comparator,
                                   [](auto& it, const auto& end) { it = end; });
}

template <typename Parent>
typename session<Parent>::const_iterator session<Parent>::find(const shared_bytes& key) const {
   // This comparator just chooses the one that isn't invalid.
   static auto comparator = [](const auto& left, const auto& right) {
      if (!left && !right) {
         return true;
      }

      if (!left) {
         return false;
      }

      return true;
   };

   return make_iterator_<const_iterator>([&](auto& ds) { return ds.find(key); }, comparator,
                                         [](auto& it, const auto& end) { it = end; });
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::begin() {
   return make_iterator_<iterator>([](auto& ds) { return std::begin(ds); }, std::less<shared_bytes>{},
                                   [](auto& it, const auto& end) { ++it; });
}

template <typename Parent>
typename session<Parent>::const_iterator session<Parent>::begin() const {
   return make_iterator_<const_iterator>([](auto& ds) { return std::begin(ds); }, std::less<shared_bytes>{},
                                         [](auto& it, const auto& end) { ++it; });
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::end() {
   return make_iterator_<iterator>([](auto& ds) { return std::end(ds); }, std::greater<shared_bytes>{},
                                   [](auto& it, const auto& end) { it = end; });
}

template <typename Parent>
typename session<Parent>::const_iterator session<Parent>::end() const {
   return make_iterator_<const_iterator>([](auto& ds) { return std::end(ds); }, std::greater<shared_bytes>{},
                                         [](auto& it, const auto& end) { it = end; });
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::lower_bound(const shared_bytes& key) {
   return make_iterator_<iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::less<shared_bytes>{},
                                   [](auto& it, const auto& end) { ++it; });
}

template <typename Parent>
typename session<Parent>::const_iterator session<Parent>::lower_bound(const shared_bytes& key) const {
   return make_iterator_<const_iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::less<shared_bytes>{},
                                         [](auto& it, const auto& end) { ++it; });
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::upper_bound(const shared_bytes& key) {
   return make_iterator_<iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<shared_bytes>{},
                                   [](auto& it, const auto& end) { ++it; });
}

template <typename Parent>
typename session<Parent>::const_iterator session<Parent>::upper_bound(const shared_bytes& key) const {
   return make_iterator_<const_iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<shared_bytes>{},
                                         [](auto& it, const auto& end) { ++it; });
}

// Moves the current iterator.
//
// \tparam Predicate A functor that indicates if we are incrementing or decrementing the current iterator.
// \tparam Comparator A functor used to determine what the current iterator is.
template <typename Parent>
template <typename Iterator_traits>
template <typename Test_predicate, typename Move_predicate>
void session<Parent>::session_iterator<Iterator_traits>::move_(const Test_predicate& test, const Move_predicate& move) {
   do {
      if (m_active_iterator != std::end(m_active_session->m_iterator_cache) && !test(m_active_iterator)) {
         // Force an update to see if we pull in a next or previous key from the current key.
         m_active_session->update_iterator_cache_(m_active_iterator->first,
                                                  { .prime_only = false, .mark_deleted = false, .overwrite = false });
         if (!test(m_active_iterator)) {
            // The test still fails.  We are at the end.
            m_active_iterator = std::end(m_active_session->m_iterator_cache);
            break;
         }
      }
      // Move to the next iterator in the cache.
      move(m_active_iterator);
      if (m_active_iterator == std::end(m_active_session->m_iterator_cache) || !m_active_iterator->second.deleted) {
         // We either found a key that hasn't been deleted or we hit the end of the iterator cache.
         break;
      }
   } while (true);
}

template <typename Parent>
template <typename Iterator_traits>
void session<Parent>::session_iterator<Iterator_traits>::move_next_() {
   auto move = [](auto& it) { ++it; };
   auto test = [](auto& it) { return it->second.next_in_cache; };

   if (m_active_iterator == std::end(m_active_session->m_iterator_cache)) {
      m_active_iterator = std::begin(m_active_session->m_iterator_cache);
   } else {
      move_(test, move);
   }
}

template <typename Parent>
template <typename Iterator_traits>
void session<Parent>::session_iterator<Iterator_traits>::move_previous_() {
   auto move = [](auto& it) { --it; };
   auto test = [&](auto& it) {
      if (it != std::end(m_active_session->m_iterator_cache)) {
         return it->second.previous_in_cache;
      }
      return true;
   };
   auto rollover = [&]() {
      if (m_active_iterator == std::begin(m_active_session->m_iterator_cache)) {
         m_active_iterator = std::end(m_active_session->m_iterator_cache);
      }
   };

   rollover();
   move_(test, move);
}

template <typename Parent>
template <typename Iterator_traits>
typename session<Parent>::template session_iterator<Iterator_traits>&
session<Parent>::session_iterator<Iterator_traits>::operator++() {
   move_next_();
   return *this;
}

template <typename Parent>
template <typename Iterator_traits>
typename session<Parent>::template session_iterator<Iterator_traits>&
session<Parent>::session_iterator<Iterator_traits>::operator--() {
   move_previous_();
   return *this;
}

template <typename Parent>
template <typename Iterator_traits>
bool session<Parent>::session_iterator<Iterator_traits>::deleted() const {
   if (m_active_iterator == std::end(m_active_session->m_iterator_cache)) {
      return false;
   }
   return m_active_iterator->second.deleted;
}

template <typename Parent>
template <typename Iterator_traits>
typename session<Parent>::template session_iterator<Iterator_traits>::value_type
session<Parent>::session_iterator<Iterator_traits>::operator*() const {
   if (m_active_iterator == std::end(m_active_session->m_iterator_cache)) {
      return std::pair{ shared_bytes::invalid(), shared_bytes::invalid() };
   }
   return std::pair{ m_active_iterator->first, m_active_session->read(m_active_iterator->first) };
}

template <typename Parent>
template <typename Iterator_traits>
typename session<Parent>::template session_iterator<Iterator_traits>::value_type
session<Parent>::session_iterator<Iterator_traits>::operator->() const {
   if (m_active_iterator == std::end(m_active_session->m_iterator_cache)) {
      return std::pair{ shared_bytes::invalid(), shared_bytes::invalid() };
   }
   return std::pair{ m_active_iterator->first, m_active_session->read(m_active_iterator->first) };
}

template <typename Parent>
template <typename Iterator_traits>
bool session<Parent>::session_iterator<Iterator_traits>::operator==(const session_iterator& other) const {
   if (m_active_iterator == std::end(m_active_session->m_iterator_cache) &&
       m_active_iterator == other.m_active_iterator) {
      return true;
   }
   if (other.m_active_iterator == std::end(m_active_session->m_iterator_cache)) {
      return false;
   }
   return this->m_active_iterator == other.m_active_iterator;
}

template <typename Parent>
template <typename Iterator_traits>
bool session<Parent>::session_iterator<Iterator_traits>::operator!=(const session_iterator& other) const {
   return !(*this == other);
}

} // namespace eosio::session
