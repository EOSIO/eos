#pragma once

#include <algorithm>
#include <string_view>

#include <boost/pool/pool.hpp>

#include <b1/session/bytes_fwd_decl.hpp>
#include <b1/session/key_value_fwd_decl.hpp>

namespace eosio::session {

// \brief An immutable type to represent a pointer and a length.
//
class bytes final {
public:
    friend class key_value;
    
    template <typename T, typename allocator>
    friend bytes make_bytes(const T* data, size_t length, allocator& a);

    template <typename allocator>
    friend bytes make_bytes(const void* data, size_t length, allocator& a);
    
    template <typename T>
    friend bytes make_bytes(const T* data, size_t length);
    
    friend key_value make_kv(bytes key, bytes value);

    template <typename key, typename value, typename allocator>
    friend key_value make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length, allocator& a);
    
    template <typename key, typename value>
    friend key_value make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length);

    template <typename allocator>
    friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a);
    
    friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length);

public:
    bytes(const bytes& b);
    bytes(bytes&& b);
    ~bytes();
    
    bytes& operator=(const bytes& b);
    bytes& operator=(bytes&& b);
    
    const void * const data() const;
    size_t length() const;
    
    bool operator==(const bytes& other) const;
    bool operator!=(const bytes& other) const;
    
    static const bytes invalid;
    
private:
    bytes() = default;
    
private:
    // The encoded address of the memory allocator that instantiated the memory this instance points to.
    // \warning The memory allocator MUST stay in scope during the lifetime of this instance.
    size_t* m_memory_allocator_address{nullptr};
    // \brief A pointer to a counter to keep track of the number of bytes instances that point to the memory address.
    // \remark When the counter reaches 0, the last instance will free the memory by invoking the free method on the memory allocator.
    // \warning Assuming that this type will be accessed synchronously.
    size_t* m_use_count_address{nullptr};
    // The size of the memory pointed to by m_data, in bytes.
    size_t* m_length{nullptr};
    // A pointer to a chunk of memory.
    void* m_data{nullptr};
};

// \brief Creates a new bytes instance with the given pointer and length.
//
// \tparam T The type stored in the array.
// \tparam allocator The memory allocator type used to manage the bytes memory. This type mush implement the "memory allocator" concept.
// \param data A pointer to an array of items to store in a bytes instance.
// \param length The number of items in the array.
// \param a The memory allocator used to instantiate the memory used by the bytes instance.
template <typename T, typename allocator>
bytes make_bytes(const T* data, size_t length, allocator& a) {
    return make_bytes(reinterpret_cast<const void*>(data), length * sizeof(T), a);
}

template <typename allocator>
bytes make_bytes(const void* data, size_t length, allocator& a) {
    auto result = bytes{};
    
    if (!data || length == 0) {
      return result;
    }

    auto chunk_length = 3 * sizeof(size_t) + length;
    auto* chunk = reinterpret_cast<char*>(a->malloc(chunk_length));
    
    result.m_memory_allocator_address = reinterpret_cast<size_t*>(chunk);
    result.m_use_count_address = reinterpret_cast<size_t*>(chunk + sizeof(size_t));
    result.m_length = reinterpret_cast<size_t*>(chunk + 2 * sizeof(size_t));
    result.m_data = reinterpret_cast<size_t*>(chunk + 3 * sizeof(size_t));

    *(result.m_memory_allocator_address) = reinterpret_cast<size_t>(&a->free_function());
    *(result.m_use_count_address) = 1;
    *(result.m_length) = length;
    memcpy(result.m_data, data, length);

    return result;
}

template <>
inline bytes make_bytes(const void* data, size_t length) {
    auto result = bytes{};
    
    result.m_memory_allocator_address = nullptr;
    result.m_use_count_address = nullptr;
    // encode the length as the pointer address of this data member.
    result.m_length = reinterpret_cast<size_t*>(length);
    result.m_data = reinterpret_cast<void*>(const_cast<void*>(data));
    
    return result;
}

// \brief Creates a new bytes instance with the given pointer and length.
//
// The returns a bytes "view" instance, in that the resulting bytes instance does not manage
// the given memory.
//
// \tparam T The type stored in the array.
// \param data A pointer to an array of items to store in a bytes instance.
// \param length The number of items in the array.
// \warning The resulting bytes instance does NOT take ownership of the given data pointer.
// \warning The data pointer must remain in scope for the lifetime of the given bytes instance.
template <typename T>
bytes make_bytes(const T* data, size_t length) {
    return make_bytes(reinterpret_cast<const void*>(data), length * sizeof(T));
}

inline const bytes bytes::invalid{};

