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

/// \brief Defines a session for reading/write data to a cache and persistent data store.
/// \tparam Parent The parent type of this session
/// \remarks Specializations of this type can be created to create new parent types that
/// modify a different data store.  For an example refer to the rocks_session type in this folder.
template <typename Parent>
class session {
 public:
   struct value_state {
      /// Indicates if the next key, in lexicographical order, is within the cache.
      bool next_in_cache{ false };
      /// Indicates if the previous key, in lexicographical order, is within the cache.
      bool previous_in_cache{ false };
      /// Indicates if this key/value pair has been deleted.
      bool deleted{ false };
      /// Indictes if this key's value has been updated.
      bool updated{ false };
      /// The current version of the key's value.
      uint64_t version{ 0 };
      /// The key's value
      shared_bytes value;
   };

   using type                = session;
   using parent_type         = Parent;
   using cache_type          = std::map<shared_bytes, value_state>;
   using parent_variant_type = std::variant<type*, parent_type*>;

   friend Parent;

   /// \brief Defines a key lexicographically ordered, cyclical iterator for traversing a session (which includes
   /// parents).
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

      /// \brief Indicates if the key this iterator refers to has been deleted in the cache or database.
      /// \return True if the key has been deleted, false otherwise.
      /// \remarks Keys can be deleted while there are iterator instances refering to them and the iterator
      /// effectively becomes invalidated.
      bool deleted() const;

      /// \brief Returns the current key that this iterator is positioned on.
      /// \remarks Prefer calling this over the dereference operator if all you want is the key value.
      /// This method does not read any values from the session heirachary (which would incur unncessary overhead if you
      /// don't care about the value.)
      const shared_bytes& key() const;

    protected:
      /// \brief Moves this iterator to another value.
      /// \tparam Test_predicate A functor with the following signature
      /// <code>bool(typename Iterator_traits::cache_iterator&)</code> which tests if the key being moved to exists in
      /// the cache. This method should return true if the desired key is in the cache, false otherwise.
      /// \tparam Move_predicate A functor with the following signature
      /// <code>void(typename Iterator_traits::cache_iterator&)</code> which moves the iterator to the desired key in
      /// the cache.
      /// \tparam Cache_update A functor with the followig signature
      /// <code>bool(typename Iterator_traits::cache_iterator&)</code> which performs any needed updates to the cache to
      /// prepare to move to the desired key.  This method should return true if the cache was updated, false otherwise.
      /// \param test The Test_predicate functor instance.
      /// \param move The Move_predicate functor instance.
      /// \param update_cache The Cache_update functor instance.
      template <typename Test_predicate, typename Move_predicate, typename Cache_update>
      void move_(const Test_predicate& test, const Move_predicate& move, Cache_update& update_cache);

      /// \brief Moves the iterator to the next key in lexicographical order.
      void move_next_();

      /// \brief Moves the iterator to the previous key in lexicographical order.
      void move_previous_();

    private:
      /// The version of the key's value that this iterator is concerned with.
      /// \remarks If the version is different than the iterator's version, then this indicates the key was deleted and
      /// this iterator is effectively invalidated.
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

   /// \brief Constructs a child session of the templated parent type.
   session(Parent& parent);

   /// \brief Constructs a child session from another instance of the same session type.
   explicit session(session& parent, std::nullptr_t);
   session(const session&) = delete;
   session(session&& other);
   ~session();

   session& operator=(const session&) = delete;

   session& operator=(session&& other);

   parent_variant_type parent() const;

   /// \brief Returns the set of keys that have been updated in this session.
   std::unordered_set<shared_bytes> updated_keys() const;

   /// \brief Returns the set of keys that have been deleted in this session.
   std::unordered_set<shared_bytes> deleted_keys() const;

   /// \brief Attaches a new parent to the session.
   void attach(Parent& parent);

   /// \brief Attaches a new parent to the session.
   void attach(session& parent);

   /// \brief Detaches from the parent.
   /// \remarks This is very similar to an undo.
   void detach();

   /// \brief Discards the changes in this session and detachs the session from its parent.
   void undo();
   /// \brief Commits the changes in this session into its parent.
   void commit();

   std::optional<shared_bytes> read(const shared_bytes& key);
   void                        write(const shared_bytes& key, const shared_bytes& value);
   bool                        contains(const shared_bytes& key);
   void                        erase(const shared_bytes& key);
   void                        clear();

   /// \brief Reads a batch of keys from this session.
   /// \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the key.
   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read(const Iterable& keys);

   /// \brief Writes a batch of key/value pairs into this session.
   /// \param key_values A type that supports iteration and returns in its iterator a pair containing shared_bytes
   /// instances that represents a key and a value.
   template <typename Iterable>
   void write(const Iterable& key_values);

