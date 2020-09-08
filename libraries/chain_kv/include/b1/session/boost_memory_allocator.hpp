#pragma once

#include <memory>

#include <boost/pool/pool.hpp>

#include <b1/session/shared_bytes_fwd_decl.hpp>

namespace eosio::session {

// \brief Defines a memory allocator backed by a boost::pool instance.
//
// \remarks Intended to follow the concept of an std allocator.
class boost_memory_allocator : public std::enable_shared_from_this<boost_memory_allocator> {
public:
    void* allocate(size_t length_bytes) const;
    void deallocate(void* data, size_t length_bytes) const;
    free_function_type& free_function();

    static std::shared_ptr<boost_memory_allocator> make();

    template <typename Other>
    bool equals(const Other& right) const;

protected:
    boost_memory_allocator() = default;
    void initialize_();

private:
    // Currently just instantiating a boost::memory instance to start.
    // We can change this to try other boost specific memory pools
    boost::pool<> m_pool{sizeof(char)};
    free_function_type m_free_function;
};

template <typename Other>
bool boost_memory_allocator::equals(const Other& right) const {
  // Just pointer comparision for now.
  return this == &right;
}

inline void boost_memory_allocator::initialize_() {
  m_free_function 
    = [weak = std::weak_ptr<boost_memory_allocator>(this->shared_from_this())](void* data, size_t length_bytes) { 
        if (auto ptr = weak.lock()) {
          ptr->deallocate(data, length_bytes); 
        }
      };
}

// \brief Allocates a chunk of memory.
//
// \param length_bytes The size of the memory, in bytes.
inline void* boost_memory_allocator::allocate(size_t length_bytes) const {
    return const_cast<boost::pool<>&>(m_pool).ordered_malloc(length_bytes);
}

// \brief Frees a chunks of memory.
//
// \param data A pointer to the memory to free.
// \param length_bytes The size of the memory, in bytes.
inline void boost_memory_allocator::deallocate(void* data, size_t /*length_bytes*/) const {
    const_cast<boost::pool<>&>(m_pool).ordered_free(data);
}

inline free_function_type& boost_memory_allocator::free_function() {
    return m_free_function;
}

inline std::shared_ptr<boost_memory_allocator> boost_memory_allocator::make() {
    struct boost_memory_allocator_ : public boost_memory_allocator {};
    auto result = std::make_shared<boost_memory_allocator_>();
    result->initialize_();
    return result;
}

}
