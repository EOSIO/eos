#pragma once

#include <set>
#include <unordered_set>

#include <b1/session/bytes.hpp>
#include <b1/session/key_value.hpp>
#include <b1/session/session_fwd_decl.hpp>

namespace eosio::session {

// Defines a session for reading/write data to a cache and persistent data store.
//
// \tparam persistent_data_store A type that persists data to disk or some other non memory backed storage device.
// \tparam cache_data_store A type that persists data in memory for quick access.
// \code{.cpp}
//  auto memory_pool = eosio::session::boost_memory_allocator::make();
//  auto pds = eosio::session::make_rocks_data_store(nullptr, memory_pool);
//  auto cds = eosio::session::make_cache(memory_pool);
//  auto fork = eosio::session::make_session(std::move(pds), std::move(cds));
//  {
//      auto block = eosio::session::make_session(fork);
//      {
//          auto transaction = eosio::session::make_session(block);
//          auto kv = eosio::session::make_kv("Hello", 5, "World", 5, transaction.backing_data_store()->memory_allocator());
//          transaction.write(std::move(kv));
//      }
//      {
//          auto transaction = eosio::session::make_session(block);
//          auto kv = eosio::session::make_kv("Hello", 5, "World2", 5, transaction.backing_data_store()->memory_allocator());
//          transaction.write(std::move(kv));
//      }
//  }
// \endcode
template <typename persistent_data_store, typename cache_data_store>
class session final {
private:
    struct session_impl;
    
public:
    using persistent_data_store_type = persistent_data_store;
    using cache_data_store_type = cache_data_store;
    
    static const session invalid;

    template <typename T, typename allocator>
    friend bytes make_bytes(const T* data, size_t length, allocator& a);
        
    friend key_value make_kv(bytes key, bytes value);

    template <typename Key, typename Value, typename allocator>
    friend key_value make_kv(const Key* key, size_t key_length, const Value* value, size_t value_length, allocator& a);
    
    template <typename allocator>
    friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a);
    
    template <typename persistent_data_store_, typename cache_data_store_>
    friend session<persistent_data_store_, cache_data_store_> make_session();

    template <typename persistent_data_store_, typename cache_data_store_>
    friend session<persistent_data_store_, cache_data_store_> make_session(persistent_data_store_ store);
    
    template <typename persistent_data_store_, typename cache_data_store_>
    friend session<persistent_data_store_, cache_data_store_> make_session(persistent_data_store_ store, cache_data_store_ cache);

    template <typename persistent_data_store_, typename cache_data_store_>
    friend session<persistent_data_store_, cache_data_store_> make_session(session<persistent_data_store_, cache_data_store_>& the_session);

    // Defines a key ordered, cyclical iterator for traversing a session (which includes parents and children of each session).
    // Basically we are iterating over a list of sessions and each session has its own cache of key_values, along with
    // iterating over a persistent data store all the while maintaining key order with the iterator.
    template <typename iterator_traits>
    class session_iterator final {
    public:
        using difference_type = typename iterator_traits::difference_type;
        using value_type = typename iterator_traits::value_type;
        using pointer = typename iterator_traits::pointer;
        using reference = typename iterator_traits::reference;
        using iterator_category = typename iterator_traits::iterator_category;

        friend session;
        
        // State information needed for iterating over the session caches.
        struct cache_iterator_state final {
            typename cache_data_store_type::iterator begin;
            typename cache_data_store_type::iterator end;
            typename cache_data_store_type::iterator current;
            std::unordered_set<bytes>* deleted_keys;
            std::unordered_set<bytes>* updated_keys;
        };
        
        // State information needed for itering over the persistent data store.
        struct database_iterator_state final {
            typename persistent_data_store_type::iterator begin;
            typename persistent_data_store_type::iterator end;
            typename persistent_data_store_type::iterator current;
        };
        
    public:
        session_iterator() = default;
        session_iterator(const session_iterator& it) = default;
        session_iterator(session_iterator&&) = default;
        
        session_iterator& operator=(const session_iterator& it) = default;
        session_iterator& operator=(session_iterator&&) = default;
        
        session_iterator& operator++();
        session_iterator operator++(int);
        session_iterator& operator--();
        session_iterator operator--(int);
        value_type operator*() const;
        value_type operator->() const;
        bool operator==(const session_iterator& other) const;
        bool operator!=(const session_iterator& other) const;
        
