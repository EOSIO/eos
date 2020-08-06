#ifndef boost_memory_pool_h
#define boost_memory_pool_h

#include <boost/pool/pool.hpp>

namespace b1::session
{

// \brief Defines a memory allocator backed by a boost::pool instance.
//
// \remarks This also demonstrates the "memory allocator" concept.  If we want to introduce another
// memory allocator type into the system, all it needs to expose is the malloc and free
// methods as demonostrated in this type.
class boost_memory_allocator final
{
public:
    auto malloc(size_t length_bytes) -> void*;
    auto free(void* data, size_t length_bytes) -> void;
    
private:
    
    // Currently just instantiating a boost::memory instance to start.
    // We can change this to try other boost specific memory pools
    boost::pool<> m_pool{sizeof(char)};
};

}

#endif /* boost_memory_pool_h */