   /// \brief Erases a batch of keys from this session.
   /// \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the key.
   template <typename Iterable>
   void erase(const Iterable& keys);

   /// \brief Writes a batch of key/values from this session into another container.
   /// \param ds The container to write into.  The type must implement a batch write method like the one defined in this
   /// type. \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the
   /// key.
   template <typename Other_data_store, typename Iterable>
   void write_to(Other_data_store& ds, const Iterable& keys);

   /// \brief Reads a batch of keys from another container into this session.
   /// \param ds The container to read from.  This type must implement a batch read method like the one defined in this
   /// type. \param keys A type that supports iteration and returns in its iterator a shared_bytes type representing the
   /// key.
   template <typename Other_data_store, typename Iterable>
   void read_from(Other_data_store& ds, const Iterable& keys);

   /// \brief Returns an iterator to the key, or the end iterator if the key is not in the cache or session heirarchy.
   /// \param key The key to search for.
   /// \return An iterator to the key if found, the end iterator otherwise.
   iterator find(const shared_bytes& key);
   iterator begin();
   iterator end();

   /// \brief Returns an iterator to the first key that is not less than, in lexicographical order, the given key.
   /// \param key The key to search on.
   /// \return An iterator to the first key that is not less than the given key, or the end iterator if there is no key
   /// that matches that criteria.
   iterator lower_bound(const shared_bytes& key);

 private:
   /// \brief Sets the lower/upper bounds of the session's cache based on the parent's cache lower/upper bound
   /// \remarks This is only invoked when constructing a session with a parent.  This method prepares the iterator cache
   /// by priming it with a lower/upper bound for iteration.  These bounds can be changed, though, within this session
   /// through adding and deleting keys through this sesion.
   void prime_cache_();

   /// \brief Searches the session heirarchy for the key that is next, in lexicographical order, from the given key.
   /// \param it An iterator pointing to the current key.
   /// \param pit An iterator pointing to the lower_bound of the key in the parent session.
   /// \param pend An end iterator of the parent session.
   /// \remarks This method is only called by update_iterator_cache_.
   template <typename It, typename Parent_it>
   void next_key_(It& it, Parent_it& pit, Parent_it& pend);

   /// \brief Searches the session heirarchy for the key that is previous, in lexicographical order, from the given key.
   /// \param it An iterator pointing to the current key.
   /// \param pit An iterator pointing to the lower_bound of the key in the parent session.
   /// \param pend An end iterator of the parent session.
   /// \remarks This method is only called by update_iterator_cache_.
   template <typename It, typename Parent_it>
   void previous_key_(It& it, Parent_it& pit, Parent_it& pbegin, Parent_it& pend);

   /// \brief Updates the cache with the previous and next keys, in lexicographical order, of the given key.
   /// \param key The key to search on.
   /// \remarks This method is invoked when reading, writing, erasing on this session and when iterating over the
   /// session.
   typename cache_type::iterator update_iterator_cache_(const shared_bytes& key);

   /// \brief Increments the given cache iterator until an iterator is found that isn't pointing to a deleted key.
   /// \param it The cache iterator to increment.
   /// \param end An end iterator of the cache.
   template <typename It>
   It& first_not_deleted_in_iterator_cache_(It& it, const It& end, bool& previous_in_cache) const;

   template <typename It>
   It& first_not_deleted_in_iterator_cache_(It& it, const It& end) const;

 private:
   parent_variant_type m_parent{ nullptr };
   cache_type          m_cache;
};

template <typename Parent>
typename session<Parent>::parent_variant_type session<Parent>::parent() const {
   return m_parent;
}

