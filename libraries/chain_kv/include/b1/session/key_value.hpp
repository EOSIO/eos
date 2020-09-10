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
   auto  chunk_size = sizeof(shared_bytes::control_block) + key_length + value_length;
   auto* chunk      = reinterpret_cast<int8_t*>(a->allocate(chunk_size));

   auto key_bytes                                      = shared_bytes{};
   key_bytes.m_chunk_start                             = chunk;
   key_bytes.m_chunk_end                               = chunk + chunk_size;
   key_bytes.m_control_block                           = reinterpret_cast<shared_bytes::control_block*>(chunk);
   key_bytes.m_control_block->memory_allocator_address = reinterpret_cast<uint64_t>(&a->free_function());
   key_bytes.m_control_block->use_count                = 2;
   key_bytes.m_data_start                              = chunk + sizeof(shared_bytes::control_block);
   key_bytes.m_data_end                                = key_bytes.m_data_start + key_length;
   memcpy(key_bytes.m_data_start, key, key_length);

   auto value_bytes            = shared_bytes{};
   value_bytes.m_chunk_start   = key_bytes.m_chunk_start;
   value_bytes.m_chunk_end     = key_bytes.m_chunk_end;
   value_bytes.m_control_block = key_bytes.m_control_block;
   value_bytes.m_data_start    = key_bytes.m_data_end;
   value_bytes.m_data_end      = value_bytes.m_data_start + value_length;
   memcpy(value_bytes.m_data_start, value, value_length);

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
