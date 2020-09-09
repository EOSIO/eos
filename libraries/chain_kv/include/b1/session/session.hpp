#pragma once

#include <queue>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <b1/session/key_value.hpp>
#include <b1/session/session_fwd_decl.hpp>
#include <b1/session/shared_bytes.hpp>

namespace eosio::session {

// Defines a session for reading/write data to a cache and persistent data store.
//
// \tparam Persistent_data_store A type that persists data to disk or some other non memory backed storage device.
// \tparam Cache_data_store A type that persists data in memory for quick access.
// \code{.cpp}
//  auto memory_pool = eosio::session::boost_memory_allocator::make();
//  auto pds = eosio::session::make_rocks_data_store(nullptr, memory_pool);
//  auto cds = eosio::session::make_cache(memory_pool);
//  auto fork = eosio::session::make_session(std::move(pds), std::move(cds));
//  {
//      auto block = eosio::session::make_session(fork);
//      {
//          auto transaction = eosio::session::make_session(block);
//          auto kv = eosio::session::make_kv("Hello", 5, "World", 5,
//          transaction.backing_data_store()->memory_allocator()); transaction.write(std::move(kv));
//      }
//      {
//          auto transaction = eosio::session::make_session(block);
//          auto kv = eosio::session::make_kv("Hello", 5, "World2", 5,
//          transaction.backing_data_store()->memory_allocator()); transaction.write(std::move(kv));
//      }
//  }
// \endcode
template <typename Persistent_data_store, typename Cache_data_store>
class session final {
 private:
   struct session_impl;

 public:
   struct iterator_state final {
      bool next_in_cache{ false };
      bool previous_in_cache{ false };
      bool deleted{ false };
   };

   using persistent_data_store_type = Persistent_data_store;
   using cache_data_store_type      = Cache_data_store;
   using iterator_cache_type        = std::map<shared_bytes, iterator_state>;

   static const session invalid;

   template <typename T, typename Allocator>
   friend shared_bytes make_shared_bytes(const T* data, size_t length, Allocator& a);

   friend key_value make_kv(shared_bytes key, shared_bytes value);

   template <typename Key, typename Value, typename Allocator>
   friend key_value make_kv(const Key* key, size_t key_length, const Value* value, size_t value_length, Allocator& a);

   template <typename Allocator>
   friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, Allocator& a);

   template <typename Persistent_data_store_, typename Cache_data_store_>
   friend session<Persistent_data_store_, Cache_data_store_> make_session();

   template <typename Persistent_data_store_, typename Cache_data_store_>
   friend session<Persistent_data_store_, Cache_data_store_> make_session(Persistent_data_store_ store);

   template <typename Persistent_data_store_, typename Cache_data_store_>
   friend session<Persistent_data_store_, Cache_data_store_> make_session(Persistent_data_store_ store,
                                                                          Cache_data_store_      cache);

   template <typename Persistent_data_store_, typename Cache_data_store_>
   friend session<Persistent_data_store_, Cache_data_store_>
   make_session(session<Persistent_data_store_, Cache_data_store_>& the_session);

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

