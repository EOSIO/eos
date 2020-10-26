#pragma once

#include <optional>
#include <queue>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

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
   struct value_state {
      bool         next_in_cache{ false };
      bool         previous_in_cache{ false };
      bool         deleted{ false };
      bool         updated{ false };
      uint64_t     version{ 0 };
      shared_bytes value;
   };

   using parent_type = std::variant<session*, Parent*>;
   using cache_type  = std::map<shared_bytes, value_state>;

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
      session_iterator(session* active_session, typename Iterator_traits::cache_iterator it, uint64_t version);
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

      bool                deleted() const;
      const shared_bytes& key() const;

    protected:
      template <typename Test_predicate, typename Move_predicate, typename Cache_update>
      void move_(const Test_predicate& test, const Move_predicate& move, Cache_update& update_cache);
      void move_next_();
      void move_previous_();

    private:
      uint64_t                                 m_iterator_version{ 0 };
      typename Iterator_traits::cache_iterator m_active_iterator;
      session*                                 m_active_session{ nullptr };
   };

   struct iterator_traits {
      using difference_type   = std::ptrdiff_t;
      using value_type        = std::pair<shared_bytes, std::optional<shared_bytes>>;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::bidirectional_iterator_tag;
      using cache_iterator    = typename cache_type::iterator;
   };
   using iterator = session_iterator<iterator_traits>;

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

   std::unordered_map<shared_bytes, bool> deltas() const;

   void attach(Parent& parent);
   void attach(session& parent);
   void detach();

   // undo stack operations
   void undo();
   void commit();

   // this part also identifies a concept.  don't know what to call it yet
   std::optional<shared_bytes> read(const shared_bytes& key) const;
   void                        write(const shared_bytes& key, const shared_bytes& value);
   bool                        contains(const shared_bytes& key) const;
   void                        erase(const shared_bytes& key);
   void                        clear();

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

   iterator find(const shared_bytes& key) const;
   iterator begin() const;
   iterator end() const;
   iterator lower_bound(const shared_bytes& key) const;

 private:
   void prime_cache_();
   template <typename It, typename Parent_it>
   void next_key_(It& it, Parent_it& pit, Parent_it& pend) const;
   template <typename It, typename Parent_it>
   void                          previous_key_(It& it, Parent_it& pit, Parent_it& pbegin, Parent_it& pend) const;
   typename cache_type::iterator update_iterator_cache_(const shared_bytes& key, bool deleted, bool overwrite) const;
   template <typename It>
   It& first_not_deleted_in_iterator_cache_(It& it, const It& end) const;

 private:
   parent_type m_parent{ nullptr };

   // Indicates if the next/previous key in lexicographical order for a given key exists in the cache.
   // The first value in the pair indicates if the previous key in lexicographical order exists in the cache.
   // The second value in the pair indicates if the next key lexicographical order exists in the cache.
   mutable cache_type m_cache;
};

template <typename Parent>
typename session<Parent>::parent_type session<Parent>::parent() const {
   return m_parent;
}

template <typename Parent>
void session<Parent>::prime_cache_() {
   // Get the bounds of the parent iterator cache.
   auto lower_key   = shared_bytes{};
   auto lower_value = std::optional<shared_bytes>{};
   auto upper_key   = shared_bytes{};
   auto upper_value = std::optional<shared_bytes>{};

   std::visit(overloaded{ [&](session<Parent>* p) {
                            auto begin = std::begin(p->m_cache);
                            auto end   = std::end(p->m_cache);
                            if (begin == end) {
                               return;
                            }
                            lower_key   = begin->first;
                            lower_value = begin->second.value;
                            upper_key   = (--end)->first;
                            upper_value = end->second.value;
                         },
                          [&](Parent* p) {
                             auto begin = std::begin(*p);
                             auto end   = std::end(*p);
                             if (begin == end) {
                                return;
                             }
                             lower_key   = begin.key();
                             lower_value = (*begin).second;
                             --end;
                             upper_key   = end.key();
                             upper_value = (*end).second;
                          } },
              m_parent);

   if (lower_key) {
      auto it = m_cache.emplace(lower_key, value_state{});
      if (it.second) {
         it.first->second.value = *lower_value;
      }
   }
   if (upper_key) {
      auto it = m_cache.emplace(upper_key, value_state{});
      if (it.second) {
         it.first->second.value = *upper_value;
      }
   }
}