template <typename Parent>
void session<Parent>::prime_cache_() {
   // Get the bounds of the parent cache and use those to
   // seed the cache of this session.
   auto update = [&](const auto& key, const auto& value) {
      auto it = m_cache.emplace(key, value_state{});
      if (it.second) {
         it.first->second.value = value;
      }
   };

   std::visit(overloaded{ [&](session<Parent>* p) {
                            auto begin = std::begin(p->m_cache);
                            auto end   = std::end(p->m_cache);
                            if (begin == end) {
                               return;
                            }
                            update(begin->first, begin->second.value);
                            --end;
                            update(end->first, end->second.value);
                         },
                          [&](Parent* p) {
                             auto begin = std::begin(*p);
                             auto end   = std::end(*p);
                             if (begin == end) {
                                return;
                             }
                             update(begin.key(), (*begin).second.value());
                             --end;
                             update(end.key(), (*end).second.value());
                          } },
              m_parent);
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
session<Parent>::session(session& parent, std::nullptr_t) : m_parent{ &parent } {
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
void session<Parent>::previous_key_(It& it, Parent_it& pit, Parent_it& pbegin, Parent_it& pend) {
   if (it->first) {
      if (pit != pbegin) {
         --pit;
         if (pit != pend) {
            auto cit = m_cache.emplace(pit.key(), value_state{});
            if (cit.second) {
               cit.first->second.value = *(*pit).second;
            }
         }
         ++pit;
      }

      auto previous_it = it;
      if (previous_it != std::begin(m_cache)) {
         --previous_it;
         previous_it->second.next_in_cache = true;
         it->second.previous_in_cache      = true;
      }
   }
}

template <typename Parent>
template <typename It, typename Parent_it>
void session<Parent>::next_key_(It& it, Parent_it& pit, Parent_it& pend) {
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

      auto next_it = it;
      ++next_it;
      if (next_it != std::end(m_cache)) {
         next_it->second.previous_in_cache = true;
         it->second.next_in_cache          = true;
      }
   }
}

template <typename Parent>
typename session<Parent>::cache_type::iterator session<Parent>::update_iterator_cache_(const shared_bytes& key) {
   auto  result = m_cache.emplace(key, value_state{});
   auto& it     = result.first;

   if (result.second) {
      // The two keys that this new key is being inserted inbetween might already be in a global lexicographical order.
      // If so, that means we already know the global order of this new key.
      if (it != std::begin(m_cache)) {
         auto previous = it;
         --previous;
         if (previous->second.next_in_cache) {
            it->second.previous_in_cache = true;
            it->second.next_in_cache     = true;
            return it;
         }
      } else {
         auto end = std::end(m_cache);
         if (it != end) {
            auto next = it;
            ++next;
            if (next != end && next->second.previous_in_cache) {
               it->second.next_in_cache     = true;
               it->second.previous_in_cache = true;
               return it;
            }
         }
      }

      // ...otherwise we have to search through the session heirarchy to find the previous and next keys
      // of the given key.
      if (!it->second.next_in_cache || !it->second.previous_in_cache) {
         std::visit(
               [&](auto* p) {
                  auto pit = p->lower_bound(key);
                  auto end = std::end(*p);
                  if (!it->second.next_in_cache) {
                     next_key_(it, pit, end);
                  }
                  if (!it->second.previous_in_cache) {
                     auto begin = std::begin(*p);
                     previous_key_(it, pit, begin, end);
                  }
               },
               m_parent);
      }
   }

   return it;
}

template <typename Parent>
std::unordered_set<shared_bytes> session<Parent>::updated_keys() const {
   auto results = std::unordered_set<shared_bytes>{};
   for (const auto& it : m_cache) {
      if (it.second.updated) {
         results.emplace(it.first);
      }
   }
   return results;
}

template <typename Parent>
std::unordered_set<shared_bytes> session<Parent>::deleted_keys() const {
   auto results = std::unordered_set<shared_bytes>{};
   for (const auto& it : m_cache) {
      if (it.second.deleted) {
         results.emplace(it.first);
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
std::optional<shared_bytes> session<Parent>::read(const shared_bytes& key) {
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
      // Update the "iterator cache".
      auto it          = update_iterator_cache_(key);
      it->second.value = *value;
   }

   return value;
}

template <typename Parent>
void session<Parent>::write(const shared_bytes& key, const shared_bytes& value) {
   auto it            = update_iterator_cache_(key);
   it->second.value   = value;
   it->second.deleted = false;
   it->second.updated = true;
}

template <typename Parent>
bool session<Parent>::contains(const shared_bytes& key) {
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
               auto it          = update_iterator_cache_(key);
               it->second.value = std::move(*value);
               return true;
            }
            return false;
         },
         m_parent);
}

template <typename Parent>
void session<Parent>::erase(const shared_bytes& key) {
   auto it            = update_iterator_cache_(key);
   it->second.deleted = true;
   it->second.updated = false;
   ++it->second.version;
}

