#pragma once

#include <cassert>
#include <session/shared_bytes.hpp>

namespace eosio::session {

// An immutable structure to represent a key/value pairing.
class key_value final {
 public:
   friend key_value make_kv(shared_bytes key, shared_bytes value);

   template <typename Key, typename Value>
   friend key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

   template <typename Key, typename Value>
   friend key_value make_kv_view(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

   friend key_value make_kv(const int8_t* key, size_t key_length, const int8_t* value, size_t value_length);

   friend key_value make_kv_view(const int8_t* key, size_t key_length, const int8_t* value, size_t value_length);

 public:
   const shared_bytes& key() const;
   const shared_bytes& value() const;

   bool operator==(const key_value& other) const;
   bool operator!=(const key_value& other) const;

   bool operator!() const;
        operator bool() const;

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
// \param the_key A pointer to the key. \param key_length The size of the memory pointed by
// the key. \param the_value A pointer to the value. \param value_length The size of the memory pointed by the value.
// \returns A key_value instance.
// \remarks This factory guarentees that the memory needed for the key and value will be contiguous in memory.
template <typename Key, typename Value>
key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length) {
   auto total_key_length   = key_length * sizeof(Key);
   auto total_value_length = value_length * sizeof(Value);
   return make_kv(reinterpret_cast<const int8_t*>(the_key), total_key_length,
                  reinterpret_cast<const int8_t*>(the_value), total_value_length);
}

template <typename Key, typename Value>
key_value make_kv_view(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length) {
   return make_kv_view(reinterpret_cast<const int8_t*>(the_key), key_length * sizeof(Key),
                       reinterpret_cast<const int8_t*>(the_value), value_length * sizeof(Key));
}

inline key_value make_kv_view(const int8_t* key, size_t key_length, const int8_t* value, size_t value_length) {
   auto kv    = key_value{};
   kv.m_key   = make_shared_bytes_view(key, key_length);
   kv.m_value = make_shared_bytes_view(value, value_length);
   return kv;
}

// Instantiaties a key_value instance from the given key/value pointers.
//
// \param key A pointer to the key. \param key_length The size of the memory pointed by the
// key. \param value A pointer to the value. \param value_length The size of the memory pointed by the value.
// \returns A key_value instance.
// \remarks This factory guarentees that the memory needed for the key and value will be contiguous in memory.
inline key_value make_kv(const int8_t* key, size_t key_length, const int8_t* value, size_t value_length) {
   assert(key && key_length > 0);
   assert(value && value_length > 0);

   auto* chunk = std::allocator<int8_t>{}.allocate(key_length + value_length);
   memcpy(chunk, key, key_length);
   memcpy(chunk + key_length, value, value_length);

   auto deleter = [&](auto* chunk) { std::allocator<int8_t>{}.deallocate(chunk, key_length + value_length); };

   auto key_bytes          = shared_bytes{};
   key_bytes.m_data        = std::shared_ptr<int8_t>(chunk, deleter);
   key_bytes.m_start_index = 0;
   key_bytes.m_end_index   = key_length - 1;

   auto value_bytes          = shared_bytes{};
   value_bytes.m_data        = key_bytes.m_data;
   value_bytes.m_start_index = key_length;
   value_bytes.m_end_index   = value_bytes.m_start_index + value_length - 1;

   return make_kv(std::move(key_bytes), std::move(value_bytes));
}

inline const key_value key_value::invalid{};

inline const shared_bytes& key_value::key() const { return m_key; }

inline const shared_bytes& key_value::value() const { return m_value; }

inline bool key_value::operator==(const key_value& other) const {
   return m_key == other.m_key && m_value == other.m_value;
}

inline bool key_value::operator!=(const key_value& other) const { return !(*this == other); }

inline bool key_value::operator!() const { return *this == key_value::invalid; }

inline key_value::operator bool() const { return *this != key_value::invalid; }

} // namespace eosio::session