    protected:
        template <typename move_predicate, typename rollover_predicate, typename comparator>
        void move_(const move_predicate& move, const rollover_predicate& rollover,const comparator& key_comparator);
        void move_next_();
        void move_previous_();
        
    private:
        std::vector<cache_iterator_state> m_cache_iterator_states;
        database_iterator_state m_database_iterator_state;
        key_value m_current_value{value_type::invalid};
        size_t m_iterator_index{static_cast<size_t>(-1)};
    };

    struct iterator_traits {
        using difference_type = long;
        using value_type = key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
    };
    using iterator = session_iterator<iterator_traits>;

    struct const_iterator_traits {
        using difference_type = long;
        using value_type = const key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
    };
    using const_iterator = session_iterator<const_iterator_traits>;
    
    struct nested_session_t final {};
    
public:
    session(persistent_data_store ds);
    session(persistent_data_store ds, cache_data_store cache);
    session(session& session, nested_session_t);
    session(const session&) = default;
    session(session&&) = default;
    ~session();
    
    session& operator=(const session&) = default;
    session& operator=(session&&) = default;
    
    session attach(session child);
    session detach();

    // undo stack operations
    auto undo() -> void;
    auto commit() -> void;
    
    // this part also identifies a concept.  don't know what to call it yet
    const key_value read(const bytes& key) const;
    void write(key_value kv);
    bool contains(const bytes& key) const;
    void erase(const bytes& key);
    
    // returns a pair containing the key_values found and the keys not found.
    template <typename iterable>
    const std::pair<std::vector<key_value>, std::unordered_set<bytes>> read(const iterable& keys) const;
    
    template <typename iterable>
    void write(const iterable& key_values);
    
    // returns a list of the keys not found.
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
    
    const std::shared_ptr<typename persistent_data_store::allocator_type>& memory_allocator();
    const std::shared_ptr<const typename persistent_data_store::allocator_type> memory_allocator() const;
    const std::shared_ptr<persistent_data_store>& backing_data_store();
    const std::shared_ptr<const persistent_data_store>& backing_data_store() const;
    const std::shared_ptr<cache_data_store>& cache();
    const std::shared_ptr<const cache_data_store>& cache() const;
    
private:
    session();
    session(std::shared_ptr<session_impl> impl);
    
    template <typename iterator_type, typename predicate, typename comparator>
    iterator_type make_iterator_(const predicate& p, const comparator& c) const;
    
    struct session_impl final : public std::enable_shared_from_this<session_impl> {
        session_impl();
        session_impl(session_impl& parent_);
        session_impl(persistent_data_store pds);
        session_impl(persistent_data_store pds, cache_data_store cds);
        ~session_impl();
        
        auto undo() -> void;
        auto commit() -> void;
 
        std::shared_ptr<session_impl> parent;
        std::weak_ptr<session_impl> child;
        
        // The persistent data store.  This is shared across all levels of the session.
        std::shared_ptr<persistent_data_store> backing_data_store;
        
        // The cache used by this session instance.  This will include all new/update key_values
        // and may include values read from the persistent data store.
        cache_data_store cache;
        // keys that have been updated during this session.
        std::unordered_set<bytes> updated_keys;
        // keys that have been deleted during this session.
        std::unordered_set<bytes> deleted_keys;
    };
    
