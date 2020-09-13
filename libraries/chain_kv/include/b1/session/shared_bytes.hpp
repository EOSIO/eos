#pragma once

#include <algorithm>
#include <string_view>

#include <boost/pool/pool.hpp>

#include <b1/session/key_value_fwd_decl.hpp>
#include <b1/session/shared_bytes_fwd_decl.hpp>

namespace eosio::session {

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes final {
 public:
   friend class key_value;

   template <typename T, typename Allocator>
   friend shared_bytes make_shared_bytes(const T* data, size_t length, Allocator& a);

   template <typename Allocator>
   friend shared_bytes make_shared_bytes(const void* data, size_t length, Allocator& a);

   template <typename T>
   friend shared_bytes make_shared_bytes(const T* data, size_t length);

   friend key_value make_kv(shared_bytes key, shared_bytes value);

   template <typename Key, typename Value, typename Allocator>
   friend key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length,
                            Allocator& a);

   template <typename Key, typename Value>
   friend key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

   template <typename Allocator>
   friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, Allocator& a);

   friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length);

 public:
   shared_bytes(const shared_bytes& b);
   shared_bytes(shared_bytes&& b);
   ~shared_bytes();

   shared_bytes& operator=(const shared_bytes& b);
   shared_bytes& operator=(shared_bytes&& b);

   const int8_t* const data() const;
   size_t              length() const;

   bool operator==(const shared_bytes& other) const;
   bool operator!=(const shared_bytes& other) const;

   static const shared_bytes invalid;

 private:
   shared_bytes() = default;

 private:
   struct control_block {
      // The encoded address of the memory allocator that instantiated the memory this instance points to.
      // \warning The memory allocator MUST stay in scope during the lifetime of this instance.
      uint64_t memory_allocator_address{ 0 };
      // \brief A counter to keep track of the number of shared_bytes instances that point to the memory
      // address.
      // \remark When the counter reaches 0, the last instance will free the memory by invoking the free method on
      // the memory allocator.
      // \warning Assuming that this type will be accessed synchronously.
      size_t use_count{ 0 };
   };
   // To allow for sharing a chunk of memory across multiple shared_byte instances, we need to keep track
   // of the chunk start in each instance to pass back to the allocator.
   int8_t*        m_chunk_start{ nullptr };
   int8_t*        m_chunk_end{ nullptr };
   control_block* m_control_block{ nullptr };
   int8_t*        m_data_start{ nullptr };
   int8_t*        m_data_end{ nullptr };
};

// \brief Creates a new shared_bytes instance with the given pointer and length.
//
// \tparam T The type stored in the array.
// \tparam allocator The memory allocator type used to manage the shared_bytes memory. This type mush implement the
// "memory allocator" concept. \param data A pointer to an array of items to store in a shared_bytes instance. \param
// length The number of items in the array. \param a The memory allocator used to instantiate the memory used by the
// shared_bytes instance.
template <typename T, typename Allocator>
shared_bytes make_shared_bytes(const T* data, size_t length, Allocator& a) {
   return make_shared_bytes(reinterpret_cast<const void*>(data), length * sizeof(T), a);
}

template <typename Allocator>
shared_bytes make_shared_bytes(const void* data, size_t length, Allocator& a) {
   auto result = shared_bytes{};

   if (!data || length == 0) {
      return result;
   }

   auto  chunk_length                               = sizeof(shared_bytes::control_block) + length;
   auto* chunk                                      = reinterpret_cast<int8_t*>(a->allocate(chunk_length));
   result.m_chunk_start                             = chunk;
   result.m_chunk_end                               = chunk + chunk_length;
   result.m_control_block                           = reinterpret_cast<shared_bytes::control_block*>(chunk);
   result.m_control_block->memory_allocator_address = reinterpret_cast<uint64_t>(&a->free_function());
   result.m_control_block->use_count                = 1;
   result.m_data_start                              = chunk + sizeof(shared_bytes::control_block);
   result.m_data_end                                = result.m_data_start + length;
   memcpy(result.m_data_start, data, length);

   return result;
}

template <>
inline shared_bytes make_shared_bytes(const void* data, size_t length) {
   auto result = shared_bytes{};

   // This is a "shared_bytes view".  Encode the date using the chunk start and chunk end data members.
   result.m_data_start = reinterpret_cast<int8_t*>(const_cast<void*>(data));
   result.m_data_end   = result.m_data_start + length;

   return result;
}

// \brief Creates a new shared_bytes instance with the given pointer and length.
//
// This returns a shared_bytes "view" instance, in that the resulting shared_bytes instance does not manage
// the given memory.
//
// \tparam T The type stored in the array.
// \param data A pointer to an array of items to store in a shared_bytes instance.
// \param length The number of items in the array.
// \warning The resulting shared_bytes instance does NOT take ownership of the given data pointer.
// \warning The data pointer must remain in scope for the lifetime of the given shared_bytes instance.
template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length) {
   return make_shared_bytes(reinterpret_cast<const void*>(data), length * sizeof(T));
}

