#pragma once

#include <memory>

#include <boost/pool/pool.hpp>

#include <b1/session/bytes_fwd_decl.hpp>

namespace b1::session
{

// \brief Defines a memory allocator backed by a boost::pool instance.
//
// \remarks This also demonstrates the "memory allocator" concept.  If we want to introduce another
// memory allocator type into the system, all it needs to expose is the malloc and free
// methods as demonostrated in this type.
class boost_memory_allocator 
: std::enable_shared_from_this<boost_memory_allocator>
{
public:
    auto malloc(size_t length_bytes) const -> void*;
    auto free(void* data, size_t length_bytes) const -> void;
    auto free_function() -> free_function_type&;

    static auto make() -> std::shared_ptr<boost_memory_allocator>;

    template <typename other>
    bool equals(const other& right) const;

protected:
    boost_memory_allocator() = default;

private:
    // Currently just instantiating a boost::memory instance to start.
    // We can change this to try other boost specific memory pools
    boost::pool<> m_pool{sizeof(char)};
    free_function_type m_free_function{[&](void* data, size_t length_bytes){ this->free(data, length_bytes); }};
};

template <typename other>
bool boost_memory_allocator::equals(const other& right) const
{
  // Just pointer comparision for now.
  return this == &right;
}

// \brief Allocates a chunk of memory.
//
// \param length_bytes The size of the memory, in bytes.
inline auto boost_memory_allocator::malloc(size_t length_bytes) const -> void*
{
    return const_cast<boost::pool<>&>(m_pool).ordered_malloc(length_bytes);
}

// \brief Frees a chunks of memory.
//
// \param data A pointer to the memory to free.
// \param length_bytes The size of the memory, in bytes.
inline auto boost_memory_allocator::free(void* data, size_t length_bytes) const -> void
{
    const_cast<boost::pool<>&>(m_pool).ordered_free(data, length_bytes);
}

inline auto boost_memory_allocator::free_function() -> free_function_type&
{
    return m_free_function;
}

inline auto boost_memory_allocator::make() -> std::shared_ptr<boost_memory_allocator>
{
    struct boost_memory_allocator_ : public boost_memory_allocator {};
    return std::make_shared<boost_memory_allocator_>();
}

}