template <typename Parent>
void session<Parent>::clear() {
   m_cache.clear();
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
session<Parent>::session(session&& other) : m_parent{ std::move(other.m_parent) }, m_cache{ std::move(other.m_cache) } {
   session* null_parent = nullptr;
   other.m_parent       = null_parent;
}

template <typename Parent>
session<Parent>& session<Parent>::operator=(session&& other) {
   if (this == &other) {
      return *this;
   }

   m_parent = std::move(other.m_parent);
   m_cache  = std::move(other.m_cache);

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
template <typename It, typename Parent_it>
void session<Parent>::previous_key_(It& it, Parent_it& pit, Parent_it& pbegin, Parent_it& pend) const {
   if (it->first) {
      auto find_previous_not_deleted_in_cache = [&](auto& key, auto& it, const auto& begin, const auto& end) {
         if (it == begin && it->first >= key) {
            it = end;
            return;
         }

         while (it != begin && (it->second.deleted || it->first >= key)) { --it; }

         if (it->second.deleted || it->first >= key) {
            it = end;
         }
      };

      if (pit != pbegin) {
         size_t decrement_count = 0;
         while (pit.key() >= it->first) {
            --pit;
            ++decrement_count;
            if (pit == pend) {
               break;
            }
         }

         if (pit != pend) {
            auto cit = m_cache.emplace(pit.key(), value_state{});
            if (cit.second) {
               cit.first->second.value = *(*pit).second;
            }
         }

         for (size_t i = 0; i < decrement_count; ++i) { ++pit; }
      }

      auto& it_cache = const_cast<cache_type&>(m_cache);
      auto  lower_it = it_cache.lower_bound(it->first);
      auto  end      = std::end(it_cache);
      auto  begin    = std::begin(it_cache);
      find_previous_not_deleted_in_cache(it->first, lower_it, begin, end);

      if (lower_it != end) {
         lower_it->second.next_in_cache = true;
         it->second.previous_in_cache   = true;
      }
   }
}

template <typename Parent>
template <typename It, typename Parent_it>
void session<Parent>::next_key_(It& it, Parent_it& pit, Parent_it& pend) const {
   if (it->first) {
      bool decrement = false;
      if (pit.key() == it->first) {
         ++pit;
         decrement = true;
      }
      if (pit != pend) {
         auto cit = m_cache.emplace(pit.key(), value_state{});
         if (cit.second) {
            cit.first->second.value = *(*pit).second;
         }
      }
      if (decrement) {
         --pit;
      }

      auto& it_cache = const_cast<cache_type&>(m_cache);
      auto  lower_it = it_cache.lower_bound(it->first);
      auto  end      = std::end(it_cache);
      first_not_deleted_in_iterator_cache_(lower_it, end);
      if (lower_it != end) {
         if ((*lower_it).first == it->first) {
            ++lower_it;
            first_not_deleted_in_iterator_cache_(lower_it, end);
         }
         if (lower_it != end) {
            lower_it->second.previous_in_cache = true;
            it->second.next_in_cache           = true;
         }
      }
   }
}

template <typename Parent>
typename std::unordered_map<shared_bytes, bool> session<Parent>::deltas() const {
   auto results = std::unordered_map<shared_bytes, bool>{};
   for (const auto& it : m_cache) {
      if (it.second.deleted) {
         results.emplace(it.first, false).first.second = true;
      } else if (it.second.updated) {
         results.emplace(it.first, false).first.second = false;
      }
   }
   return results;
}

template <typename Parent>
void session<Parent>::attach(Parent& parent) {
   m_parent = &parent;
   prime_cache_();
}

template <typename Parent>
void session<Parent>::attach(session& parent) {
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
   if (m_cache.empty()) {
      // Nothing to commit.
      return;
   }

   auto write_through = [&](auto& ds) {
      auto deletes = std::unordered_set<shared_bytes>{};
      auto updates = std::unordered_map<shared_bytes, shared_bytes>{};

      for (const auto& p : m_cache) {
         if (p.second.deleted) {
            deletes.emplace(p.first);
         } else if (p.second.updated) {
            updates.emplace(p.first, p.second.value);
         }
      }

      ds.erase(deletes);
      ds.write(updates);
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
typename session<Parent>::cache_type::iterator
session<Parent>::update_iterator_cache_(const shared_bytes& key, bool deleted, bool overwrite) const {
   auto  result = m_cache.emplace(key, value_state{});
   auto& it     = result.first;

   if (overwrite) {
      it->second.deleted = deleted;
      if (deleted) {
         ++it->second.version;
      }
   }

   if (result.second) {
      // The two keys that this new key is being inserted in between might already be in a global lexicographical order.
      // If so, that means we already know the global order of this new key.
      if (it != std::begin(m_cache)) {
         auto previous = it;
         --previous;
         if (previous->second.next_in_cache) {
            it->second.previous_in_cache = true;
            it->second.next_in_cache = true;
            return it;
         }
      }
      auto end = std::end(m_cache);
      if (it != end) {
         auto next = it;
         ++next;
         if (next != end && next->second.previous_in_cache) {
            it->second.next_in_cache = true;
            it->second.previous_in_cache = true;
            return it;
         }
      }

      if (!it->second.next_in_cache && !it->second.previous_in_cache) {
         std::visit(
               [&](auto* p) {
                  auto pit   = p->lower_bound(key);
                  auto end   = std::end(*p);
                  auto begin = std::begin(*p);
                  next_key_(it, pit, end);
                  previous_key_(it, pit, begin, end);
               },
               m_parent);
      } else if (!it->second.next_in_cache) {
         std::visit(
               [&](auto* p) {
                  auto pit = p->lower_bound(key);
                  auto end = std::end(*p);
                  next_key_(it, pit, end);
               },
               m_parent);
      } else if (!it->second.previous_in_cache) {
         std::visit(
               [&](auto* p) {
                  auto pit   = p->lower_bound(key);
                  auto end   = std::end(*p);
                  auto begin = std::begin(*p);
                  previous_key_(it, pit, begin, end);
               },
               m_parent);
      }
   }

   return it;
}

template <typename Parent>
std::optional<shared_bytes> session<Parent>::read(const shared_bytes& key) const {
   // Find the key within the session.
   // Check this level first and then traverse up to the parent to see if this key/value
   // has been read and/or update.
   auto it = m_cache.find(key);
   if (it != std::end(m_cache) && it->second.deleted) {
      // key has been deleted at this level.
      return {};
   }

   if (it != std::end(m_cache) && it->second.value) {
      return it->second.value;
   }

   auto value = std::optional<shared_bytes>{};
   std::visit(
         [&](auto* p) {
            if (p) {
               value = p->read(key);
            }
         },
         m_parent);

   if (value) {
      auto it          = update_iterator_cache_(key, false, false);
      it->second.value = *value;
   }

   return value;
}

template <typename Parent>
void session<Parent>::write(const shared_bytes& key, const shared_bytes& value) {
   auto it            = update_iterator_cache_(key, false, true);
   it->second.value   = value;
   it->second.deleted = false;
   it->second.updated = true;
}

template <typename Parent>
bool session<Parent>::contains(const shared_bytes& key) const {
   // Traverse the heirarchy to see if this session (and its parent session)
   // has already read the key into memory.

   auto it = m_cache.find(key);
   if (it != std::end(m_cache) && it->second.deleted) {
      // deleted.
      return false;
   }

   if (it != std::end(m_cache) && it->second.value) {
      return true;
   }

   return std::visit(
         [&](auto* p) {
            auto value = p->read(key);
            if (value) {
               auto it          = update_iterator_cache_(key, false, false);
               it->second.value = std::move(*value);
               return true;
            }
            return false;
         },
         m_parent);
}

template <typename Parent>
void session<Parent>::erase(const shared_bytes& key) {
   auto it            = update_iterator_cache_(key, true, true);
   it->second.deleted = true;
   it->second.updated = false;
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
      if (value != shared_bytes{}) {
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
      if (value != shared_bytes{}) {
         results.emplace_back(shared_bytes(key.data(), key.size()), shared_bytes(value.data(), value.size()));
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
template <typename It>
It& session<Parent>::first_not_deleted_in_iterator_cache_(It& it, const It& end) const {
   while (it != end && it->second.deleted) { ++it; }
   return it;
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::find(const shared_bytes& key) const {
   auto  version  = uint64_t{ 0 };
   auto& it_cache = const_cast<cache_type&>(m_cache);
   auto  end      = std::end(it_cache);
   auto  it       = it_cache.find(key);
   if (it == end) {
      std::visit(
            [&](auto* p) {
               auto pit = p->find(key);
               if (pit != std::end(*p)) {
                  auto result = it_cache.emplace(key, value_state{});
                  it          = result.first;
                  if (result.second) {
                     it->second.value = *(*pit).second;
                  }
               }
            },
            m_parent);
   }

   if (it != end) {
      version = it->second.version;
      if (it->second.deleted) {
         it = std::move(end);
      }
   }
   return { const_cast<session*>(this), std::move(it), version };
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::begin() const {
   auto& it_cache = const_cast<cache_type&>(m_cache);
   auto  end      = std::end(it_cache);
   auto  it       = std::begin(it_cache);
   auto  version  = uint64_t{ 0 };

   if (it != end && it->second.deleted && !it->second.next_in_cache) {
      std::visit(
            [&](auto* p) {
               auto pit  = std::begin(*p);
               auto pend = std::end(*p);
               if (pit != pend) {
                  auto result = it_cache.emplace(pit.key(), value_state{});
                  it          = result.first;
                  if (result.second) {
                     it->second.value = *(*pit).second;
                  }
                  first_not_deleted_in_iterator_cache_(it, end);
               }
            },
            m_parent);
   } else {
      first_not_deleted_in_iterator_cache_(it, end);
   }

   if (it != end) {
      version = it->second.version;
   }
   return { const_cast<session*>(this), std::move(it), version };
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::end() const {
   auto& it_cache = const_cast<cache_type&>(m_cache);
   return { const_cast<session*>(this), std::end(it_cache), 0 };
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::lower_bound(const shared_bytes& key) const {
   auto& it_cache = const_cast<cache_type&>(m_cache);
   auto  version  = uint64_t{ 0 };
   auto  end      = std::end(it_cache);
   auto  it       = it_cache.lower_bound(key);
   first_not_deleted_in_iterator_cache_(it, end);
   if (it == end || ((*it).first != key && !(*it).second.previous_in_cache)) {
      // So either:
      // 1.  We didn't find a key in the iterator cache (pending_key is invalid).
      // 2.  The pending_key is not exactly the key and the found key in the iterator cache currently doesn't know what
      // its previous key is.
      auto pending_key = shared_bytes{};
      if (it != end) {
         pending_key = (*it).first;
      }
      std::visit(
            [&](auto* p) {
               auto pit  = p->lower_bound(key);
               auto pend = std::end(*p);
               if (pit != pend) {
                  if (!pending_key || pit.key() < pending_key) {
                     auto result = it_cache.emplace(pit.key(), value_state{});
                     it          = result.first;
                     if (result.second) {
                        it->second.value = *(*pit).second;
                     }
                     first_not_deleted_in_iterator_cache_(it, end);
                  }
               }
            },
            m_parent);
   }
   if (it != end) {
      version = it->second.version;
      if (it->second.deleted) {
         it = std::move(end);
      }
   }
   return { const_cast<session*>(this), std::move(it), version };
}

template <typename Parent>
template <typename Iterator_traits>
session<Parent>::session_iterator<Iterator_traits>::session_iterator(session* active_session,
                                                                     typename Iterator_traits::cache_iterator it,
                                                                     uint64_t                                 version)
    : m_iterator_version{ version }, m_active_iterator{ std::move(it) }, m_active_session{ active_session } {}

// Moves the current iterator.
//
// \tparam Predicate A functor that indicates if we are incrementing or decrementing the current iterator.
// \tparam Comparator A functor used to determine what the current iterator is.
template <typename Parent>
template <typename Iterator_traits>
template <typename Test_predicate, typename Move_predicate, typename Cache_update>
void session<Parent>::session_iterator<Iterator_traits>::move_(const Test_predicate& test, const Move_predicate& move,
                                                               Cache_update& update_cache) {
   do {
      if (m_active_iterator != std::end(m_active_session->m_cache) && !test(m_active_iterator)) {
         // Force an update to see if we pull in a next or previous key from the current key.
         if (!update_cache(m_active_iterator)) {
            // The test still fails.  We are at the end.
            m_active_iterator = std::end(m_active_session->m_cache);
            break;
         }
      }
      // Move to the next iterator in the cache.
      move(m_active_iterator);
      if (m_active_iterator == std::end(m_active_session->m_cache) || !m_active_iterator->second.deleted) {
         // We either found a key that hasn't been deleted or we hit the end of the iterator cache.
         break;
      }
   } while (true);
}

template <typename Parent>
template <typename Iterator_traits>
void session<Parent>::session_iterator<Iterator_traits>::move_next_() {
   auto move         = [](auto& it) { ++it; };
   auto test         = [](auto& it) { return it->second.next_in_cache; };
   auto update_cache = [&](auto& it) mutable {
      auto value = std::optional<shared_bytes>{};
      auto key   = std::visit(
            [&](auto* p) {
               auto pit = p->find(it->first);
               if (pit != std::end(*p)) {
                  ++pit;
               }
               value = (*pit).second;
               return pit.key();
            },
            m_active_session->m_parent);
      if (key) {
         auto nit                            = m_active_session->m_cache.emplace(key, value_state{});
         nit.first->second.previous_in_cache = true;
         it->second.next_in_cache            = true;
         if (nit.second) {
            nit.first->second.value = std::move(*value);
         }
         return true;
      }
      return false;
   };

   if (m_active_iterator == std::end(m_active_session->m_cache)) {
      m_active_iterator = std::begin(m_active_session->m_cache);
   } else {
      move_(test, move, update_cache);
   }
}

template <typename Parent>
template <typename Iterator_traits>
void session<Parent>::session_iterator<Iterator_traits>::move_previous_() {
   auto move = [](auto& it) { --it; };
   auto test = [&](auto& it) {
      if (it != std::end(m_active_session->m_cache)) {
         return it->second.previous_in_cache;
      }
      return true;
   };
   auto update_cache = [&](auto& it) mutable {
      auto value = std::optional<shared_bytes>{};
      auto key   = std::visit(
            [&](auto* p) {
               auto pit = p->find(it->first);
               if (pit != std::end(*p)) {
                  --pit;
               }
               value = (*pit).second;
               return pit.key();
            },
            m_active_session->m_parent);
      if (key) {
         auto nit                        = m_active_session->m_cache.emplace(key, value_state{});
         nit.first->second.next_in_cache = true;
         it->second.previous_in_cache    = true;
         if (nit.second) {
            nit.first->second.value = std::move(*value);
         }
         return true;
      }
      return false;
   };

   if (m_active_iterator == std::begin(m_active_session->m_cache)) {
      m_active_iterator = std::end(m_active_session->m_cache);
   } else {
      move_(test, move, update_cache);
   }
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
   if (m_active_iterator == std::end(m_active_session->m_cache)) {
      return false;
   }

   return m_active_iterator->second.deleted || m_iterator_version != m_active_iterator->second.version;
}

template <typename Parent>
template <typename Iterator_traits>
const shared_bytes& session<Parent>::session_iterator<Iterator_traits>::key() const {
   if (m_active_iterator == std::end(m_active_session->m_cache)) {
      static auto empty = shared_bytes{};
      return empty;
   }
   return m_active_iterator->first;
}

template <typename Parent>
template <typename Iterator_traits>
typename session<Parent>::template session_iterator<Iterator_traits>::value_type
session<Parent>::session_iterator<Iterator_traits>::operator*() const {
   if (m_active_iterator == std::end(m_active_session->m_cache)) {
      return std::pair{ shared_bytes{}, std::optional<shared_bytes>{} };
   }
   return std::pair{ m_active_iterator->first, m_active_iterator->second.value };
}

template <typename Parent>
template <typename Iterator_traits>
typename session<Parent>::template session_iterator<Iterator_traits>::value_type
session<Parent>::session_iterator<Iterator_traits>::operator->() const {
   if (m_active_iterator == std::end(m_active_session->m_cache)) {
      return std::pair{ shared_bytes{}, std::optional<shared_bytes>{} };
   }
   return std::pair{ m_active_iterator->first, m_active_iterator->second.value };
}

template <typename Parent>
template <typename Iterator_traits>
bool session<Parent>::session_iterator<Iterator_traits>::operator==(const session_iterator& other) const {
   auto end = std::end(m_active_session->m_cache);
   if (m_active_iterator == end && m_active_iterator == other.m_active_iterator) {
      return true;
   }
   if (other.m_active_iterator == end) {
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
