#pragma once

#include <queue>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <b1/session/bytes.hpp>
#include <b1/session/key_value.hpp>
#include <b1/session/session_fwd_decl.hpp>

namespace eosio::session {

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
    struct iterator_state {
        bool next_in_cache{false};
        bool previous_in_cache{false};
        bool deleted{false};
    };

    using persistent_data_store_type = persistent_data_store;
    using cache_data_store_type = cache_data_store;
    using iterator_cache_type = std::map<bytes, iterator_state>;
    
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
        template <typename test_predicate, typename db_test_predicate, typename move_predicate>
        void move_(const test_predicate& test, const db_test_predicate& db_test, const move_predicate& move);
        void move_next_();
        void move_previous_();
        
    private:
        typename iterator_cache_type::iterator m_active_iterator;
        session* m_active_session{nullptr};
    };

    struct iterator_traits {
        using difference_type = long;
        using value_type = key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
        using cache_iterator = typename cache_data_store::iterator;
        using data_store_iterator = typename persistent_data_store::iterator;
    };
    using iterator = session_iterator<iterator_traits>;

    struct const_iterator_traits {
        using difference_type = long;
        using value_type = const key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
        using cache_iterator = typename cache_data_store::iterator;
        using data_store_iterator = typename persistent_data_store::iterator;
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
    void clear();
    
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

    std::pair<bytes, bytes> bounds_(const bytes& key) const;
    void update_iterator_cache_(const bytes& key, bool overwrite = true, bool erase = false) const;
    bool is_deleted_(const bytes& key) const;

    struct session_impl final : public std::enable_shared_from_this<session_impl> {
        session_impl();
        session_impl(session_impl& parent_);
        session_impl(persistent_data_store pds);
        session_impl(persistent_data_store pds, cache_data_store cds);
        ~session_impl();
        
        void undo();
        void commit();
        void clear();
 
        std::shared_ptr<session_impl> parent;
        std::weak_ptr<session_impl> child;
        
        // The persistent data store.  This is shared across all levels of the session.
        std::shared_ptr<persistent_data_store> backing_data_store;
        
        // The cache used by this session instance.  This will include all new/update key_values
        // and may include values read from the persistent data store.
        cache_data_store cache;

        // Indicates if the next/previous key in lexicographical order for a given key exists in the cache.
        // The first value in the pair indicates if the previous key in lexicographical order exists in the cache.
        // The second value in the pair indicates if the next key lexicographical order exists in the cache.
        iterator_cache_type iterator_cache;

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

template <typename cache_data_store, typename persistent_data_store>
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

    // Prime the iterator cache with the begin and end values from the parent.
    auto begin = std::begin(parent_.iterator_cache);
    auto end = std::end(parent_.iterator_cache);
    if (begin != end) {
        iterator_cache.emplace(begin->first, iterator_state{});
        iterator_cache.emplace((--end)->first, iterator_state{});
    }
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session_impl::session_impl(persistent_data_store pds)
: backing_data_store{std::make_shared<persistent_data_store>(std::move(pds))},
  cache{pds.memory_allocator()} {
    // Prime the iterator cache with the begin and end values from the database.
    auto begin = std::begin(*backing_data_store);
    auto end = std::end(*backing_data_store);
    if (begin != end) {
        iterator_cache.emplace(begin.first, iterator_state{});
        iterator_cache.emplace((--end).first, iterator_state{});
    }
}

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store>::session_impl::session_impl(persistent_data_store pds, cache_data_store cds)
: backing_data_store{std::make_shared<persistent_data_store>(std::move(pds))},
  cache{std::move(cds)} {
    auto emplace_bounds = [&](const auto& container) {
        auto begin = std::begin(container);
        auto end = std::end(container);
        if (begin != end) {
            iterator_cache.emplace((*begin).key(), iterator_state{});
            iterator_cache.emplace((*--end).key(), iterator_state{});
        }
    };
    // Prime the iterator cache with the begin and end values from cache and the database.
    emplace_bounds(*backing_data_store);
    emplace_bounds(cache);
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
    clear();
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
        clear();
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
void session<persistent_data_store, cache_data_store>::session_impl::clear() {
    deleted_keys.clear();
    updated_keys.clear();
    cache.clear();
    iterator_cache.clear();
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
std::pair<bytes, bytes> session<persistent_data_store, cache_data_store>::bounds_(const bytes& key) const {
    auto lower_bound_key = bytes::invalid;
    auto upper_bound_key = bytes::invalid;

    auto lower_bound = [&, this](const auto& container, const auto& key) {
        auto it = container.lower_bound(key);
        if (it != std::end(container) && it != std::begin(container)) {
            --it;
            while (is_deleted_((*it).key())) {
                if (it == std::begin(container)) {
                    return bytes::invalid;
                }
                --it;
            }
            return (*it).key();
        }
        return bytes::invalid;
    };

    auto upper_bound = [&, this](const auto& container, const auto& key) {
        auto it = container.upper_bound(key);
        if (it == std::end(container)) {
            return bytes::invalid;
        }

        while (is_deleted_((*it).key())) {
            ++it;
            if (it == std::end(container)) {
                return bytes::invalid;
            }
        }

        return (*it).key();
    };

    auto test_set = [&](auto& current_key, const auto& pending_key, const auto& comparator) {
        if (current_key == bytes::invalid && pending_key == bytes::invalid) {
            return;
        }

        if (current_key == bytes::invalid) {
            current_key = pending_key;
            return;
        }

        if (comparator(pending_key, current_key)) {
            current_key = pending_key;
            return;
        }
    };

    auto current = m_impl;
    while (current) {
        if (auto lower_bound_key_ = lower_bound(current->cache, key); lower_bound_key_ != bytes::invalid) {
            test_set(lower_bound_key, lower_bound_key_, std::greater<bytes>{});
        }
        if (auto upper_bound_key_ = upper_bound(current->cache, key); upper_bound_key_ != bytes::invalid) {
            test_set(upper_bound_key, upper_bound_key_, std::less<bytes>{});
        }

        current = current->parent;
    }

    if (auto lower_bound_key_ = lower_bound(*m_impl->backing_data_store, key); lower_bound_key_ != bytes::invalid) {
        test_set(lower_bound_key, lower_bound_key_, std::greater<bytes>{});
    }

    if (auto upper_bound_key_ = upper_bound(*m_impl->backing_data_store, key); upper_bound_key_ != bytes::invalid) {
        test_set(upper_bound_key, upper_bound_key_, std::less<bytes>{});
    }

    return std::pair{lower_bound_key, upper_bound_key};
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::update_iterator_cache_(const bytes& key, bool overwrite, bool erase) const {
    auto it = m_impl->iterator_cache.emplace(key, iterator_state{}).first;

    if (overwrite) {
        it->second.deleted = erase;
    }

    if (it->second.next_in_cache && it->second.previous_in_cache) {
        return;
    }

    auto [lower_bound, upper_bound] = bounds_(key);

    if (lower_bound != bytes::invalid) {
        auto lower_it = m_impl->iterator_cache.find(lower_bound);
        if (lower_it == std::end(m_impl->iterator_cache)) {
            lower_it = m_impl->iterator_cache.emplace(lower_bound, iterator_state{}).first;
        }
        lower_it->second.next_in_cache = true;
        it->second.previous_in_cache = true;
    }

    if (upper_bound != bytes::invalid) {
        auto upper_it = m_impl->iterator_cache.find(upper_bound);
        if (upper_it == std::end(m_impl->iterator_cache)) {
            upper_it = m_impl->iterator_cache.emplace(upper_bound, iterator_state{}).first;
        }
        upper_it->second.previous_in_cache = true;
        it->second.next_in_cache = true;
    }
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

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::write(key_value kv) {
    if (!m_impl) {
        return;
    }
    auto key = kv.key();
    m_impl->updated_keys.emplace(key);
    m_impl->deleted_keys.erase(key);
    m_impl->cache.write(std::move(kv));
    update_iterator_cache_(key);
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
            update_iterator_cache_(key, false);
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
    update_iterator_cache_(key, true, true);
}

template <typename persistent_data_store, typename cache_data_store>
void session<persistent_data_store, cache_data_store>::clear() {
    if (!m_impl) {
        return;
    }
    m_impl->clear();
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

template <typename persistent_data_store, typename cache_data_store>
bool session<persistent_data_store, cache_data_store>::is_deleted_(const bytes& key) const {
    auto current = m_impl;
    while (current) {
      if (current->updated_keys.find(key) != std::end(current->updated_keys)) {
          return false;
      }

      if (current->deleted_keys.find(key) != std::end(current->deleted_keys)) {
          return true;
      }

      current = current->parent;
    }
    return false;
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
    new_iterator.m_active_session = const_cast<session*>(this);
    new_iterator.m_active_iterator = std::end(m_impl->iterator_cache);
  
    // Move to the head.  We start at the head
    // because we want the tail to win any tie breaks.
    auto parent = m_impl;
    while(parent->parent) {
        parent = parent->parent;
    }

    // Assume that backing data store wins.
    auto it = p(*m_impl->backing_data_store);
    auto current_key = it != std::end(*m_impl->backing_data_store) ? (*it).key() : bytes::invalid;

    // Check the other levels to see which key we should start at.
    auto current = parent;
    while (current) {
        auto pending = p(current->cache);

        if (current_key != bytes::invalid 
            && current->deleted_keys.find(current_key) != std::end(current->deleted_keys))
        {
            // Key is deleted at this level
            current_key = bytes::invalid;
        }

        if (pending == std::end(current->cache)) {
            current = current->child.lock();
            continue;
        }

        auto pending_key = (*pending).key();
        auto it = current->iterator_cache.find(pending_key);

        if (it->second.deleted) {
            current = current->child.lock();
            continue;
        }

        if (current_key == bytes::invalid || c(pending_key, current_key)) {
            current_key = pending_key;
        }

        current = current->child.lock();
    }

    // Update the iterator cache.
    if (current_key != bytes::invalid) {
        new_iterator.m_active_iterator = m_impl->iterator_cache.find(current_key);
        if (new_iterator.m_active_iterator->second.deleted) {
            new_iterator.m_active_iterator = std::end(m_impl->iterator_cache);
        } else {
            session{m_impl}.update_iterator_cache_(current_key, false);
        }
    }

    return new_iterator;
}

template <typename persistent_data_store, typename cache_data_store>
typename session<persistent_data_store, cache_data_store>::iterator session<persistent_data_store, cache_data_store>::find(const bytes& key) {
    // This comparator just chooses the one that isn't invalid.
    static auto comparator = [](const auto& left, const auto& right) {
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
    static auto comparator = [](const auto& left, const auto& right) {
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
template <typename test_predicate, typename db_test_predicate, typename move_predicate>
void session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::move_(const test_predicate& test, const db_test_predicate& db_test, const move_predicate& move) {
    auto set = false;

    if (test(m_active_iterator)) {
        move(m_active_iterator);
        while (m_active_iterator->second.deleted) {
            move(m_active_iterator);
        }
        set = true;
    } else {
        auto current = m_active_session->m_impl->parent;
        while (current) {
            auto it = current->iterator_cache.find(m_active_iterator->first);
            if (test(it)) {
                move(it);

                while (it->second.deleted) {
                    move(it);
                }

                m_active_session->update_iterator_cache_(it->first, false);
                m_active_iterator = m_active_session->m_impl->iterator_cache.find(it->first);
                set = true;
                break;
            }
            current = current->parent;
        }

        if (!current) {
            // We didn't find the key in the cache
            auto it = m_active_session->m_impl->backing_data_store->find(m_active_iterator->first);
            if (db_test(it)) {
                move(it);
                if (it != std::end(*m_active_session->m_impl->backing_data_store)) {
                    auto key = (*it).key();
                    m_active_session->update_iterator_cache_(key, false);
                    m_active_iterator = m_active_session->m_impl->iterator_cache.find(key);
                    set = true;
                } 
            }
        }
    }

    if (!set) {
        // We didn't find a value. Or we actually moved to the end.
        m_active_iterator = std::end(m_active_session->m_impl->iterator_cache);
    }
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
void session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::move_next_() {
    auto move = [](auto& it) {
        ++it;
    };
    auto test = [](auto& it) {
        return it->second.next_in_cache;
    };
    auto db_test = [&](auto& it) {
        return it != std::end(*m_active_session->m_impl->backing_data_store);
    };
    auto rollover = [&]() {
        if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
            // Rollover...
            m_active_iterator = std::begin(m_active_session->m_impl->iterator_cache);
        }
    };

    move_(test, db_test, move);
    rollover();
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
void session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::move_previous_() {
   auto move = [](auto& it) {
        --it;
    };
    auto test = [&](auto& it) {
        if (it != std::end(m_active_session->m_impl->iterator_cache)) {
            return it->second.previous_in_cache;
        }
        return true;
    };
    auto db_test = [&](auto& it) {
        return it != std::begin(*m_active_session->m_impl->backing_data_store);
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
    move_(test, db_test, move);
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
    if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
        return key_value::invalid;
    }
    return m_active_session->read(m_active_iterator->first);
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
typename session_iterator_alias<persistent_data_store, cache_data_store, iterator_traits>::value_type session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator->() const {
    if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
        return key_value::invalid;
    }
    return m_active_session->read(m_active_iterator->first);
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
bool session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator==(const session_iterator& other) const {
    if (m_active_iterator == std::end(m_active_session->m_impl->iterator_cache) 
        && m_active_iterator == other.m_active_iterator) {
        return true;
    }
    if (other.m_active_iterator == std::end(m_active_session->m_impl->iterator_cache)) {
        return false;
    }
    return this->m_active_iterator->first == other.m_active_iterator->first;
}

template <typename persistent_data_store, typename cache_data_store>
template <typename iterator_traits>
bool session<persistent_data_store, cache_data_store>::session_iterator<iterator_traits>::operator!=(const session_iterator& other) const {
    return !(*this == other);
}

}

