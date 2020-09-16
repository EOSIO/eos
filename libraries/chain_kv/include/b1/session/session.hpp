#pragma once

#include <queue>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <session/cache.hpp>
#include <session/key_value.hpp>
#include <session/session_fwd_decl.hpp>
#include <session/shared_bytes.hpp>

namespace eosio::session {

// Defines a session for reading/write data to a cache and persistent data store.
//
// \tparam Data_store A type that persists data to disk or some other non memory backed storage device.
template <typename Data_store>
class session final {
 private:
   struct session_impl;

 public:
   struct iterator_state final {
      bool next_in_cache{ false };
      bool previous_in_cache{ false };
      bool deleted{ false };
   };

   using data_store_type       = Data_store;
   using cache_data_store_type = cache;
   using iterator_cache_type   = std::map<shared_bytes, iterator_state>;

   static const session invalid;

   template <typename Data_store_>
   friend session<Data_store_> make_session();

   template <typename Data_store_>
   friend session<Data_store_> make_session(Data_store_ store);

   template <typename Data_store_>
   friend session<Data_store_> make_session(session<Data_store_>& the_session);

   // Defines a key ordered, cyclical iterator for traversing a session (which includes parents and children of each
   // session). Basically we are iterating over a list of sessions and each session has its own cache of key_values,
   // along with iterating over a persistent data store all the while maintaining key order with the iterator.
   template <typename Iterator_traits>
   class session_iterator final {
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
      session_iterator  operator++(int);
      session_iterator& operator--();
      session_iterator  operator--(int);
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
      typename iterator_cache_type::iterator m_active_iterator;
      session*                               m_active_session{ nullptr };
   };

   struct iterator_traits final {
      using difference_type     = long;
      using value_type          = key_value;
      using pointer             = value_type*;
      using reference           = value_type&;
      using iterator_category   = std::bidirectional_iterator_tag;
      using cache_iterator      = typename cache::iterator;
      using data_store_iterator = typename Data_store::iterator;
   };
   using iterator = session_iterator<iterator_traits>;

   struct const_iterator_traits final {
      using difference_type     = long;
      using value_type          = const key_value;
      using pointer             = value_type*;
      using reference           = value_type&;
      using iterator_category   = std::bidirectional_iterator_tag;
      using cache_iterator      = typename cache::iterator;
      using data_store_iterator = typename Data_store::iterator;
   };
   using const_iterator = session_iterator<const_iterator_traits>;

   struct nested_session_t final {};

 public:
   session(Data_store ds);
   session(session& session, nested_session_t);
   session(const session&) = default;
   session(session&&)      = default;
   ~session();

   session& operator=(const session&) = default;
   session& operator=(session&&) = default;

   session attach(session child);
   session detach();

   // undo stack operations
   void undo();
   void commit();

   // this part also identifies a concept.  don't know what to call it yet
   const key_value read(const shared_bytes& key) const;
   void            write(key_value kv);
   bool            contains(const shared_bytes& key) const;
   void            erase(const shared_bytes& key);
   void            clear();

   // returns a pair containing the key_values found and the keys not found.
   template <typename Iterable>
   const std::pair<std::vector<key_value>, std::unordered_set<shared_bytes>> read(const Iterable& keys) const;

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
   session();
   session(std::shared_ptr<session_impl> impl);

   template <typename Iterator_type, typename Predicate, typename Comparator, typename Move>
   Iterator_type make_iterator_(const Predicate& predicate, const Comparator& comparator, const Move& move,
                                bool prime_cache_only = false) const;

   struct session_impl final : public std::enable_shared_from_this<session_impl> {
      session_impl();
      session_impl(session_impl& parent_);
      session_impl(Data_store pds);
      ~session_impl();

      void                                  prime_cache();
      std::pair<shared_bytes, shared_bytes> bounds(const shared_bytes& key);

