#pragma once

#include <algorithm>
#include <string_view>

#include <boost/pool/pool.hpp>

#include <b1/session/shared_bytes_fwd_decl.hpp>
#include <b1/session/key_value_fwd_decl.hpp>

namespace eosio::session {

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes final {
public:
    friend class key_value;
    
    template <typename T, typename Allocator>
    friend shared_bytes make_shared_bytes(const T* data, size_t length, Allocator& a);

    template <typename Allocator>
    friend shared_bytes make_shared_bytes(const void* data, size_t length, Allocator& a);
    
    template <typename T>
    friend shared_bytes make_shared_bytes(const T* data, size_t length);
    
    friend key_value make_kv(shared_bytes key, shared_bytes value);

    template <typename Key, typename Value, typename Allocator>
    friend key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length, Allocator& a);
    
    template <typename Key, typename Value>
    friend key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

    template <typename Allocator>
    friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, Allocator& a);
    
    friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length);

public:
    shared_bytes(const shared_bytes& b);
    shared_bytes(shared_bytes&& b);
    ~shared_bytes();
    
    shared_bytes& operator=(const shared_bytes& b);
    shared_bytes& operator=(shared_bytes&& b);
    
    const void * const data() const;
    size_t length() const;
    
    bool operator==(const shared_bytes& other) const;
    bool operator!=(const shared_bytes& other) const;
    
    static const shared_bytes invalid;
    
private:
    shared_bytes() = default;
    
private:
    // To allow for sharing a chunk of memory across multiple shared_byte instances, we need to keep track
    // of the chunk start in each instance to pass back to the allocator.
    uint64_t* m_chunk_start{nullptr};
    // The encoded address of the memory allocator that instantiated the memory this instance points to.
    // \warning The memory allocator MUST stay in scope during the lifetime of this instance.
    uint64_t* m_memory_allocator_address{nullptr};
    // \brief A pointer to a counter to keep track of the number of shared_bytes instances that point to the memory address.
    // \remark When the counter reaches 0, the last instance will free the memory by invoking the free method on the memory allocator.
    // \warning Assuming that this type will be accessed synchronously.
    size_t* m_use_count_address{nullptr};
    // The size of the memory pointed to by m_data, in bytes.
    size_t* m_length{nullptr};
    // A pointer to a chunk of memory.
    void* m_data{nullptr};
};

// \brief Creates a new shared_bytes instance with the given pointer and length.
//
// \tparam T The type stored in the array.
// \tparam allocator The memory allocator type used to manage the shared_bytes memory. This type mush implement the "memory allocator" concept.
// \param data A pointer to an array of items to store in a shared_bytes instance.
// \param length The number of items in the array.
// \param a The memory allocator used to instantiate the memory used by the shared_bytes instance.
template <typename T, typename Allocator>
shared_bytes make_shared_bytes(const T* data, size_t length, Allocator& a) {
    return make_shared_bytes(reinterpret_cast<const void*>(data), length * sizeof(T), a);
}

template <typename Allocator>
shared_bytes make_shared_bytes(const void* data, size_t length, Allocator& a) {
    auto result = shared_bytes{};
    
    if (!data || length == 0) {
      return result;
    }

    auto chunk_length = 2 * sizeof(uint64_t) + 2 * sizeof(size_t) + length;
    auto* chunk = reinterpret_cast<char*>(a->allocate(chunk_length));
    
    result.m_chunk_start = reinterpret_cast<uint64_t*>(chunk);
    result.m_memory_allocator_address = reinterpret_cast<uint64_t*>(chunk + sizeof(uint64_t));
    result.m_use_count_address = reinterpret_cast<size_t*>(chunk + 2 *sizeof(uint64_t));
    result.m_length = reinterpret_cast<size_t*>(chunk + 2 * sizeof(uint64_t) + sizeof(size_t));
    result.m_data = reinterpret_cast<size_t*>(chunk + 2 * sizeof(uint64_t) + 2 * sizeof(size_t));

    *(result.m_chunk_start) = reinterpret_cast<uint64_t>(chunk);
    *(result.m_memory_allocator_address) = reinterpret_cast<uint64_t>(&a->free_function());
    *(result.m_use_count_address) = 1;
    *(result.m_length) = length;
    memcpy(result.m_data, data, length);

    return result;
}

template <>
inline shared_bytes make_shared_bytes(const void* data, size_t length) {
    auto result = shared_bytes{};
    
    // encode the length as the pointer address of this data member.
    result.m_length = reinterpret_cast<size_t*>(length);
    result.m_data = const_cast<void*>(data);
    
    return result;
}

// \brief Creates a new shared_bytes instance with the given pointer and length.
//
// This returns a shared_bytes "view" instance, in that the resulting shared_bytes instance does not manage
// the given memory.
//
// \tparam T The type stored in the array.
// \param data A pointer to an array of items to store in a shared_bytes instance.
// \param length The number of items in the array.
// \warning The resulting shared_bytes instance does NOT take ownership of the given data pointer.
// \warning The data pointer must remain in scope for the lifetime of the given shared_bytes instance.
template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length) {
    return make_shared_bytes(reinterpret_cast<const void*>(data), length * sizeof(T));
}

