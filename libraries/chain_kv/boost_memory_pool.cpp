#include <b1/session/boost_memory_pool.hpp>

using namespace b1::session;

// \brief Allocates a chunk of memory.
//
// \param length_bytes The size of the memory, in bytes.
auto boost_memory_allocator::malloc(size_t length_bytes) -> void*
{
    return m_pool.ordered_malloc(length_bytes);
}

// \brief Frees a chunks of memory.
//
// \param data A pointer to the memory to free.
// \param length_bytes The size of the memory, in bytes.
auto boost_memory_allocator::free(void* data, size_t length_bytes) -> void
{
    m_pool.ordered_free(data, length_bytes);
}

auto boost_memory_allocator::free_function() -> free_function_type&
{
    return m_free_function;
}

auto boost_memory_allocator::make() -> std::shared_ptr<boost_memory_allocator>()
{
    return std::make_shared<boost_memory_allocator>();
}