      struct iterator_cache_params final {
         bool prime_only{ false };
         bool recalculate{ false };
         bool mark_deleted{ false };
         bool overwrite{ true };
      };
      void update_iterator_cache(const shared_bytes& key, const iterator_cache_params& params);

      void undo();
      void commit();
      void clear();

      session_impl* parent{ nullptr };
      session_impl* child{ nullptr };

      // The persistent data store.  This is shared across all levels of the session.
      std::shared_ptr<Data_store> data_store;

      // The cache used by this session instance.  This will include all new/update key_values
      // and may include values read from the persistent data store.
      eosio::session::cache cache;

      // Indicates if the next/previous key in lexicographical order for a given key exists in the cache.
      // The first value in the pair indicates if the previous key in lexicographical order exists in the cache.
      // The second value in the pair indicates if the next key lexicographical order exists in the cache.
      iterator_cache_type iterator_cache;

      // keys that have been updated during this session.
      std::unordered_set<shared_bytes> updated_keys;

      // keys that have been deleted during this session.
      std::unordered_set<shared_bytes> deleted_keys;
   };

   // session implement the PIMPL idiom to allow for stack based semantics.
   std::shared_ptr<session_impl> m_impl;
};

template <typename Data_store>
session<Data_store> make_session() {
   using session_type = session<Data_store>;
   return session_type{};
}

template <typename Data_store>
session<Data_store> make_session(Data_store store) {
   using session_type = session<Data_store>;
   return session_type{ std::move(store) };
}

template <typename Data_store>
session<Data_store> make_session(session<Data_store>& the_session) {
   using session_type                = session<Data_store>;
   auto new_session                  = session_type{ the_session, typename session_type::nested_session_t{} };
   new_session.m_impl->parent->child = new_session.m_impl.get();
   return new_session;
}

template <typename Data_store>
session<Data_store>::session_impl::session_impl() : data_store{ std::make_shared<Data_store>() } {}

template <typename Data_store>
session<Data_store>::session_impl::session_impl(session_impl& parent_)
    : parent{ &parent_ }, data_store{ parent_.data_store } {
   if (parent->child) {
      parent->child->data_store = nullptr;
      parent->child->parent     = nullptr;
   }
}

template <typename Data_store>
session<Data_store>::session_impl::session_impl(Data_store pds)
    : data_store{ std::make_shared<Data_store>(std::move(pds)) } {}

template <typename Data_store>
void session<Data_store>::session_impl::prime_cache() {
   iterator_cache.clear();

   auto keys_to_remove = std::set<shared_bytes>{};
   for (const auto& kv : cache) {
      auto key     = kv.key();
      auto updated = updated_keys.find(key);
      if (updated == std::end(updated_keys)) {
         keys_to_remove.emplace(key);
      }
   }
   if (!keys_to_remove.empty()) {
      cache.erase(keys_to_remove);
   }

   if (child) {
      child->prime_cache();
   }
}

template <typename Data_store>
session<Data_store>::session_impl::~session_impl() {
   commit();
   if (parent) {
     parent->child = nullptr;
   }
   if (child) {
     child->parent = nullptr;
   }
}

template <typename Data_store>
void session<Data_store>::session_impl::undo() {
   // TODO:  Should I connect the child of this to the parent of this?
   if (parent) {
      parent->child = child;
   }

   if (child) {
      child->parent = parent;
   }

   parent     = nullptr;
   child      = nullptr;
   data_store = nullptr;
   clear();
}

template <typename Data_store>
void session<Data_store>::session_impl::commit() {
   if (!parent && !data_store) {
      // undo
      return;
   }

   if (updated_keys.empty() && deleted_keys.empty()) {
      // Nothing to commit.
      return;
   }

   auto write_through = [&](auto& ds) {
      ds.erase(deleted_keys);
      cache.write_to(ds, updated_keys);
      clear();
   };

   if (parent) {
      // squash
      auto parent_session = session{ parent->shared_from_this() };
      write_through(parent_session);
      return;
   }

   // commit
   write_through(*data_store);
}

template <typename Data_store>
void session<Data_store>::session_impl::clear() {
   deleted_keys.clear();
   updated_keys.clear();
   cache.clear();
   iterator_cache.clear();
}

template <typename Data_store>
const session<Data_store> session<Data_store>::invalid{};

template <typename Data_store>
session<Data_store>::session() : m_impl{ std::make_shared<session_impl>() } {}

template <typename Data_store>
session<Data_store>::session(std::shared_ptr<session_impl> impl) : m_impl{ std::move(impl) } {}

template <typename Data_store>
session<Data_store>::session(Data_store ds) : m_impl{ std::make_shared<session_impl>(std::move(ds)) } {}

template <typename Data_store>
session<Data_store>::session(session& session, nested_session_t)
    : m_impl{ std::make_shared<session_impl>(*session.m_impl) } {}

template <typename Data_store>
session<Data_store>::~session() {}

template <typename Data_store>
void session<Data_store>::undo() {
   if (!m_impl) {
      return;
   }

   m_impl->undo();
}

template <typename Data_store>
std::pair<shared_bytes, shared_bytes> session<Data_store>::session_impl::bounds(const shared_bytes& key) {
   auto lower_bound_key = shared_bytes::invalid;
   auto upper_bound_key = shared_bytes::invalid;

   auto s = session{ this->shared_from_this() };

   // We need to be careful about requesting iterators on the session since it will be rentrant into this
   // method.  Ask for the iterator in a way that isn't reentrant.  We can do this by invoking the make_iterator_
   // method directly.
   auto lower = [&](auto& ds) {
      auto it = ds.lower_bound(key);
      if (it != std::begin(ds) && it != std::end(ds)) {
         return --it;
      }
      return std::end(ds);
   };
   auto lower_bound = s.template make_iterator_<iterator>(
         lower, std::less<shared_bytes>{}, [](auto& it, auto& end) { ++it; }, true);
   auto upper_bound =
         s.template make_iterator_<iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<shared_bytes>{},
                                             [](auto& it, auto& end) { ++it; }, true);
   auto end = s.template make_iterator_<iterator>([&](auto& ds) { return std::end(ds); }, std::greater<shared_bytes>{},
                                                  [](auto& it, auto& end) { it = end; }, true);

