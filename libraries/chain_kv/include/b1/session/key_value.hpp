#pragma once

#include <b1/session/bytes.hpp>

namespace b1::session
{

// An immutable structure to represent a key/value pairing.
class key_value final
{
public:
    friend auto make_kv(bytes key, bytes value) -> key_value;

    template <typename key, typename value, typename allocator>
    friend auto make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length, allocator& a) -> key_value;
    
    template <typename allocator>
    friend auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a) -> key_value;
    
    template <typename key, typename value>
    friend auto make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length) -> key_value;

    friend auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length) -> key_value;
public:
    auto key() const -> const bytes&;
    auto value() const -> const bytes&;
    
    auto operator==(const key_value& other) const -> bool;
    auto operator!=(const key_value& other) const -> bool;
    
    static const key_value invalid;

private:
    key_value() = default;
    
private:
    bytes m_key;
    bytes m_value;
};

// Instantiates a key_value instance from the given key/value bytes.
//
// \param key The key.
// \param value The value.
// \returns A key_value instance.
// \remarks This factory does not guarentee that the the memory used for the key/value will be contiguous in memory.
inline auto make_kv(bytes key, bytes value) -> key_value
{
    auto result = key_value{};
    result.m_key = std::move(key);
    result.m_value = std::move(value);
    return result;
}

// Instantiaties a key_value instance from the given key/value pointers.
//
// \tparam key The pointer type of the key.
// \tparam value The pointer type of the value.
// \tparam allocator The memory allocator type used to manage the bytes memory. This type mush implement the "memory allocator" concept.
// \param the_key A pointer to the key.
// \param key_length The size of the memory pointed by the key.
// \param the_value A pointer to the value.
// \param value_length The size of the memory pointed by the value.
// \param a The memory allocator used to managed the memory used by the key_value instance.
// \returns A key_value instance.
// \remarks This factory guarentees that the memory needed for the key and value will be contiguous in memory.
template <typename key, typename value, typename allocator>
auto make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length, allocator& a) -> key_value
{
    auto total_key_length = key_length * sizeof(key);
    auto total_value_length = value_length * sizeof(value);
    return make_kv(reinterpret_cast<const void*>(the_key), total_key_length, reinterpret_cast<const void*>(the_value), total_value_length, a);
}

template <typename key, typename value>
auto make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length) -> key_value
{
    return make_kv(reinterpret_cast<const void*>(the_key), key_length * sizeof(key), reinterpret_cast<const void*>(the_value), value_length * sizeof(key));
}

inline auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length) -> key_value
{
    auto kv = key_value{};
    kv.m_key = make_bytes(key, key_length);
    kv.m_value = make_bytes(value, value_length);
    return kv;
}


// Instantiaties a key_value instance from the given key/value pointers.
//
// \tparam allocator The memory allocator type used to manage the bytes memory. This type mush implement the "memory allocator" concept.
// \param key A pointer to the key.
// \param key_length The size of the memory pointed by the key.
// \param value A pointer to the value.
// \param value_length The size of the memory pointed by the value.
// \param a The memory allocator used to managed the memory used by the key_value instance.
// \returns A key_value instance.
// \remarks This factory guarentees that the memory needed for the key and value will be contiguous in memory.
template <typename allocator>
auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a) -> key_value
{
    auto key_chunk_length = key_length == 0 ? key_length : key_length + 3 * sizeof(size_t);
    auto value_chunk_length = value_length == 0 ? value_length : value_length + 3 * sizeof(size_t);
    auto* chunk = reinterpret_cast<char*>(a->malloc(key_chunk_length + value_chunk_length));
    
    auto populate_bytes = [&a](const void* data, size_t length, char* chunk) mutable
    {
        auto result = bytes{};
        
        if (!data || length == 0)
        {
          return result;
        }
        
        result.m_memory_allocator_address = reinterpret_cast<size_t*>(chunk); 
        result.m_use_count_address = reinterpret_cast<size_t*>(chunk + sizeof(size_t));
        result.m_length = reinterpret_cast<size_t*>(chunk + 2 * sizeof(size_t));
        result.m_data = reinterpret_cast<size_t*>(chunk + 3 * sizeof(size_t));
        
        *(result.m_memory_allocator_address) = reinterpret_cast<size_t>(&a->free_function());
        *(result.m_use_count_address) = 1;
        *(result.m_length) = length;
        memcpy(result.m_data, reinterpret_cast<const void*>(data), length);
        
        return result;
    };
    
    auto key_bytes = populate_bytes(reinterpret_cast<const void*>(key), key_length, chunk);
    auto value_bytes = populate_bytes(reinterpret_cast<const void*>(value), value_length, chunk + 3 * sizeof(size_t) + key_length);
    return make_kv(std::move(key_bytes), std::move(value_bytes));
}

inline const key_value key_value::invalid{};

inline auto key_value::key() const -> const bytes&
{
    return m_key;
}

inline auto key_value::value() const -> const bytes&
{
    return m_value;
}

inline auto key_value::operator==(const key_value& other) const -> bool
{
    return m_key == other.m_key && m_value == other.m_value;
}

inline auto key_value::operator!=(const key_value& other) const -> bool
{
    return !(*this == other);
}

}
