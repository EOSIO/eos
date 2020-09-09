#pragma once

#include <b1/session/shared_bytes.hpp>

namespace eosio::session {

// An immutable structure to represent a key/value pairing.
class key_value final {
 public:
   friend key_value make_kv(shared_bytes key, shared_bytes value);

   template <typename Key, typename Value, typename Allocator>
   friend key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length,
                            Allocator& a);

   template <typename Allocator>
   friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, Allocator& a);

   template <typename Key, typename Value>
   friend key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

   friend key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length);

 public:
   const shared_bytes& key() const;
   const shared_bytes& value() const;

   bool operator==(const key_value& other) const;
   bool operator!=(const key_value& other) const;

   static const key_value invalid;

 private:
   key_value() = default;

 private:
   shared_bytes m_key;
   shared_bytes m_value;
};

// Instantiates a key_value instance from the given key/value shared_bytes.
//
// \param key The key.
// \param value The value.
// \returns A key_value instance.
// \remarks This factory does not guarentee that the the memory used for the key/value will be contiguous in memory.
inline key_value make_kv(shared_bytes key, shared_bytes value) {
   auto result    = key_value{};
   result.m_key   = std::move(key);
   result.m_value = std::move(value);
   return result;
}

// Instantiaties a key_value instance from the given key/value pointers.
//
// \tparam Key The pointer type of the key.
// \tparam Value The pointer type of the value.
// \tparam Allocator The memory allocator type used to manage the shared_bytes memory. This type mush implement the
// "memory allocator" concept. \param the_key A pointer to the key. \param key_length The size of the memory pointed by
// the key. \param the_value A pointer to the value. \param value_length The size of the memory pointed by the value.
// \param a The memory allocator used to managed the memory used by the key_value instance.
// \returns A key_value instance.
// \remarks This factory guarentees that the memory needed for the key and value will be contiguous in memory.
template <typename Key, typename Value, typename Allocator>
key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length, Allocator& a) {
   auto total_key_length   = key_length * sizeof(Key);
   auto total_value_length = value_length * sizeof(Value);
   return make_kv(reinterpret_cast<const void*>(the_key), total_key_length, reinterpret_cast<const void*>(the_value),
                  total_value_length, a);
}

template <typename Key, typename Value>
key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length) {
   return make_kv(reinterpret_cast<const void*>(the_key), key_length * sizeof(Key),
                  reinterpret_cast<const void*>(the_value), value_length * sizeof(Key));
}

inline key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length) {
   auto kv    = key_value{};
   kv.m_key   = make_shared_bytes(key, key_length);
   kv.m_value = make_shared_bytes(value, value_length);
   return kv;
}

// Instantiaties a key_value instance from the given key/value pointers.
//
// \tparam Allocator The memory allocator type used to manage the shared_bytes memory. This type mush implement the
// "memory allocator" concept. \param key A pointer to the key. \param key_length The size of the memory pointed by the
// key. \param value A pointer to the value. \param value_length The size of the memory pointed by the value. \param a
// The memory allocator used to managed the memory used by the key_value instance. \returns A key_value instance.
// \remarks This factory guarentees that the memory needed for the key and value will be contiguous in memory.
template <typename Allocator>
key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, Allocator& a) {
   auto  prefix_size        = 2 * sizeof(uint64_t) + 2 * sizeof(size_t);
   auto  key_chunk_length   = key_length == 0 ? key_length : key_length + prefix_size;
   auto  value_chunk_length = value_length == 0 ? value_length : value_length + prefix_size;
   auto* chunk              = reinterpret_cast<char*>(a->allocate(key_chunk_length + value_chunk_length));

   auto populate_bytes = [&a](const void* data, size_t length, char* chunk) mutable {
      auto result = shared_bytes{};

      if (!data || length == 0) {
         return result;
      }

      result.m_chunk_start              = reinterpret_cast<uint64_t*>(chunk);
      result.m_memory_allocator_address = reinterpret_cast<uint64_t*>(chunk + sizeof(uint64_t));
      result.m_use_count_address        = reinterpret_cast<size_t*>(chunk + 2 * sizeof(uint64_t));
      result.m_length                   = reinterpret_cast<size_t*>(chunk + 2 * sizeof(uint64_t) + sizeof(size_t));
      result.m_data                     = reinterpret_cast<size_t*>(chunk + 2 * sizeof(uint64_t) + 2 * sizeof(size_t));

      *(result.m_memory_allocator_address) = reinterpret_cast<uint64_t>(&a->free_function());
      *(result.m_use_count_address)        = 1;
      *(result.m_length)                   = length;
      memcpy(result.m_data, data, length);

      return result;
   };

   auto key_bytes   = populate_bytes(reinterpret_cast<const void*>(key), key_length, chunk);
   auto value_bytes = populate_bytes(reinterpret_cast<const void*>(value), value_length,
                                     chunk + 2 * sizeof(uint64_t) + 2 * sizeof(size_t) + key_length);

   // Keep track of the start of the chunk.
   *(key_bytes.m_chunk_start)   = reinterpret_cast<uint64_t>(chunk);
   *(value_bytes.m_chunk_start) = reinterpret_cast<uint64_t>(chunk);

   // Share the use counts
   *(key_bytes.m_use_count_address) = 2;
   value_bytes.m_use_count_address  = key_bytes.m_use_count_address;

   return make_kv(std::move(key_bytes), std::move(value_bytes));
}

inline const key_value key_value::invalid{};

inline const shared_bytes& key_value::key() const { return m_key; }

inline const shared_bytes& key_value::value() const { return m_value; }

inline bool key_value::operator==(const key_value& other) const {
   return m_key == other.m_key && m_value == other.m_value;
}

inline bool key_value::operator!=(const key_value& other) const { return !(*this == other); }

} // namespace eosio::session