    // session implement the PIMPL idiom to allow for stack based semantics.
    std::shared_ptr<session_impl> m_impl;
};

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session() {
    using session_type = session<persistent_data_store, cache_data_store>;
    return session_type{};
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session(persistent_data_store store) {
    using session_type = session<persistent_data_store, cache_data_store>;
    return session_type{std::move(store)};
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session(persistent_data_store store, cache_data_store cache) {
    using session_type = session<persistent_data_store, cache_data_store>;
    return session_type{std::move(store), std::move(cache)};
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session(session<persistent_data_store, cache_data_store>& the_session) {
    using session_type = session<persistent_data_store, cache_data_store>;
    auto new_session = session_type{the_session, typename session_type::nested_session_t{}};
    new_session.m_impl->parent->child = new_session.m_impl->shared_from_this();
    return new_session;
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session_impl::session_impl()
: backing_data_store{std::make_shared<persistent_data_store>()} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session_impl::session_impl(session_impl& parent_)
: parent{parent_.shared_from_this()},
  backing_data_store{parent_.backing_data_store},
  cache{parent_.cache.memory_allocator()} {
    if (auto child_ptr = parent->child.lock()) {
        child_ptr->backing_data_store = nullptr;
        child_ptr->parent = nullptr;
    }
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session_impl::session_impl(persistent_data_store pds)
: backing_data_store{std::make_shared<persistent_data_store>(std::move(pds))},
  cache{pds.memory_allocator()} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session_impl::session_impl(persistent_data_store pds, cache_data_store cds)
: backing_data_store{std::make_shared<persistent_data_store>(std::move(pds))},
  cache{std::move(cds)} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session_impl::~session_impl() {
    // When this goes out of scope:
    // 1.  If there is no parent and no backing data store, then this session has been abandoned (undo)
    // 2.  If there is no parent, then this session is being committed to the data store. (commit)
    // 3.  If there is a parent, then this session is being merged into the parent. (squash)
    commit();
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::session_impl::undo() {
    if (parent) {
        parent->child = child;
    }
    
    if (auto child_ptr = child.lock()) {
        child_ptr->parent = parent;
    }
    
    parent = nullptr;
    child.reset();
    backing_data_store = nullptr;
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::session_impl::commit() {
    if (!parent && !backing_data_store) {
        // undo
        return;
    }
    
    auto write_through = [&](auto& ds) {
        ds.erase(deleted_keys);
        cache.write_to(ds, updated_keys);
        undo();
    };
    
    if (parent) {
        // squash
        auto parent_session = session{parent};
        write_through(parent_session);
        return;
    }
    
    // commit
    write_through(*backing_data_store);
}

template <typename persistent_data_store, typename cache_data_store>
const session<persistent_data_store, cache_data_store> session<persistent_data_store, cache_data_store>::invalid{};

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session()
: m_impl{std::make_shared<session_impl>()} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session(std::shared_ptr<session_impl> impl)
: m_impl{std::move(impl)} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session(persistent_data_store ds)
: m_impl{std::make_shared<session_impl>(std::move(ds))} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session(persistent_data_store ds, cache_data_store cache)
: m_impl{std::make_shared<session_impl>(std::move(ds), std::move(cache))} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session(session& session, nested_session_t)
: m_impl{std::make_shared<session_impl>(*session.m_impl)} {
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::~session() {
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::undo() {
    if (!m_impl) {
        return;
    }
    
    m_impl->undo();
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> session<persistent_data_store, cache_data_store>::attach(session child) {
    if (!m_impl) {
        return session<persistent_data_store, cache_data_store>::invalid;
    }

    auto current_child = detach();

    child.m_impl->parent = m_impl;
    child.m_impl->backing_data_store = m_impl->backing_data_store;
    m_impl->child = child.m_impl;

    return current_child;
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> session<persistent_data_store, cache_data_store>::detach() {
    if (!m_impl) {
        return session<persistent_data_store, cache_data_store>::invalid;
    }

    auto current_child = m_impl->child.lock();
    if (current_child) {
        current_child->parent = nullptr;
        current_child->backing_data_store = nullptr;
    }
    m_impl->child.reset();

    return {current_child};
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::commit() {
    if (!m_impl) {
        return;
    }
    
    m_impl->commit();
}

template <typename persistent_data_store, typename cache_data_store>
const key_value session<persistent_data_store, cache_data_store>::read(const bytes& key) const {
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
            m_impl->cache.write(kv);
            return kv;
        }
        
        parent = parent->parent.get();
    }
    
    // If we haven't found the key in the session yet, read it from the persistent data store.
    if (m_impl->backing_data_store) {
        auto kv = m_impl->backing_data_store->read(key);
        if (kv != key_value::invalid) {
            m_impl->cache.write(kv);
            return kv;
        }
    }
    
    return key_value::invalid;
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::write(key_value kv) {
    if (!m_impl) {
        return;
    }
    m_impl->updated_keys.emplace(kv.key());
    m_impl->deleted_keys.erase(kv.key());
    m_impl->cache.write(std::move(kv));
}

template <typename persistent_data_store, typename cache_data_store>
bool session<persistent_data_store, cache_data_store>::contains(const bytes& key) const {
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
            return true;
        }
        
        parent = parent->parent.get();
    }
    
    // If we get to this point, the key hasn't been read into memory.  Check to see if it exists in the persistent data store.
    return m_impl->backing_data_store->contains(key);
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::erase(const bytes& key) {
    if (!m_impl) {
        return;
    }
    m_impl->deleted_keys.emplace(key);
    m_impl->updated_keys.erase(key);
    m_impl->cache.erase(key);
}

// Reads a batch of keys from the session.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
// \returns An std::pair where the first item is list of the found key_values and the second item is a set of the keys not found.
template <typename persistent_data_store, typename cache_data_store>
template <typename iterable>
const std::pair<std::vector<key_value>, std::unordered_set<bytes>> session<persistent_data_store, cache_data_store>::read(const iterable& keys) const {
    if (!m_impl) {
        return {};
    }
    
    auto not_found = std::unordered_set<bytes>{};
    auto kvs = std::vector<key_value>{};
    
    for (const auto& key : keys) {
        auto found = false;
        auto deleted = false;
        auto* parent = m_impl.get();
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
                found = true;
                m_impl->cache.write(kv);
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
    
    return {std::move(kvs), std::move(not_found)};
}

// Writes a batch of key_values to the session.
//
// \tparam iterable Any type that can be used within a range based for loop and returns key_value instances in its iterator.
// \param key_values An iterable instance that returns key_value instances in its iterator.
template <typename persistent_data_store, typename cache_data_store>
template <typename iterable>
void session<persistent_data_store, cache_data_store>::write(const iterable& key_values) {
    if (!m_impl) {
        return;
    }
    
    // Currently the batch write will just iteratively call the non batch write
    for (const auto& kv : key_values) {
        write(kv);
    }
}

// Erases a batch of key_values from the session.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename persistent_data_store, typename cache_data_store>
template <typename iterable>
void session<persistent_data_store, cache_data_store>::erase(const iterable& keys) {
    if (!m_impl) {
        return;
    }
    
    // Currently the batch erase will just iteratively call the non batch erase
    for (const auto& key : keys) {
        erase(key);
    }
}

template <typename persistent_data_store, typename cache_data_store>
template <typename data_store, typename iterable>
void session<persistent_data_store, cache_data_store>::write_to(data_store& ds, const iterable& keys) const {
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
            
            results.emplace_back(make_kv(kv.key().data(), kv.key().length(), kv.value().data(), kv.value().length(), ds.memory_allocator()));
            break;
        }
    }
    ds.write(results);
}

template <typename persistent_data_store, typename cache_data_store>
template <typename data_store, typename iterable>
void session<persistent_data_store, cache_data_store>::read_from(const data_store& ds, const iterable& keys) {
    if (!m_impl) {
        return;
    }
    ds.write_to(*this, keys);
}

// Factor for creating the initial session iterator.
//
// \tparam predicate A functor used for getting the starting iterating in the cache list and the persistent data store.
// \tparam comparator A functor for determining if the "current" iterator points to the cache list or the persistent data store.
template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_type, typename predicate, typename comparator>
iterator_type session<persistent_data_store, cache_data_store>::make_iterator_(const predicate& p, const comparator& c) const {
    if (!m_impl) {
        // For some reason the pimpl is nullptr.  Return an "invalid" iterator.
        return iterator_type{};
    }
    
    auto new_iterator = iterator_type{};
    
    // Create the cache iterator states for levels from here up to the parent.
    // We want the child at the front of the list and the parent at the end.
    // TODO:  Should this walk over the entire session list or just from this session up to the parent.
    auto* current = m_impl.get();
    while (current) {
        auto state = typename iterator_type::cache_iterator_state {
          .begin = std::begin(current->cache),
          .end = std::end(current->cache),
          .current = p(current->cache),
          .deleted_keys = &current->deleted_keys,
          .updated_keys = &current->updated_keys
        };
        new_iterator.m_cache_iterator_states.emplace_back(std::move(state));
        current = current->parent.get();
    }
    
    new_iterator.m_database_iterator_state.begin = std::begin(*m_impl->backing_data_store);
    new_iterator.m_database_iterator_state.end = std::end(*m_impl->backing_data_store);
    new_iterator.m_database_iterator_state.current = p(*m_impl->backing_data_store);

    // Find the current value of the iterator
    auto is_deleted = [&](auto& key, auto index) {
        for (size_t i = 0; i <= index; ++i) {
            auto& current_state = new_iterator.m_cache_iterator_states[i];

            if (current_state.updated_keys->find(key) != std::end(*current_state.updated_keys)) {
                return false;
            }

            if (current_state.deleted_keys->find(key) != std::end(*current_state.deleted_keys)) {
                return true;
            }
        }
        return false;
    };

    new_iterator.m_current_value = key_value::invalid;

    // Get the first value not at the end.
    auto index = size_t{0};
    for (; index < new_iterator.m_cache_iterator_states.size(); ++index) {
        auto& current_state = new_iterator.m_cache_iterator_states[index];
        if (current_state.current == current_state.end) {
            continue;
        }
        auto pending = *current_state.current;
        if (is_deleted(pending.key(), index)) {
            continue;
        }
        new_iterator.m_current_value = pending;
        new_iterator.m_iterator_index = index;
        break;
    }

    // Determine what the initial key_value should be.
    for (size_t i = index + 1; i < new_iterator.m_cache_iterator_states.size(); ++i) {
        auto& current_state = new_iterator.m_cache_iterator_states[i];
        if (current_state.current == current_state.end) {
            continue;
        }
        
        auto pending = *current_state.current;
        auto& pending_key = pending.key();
        auto& current_key = new_iterator.m_current_value.key();

        if (is_deleted(pending_key, i)) {
            continue;
        }

        if (!c(pending_key, current_key)) {
            continue;
        }

        if (pending_key == current_key) {
            // we want the first one found
            continue;
        }

        new_iterator.m_current_value = std::move(pending);
        new_iterator.m_iterator_index = i;
    }

    if (new_iterator.m_database_iterator_state.current != new_iterator.m_database_iterator_state.end) {
        auto pending = *new_iterator.m_database_iterator_state.current;
        auto& pending_key = pending.key();
        auto& current_key = new_iterator.m_current_value.key();

        if (!is_deleted(pending_key, new_iterator.m_cache_iterator_states.size() - 1)
            && pending_key != current_key
            && c(pending_key, current_key)) {

            new_iterator.m_current_value = pending;
            new_iterator.m_iterator_index = new_iterator.m_cache_iterator_states.size();
        }
    }
    
    return new_iterator;
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::iterator session<persistent_data_store, cache_data_store>::find(const bytes& key) {
    // This comparator just chooses the one that isn't invalid.
    static auto comparator = [](auto& left, auto& right) {
        if (left == bytes::invalid && right == bytes::invalid) {
            return true;
        }
        
        if (left == bytes::invalid) {
            return false;
        }
        
        return true;
    };
    
    return make_iterator_<iterator>([&](auto& ds) { return ds.find(key); }, comparator);
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::const_iterator session<persistent_data_store, cache_data_store>::find(const bytes& key) const {
    // This comparator just chooses the one that isn't invalid.
    static auto comparator = [](auto& left, auto& right) {
        if (left == bytes::invalid && right == bytes::invalid) {
            return true;
        }
        
        if (left == bytes::invalid) {
            return false;
        }
        
        return true;
    };
    
    return make_iterator_<const_iterator>([&](auto& ds) { return ds.find(key); }, comparator);
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::iterator session<persistent_data_store, cache_data_store>::begin() {
    return make_iterator_<iterator>([](auto& ds) { return std::begin(ds); }, std::less<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::const_iterator session<persistent_data_store, cache_data_store>::begin() const {
    return make_iterator_<const_iterator>([](auto& ds) { return std::begin(ds); }, std::less<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::iterator session<persistent_data_store, cache_data_store>::end() {
    return make_iterator_<iterator>([](auto& ds) { return std::end(ds); }, std::greater<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::const_iterator session<persistent_data_store, cache_data_store>::end() const {
    return make_iterator_<const_iterator>([](auto& ds) { return std::end(ds); }, std::greater<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::iterator session<persistent_data_store, cache_data_store>::lower_bound(const bytes& key) {
    return make_iterator_<iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::greater<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::const_iterator session<persistent_data_store, cache_data_store>::lower_bound(const bytes& key) const {
    return make_iterator_<const_iterator>([&](auto& ds) { return ds.lower_bound(key); }, std::greater<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::iterator session<persistent_data_store, cache_data_store>::upper_bound(const bytes& key) {
    return make_iterator_<iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::const_iterator session<persistent_data_store, cache_data_store>::upper_bound(const bytes& key) const {
    return make_iterator_<const_iterator>([&](auto& ds) { return ds.upper_bound(key); }, std::less<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
const std::shared_ptr<typename persistent_data_store::allocator_type>& session<persistent_data_store, cache_data_store>::memory_allocator() {
    if (!m_impl) {
        static auto invalid = std::shared_ptr<typename persistent_data_store::allocator_type>{};
        return invalid;
    }
    
    if (m_impl->backing_data_store) {
      return m_impl->backing_data_store->memory_allocator();
    } 
    return m_impl->cache.memory_allocator();
}

template <typename persistent_data_store, typename cache_data_store>
const std::shared_ptr<const typename persistent_data_store::allocator_type> session<persistent_data_store, cache_data_store>::memory_allocator() const {
    if (!m_impl) {
        static auto invalid = std::shared_ptr<typename persistent_data_store::allocator_type>{};
        return invalid;
    }
    
    if (m_impl->backing_data_store) {
      return m_impl->backing_data_store->memory_allocator();
    } 
    return m_impl->cache.memory_allocator();
}

template <typename persistent_data_store, typename cache_data_store>
const std::shared_ptr<persistent_data_store>& session<persistent_data_store, cache_data_store>::backing_data_store() {
    if (!m_impl) {
        static auto invalid = std::shared_ptr<persistent_data_store>{};
        return invalid;
    }
    
    return m_impl->backing_data_store;
}

template <typename persistent_data_store, typename cache_data_store>
const std::shared_ptr<const persistent_data_store>& session<persistent_data_store, cache_data_store>::backing_data_store() const {
    if (!m_impl) {
        static auto invalid = std::shared_ptr<persistent_data_store>{};
        return invalid;
    }
    
    return m_impl->backing_data_store;
}

template <typename persistent_data_store, typename cache_data_store>
const std::shared_ptr<cache_data_store>& session<persistent_data_store, cache_data_store>::cache() {
    if (!m_impl) {
        static auto invalid = std::shared_ptr<cache_data_store>{};
        return invalid;
    }
    
    return m_impl->cache;
}

template <typename persistent_data_store, typename cache_data_store>
const std::shared_ptr<const cache_data_store>& session<persistent_data_store, cache_data_store>::cache() const {
    if (!m_impl) {
        static auto invalid = std::shared_ptr<cache_data_store>{};
        return invalid;
    }
    
    return m_impl->cache;
}

// Moves the current iterator.
//
// \tparam predicate A functor that indicates if we are incrementing or decrementing the current iterator.
// \tparam comparator A functor used to determine what the current iterator is.
template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
template <typename move_predicate, typename rollover_predicate, typename comparator>
void session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::move_(const move_predicate& move, const rollover_predicate& rollover, const comparator& key_comparator) {
    // Checks if the key has been deleted in this session.
    auto is_deleted = [&](auto& key, auto index) {
        for (size_t i = 0; i <= index; ++i) {
            // Remember that the list of cache iterator state has the child at the front
            // and the parent at the back.  So this is going to check from the child to
            // the current parent to see if this key has been deleted in any of those sessions.
            auto& current_state = m_cache_iterator_states[i];
            
            if (current_state.updated_keys->find(key) != std::end(*current_state.updated_keys)) {
                return false;
            }
            
            if (current_state.deleted_keys->find(key) != std::end(*current_state.deleted_keys)) {
                return true;
            }
        }
        return false;
    };

    // Finds the next key in order that is not marked for deletion within
    // a session.
    auto find_next = [&](auto& state, auto index, auto& previous_key) {
        if (state.current == state.end) {
            return key_value::invalid;
        }

        auto pending = *state.current;
        while (true) {
            auto deleted = is_deleted(pending.key(), index);
            if (pending.key() != previous_key && !deleted) {
                // The pending key cannot be the last current key
                // and it cannot be deleted in a child session.
                return pending;
            }

            move(state);
            
            if (state.current == state.end) {
                // We hit the end.  Nothing to do until the iterator is rolled over.
                return key_value::invalid;
            }

            pending = *state.current;
        }
    };

    // Compares the pneding key/value against the current key/value
    // and possibly sets the pending key/value if it compares next in
    // order against the current key/value.
    auto test_set = [&](auto pending, auto index) {
        if (pending == key_value::invalid) {
            return;
        }

        if (m_current_value != key_value::invalid) {
            auto& pending_key = pending.key();
            auto& current_key = m_current_value.key();
            if (pending_key == current_key || !key_comparator(pending_key, current_key)) {
                // Tie breaks will always prefer what is set in the current value.
                // pending key has to be less than the current key.
                return;
            }
        }
        
        m_current_value = std::move(pending);
        m_iterator_index = index;
    };

    // Move the current iterator.
    if (m_iterator_index < m_cache_iterator_states.size()) {
        move(m_cache_iterator_states[m_iterator_index]);
    } else if (m_iterator_index == m_cache_iterator_states.size()) {
        move(m_database_iterator_state);
    }

    auto previous_key = m_current_value.key();
    m_current_value = key_value::invalid;    
    m_iterator_index = static_cast<size_t>(-1);

    auto move_iterator = [&](auto need_rollover) { 
        auto end_count = size_t{0};

        // For each session in the list, find the key in that session
        // that depending on the comparator function is either the smallest or largest.
        for (size_t i = 0; i < m_cache_iterator_states.size(); ++i) {
            auto& state = m_cache_iterator_states[i];
            
            if (need_rollover) {
                rollover(state);
            }

            auto next = find_next(state, i, previous_key);
            if (next == key_value::invalid) {
                ++end_count;
                continue;
            }
            test_set(next, i);
        }

        if (need_rollover) {
            rollover(m_database_iterator_state);
        }

        // Then get the next key from the database and see if that is larger or smaller,
        // depending on the comparator, than the currently set value.
        auto next = find_next(m_database_iterator_state, m_cache_iterator_states.size() - 1, previous_key);

        if (next != key_value::invalid) {
            test_set(next, m_cache_iterator_states.size());
        } else {
            ++end_count;
        }

        return end_count;
    };

    if (move_iterator(false) == m_cache_iterator_states.size() + 1) {
        // A rollover needs to occur and then we need to determine the next key in order
        // based on the comparator function.
        move_iterator(true);
    }
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
void session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::move_next_() {
    // increment the current iterator and set the next current.
    auto move = [](auto& it) { 
        if (it.begin == it.end) {
            // There is nothing to iterate over.
            return;
        }

        if (it.current == it.end) {
            // This lambda doesn't perform the roll over.
            return;
        }

        ++it.current;
    };

    auto rollover = [](auto& it) {
      if (it.begin == it.end) {
          // There is nothing to iterate over.
          return;
      }
      it.current = it.begin;
    };

    move_(move, rollover, std::less<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
void session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::move_previous_() {
    // decrement the current iterator and set the next current.
    auto move = [](auto& it) { 
        if (it.begin == it.end) {
            // There is nothing to iterate over.
            return;
        }

        if (it.current == it.begin) {
            // set to the end.
            it.current = it.end;
            return;
        }

        --it.current;
    };

    auto rollover = [](auto& it) {
      if (it.begin == it.end) {
          // There is nothing to iterate over.
          return;
      }
      it.current = it.end;
      --it.current;
    };

    move_(move, rollover, std::greater<bytes>{});
}

template <typename persistent_data_store, typename cache_data_store, typename iterator_traits>
using session_iterator_alias = typename session<persistent_data_store, cache_data_store>::template session_iterator<iterator_traits>;

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
session_iterator_alias<persistent_data_store, cache_data_store, iterator_traits>& session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator++() {
    move_next_();
    return *this;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
session_iterator_alias<persistent_data_store, cache_data_store, iterator_traits> session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator++(int) {
    auto new_iterator = *this;
    move_next_();
    return new_iterator;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
session_iterator_alias<persistent_data_store, cache_data_store, iterator_traits>& session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator--() {
    move_previous_();
    return *this;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
session_iterator_alias<persistent_data_store, cache_data_store, iterator_traits> session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator--(int) {
    auto new_iterator = *this;
    move_previous_();
    return new_iterator;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
typename session_iterator_alias<persistent_data_store, cache_data_store, iterator_traits>::value_type session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator*() const {
    return m_current_value;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
typename session_iterator_alias<persistent_data_store, cache_data_store, iterator_traits>::value_type session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator->() const {
    return m_current_value;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
bool session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator==(const session_iterator& other) const {
    return m_current_value == other.m_current_value;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
bool session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator!=(const session_iterator& other) const {
    return !(*this == other);
}

}