inline const shared_bytes shared_bytes::invalid{};

inline shared_bytes::shared_bytes(const shared_bytes& b)
: m_chunk_start{b.m_chunk_start},
  m_memory_allocator_address{b.m_memory_allocator_address},
  m_use_count_address{b.m_use_count_address},
  m_length{b.m_length},
  m_data{b.m_data} {

    if (m_use_count_address) {
        ++(*m_use_count_address);
    }
}

inline shared_bytes::shared_bytes(shared_bytes&& b)
: m_chunk_start{b.m_chunk_start},
  m_memory_allocator_address{std::move(b.m_memory_allocator_address)},
  m_use_count_address{std::move(b.m_use_count_address)},
  m_length{std::move(b.m_length)},
  m_data{std::move(b.m_data)} {
    b.m_chunk_start = nullptr;
    b.m_memory_allocator_address = nullptr;
    b.m_use_count_address = nullptr;
    b.m_length = nullptr;
    b.m_data = nullptr;
}

inline shared_bytes::~shared_bytes() {
    if (!m_chunk_start || !m_data || !m_length || !m_use_count_address) {
        return;
    }
    
    if (--(*m_use_count_address) != 0) {
        return;
    }
    
    // The memory pool must remain in scope for the lifetime of this instance.
    auto* free_function = reinterpret_cast<free_function_type*>(*m_memory_allocator_address);
    (*free_function)(reinterpret_cast<void*>(*m_chunk_start), sizeof(free_function_type) + 3 * sizeof(size_t) + *m_length);
}

inline shared_bytes& shared_bytes::operator=(const shared_bytes& b) {
    if (this == &b) {
        return *this;
    }
    
    m_chunk_start = b.m_chunk_start;
    m_memory_allocator_address = b.m_memory_allocator_address;
    m_use_count_address = b.m_use_count_address;
    m_length = b.m_length;
    m_data = b.m_data;
    
    if (m_use_count_address) {
        ++(*m_use_count_address);
    }
    
    return *this;
}

inline shared_bytes& shared_bytes::operator=(shared_bytes&& b) {
    if (this == &b) {
        return *this;
    }
    
    m_chunk_start = b.m_chunk_start;
    m_memory_allocator_address = b.m_memory_allocator_address;
    m_use_count_address = b.m_use_count_address;
    m_length = b.m_length;
    m_data = b.m_data;
    
    b.m_chunk_start = nullptr;
    b.m_memory_allocator_address = nullptr;
    b.m_use_count_address = nullptr;
    b.m_length = nullptr;
    b.m_data = nullptr;
    
    return *this;
}

inline const void * const shared_bytes::data() const {
    return m_data;
}

inline size_t shared_bytes::length() const {
    if (!m_memory_allocator_address) {
        // The size was encoded as the address of this data member.
        return reinterpret_cast<size_t>(m_length);
    }

    if (!m_length) {
        return 0;
    }
    
    return *m_length;
}

inline bool shared_bytes::operator==(const shared_bytes& other) const {
    if (m_length && other.m_length && m_data && other.m_data) {
        if (length() != other.length()) {
            return false;
        }
        
        return memcmp(m_data, other.m_data, length()) == 0 ? true : false;
    }
    
    // This is just checking if all the pointers are null or not.
    return m_length == other.m_length && m_data == other.m_data;
}

inline bool shared_bytes::operator!=(const shared_bytes& other) const {
    return !(*this == other);
}

}

namespace std {

template <>
struct less<eosio::session::shared_bytes> final {
    bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
        if (lhs != eosio::session::shared_bytes::invalid && rhs != eosio::session::shared_bytes::invalid) {
            return std::string_view{reinterpret_cast<const char*>(lhs.data()), lhs.length()} < std::string_view{reinterpret_cast<const char*>(rhs.data()), rhs.length()};
        }
        return lhs == eosio::session::shared_bytes::invalid;
    };
};

template <>
struct greater<eosio::session::shared_bytes> final {
    bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
        if (lhs != eosio::session::shared_bytes::invalid && rhs != eosio::session::shared_bytes::invalid) {
            return std::string_view{reinterpret_cast<const char*>(lhs.data()), lhs.length()} > std::string_view{reinterpret_cast<const char*>(rhs.data()), rhs.length()};
        }
        return lhs == eosio::session::shared_bytes::invalid;
    };
};

template <>
struct hash<eosio::session::shared_bytes> final {
  size_t operator()(const eosio::session::shared_bytes& b) const {
    return std::hash<std::string_view>{}({reinterpret_cast<const char*>(b.data()), b.length()});
  }
};

template <>
struct equal_to<eosio::session::shared_bytes> final {
    bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
        return std::string_view{reinterpret_cast<const char*>(lhs.data()), lhs.length()} == std::string_view{reinterpret_cast<const char*>(rhs.data()), rhs.length()};
    }
};

}