   if (lower_bound != end) {
      lower_bound_key = lower_bound.m_active_iterator->first;
   }

   if (upper_bound != end) {
      upper_bound_key = upper_bound.m_active_iterator->first;
   }

   return std::pair{ lower_bound_key, upper_bound_key };
}

template <typename Data_store>
void session<Data_store>::session_impl::update_iterator_cache(const shared_bytes&          key,
                                                              const iterator_cache_params& params) {
   auto result = iterator_cache.try_emplace(key, iterator_state{});

   if (params.prime_only) {
      // We only want to make sure the key exists in the iterator cache.
      return;
   }

   auto& it = result.first;

   if (params.overwrite) {
      // Force an overwrite
      it->second.deleted = params.mark_deleted;
   }

   if (!params.recalculate && it->second.next_in_cache && it->second.previous_in_cache) {
      // Only find the previous and next keys if we are forced to or if the current
      // key has one of the flags markes as false.  We check when the flags are false
      // because a new key might have come through that could change that result.  We may
      // do a recalculation on a deletion to also change the flags.
      return;
   }

   auto [lower_bound, upper_bound] = bounds(key);

   if (lower_bound != shared_bytes::invalid) {
      auto lower_it                        = iterator_cache.try_emplace(lower_bound, iterator_state{});
      lower_it.first->second.next_in_cache = true;
      it->second.previous_in_cache         = true;
   }

   if (upper_bound != shared_bytes::invalid) {
      auto upper_it                            = iterator_cache.try_emplace(upper_bound, iterator_state{});
      upper_it.first->second.previous_in_cache = true;
      it->second.next_in_cache                 = true;
   }
}