inline bytes::bytes(const bytes& b)
: m_memory_allocator_address{b.m_memory_allocator_address},
  m_use_count_address{b.m_use_count_address},
  m_length{b.m_length},
  m_data{b.m_data} {

    if (m_use_count_address) {
        ++(*m_use_count_address);
    }
}

inline bytes::bytes(bytes&& b)
: m_memory_allocator_address{std::move(b.m_memory_allocator_address)},
  m_use_count_address{std::move(b.m_use_count_address)},
  m_length{std::move(b.m_length)},
  m_data{std::move(b.m_data)} {

    b.m_memory_allocator_address = nullptr;
    b.m_use_count_address = nullptr;
    b.m_length = nullptr;
    b.m_data = nullptr;
}

inline bytes::~bytes() {
    if (!m_data || !m_length || !m_use_count_address) {
        return;
    }
    
    if (--(*m_use_count_address) != 0) {
        return;
    }
    
    // The memory pool must remain in scope for the lifetime of this instance.
    auto* free_function = reinterpret_cast<free_function_type*>(*m_memory_allocator_address);
    (*free_function)(m_data, 3 * sizeof(size_t) + *m_length);
}

inline bytes& bytes::operator=(const bytes& b) {
    if (this == &b) {
        return *this;
    }
    
    m_memory_allocator_address = b.m_memory_allocator_address;
    m_use_count_address = b.m_use_count_address;
    m_length = b.m_length;
    m_data = b.m_data;
    
    if (m_use_count_address) {
        ++(*m_use_count_address);
    }
    
    return *this;
}

inline bytes& bytes::operator=(bytes&& b) {
    if (this == &b) {
        return *this;
    }
    
    m_memory_allocator_address = b.m_memory_allocator_address;
    m_use_count_address = b.m_use_count_address;
    m_length = b.m_length;
    m_data = b.m_data;
    
    b.m_memory_allocator_address = nullptr;
    b.m_use_count_address = nullptr;
    b.m_length = nullptr;
    b.m_data = nullptr;
    
    return *this;
}

inline const void * const bytes::data() const {
    return m_data;
}

inline size_t bytes::length() const {
    if (!m_memory_allocator_address) {
        // The size was encoded as the address of this data member.
        return reinterpret_cast<size_t>(m_length);
    }

    if (!m_length) {
        return 0;
    }
    
    return *m_length;
}

inline bool bytes::operator==(const bytes& other) const {
    if (m_length && other.m_length && m_data && other.m_data) {
        if (length() != other.length()) {
            return false;
        }
        
        return memcmp(m_data, other.m_data, length()) == 0 ? true : false;
    }
    
    // This is just checking if all the pointers are null or not.
    return m_length == other.m_length && m_data == other.m_data;
}

inline bool bytes::operator!=(const bytes& other) const {
    return !(*this == other);
}

}

namespace std {

template <>
struct less<eosio::session::bytes> final {
public:
    bool operator()(const eosio::session::bytes& lhs, const eosio::session::bytes& rhs) const {
        if (lhs == eosio::session::bytes::invalid && rhs == eosio::session::bytes::invalid) {
            return false;
        }
        if (lhs == eosio::session::bytes::invalid) {
            return false;
        }
        if (rhs == eosio::session::bytes::invalid) {
            return true;
        }

        return std::string_view{reinterpret_cast<const char*>(lhs.data()), lhs.length()} < std::string_view{reinterpret_cast<const char*>(rhs.data()), rhs.length()};
    };
};

template <>
struct greater<eosio::session::bytes> final {
public:
    bool operator()(const eosio::session::bytes& lhs, const eosio::session::bytes& rhs) const {
        if (lhs == eosio::session::bytes::invalid && rhs == eosio::session::bytes::invalid) {
            return false;
        }
        if (lhs == eosio::session::bytes::invalid) {
            return false;
        }
        if (rhs == eosio::session::bytes::invalid) {
            return true;
        }

        return std::string_view{reinterpret_cast<const char*>(lhs.data()), lhs.length()} > std::string_view{reinterpret_cast<const char*>(rhs.data()), rhs.length()};
    };
};

template <>
struct hash<eosio::session::bytes> final {
public:
  size_t operator()(const eosio::session::bytes& b) const {
    return std::hash<std::string_view>{}({reinterpret_cast<const char*>(b.data()), b.length()});
  }
};

template <>
struct equal_to<eosio::session::bytes> final
{
public:
    auto operator()(const eosio::session::bytes& lhs, const eosio::session::bytes& rhs) const -> bool
    {
        return std::string_view{reinterpret_cast<const char*>(lhs.data()), lhs.length()} == std::string_view{reinterpret_cast<const char*>(rhs.data()), rhs.length()};
    }
};

}