template <typename Parent>
template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
session<Parent>::read(const Iterable& keys) {
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

template <typename Parent>
template <typename Iterable>
void session<Parent>::write(const Iterable& key_values) {
   // Currently the batch write will just iteratively call the non batch write
   for (const auto& kv : key_values) { write(kv.first, kv.second); }
}

template <typename Parent>
template <typename Iterable>
void session<Parent>::erase(const Iterable& keys) {
   // Currently the batch erase will just iteratively call the non batch erase
   for (const auto& key : keys) { erase(key); }
}

template <typename Parent>
template <typename Other_data_store, typename Iterable>
void session<Parent>::write_to(Other_data_store& ds, const Iterable& keys) {
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
void session<Parent>::read_from(Other_data_store& ds, const Iterable& keys) {
   ds.write_to(*this, keys);
}

template <typename Parent>
template <typename It>
It& session<Parent>::first_not_deleted_in_iterator_cache_(It& it, const It& end, bool& previous_in_cache) const {
   auto previous_known       = true;
   auto update_previous_flag = [&](auto& it) {
      if (previous_known) {
         previous_known = it->second.previous_in_cache;
      }
   };

   while (it != end && it->second.deleted) {
      update_previous_flag(it);
      ++it;
   }
   update_previous_flag(it);
   previous_in_cache = previous_known;
   return it;
}

template <typename Parent>
template <typename It>
It& session<Parent>::first_not_deleted_in_iterator_cache_(It& it, const It& end) const {
   while (it != end) {
      auto find_it = m_cache.find(it.key());
      if (find_it == std::end(m_cache) || !find_it->second.deleted) {
         return it;
      }
      ++it;
   }

   return it;
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::find(const shared_bytes& key) {
   auto version = uint64_t{ 0 };
   auto end     = std::end(m_cache);
   auto it      = m_cache.find(key);
   if (it == end) {
      // We didn't find the key in our cache, ask the parent if he has one.
      std::visit(
            [&](auto* p) {
               auto pit = p->find(key);
               if (pit != std::end(*p)) {
                  auto result = m_cache.emplace(key, value_state{});
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
typename session<Parent>::iterator session<Parent>::begin() {
   auto end     = std::end(m_cache);
   auto begin   = std::begin(m_cache);
   auto it      = begin;
   auto version = uint64_t{ 0 };

   auto previous_in_cache = true;
   first_not_deleted_in_iterator_cache_(it, end, previous_in_cache);

   if (it == end || (it != begin && !previous_in_cache)) {
      // We have a begin iterator in this session, but we don't have enough
      // information to determine if that iterator is globally the begin iterator.
      // We need to ask the parent for its begin iterator and compare the two
      // to see which comes first lexicographically.
      auto pending_key = shared_bytes{};
      if (it != end) {
         pending_key = (*it).first;
      }
      std::visit(
            [&](auto* p) {
               auto pit  = std::begin(*p);
               auto pend = std::end(*p);
               first_not_deleted_in_iterator_cache_(pit, pend);
               if (pit != pend && pit.key() < pending_key) {
                  auto result = m_cache.emplace(pit.key(), value_state{});
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
   }
   return { const_cast<session*>(this), std::move(it), version };
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::end() {
   return { const_cast<session*>(this), std::end(m_cache), 0 };
}

template <typename Parent>
typename session<Parent>::iterator session<Parent>::lower_bound(const shared_bytes& key) {
   auto version = uint64_t{ 0 };
   auto end     = std::end(m_cache);
   auto it      = m_cache.lower_bound(key);

   auto previous_in_cache = true;
   first_not_deleted_in_iterator_cache_(it, end, previous_in_cache);

   if (it == end || ((*it).first != key && !previous_in_cache)) {
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
               first_not_deleted_in_iterator_cache_(pit, pend);
               if (pit != pend) {
                  if (!pending_key || pit.key() < pending_key) {
                     auto result = m_cache.emplace(pit.key(), value_state{});
                     it          = result.first;
                     if (result.second) {
                        it->second.value = *(*pit).second;
                     }
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

      auto pending_key = eosio::session::shared_bytes{};
      auto end         = std::end(m_active_session->m_cache);
      if (it != end) {
         ++it;
         if (it != end) {
            pending_key = it->first;
         }
         --it;
      }

      auto key = std::visit(
            [&](auto* p) {
               auto pit = p->lower_bound(it->first);
               if (pit != std::end(*p) && pit.key() == it->first) {
                  ++pit;
               }
               value = (*pit).second;
               return pit.key();
            },
            m_active_session->m_parent);

      // We have two candidates for the next key.
      // 1. The next key in order in this sessions cache.
      // 2. The next key in lexicographical order retrieved from the sessions parent.
      // Choose which one it is.
      if (!key || (pending_key && pending_key < key)) {
         key = pending_key;
      }

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

      auto pending_key = eosio::session::shared_bytes{};
      if (it != std::begin(m_active_session->m_cache)) {
         --it;
         pending_key = it->first;
         ++it;
      }

      auto key = std::visit(
            [&](auto* p) {
               auto pit = p->lower_bound(it->first);
               if (pit != std::begin(*p)) {
                  --pit;
               } else {
                  return eosio::session::shared_bytes{};
               }

               value = (*pit).second;
               return pit.key();
            },
            m_active_session->m_parent);

      // We have two candidates to consider.
      // 1. The key returned by decrementing the iterator on this session's cache.
      // 2. The key returned by calling lower_bound on the parent of this session.
      // We want the larger of the two.
      if (!key || (pending_key && pending_key > key)) {
         key = pending_key;
      }

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