template <typename Data_store>
session<Data_store> session<Data_store>::attach(session child) {
   if (!m_impl) {
      return session<Data_store>::invalid;
   }

   auto current_child = detach();

   child.m_impl->parent     = m_impl.get();
   child.m_impl->data_store = m_impl->data_store;
   m_impl->child            = child.m_impl.get();

   child.m_impl->prime_cache();

   return current_child;
}

template <typename Data_store>
session<Data_store> session<Data_store>::detach() {
   if (!m_impl) {
      return session<Data_store>::invalid;
   }

   auto current_child = m_impl->child;
   if (current_child) {
      current_child->parent     = nullptr;
      current_child->data_store = nullptr;
   }
   m_impl->child = nullptr;

   return { current_child ? current_child->shared_from_this() : nullptr };
}

template <typename Data_store>
void session<Data_store>::commit() {
   if (!m_impl) {
      return;
   }

   m_impl->commit();
}

template <typename Data_store>
const key_value session<Data_store>::read(const shared_bytes& key) const {
   if (!m_impl) {
      return key_value::invalid;
   }

   // Find the key within the session.
   // Check this level first and then traverse up to the parent to see if this key/value
   // has been read and/or update.
   auto* parent = m_impl.get();
   while (parent) {
      if (parent->deleted_keys.find(key) != std::end(parent->deleted_keys)) {
         // key has been deleted at this level.
         return key_value::invalid;
      }

      auto& kv = parent->cache.read(key);
      if (kv != key_value::invalid) {
         if (parent != m_impl.get()) {
            m_impl->cache.write(kv);
            m_impl->update_iterator_cache(
                  kv.key(), { .prime_only = false, .recalculate = true, .mark_deleted = false, .overwrite = false });
         }
         return kv;
      }

      parent = parent->parent;
   }

   // If we haven't found the key in the session yet, read it from the persistent data store.
   if (m_impl->data_store) {
      auto kv = m_impl->data_store->read(key);
      if (kv != key_value::invalid) {
         m_impl->cache.write(kv);
         m_impl->update_iterator_cache(
               kv.key(), { .prime_only = false, .recalculate = true, .mark_deleted = false, .overwrite = false });
         return kv;
      }
   }

   return key_value::invalid;
}

template <typename Data_store>
void session<Data_store>::write(key_value kv) {
   if (!m_impl) {
      return;
   }
   auto key = kv.key();
   m_impl->updated_keys.emplace(key);
   m_impl->deleted_keys.erase(key);
   m_impl->cache.write(std::move(kv));
   m_impl->update_iterator_cache(
         key, { .prime_only = false, .recalculate = true, .mark_deleted = false, .overwrite = true });
}

template <typename Data_store>
bool session<Data_store>::contains(const shared_bytes& key) const {
   if (!m_impl) {
      return false;
   }

   // Traverse the heirarchy to see if this session (and its parent session)
   // has already read the key into memory.
   auto* parent = m_impl.get();
   while (parent) {
      if (parent->deleted_keys.find(key) != std::end(parent->deleted_keys)) {
         return false;
      }

      if (parent->cache.contains(key)) {
         m_impl->update_iterator_cache(
               key, { .prime_only = false, .recalculate = true, .mark_deleted = false, .overwrite = false });
         return true;
      }

      parent = parent->parent;
   }

   // If we get to this point, the key hasn't been read into memory.  Check to see if it exists in the persistent data
   // store.
   return m_impl->data_store->contains(key);
}

template <typename Data_store>
void session<Data_store>::erase(const shared_bytes& key) {
   if (!m_impl) {
      return;
   }
   m_impl->deleted_keys.emplace(key);
   m_impl->updated_keys.erase(key);
   m_impl->cache.erase(key);
   m_impl->update_iterator_cache(key,
                                 { .prime_only = false, .recalculate = true, .mark_deleted = true, .overwrite = true });
}

template <typename Data_store>
void session<Data_store>::clear() {
   if (!m_impl) {
      return;
   }
   m_impl->clear();
}

