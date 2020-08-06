#ifndef key_value_h
#define key_value_h

#include <b1/session/bytes_fwd_decl.hpp>

namespace b1::session
{

// An immutable structure to represent a key/value pairing.
class key_value final
{
public:
    friend auto make_kv(bytes key, bytes value) -> key_value;

    template <typename Key, typename Value, typename allocator>
    friend auto make_kv(const Key* key, size_t key_length, const Value* value, size_t value_length, allocator& a) -> key_value;
    
    template <typename allocator>
    friend auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a) -> key_value;
    
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
// \tparam Key The pointer type of the key.
// \tparam Value The pointer type of the value.
// \tparam allocator The memory allocator type used to manage the bytes memory. This type mush implement the "memory allocator" concept.
// \param key A pointer to the key.
// \param key_length The size of the memory pointed by the key.
// \param value A pointer to the value.
// \param value_length The size of the memory pointed by the value.
// \param a The memory allocator used to managed the memory used by the key_value instance.
// \returns A key_value instance.
// \remarks This factory guarentees that the memory needed for the key and value will be contiguous in memory.
template <typename Key, typename Value, typename allocator>
auto make_kv(const Key* key, size_t key_length, const Value* value, size_t value_length, allocator& a) -> key_value
{
    auto total_key_length = key_length * sizeof(Key);
    auto total_value_length = value_length * sizeof(Value);
    return make_kv(reinterpret_cast<const void*>(key), total_key_length, reinterpret_cast<const void*>(value), total_value_length, a);
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
    auto* chunk = reinterpret_cast<char*>(a->malloc(key_length + value_length + 6 * sizeof(size_t)));
    
    auto populate_bytes = [&a](const void* data, size_t length, void* chunk)
    {
        auto result = bytes{};
        
        result.m_memory_allocator_address = reinterpret_cast<size_t*>(chunk);
        result.m_use_count_address = result.m_memory_allocator_address + sizeof(size_t);
        result.m_length = result.m_use_count_address + sizeof(size_t);
        result.m_data = result.m_length + sizeof(size_t);
        
        *(result.m_memory_allocator_address) = reinterpret_cast<size_t>(a.get());
        *(result.m_use_count_address) = 1;
        *(result.m_length) = length;
        memcpy(result.m_data, reinterpret_cast<const void*>(data), length);
        
        return result;
    };
    
    auto key_bytes = populate_bytes(reinterpret_cast<const void*>(key), key_length, chunk);
    auto value_bytes = populate_bytes(reinterpret_cast<const void*>(value), value_length, chunk + 3 * sizeof(size_t) + key_length);
    return make_kv(std::move(key_bytes), std::move(value_bytes));
}

}


#endif /* key_value_h */