    protected:
      template <typename Test_predicate, typename DB_test_predicate, typename Move_predicate>
      void move_(const Test_predicate& test, const DB_test_predicate& db_test, const Move_predicate& move);
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
      using cache_iterator      = typename Cache_data_store::iterator;
      using data_store_iterator = typename Persistent_data_store::iterator;
   };
   using iterator = session_iterator<iterator_traits>;

   struct const_iterator_traits final {
      using difference_type     = long;
      using value_type          = const key_value;
      using pointer             = value_type*;
      using reference           = value_type&;
      using iterator_category   = std::bidirectional_iterator_tag;
      using cache_iterator      = typename Cache_data_store::iterator;
      using data_store_iterator = typename Persistent_data_store::iterator;
   };
   using const_iterator = session_iterator<const_iterator_traits>;

   struct nested_session_t final {};

 public:
   session(Persistent_data_store ds);
   session(Persistent_data_store ds, Cache_data_store cache);
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

   const std::shared_ptr<typename Persistent_data_store::allocator_type>&      memory_allocator();
   const std::shared_ptr<const typename Persistent_data_store::allocator_type> memory_allocator() const;
   const std::shared_ptr<Persistent_data_store>&                               backing_data_store();
   const std::shared_ptr<const Persistent_data_store>&                         backing_data_store() const;
   const std::shared_ptr<Cache_data_store>&                                    cache();
   const std::shared_ptr<const Cache_data_store>&                              cache() const;

 private:
   session();
   session(std::shared_ptr<session_impl> impl);

   template <typename Iterator_type, typename Predicate, typename Comparator>
   Iterator_type make_iterator_(const Predicate& p, const Comparator& c) const;

   std::pair<shared_bytes, shared_bytes> bounds_(const shared_bytes& key) const;
   void update_iterator_cache_(const shared_bytes& key, bool overwrite = true, bool erase = false) const;
   bool is_deleted_(const shared_bytes& key, const std::shared_ptr<session_impl>& current_session) const;

   struct session_impl final : public std::enable_shared_from_this<session_impl> {
      session_impl();
      session_impl(session_impl& parent_);
      session_impl(Persistent_data_store pds);
      session_impl(Persistent_data_store pds, Cache_data_store cds);
      ~session_impl();

      void undo();
      void commit();
      void clear();

      std::shared_ptr<session_impl> parent;
      std::weak_ptr<session_impl>   child;

      // The persistent data store.  This is shared across all levels of the session.
      std::shared_ptr<Persistent_data_store> backing_data_store;

      // The cache used by this session instance.  This will include all new/update key_values
      // and may include values read from the persistent data store.
      Cache_data_store cache;

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

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store> make_session() {
   using session_type = session<Persistent_data_store, Cache_data_store>;
   return session_type{};
}

template <typename Cache_data_store, typename Persistent_data_store>
session<Persistent_data_store, Cache_data_store> make_session(Persistent_data_store store) {
   using session_type = session<Persistent_data_store, Cache_data_store>;
   return session_type{ std::move(store) };
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store> make_session(Persistent_data_store store, Cache_data_store cache) {
   using session_type = session<Persistent_data_store, Cache_data_store>;
   return session_type{ std::move(store), std::move(cache) };
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>
make_session(session<Persistent_data_store, Cache_data_store>& the_session) {
   using session_type                = session<Persistent_data_store, Cache_data_store>;
   auto new_session                  = session_type{ the_session, typename session_type::nested_session_t{} };
   new_session.m_impl->parent->child = new_session.m_impl->shared_from_this();
   return new_session;
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session_impl::session_impl()
    : backing_data_store{ std::make_shared<Persistent_data_store>() } {}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session_impl::session_impl(session_impl& parent_)
    : parent{ parent_.shared_from_this() }, backing_data_store{ parent_.backing_data_store }, cache{
         parent_.cache.memory_allocator()
      } {
   if (auto child_ptr = parent->child.lock()) {
      child_ptr->backing_data_store = nullptr;
      child_ptr->parent             = nullptr;
   }

   // Prime the iterator cache with the begin and end values from the parent.
   auto begin = std::begin(parent_.iterator_cache);
   auto end   = std::end(parent_.iterator_cache);
   if (begin != end) {
      iterator_cache.emplace(begin->first, iterator_state{});
      iterator_cache.emplace((--end)->first, iterator_state{});
   }
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session_impl::session_impl(Persistent_data_store pds)
    : backing_data_store{ std::make_shared<Persistent_data_store>(std::move(pds)) }, cache{ pds.memory_allocator() } {
   // Prime the iterator cache with the begin and end values from the database.
   auto begin = std::begin(*backing_data_store);
   auto end   = std::end(*backing_data_store);
   if (begin != end) {
      iterator_cache.emplace((*begin).key(), iterator_state{});
      iterator_cache.emplace((*--end).key(), iterator_state{});
   }
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session_impl::session_impl(Persistent_data_store pds,
                                                                             Cache_data_store      cds)
    : backing_data_store{ std::make_shared<Persistent_data_store>(std::move(pds)) }, cache{ std::move(cds) } {
   auto emplace_bounds = [&](const auto& container) {
      auto begin = std::begin(container);
      auto end   = std::end(container);
      if (begin != end) {
         iterator_cache.emplace((*begin).key(), iterator_state{});
         iterator_cache.emplace((*--end).key(), iterator_state{});
      }
   };
   // Prime the iterator cache with the begin and end values from cache and the database.
   emplace_bounds(*backing_data_store);
   emplace_bounds(cache);
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session_impl::~session_impl() {
   // When this goes out of scope:
   // 1.  If there is no parent and no backing data store, then this session has been abandoned (undo)
   // 2.  If there is no parent, then this session is being committed to the data store. (commit)
   // 3.  If there is a parent, then this session is being merged into the parent. (squash)
   commit();
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::session_impl::undo() {
   if (parent) {
      parent->child = child;
   }

   if (auto child_ptr = child.lock()) {
      child_ptr->parent = parent;
   }

   parent = nullptr;
   child.reset();
   backing_data_store = nullptr;
   clear();
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::session_impl::commit() {
   if (!parent && !backing_data_store) {
      // undo
      return;
   }

   auto write_through = [&](auto& ds) {
      ds.erase(deleted_keys);
      cache.write_to(ds, updated_keys);
      clear();
   };

   if (parent) {
      // squash
      auto parent_session = session{ parent };
      write_through(parent_session);
      return;
   }

   // commit
   write_through(*backing_data_store);
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::session_impl::clear() {
   deleted_keys.clear();
   updated_keys.clear();
   cache.clear();
   iterator_cache.clear();
}

template <typename Persistent_data_store, typename Cache_data_store>
const session<Persistent_data_store, Cache_data_store> session<Persistent_data_store, Cache_data_store>::invalid{};

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session() : m_impl{ std::make_shared<session_impl>() } {}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session(std::shared_ptr<session_impl> impl)
    : m_impl{ std::move(impl) } {}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session(Persistent_data_store ds)
    : m_impl{ std::make_shared<session_impl>(std::move(ds)) } {}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session(Persistent_data_store ds, Cache_data_store cache)
    : m_impl{ std::make_shared<session_impl>(std::move(ds), std::move(cache)) } {}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::session(session& session, nested_session_t)
    : m_impl{ std::make_shared<session_impl>(*session.m_impl) } {}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>::~session() {}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::undo() {
   if (!m_impl) {
      return;
   }

   m_impl->undo();
}

template <typename Persistent_data_store, typename Cache_data_store>
std::pair<shared_bytes, shared_bytes>
session<Persistent_data_store, Cache_data_store>::bounds_(const shared_bytes& key) const {
   auto lower_bound_key = shared_bytes::invalid;
   auto upper_bound_key = shared_bytes::invalid;

   auto lower_bound = [&, this](const auto& session, const auto& container, const auto& key) {
      auto it = container.lower_bound(key);
      if (it != std::end(container) && it != std::begin(container)) {
         --it;
         while (is_deleted_((*it).key(), session)) {
            if (it == std::begin(container)) {
               return shared_bytes::invalid;
            }
            --it;
         }
         return (*it).key();
      }
      return shared_bytes::invalid;
   };

   auto upper_bound = [&, this](const auto& session, const auto& container, const auto& key) {
      auto it = container.upper_bound(key);
      if (it == std::end(container)) {
         return shared_bytes::invalid;
      }

      while (is_deleted_((*it).key(), session)) {
         ++it;
         if (it == std::end(container)) {
            return shared_bytes::invalid;
         }
      }

      return (*it).key();
   };

   auto test_set = [&](auto& current_key, const auto& pending_key, const auto& comparator) {
      if (current_key == shared_bytes::invalid && pending_key == shared_bytes::invalid) {
         return;
      }

      if (current_key == shared_bytes::invalid) {
         current_key = pending_key;
         return;
      }

      if (comparator(pending_key, current_key)) {
         current_key = pending_key;
         return;
      }
   };

   auto parent  = m_impl;
   auto current = m_impl;
   while (current) {
      if (auto lower_bound_key_ = lower_bound(current, current->cache, key);
          lower_bound_key_ != shared_bytes::invalid) {
         test_set(lower_bound_key, lower_bound_key_, std::greater<shared_bytes>{});
      }
      if (auto upper_bound_key_ = upper_bound(current, current->cache, key);
          upper_bound_key_ != shared_bytes::invalid) {
         test_set(upper_bound_key, upper_bound_key_, std::less<shared_bytes>{});
      }

      parent  = current;
      current = current->parent;
   }

   if (auto lower_bound_key_ = lower_bound(parent, *m_impl->backing_data_store, key);
       lower_bound_key_ != shared_bytes::invalid) {
      test_set(lower_bound_key, lower_bound_key_, std::greater<shared_bytes>{});
   }

   if (auto upper_bound_key_ = upper_bound(parent, *m_impl->backing_data_store, key);
       upper_bound_key_ != shared_bytes::invalid) {
      test_set(upper_bound_key, upper_bound_key_, std::less<shared_bytes>{});
   }

   return std::pair{ lower_bound_key, upper_bound_key };
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::update_iterator_cache_(const shared_bytes& key, bool overwrite,
                                                                              bool erase) const {
   auto it = m_impl->iterator_cache.emplace(key, iterator_state{}).first;

   if (overwrite) {
      it->second.deleted = erase;
   }

   if (it->second.next_in_cache && it->second.previous_in_cache) {
      // The bounds have already been set.  No need to search for them again.
      return;
   }

   auto [lower_bound, upper_bound] = bounds_(key);

   if (lower_bound != shared_bytes::invalid) {
      auto lower_it = m_impl->iterator_cache.find(lower_bound);
      if (lower_it == std::end(m_impl->iterator_cache)) {
         lower_it = m_impl->iterator_cache.emplace(lower_bound, iterator_state{}).first;
      }
      lower_it->second.next_in_cache = true;
      it->second.previous_in_cache   = true;
   }

   if (upper_bound != shared_bytes::invalid) {
      auto upper_it = m_impl->iterator_cache.find(upper_bound);
      if (upper_it == std::end(m_impl->iterator_cache)) {
         upper_it = m_impl->iterator_cache.emplace(upper_bound, iterator_state{}).first;
      }
      upper_it->second.previous_in_cache = true;
      it->second.next_in_cache           = true;
   }
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store>
session<Persistent_data_store, Cache_data_store>::attach(session child) {
   if (!m_impl) {
      return session<Persistent_data_store, Cache_data_store>::invalid;
   }

   auto current_child = detach();

   child.m_impl->parent             = m_impl;
   child.m_impl->backing_data_store = m_impl->backing_data_store;
   m_impl->child                    = child.m_impl;

   return current_child;
}

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store> session<Persistent_data_store, Cache_data_store>::detach() {
   if (!m_impl) {
      return session<Persistent_data_store, Cache_data_store>::invalid;
   }

   auto current_child = m_impl->child.lock();
   if (current_child) {
      current_child->parent             = nullptr;
      current_child->backing_data_store = nullptr;
   }
   m_impl->child.reset();

   return { current_child };
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::commit() {
   if (!m_impl) {
      return;
   }

   m_impl->commit();
}

template <typename Persistent_data_store, typename Cache_data_store>
const key_value session<Persistent_data_store, Cache_data_store>::read(const shared_bytes& key) const {
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
            update_iterator_cache_(kv.key(), false);
         }
         return kv;
      }

      parent = parent->parent.get();
   }

   // If we haven't found the key in the session yet, read it from the persistent data store.
   if (m_impl->backing_data_store) {
      auto kv = m_impl->backing_data_store->read(key);
      if (kv != key_value::invalid) {
         m_impl->cache.write(kv);
         update_iterator_cache_(kv.key(), false);
         return kv;
      }
   }

   return key_value::invalid;
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::write(key_value kv) {
   if (!m_impl) {
      return;
   }
   auto key = kv.key();
   m_impl->updated_keys.emplace(key);
   m_impl->deleted_keys.erase(key);
   m_impl->cache.write(std::move(kv));
   update_iterator_cache_(key);
}

template <typename Persistent_data_store, typename Cache_data_store>
bool session<Persistent_data_store, Cache_data_store>::contains(const shared_bytes& key) const {
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
         update_iterator_cache_(key, false);
         return true;
      }

      parent = parent->parent.get();
   }

   // If we get to this point, the key hasn't been read into memory.  Check to see if it exists in the persistent data
   // store.
   return m_impl->backing_data_store->contains(key);
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::erase(const shared_bytes& key) {
   if (!m_impl) {
      return;
   }
   m_impl->deleted_keys.emplace(key);
   m_impl->updated_keys.erase(key);
   m_impl->cache.erase(key);
   update_iterator_cache_(key, true, true);
}

template <typename Persistent_data_store, typename Cache_data_store>
void session<Persistent_data_store, Cache_data_store>::clear() {
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
template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterable>
const std::pair<std::vector<key_value>, std::unordered_set<shared_bytes>>
session<Persistent_data_store, Cache_data_store>::read(const Iterable& keys) const {
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
               update_iterator_cache_(kv.key(), false);
            }
            found = true;
            kvs.emplace_back(kv);
            break;
         }
         parent = parent->parent.get();
      }

      if (!found) {
         not_found.emplace(key);
      }
   }

   // For all the keys that were found in one of the session caches, read them from the persistent data store
   // and write them into our cache.
   if (m_impl->backing_data_store) {
      auto [found, missing] = m_impl->backing_data_store->read(not_found);
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
template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterable>
void session<Persistent_data_store, Cache_data_store>::write(const Iterable& key_values) {
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
template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterable>
void session<Persistent_data_store, Cache_data_store>::erase(const Iterable& keys) {
   if (!m_impl) {
      return;
   }

   // Currently the batch erase will just iteratively call the non batch erase
   for (const auto& key : keys) { erase(key); }
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Data_store, typename Iterable>
void session<Persistent_data_store, Cache_data_store>::write_to(Data_store& ds, const Iterable& keys) const {
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
            parent = parent->parent.get();
            continue;
         }

         results.emplace_back(make_kv(kv.key().data(), kv.key().length(), kv.value().data(), kv.value().length(),
                                      ds.memory_allocator()));
         break;
      }
   }
   ds.write(results);
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Data_store, typename Iterable>
void session<Persistent_data_store, Cache_data_store>::read_from(const Data_store& ds, const Iterable& keys) {
   if (!m_impl) {
      return;
   }
   ds.write_to(*this, keys);
}

// Given a session and a key read from that session, check if it has been deleted by any child sessions.
template <typename Persistent_data_store, typename Cache_data_store>
bool session<Persistent_data_store, Cache_data_store>::is_deleted_(
      const shared_bytes& key, const std::shared_ptr<session_impl>& current_session) const {
   if (current_session->child.expired()) {
      return false;
   }

   auto current = current_session->child.lock();
   while (current) {
      if (current->updated_keys.find(key) != std::end(current->updated_keys)) {
         return false;
      }

      if (current->deleted_keys.find(key) != std::end(current->deleted_keys)) {
         return true;
      }

      current = current->child.lock();
   }
   return false;
}

// Factor for creating the initial session iterator.
//
// \tparam Predicate A functor used for getting the starting iterating in the cache list and the persistent data store.
// \tparam Comparator A functor for determining if the "current" iterator points to the cache list or the persistent
// data store.
template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_type, typename Predicate, typename Comparator>
Iterator_type session<Persistent_data_store, Cache_data_store>::make_iterator_(const Predicate&  p,
                                                                               const Comparator& c) const {
   if (!m_impl) {
      // For some reason the pimpl is nullptr.  Return an "invalid" iterator.
      return Iterator_type{};
   }

   auto new_iterator              = Iterator_type{};
   new_iterator.m_active_session  = const_cast<session*>(this);
   new_iterator.m_active_iterator = std::end(m_impl->iterator_cache);

   // Move to the head.  We start at the head because we want to end up with
   // an iterator on the session at the tail of the session list.
   auto parent = m_impl;
   while (parent->parent) { parent = parent->parent; }

   // So start at the database.
   auto it          = p(*m_impl->backing_data_store);
   auto current_key = it != std::end(*m_impl->backing_data_store) ? (*it).key() : shared_bytes::invalid;

   // Check the other levels to see which key we should start at.
   auto current = parent;
   while (current) {
      auto pending = p(current->cache);

      if (current_key != shared_bytes::invalid &&
          current->deleted_keys.find(current_key) != std::end(current->deleted_keys)) {
         // Key is deleted at this level
         current_key = shared_bytes::invalid;
      }

      if (pending == std::end(current->cache)) {
         current = current->child.lock();
         continue;
      }

      auto pending_key = (*pending).key();
      auto it          = current->iterator_cache.find(pending_key);

      if (it->second.deleted) {
         current = current->child.lock();
         continue;
      }

      if (current_key == shared_bytes::invalid || c(pending_key, current_key)) {
         current_key = pending_key;
      }

      current = current->child.lock();
   }

   // Update the iterator cache.
   if (current_key != shared_bytes::invalid) {
      new_iterator.m_active_iterator = m_impl->iterator_cache.find(current_key);
      if (new_iterator.m_active_iterator->second.deleted) {
         new_iterator.m_active_iterator = std::end(m_impl->iterator_cache);
      } else {
         session{ m_impl }.update_iterator_cache_(current_key, false);
      }
   }

   return new_iterator;
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::iterator
session<Persistent_data_store, Cache_data_store>::find(const shared_bytes& key) {
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

   return make_iterator_<iterator>([&](auto& ds) { return ds.find(key); }, comparator);
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::const_iterator
session<Persistent_data_store, Cache_data_store>::find(const shared_bytes& key) const {
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

   return make_iterator_<const_iterator>([&](auto& ds) { return ds.find(key); }, comparator);
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::iterator
session<Persistent_data_store, Cache_data_store>::begin() {
   return make_iterator_<iterator>([](auto& ds) { return std::begin(ds); }, std::less<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::const_iterator
session<Persistent_data_store, Cache_data_store>::begin() const {
   return make_iterator_<const_iterator>([](auto& ds) { return std::begin(ds); }, std::less<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::iterator
session<Persistent_data_store, Cache_data_store>::end() {
   return make_iterator_<iterator>([](auto& ds) { return std::end(ds); }, std::greater<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::const_iterator
session<Persistent_data_store, Cache_data_store>::end() const {
   return make_iterator_<const_iterator>([](auto& ds) { return std::end(ds); }, std::greater<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::iterator
session<Persistent_data_store, Cache_data_store>::lower_bound(const shared_bytes& key) {
   return make_iterator_<iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::greater<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::const_iterator
session<Persistent_data_store, Cache_data_store>::lower_bound(const shared_bytes& key) const {
   return make_iterator_<const_iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::greater<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::iterator
session<Persistent_data_store, Cache_data_store>::upper_bound(const shared_bytes& key) {
   return make_iterator_<iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
typename session<Persistent_data_store, Cache_data_store>::const_iterator
session<Persistent_data_store, Cache_data_store>::upper_bound(const shared_bytes& key) const {
   return make_iterator_<const_iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<shared_bytes>{});
}

template <typename Persistent_data_store, typename Cache_data_store>
const std::shared_ptr<typename Persistent_data_store::allocator_type>&
session<Persistent_data_store, Cache_data_store>::memory_allocator() {
   if (!m_impl) {
      static auto invalid = std::shared_ptr<typename Persistent_data_store::allocator_type>{};
      return invalid;
   }

   if (m_impl->backing_data_store) {
      return m_impl->backing_data_store->memory_allocator();
   }

   return m_impl->cache.memory_allocator();
}

template <typename Persistent_data_store, typename Cache_data_store>
const std::shared_ptr<const typename Persistent_data_store::allocator_type>
session<Persistent_data_store, Cache_data_store>::memory_allocator() const {
   if (!m_impl) {
      static auto invalid = std::shared_ptr<typename Persistent_data_store::allocator_type>{};
      return invalid;
   }

   if (m_impl->backing_data_store) {
      return m_impl->backing_data_store->memory_allocator();
   }

   return m_impl->cache.memory_allocator();
}

template <typename Persistent_data_store, typename Cache_data_store>
const std::shared_ptr<Persistent_data_store>& session<Persistent_data_store, Cache_data_store>::backing_data_store() {
   if (!m_impl) {
      static auto invalid = std::shared_ptr<Persistent_data_store>{};
      return invalid;
   }

   return m_impl->backing_data_store;
}

template <typename Persistent_data_store, typename Cache_data_store>
const std::shared_ptr<const Persistent_data_store>&
session<Persistent_data_store, Cache_data_store>::backing_data_store() const {
   if (!m_impl) {
      static auto invalid = std::shared_ptr<Persistent_data_store>{};
      return invalid;
   }

   return m_impl->backing_data_store;
}

template <typename Persistent_data_store, typename Cache_data_store>
const std::shared_ptr<Cache_data_store>& session<Persistent_data_store, Cache_data_store>::cache() {
   if (!m_impl) {
      static auto invalid = std::shared_ptr<Cache_data_store>{};
      return invalid;
   }

   return m_impl->cache;
}

template <typename Persistent_data_store, typename Cache_data_store>
const std::shared_ptr<const Cache_data_store>& session<Persistent_data_store, Cache_data_store>::cache() const {
   if (!m_impl) {
      static auto invalid = std::shared_ptr<Cache_data_store>{};
      return invalid;
   }

   return m_impl->cache;
}

// Moves the current iterator.
//
// \tparam Predicate A functor that indicates if we are incrementing or decrementing the current iterator.
// \tparam Comparator A functor used to determine what the current iterator is.
template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
template <typename Test_predicate, typename DB_test_predicate, typename Move_predicate>
void session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::move_(
      const Test_predicate& test, const DB_test_predicate& db_test, const Move_predicate& move) {
   auto set = false;

   if (test(m_active_iterator)) {
      // If the next/previous value is in the active session's cache, just increment/decrement the iterator
      // until one is encountered that is not deleted.
      move(m_active_iterator);
      while (m_active_iterator->second.deleted) { move(m_active_iterator); }
      set = true;
   } else {
      auto current = m_active_session->m_impl->parent;
      while (current) {
         // The next/previous key isn't in the active session's cache, move up to the head of the session list
         // until we find the next/previous key.
         auto it = current->iterator_cache.find(m_active_iterator->first);
         if (test(it)) {
            move(it);

            while (it->second.deleted) { move(it); }

            m_active_session->update_iterator_cache_(it->first, false);
            m_active_iterator = m_active_session->m_impl->iterator_cache.find(it->first);
            set               = true;
            break;
         }
         current = current->parent;
      }

      if (!current) {
         // We didn't find the key in any of the caches.  The database will tell us what is next.
         auto it = m_active_session->m_impl->backing_data_store->find(m_active_iterator->first);
         if (db_test(it)) {
            move(it);
            if (it != std::end(*m_active_session->m_impl->backing_data_store)) {
               // We don't need to check if the key is deleted.  That information should have been encoded in the
               // sessions above this one and those keys would have been skipped.
               auto key = (*it).key();
               m_active_session->update_iterator_cache_(key, false);
               m_active_iterator = m_active_session->m_impl->iterator_cache.find(key);
               set               = true;
            }
         }
      }
   }

   if (!set) {
      // We didn't find a value. Or we moved to the end iterator.
      m_active_iterator = std::end(m_active_session->m_impl->iterator_cache);
   }
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
void session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::move_next_() {
   auto move     = [](auto& it) { ++it; };
   auto test     = [](auto& it) { return it->second.next_in_cache; };
   auto db_test  = [&](auto& it) { return it != std::end(*m_active_session->m_impl->backing_data_store); };
   auto rollover = [&]() {
      if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
         // Rollover...
         m_active_iterator = std::begin(m_active_session->m_impl->iterator_cache);
      }
   };

   move_(test, db_test, move);
   rollover();
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
void session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::move_previous_() {
   auto move = [](auto& it) { --it; };
   auto test = [&](auto& it) {
      if (it != std::end(m_active_session->m_impl->iterator_cache)) {
         return it->second.previous_in_cache;
      }
      return true;
   };
   auto db_test  = [&](auto& it) { return it != std::begin(*m_active_session->m_impl->backing_data_store); };
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
   move_(test, db_test, move);
}

template <typename Persistent_data_store, typename Cache_data_store, typename Iterator_traits>
using session_iterator_alias =
      typename session<Persistent_data_store, Cache_data_store>::template session_iterator<Iterator_traits>;

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
session_iterator_alias<Persistent_data_store, Cache_data_store, Iterator_traits>&
session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator++() {
   move_next_();
   return *this;
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
session_iterator_alias<Persistent_data_store, Cache_data_store, Iterator_traits>
session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator++(int) {
   auto new_iterator = *this;
   move_next_();
   return new_iterator;
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
session_iterator_alias<Persistent_data_store, Cache_data_store, Iterator_traits>&
session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator--() {
   move_previous_();
   return *this;
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
session_iterator_alias<Persistent_data_store, Cache_data_store, Iterator_traits>
session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator--(int) {
   auto new_iterator = *this;
   move_previous_();
   return new_iterator;
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
typename session_iterator_alias<Persistent_data_store, Cache_data_store, Iterator_traits>::value_type
session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator*() const {
   if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
      return key_value::invalid;
   }
   return m_active_session->read(m_active_iterator->first);
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
typename session_iterator_alias<Persistent_data_store, Cache_data_store, Iterator_traits>::value_type
session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator->() const {
   if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
      return key_value::invalid;
   }
   return m_active_session->read(m_active_iterator->first);
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
bool session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator==(
      const session_iterator& other) const {
   if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache) &&
       m_active_iterator == other.m_active_iterator) {
      return true;
   }
   if (other.m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
      return false;
   }
   return this->m_active_iterator->first == other.m_active_iterator->first;
}

template <typename Persistent_data_store, typename Cache_data_store>
template <typename Iterator_traits>
bool session<Persistent_data_store, Cache_data_store>::session_iterator<Iterator_traits>::operator!=(
      const session_iterator& other) const {
   return !(*this == other);
}

} // namespace eosio::session