// Reads a batch of keys from the session.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An iterable instance that returns shared_bytes instances in its iterator. \returns An std::pair
// where the first item is list of the found key_values and the second item is a set of the keys not found.
template <typename Data_store>
template <typename Iterable>
const std::pair<std::vector<key_value>, std::unordered_set<shared_bytes>>
session<Data_store>::read(const Iterable& keys) const {
   if (!m_impl) {
      return {};
   }

   auto not_found = std::unordered_set<shared_bytes>{};
   auto kvs       = std::vector<key_value>{};

   for (const auto& key : keys) {
      auto  found   = false;
      auto  deleted = false;
      auto* parent  = m_impl.get();
      while (parent) {
         if (parent->deleted_keys.find(key) != std::end(parent->deleted_keys)) {
            // key has been deleted at this level.  This means it no longer exists within
            // this session.  There is no point in continuing the search.
            deleted = true;
            break;
         }

         auto& kv = parent->cache.read(key);
         if (kv != key_value::invalid) {
            // We found the key.  Write it to our cache and bail out.
            if (parent != m_impl.get()) {
               m_impl->cache.write(kv);
               m_impl->update_iterator_cache(
                     kv.key(), { .prime_only = false, .recalculate = true, .mark_deleted = false, .overwrite = false });
            }
            found = true;
            kvs.emplace_back(kv);
            break;
         }
         parent = parent->parent;
      }

      if (!found) {
         not_found.emplace(key);
      }
   }

   // For all the keys that were found in one of the session caches, read them from the persistent data store
   // and write them into our cache.
   if (m_impl->data_store) {
      auto [found, missing] = m_impl->data_store->read(not_found);
      if (!found.empty()) {
         m_impl->cache.write(found);
      }
      not_found = std::move(missing);
      kvs.insert(std::end(kvs), std::begin(found), std::end(found));
   }

   return { std::move(kvs), std::move(not_found) };
}

// Writes a batch of key_values to the session.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns key_value instances in its
// iterator. \param key_values An iterable instance that returns key_value instances in its iterator.
template <typename Data_store>
template <typename Iterable>
void session<Data_store>::write(const Iterable& key_values) {
   if (!m_impl) {
      return;
   }

   // Currently the batch write will just iteratively call the non batch write
   for (const auto& kv : key_values) { write(kv); }
}

// Erases a batch of key_values from the session.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An iterable instance that returns shared_bytes instances in its iterator.
template <typename Data_store>
template <typename Iterable>
void session<Data_store>::erase(const Iterable& keys) {
   if (!m_impl) {
      return;
   }

   // Currently the batch erase will just iteratively call the non batch erase
   for (const auto& key : keys) { erase(key); }
}

template <typename Data_store>
template <typename Other_data_store, typename Iterable>
void session<Data_store>::write_to(Other_data_store& ds, const Iterable& keys) const {
   if (!m_impl) {
      return;
   }

   auto results = std::vector<key_value>{};
   for (const auto& key : keys) {
      auto* parent = m_impl.get();
      while (parent) {
         if (parent->deleted_keys.find(key) != std::end(parent->deleted_keys)) {
            // key/value has been deleted in this session.  Bail out.
            break;
         }

         auto& kv = parent->cache.read(key);
         if (kv == key_value::invalid) {
            parent = parent->parent;
            continue;
         }

         results.emplace_back(make_kv(kv.key().data(), kv.key().size(), kv.value().data(), kv.value().size()));
         break;
      }
   }
   ds.write(results);
}

template <typename Data_store>
template <typename Other_data_store, typename Iterable>
void session<Data_store>::read_from(const Other_data_store& ds, const Iterable& keys) {
   if (!m_impl) {
      return;
   }
   ds.write_to(*this, keys);
}

