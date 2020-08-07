#ifndef boost_memory_pool_h
#define boost_memory_pool_h

#include <memory>

#include <boost/pool/pool.hpp>

namespace b1::session
{

// \brief Defines a memory allocator backed by a boost::pool instance.
//
// \remarks This also demonstrates the "memory allocator" concept.  If we want to introduce another
// memory allocator type into the system, all it needs to expose is the malloc and free
// methods as demonostrated in this type.
class boost_memory_allocator final 
: std::enable_shared_from_this<boost_memory_allocator>
{
public:
    auto malloc(size_t length_bytes) -> void*;
    auto free(void* data, size_t length_bytes) -> void;
    auto free_function() -> free_function_type&;

    static auto make() -> std::shared_ptr<boost_memory_allocator>();

    template <typename other>
    bool operator==(const boost_memory_allocator& left, const other& right) const;
    template <typename other>
    bool operator!=(const boost_memory_allocator& left, const other& right) const;

private:
    boost_memory_allocator() = default;

    // Currently just instantiating a boost::memory instance to start.
    // We can change this to try other boost specific memory pools
    boost::pool<> m_pool{sizeof(char)};
    free_function_type m_free_function{[&](void* data, size_t length_bytes){ this->free(data, length_bytes); }};
};

template <typename other>
bool boost_memory_allocator::operator==(const boost_memory_allocator& left, const other& right) const
{
  // Just pointer comparision for now.
  return &left == &right;
}

template <typename other>
bool boost_memory_allocator::operator!=(const boost_memory_allocator& left, const other& right) const
{
  // Just pointer comparision for now.
  return &left != &right;
}

}

#endif /* boost_memory_pool_h */
