#include <session/boost_memory_pool.hpp>

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