// Factor for creating the initial session iterator.
//
// \tparam Predicate A functor used for getting the starting iterating in the cache list and the persistent data store.
// \tparam Comparator A functor for determining if the "current" iterator points to the cache list or the persistent
// data store.
template <typename Data_store>
template <typename Iterator_type, typename Predicate, typename Comparator, typename Move>
Iterator_type session<Data_store>::make_iterator_(const Predicate& predicate, const Comparator& comparator,
                                                  const Move& move, bool prime_cache_only) const {
   // This method is the workhorse of the iterator and iterator cache updating.
   // It will search through all the session levels to find the best key that matches
   // the requested iterator.  Once that key has been identified, it will force an update
   // on the iterator cache by inserting that key and (possibly) the previous key from that key
   // and the next key from that key.

   if (!m_impl) {
      // For some reason the pimpl is nullptr.  Return an "invalid" iterator.
      return Iterator_type{};
   }

   auto new_iterator              = Iterator_type{};
   new_iterator.m_active_session  = const_cast<session*>(this);
   new_iterator.m_active_iterator = std::end(m_impl->iterator_cache);

   // Move to the head.  We start at the head because we want to end up with
   // an iterator on the session at the tail of the session list.
   auto* parent = m_impl.get();
   while (parent->parent) { parent = parent->parent; }

   auto deleted = [&](auto& session, auto& key) {
      // Checks if a given key has been deleted in any descendants of the given session
      // or if the key has been added in any descendent after it has been deleted.
      auto child  = session->child;
      auto result = false;
      while (child) {
         if (child->deleted_keys.find(key) != std::end(child->deleted_keys)) {
            result = true;
         } else if (child->updated_keys.find(key) != std::end(child->updated_keys)) {
            result = false;
         }

         if (child == m_impl.get()) {
            break;
         }
         child = child->child;
      }
      return result;
   };

   auto find_first_not = [&](auto& session, auto& it, auto end_it, const auto& predicate) {
      // Return the first key that passes the given predicate.
      if (it == end_it) {
         return shared_bytes::invalid;
      }
      auto pending_key = (*it).key();
      while (true) {
         if (!predicate(session, pending_key)) {
            return pending_key;
         }
         move(it, end_it);
         if (it == end_it) {
            return shared_bytes::invalid;
         }
         pending_key = (*it).key();
      }
   };

   // Start at the database.
   auto it          = predicate(*m_impl->data_store);
   auto end         = std::end(*m_impl->data_store);
   auto current_key = find_first_not(parent, it, end, deleted);

   // Check the other session levels to see which key we should start at.
   // We start at the parent and work our way down to "this" session.
   auto* current = parent;
   while (current) {
      auto pending     = predicate(current->cache);
      auto end         = std::end(current->cache);
      auto pending_key = find_first_not(parent, pending, end, deleted);

      if (pending == end) {
         current = current->child;
         continue;
      }

      if (current_key == shared_bytes::invalid || comparator(pending_key, current_key)) {
         current_key = pending_key;
      }

      // Stop at this session.
      if (current == m_impl.get()) {
         break;
      }

      current = current->child;
   }

   if (current_key != shared_bytes::invalid) {
      // Update the iterator cache with this key.  It has to exist in the cache before
      // we can get an iterator to it.  In some cases, we will only be inserting the new
      // key and bypassing the search for its previous and next key.
      m_impl->update_iterator_cache(
            current_key,
            { .prime_only = prime_cache_only, .recalculate = true, .mark_deleted = false, .overwrite = false });
      new_iterator.m_active_iterator = m_impl->iterator_cache.find(current_key);

      if (new_iterator.m_active_iterator->second.deleted) {
         new_iterator.m_active_iterator = std::end(m_impl->iterator_cache);
      }
   }

   return new_iterator;
}

template <typename Data_store>
typename session<Data_store>::iterator session<Data_store>::find(const shared_bytes& key) {
   // This comparator just chooses the one that isn't invalid.
   static auto comparator = [](const auto& left, const auto& right) {
      if (left == shared_bytes::invalid && right == shared_bytes::invalid) {
         return true;
      }

      if (left == shared_bytes::invalid) {
         return false;
      }

      return true;
   };

   return make_iterator_<iterator>([&](auto& ds) { return ds.find(key); }, comparator,
                                   [](auto& it, auto& end) { it = end; });
}

