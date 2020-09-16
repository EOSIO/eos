#pragma once

#include <session/shared_bytes_fwd_decl.hpp>

namespace eosio::session {

class key_value;

inline key_value make_kv(shared_bytes key, shared_bytes value);

template <typename Key, typename Value>
key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

template <typename Key, typename Value>
key_value make_kv_view(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

inline key_value make_kv(const int8_t* key, size_t key_length, const int8_t* value, size_t value_length);

inline key_value make_kv_view(const int8_t* key, size_t key_length, const int8_t* value, size_t value_length);

} // namespace eosio::session