inline const shared_bytes shared_bytes::invalid{};

inline shared_bytes::shared_bytes(const shared_bytes& b)
    : m_chunk_start{ b.m_chunk_start }, m_chunk_end{ b.m_chunk_end }, m_control_block{ b.m_control_block },
      m_data_start{ b.m_data_start }, m_data_end{ b.m_data_end } {

   if (m_control_block) {
      ++(m_control_block->use_count);
   }
}

inline shared_bytes::shared_bytes(shared_bytes&& b)
    : m_chunk_start{ b.m_chunk_start }, m_chunk_end{ b.m_chunk_end }, m_control_block{ b.m_control_block },
      m_data_start{ b.m_data_start }, m_data_end{ b.m_data_end } {
   b.m_chunk_start   = nullptr;
   b.m_chunk_end     = nullptr;
   b.m_control_block = nullptr;
   b.m_data_start    = nullptr;
   b.m_data_end      = nullptr;
}

inline shared_bytes::~shared_bytes() {
   if (!m_chunk_start || !m_chunk_end || !m_control_block || !m_data_start || !m_data_end) {
      return;
   }

   if (--(m_control_block->use_count) != 0) {
      return;
   }

   // The memory pool must remain in scope for the lifetime of this instance.
   auto* free_function = reinterpret_cast<free_function_type*>(m_control_block->memory_allocator_address);
   (*free_function)(m_chunk_start, m_chunk_end - m_chunk_start);
}

inline shared_bytes& shared_bytes::operator=(const shared_bytes& b) {
   if (this == &b) {
      return *this;
   }

   m_chunk_start   = b.m_chunk_start;
   m_chunk_end     = b.m_chunk_end;
   m_control_block = b.m_control_block;
   m_data_start    = b.m_data_start;
   m_data_end      = b.m_data_end;

   if (m_control_block) {
      ++(m_control_block->use_count);
   }

   return *this;
}

inline shared_bytes& shared_bytes::operator=(shared_bytes&& b) {
   if (this == &b) {
      return *this;
   }

   m_chunk_start   = b.m_chunk_start;
   m_chunk_end     = b.m_chunk_end;
   m_control_block = b.m_control_block;
   m_data_start    = b.m_data_start;
   m_data_end      = b.m_data_end;

   b.m_chunk_start   = nullptr;
   b.m_chunk_end     = nullptr;
   b.m_control_block = nullptr;
   b.m_data_start    = nullptr;
   b.m_data_end      = nullptr;

   return *this;
}

inline const int8_t* const shared_bytes::data() const { return m_data_start; }

inline size_t shared_bytes::length() const { return m_data_end - m_data_start; }

inline bool shared_bytes::operator==(const shared_bytes& other) const {
   auto compare = [](auto* left_start, auto* left_end, auto* right_start, auto* right_end) {
      auto left_length  = left_end - left_start;
      auto right_length = right_end - right_start;

      if (left_length != right_length) {
         return false;
      }

      return memcmp(left_start, right_start, left_length) == 0 ? true : false;
   };

   if (m_data_start && other.m_data_start) {
      return compare(m_data_start, m_data_end, other.m_data_start, other.m_data_end);
   }

   return m_chunk_start == other.m_chunk_start && m_chunk_end == other.m_chunk_end &&
          m_control_block == other.m_control_block && m_data_start == other.m_data_start &&
          m_data_end == other.m_data_end;
}

inline bool shared_bytes::operator!=(const shared_bytes& other) const { return !(*this == other); }

} // namespace eosio::session

namespace std {

template <>
struct less<eosio::session::shared_bytes> final {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      if (lhs != eosio::session::shared_bytes::invalid && rhs != eosio::session::shared_bytes::invalid) {
         return std::string_view{ reinterpret_cast<const char*>(lhs.data()), lhs.length() } <
                std::string_view{ reinterpret_cast<const char*>(rhs.data()), rhs.length() };
      }
      if (lhs == eosio::session::shared_bytes::invalid) {
         return false;
      }
      return true;
   };
};

template <>
struct greater<eosio::session::shared_bytes> final {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      if (lhs != eosio::session::shared_bytes::invalid && rhs != eosio::session::shared_bytes::invalid) {
         return std::string_view{ reinterpret_cast<const char*>(lhs.data()), lhs.length() } >
                std::string_view{ reinterpret_cast<const char*>(rhs.data()), rhs.length() };
      }
      if (lhs == eosio::session::shared_bytes::invalid) {
         return false;
      }
      return true;
   };
};

template <>
struct hash<eosio::session::shared_bytes> final {
   size_t operator()(const eosio::session::shared_bytes& b) const {
      return std::hash<std::string_view>{}({ reinterpret_cast<const char*>(b.data()), b.length() });
   }
};

template <>
struct equal_to<eosio::session::shared_bytes> final {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return std::string_view{ reinterpret_cast<const char*>(lhs.data()), lhs.length() } ==
             std::string_view{ reinterpret_cast<const char*>(rhs.data()), rhs.length() };
   }
};

} // namespace std