template <typename Data_store>
typename session<Data_store>::const_iterator session<Data_store>::find(const shared_bytes& key) const {
   // This comparator just chooses the one that isn't invalid.
   static auto comparator = [](const auto& left, const auto& right) {
      if (left == shared_bytes::invalid && right == shared_bytes::invalid) {
         return true;
      }

      if (left == shared_bytes::invalid) {
         return false;
      }

      return true;
   };

   return make_iterator_<const_iterator>([&](auto& ds) { return ds.find(key); }, comparator,
                                         [](auto& it, auto& end) { it = end; });
}

template <typename Data_store>
typename session<Data_store>::iterator session<Data_store>::begin() {
   return make_iterator_<iterator>([](auto& ds) { return std::begin(ds); }, std::less<shared_bytes>{},
                                   [](auto& it, auto& end) { ++it; });
}

template <typename Data_store>
typename session<Data_store>::const_iterator session<Data_store>::begin() const {
   return make_iterator_<const_iterator>([](auto& ds) { return std::begin(ds); }, std::less<shared_bytes>{},
                                         [](auto& it, auto& end) { ++it; });
}

template <typename Data_store>
typename session<Data_store>::iterator session<Data_store>::end() {
   return make_iterator_<iterator>([](auto& ds) { return std::end(ds); }, std::greater<shared_bytes>{},
                                   [](auto& it, auto& end) { it = end; });
}

template <typename Data_store>
typename session<Data_store>::const_iterator session<Data_store>::end() const {
   return make_iterator_<const_iterator>([](auto& ds) { return std::end(ds); }, std::greater<shared_bytes>{},
                                         [](auto& it, auto& end) { it = end; });
}

template <typename Data_store>
typename session<Data_store>::iterator session<Data_store>::lower_bound(const shared_bytes& key) {
   return make_iterator_<iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::less<shared_bytes>{},
                                   [](auto& it, auto& end) { ++it; });
}

template <typename Data_store>
typename session<Data_store>::const_iterator session<Data_store>::lower_bound(const shared_bytes& key) const {
   return make_iterator_<const_iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::less<shared_bytes>{},
                                         [](auto& it, auto& end) { ++it; });
}

template <typename Data_store>
typename session<Data_store>::iterator session<Data_store>::upper_bound(const shared_bytes& key) {
   return make_iterator_<iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<shared_bytes>{},
                                   [](auto& it, auto& end) { ++it; });
}

template <typename Data_store>
typename session<Data_store>::const_iterator session<Data_store>::upper_bound(const shared_bytes& key) const {
   return make_iterator_<const_iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<shared_bytes>{},
                                         [](auto& it, auto& end) { ++it; });
}

// Moves the current iterator.
//
// \tparam Predicate A functor that indicates if we are incrementing or decrementing the current iterator.
// \tparam Comparator A functor used to determine what the current iterator is.
template <typename Data_store>
template <typename Iterator_traits>
template <typename Test_predicate, typename Move_predicate>
void session<Data_store>::session_iterator<Iterator_traits>::move_(const Test_predicate& test,
                                                                   const Move_predicate& move) {

   do {
      if (!test(m_active_iterator)) {
         // Force an update to see if we pull in a next or previous key from the current key.
         m_active_session->m_impl->update_iterator_cache(
               m_active_iterator->first,
               { .prime_only = false, .recalculate = true, .mark_deleted = false, .overwrite = false });
         if (!test(m_active_iterator)) {
            // The test still fails.  We are at the end.
            m_active_iterator = std::end(m_active_session->m_impl->iterator_cache);
            break;
         }
      }
      // Mpve to the next iterator in the cache.
      move(m_active_iterator);
      if (!m_active_iterator->second.deleted ||
          m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
         // We either found a key that hasn't been deleted or we hit the end of the iterator cache.
         break;
      }
   } while (true);
}

