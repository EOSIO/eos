#ifndef bytes_h
#define bytes_h

#include <string_view>

#include <b1/session/bytes_fwd_decl.hpp>
#include <b1/session/key_value_fwd_decl.hpp>

namespace b1::session
{

using free_function_type = std::function<void(void* data, size_t length_bytes)>;

// \brief An immutable type to represent a pointer and a length.
//
class bytes final
{
public:
    friend class key_value;
    
    template <typename T, typename allocator>
    friend auto make_bytes(const T* data, size_t length, allocator& a) -> bytes;
    
    template <typename T>
    friend auto make_bytes(const T* data, size_t length) -> bytes;
    
    friend auto make_kv(bytes key, bytes value) -> key_value;

    template <typename key, typename value, typename allocator>
    friend auto make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length, allocator& a) -> key_value;
    
    template <typename allocator>
    friend auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a) -> key_value;
    
public:
    bytes(const bytes& b);
    bytes(bytes&& b);
    ~bytes();
    
    auto operator=(const bytes& b) -> bytes&;
    auto operator=(bytes&& b) -> bytes&;
    
    auto data() const -> const void * const;
    auto length() const -> size_t;
    
    auto operator==(const bytes& other) const -> bool;
    auto operator!=(const bytes& other) const -> bool;
    
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
auto make_bytes(const T* data, size_t length, allocator& a) -> bytes
{
    return make_bytes(reinterpret_cast<const void*>(data), length * sizeof(T));
}

template <>
auto make_bytes(const void* data, size_t length, allocator& a) -> bytes
{
    auto result = bytes{};
    
    auto chunk_length = 3 * sizeof(size_t) + length;
    auto* chunk = reinterpret_cast<char*>(a->malloc(chunk_length));
    
    result.m_memory_allocator_address = reinterpret_cast<size_t*>(chunk);
    result.m_use_count_address = result.m_memory_allocator_address + sizeof(size_t);
    result.m_length = result.m_use_count_address + sizeof(size_t);
    result.m_data = result.m_length + sizeof(size_t);
    
    *(result.m_memory_allocator_address) = reinterpret_cast<size_t>(&a->free_function();
    *(result.m_use_coun t_address) = 1;
    *(result.m_length) = length;
    memcpy(result.m_data, reinterpret_cast<const void*>(data), length);
    
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
auto make_bytes(const T* data, size_t length) -> bytes
{
    return make_bytes(reinterpret_cast<const void*>(data), length * sizeof(T));
}

template <>
auto make_bytes(const void* data, size_t length) -> bytes
{
    auto result = bytes{};
    
    result.m_memory_allocator_address = nullptr;
    result.m_use_count_address = nullptr;
    *(result.m_length) = length;
    result.m_data = reinterpret_cast<void*>(const_cast<T*>(data));
    
    return result;
}

}

namespace std
{
template <>
class less<b1::session::bytes> final
{
public:
    constexpr auto operator()(const b1::session::bytes& lhs, const b1::session::bytes& rhs) const -> bool
    {
        // Shorter keys are "less than" longer keys
        if (lhs.length() < rhs.length())
        {
            return true;
        }
        
        if (lhs.length() > rhs.length())
        {
            return false;
        }
        
        if (lhs.data() == nullptr && rhs.data() == nullptr)
        {
            return true;
        }
        
        if (!lhs.data())
        {
            return true;
        }
        
        if (!rhs.data())
        {
            return true;
        }
        
        return memcmp(lhs.data(), rhs.data(), lhs.length()) < 0;
    };
};

template <>
class greater<b1::session::bytes> final
{
public:
    constexpr auto operator()(const b1::session::bytes& lhs, const b1::session::bytes& rhs) const -> bool
    {
        // Shorter keys are "less than" longer keys
        if (lhs.length() > rhs.length())
        {
            return true;
        }
        
        if (lhs.length() < rhs.length())
        {
            return false;
        }
        
        if (lhs.data() == nullptr && rhs.data() == nullptr)
        {
            return true;
        }
        
        if (!lhs.data())
        {
            return true;
        }
        
        if (!rhs.data())
        {
            return true;
        }
        
        return memcmp(lhs.data(), rhs.data(), lhs.length()) > 0;
    };
};

template <>
class hash<b1::session::bytes> final
{
public:
  auto operator()(const b1::session::bytes& b) const -> size_t
  {
    return std::hash<std::string_view>{}({reinterpret_cast<const char*>(b.data()), b.length()});
  }
};

template <>
class equal_to<b1::session::bytes> final
{
public:
    constexpr auto operator()(const b1::session::bytes& lhs, const b1::session::bytes& rhs) const -> bool
    {
        if (lhs.length() != rhs.length())
        {
            return false;
        }
        
        if (lhs.data() == nullptr && rhs.data() == nullptr)
        {
            return true;
        }
        
        if (!lhs.data() || !rhs.data())
        {
            return false;
        }
        
        return memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
    }
};
}

#endif /* bytes_h */