template <typename Data_store>
template <typename Iterator_traits>
void session<Data_store>::session_iterator<Iterator_traits>::move_next_() {
   auto move     = [](auto& it) { ++it; };
   auto test     = [](auto& it) { return it->second.next_in_cache; };
   auto rollover = [&]() {
      if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
         // Rollover...
         m_active_iterator = std::begin(m_active_session->m_impl->iterator_cache);
      }
   };

   move_(test, move);
   rollover();
}

template <typename Data_store>
template <typename Iterator_traits>
void session<Data_store>::session_iterator<Iterator_traits>::move_previous_() {
   auto move = [](auto& it) { --it; };
   auto test = [&](auto& it) {
      if (it != std::end(m_active_session->m_impl->iterator_cache)) {
         return it->second.previous_in_cache;
      }
      return true;
   };
   auto rollover = [&]() {
      if (m_active_iterator == std::begin(m_active_session->m_impl->iterator_cache)) {
         // Rollover...
         m_active_iterator = std::end(m_active_session->m_impl->iterator_cache);
         if (m_active_iterator != std::begin(m_active_session->m_impl->iterator_cache)) {
            --m_active_iterator;
         }
      }
   };

   rollover();
   move_(test, move);
}

template <typename Data_store, typename Iterator_traits>
using session_iterator_alias = typename session<Data_store>::template session_iterator<Iterator_traits>;

template <typename Data_store>
template <typename Iterator_traits>
session_iterator_alias<Data_store, Iterator_traits>&
session<Data_store>::session_iterator<Iterator_traits>::operator++() {
   move_next_();
   return *this;
}

template <typename Data_store>
template <typename Iterator_traits>
session_iterator_alias<Data_store, Iterator_traits>
session<Data_store>::session_iterator<Iterator_traits>::operator++(int) {
   auto new_iterator = *this;
   move_next_();
   return new_iterator;
}

template <typename Data_store>
template <typename Iterator_traits>
session_iterator_alias<Data_store, Iterator_traits>&
session<Data_store>::session_iterator<Iterator_traits>::operator--() {
   move_previous_();
   return *this;
}

template <typename Data_store>
template <typename Iterator_traits>
session_iterator_alias<Data_store, Iterator_traits>
session<Data_store>::session_iterator<Iterator_traits>::operator--(int) {
   auto new_iterator = *this;
   move_previous_();
   return new_iterator;
}

template <typename Data_store>
template <typename Iterator_traits>
bool session<Data_store>::session_iterator<Iterator_traits>::deleted() const {
   if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
      return false;
   }
   return m_active_iterator->second.deleted;
}

template <typename Data_store>
template <typename Iterator_traits>
typename session_iterator_alias<Data_store, Iterator_traits>::value_type
session<Data_store>::session_iterator<Iterator_traits>::operator*() const {
   if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
      return key_value::invalid;
   }
   return m_active_session->read(m_active_iterator->first);
}

template <typename Data_store>
template <typename Iterator_traits>
typename session_iterator_alias<Data_store, Iterator_traits>::value_type
session<Data_store>::session_iterator<Iterator_traits>::operator->() const {
   if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
      return key_value::invalid;
   }
   return m_active_session->read(m_active_iterator->first);
}

template <typename Data_store>
template <typename Iterator_traits>
bool session<Data_store>::session_iterator<Iterator_traits>::operator==(const session_iterator& other) const {
   if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache) &&
       m_active_iterator == other.m_active_iterator) {
      return true;
   }
   if (other.m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
      return false;
   }
   return this->m_active_iterator->first == other.m_active_iterator->first;
}

template <typename Data_store>
template <typename Iterator_traits>
bool session<Data_store>::session_iterator<Iterator_traits>::operator!=(const session_iterator& other) const {
   return !(*this == other);
}

} // namespace eosio::session
