#pragma once

#include <b1/session/shared_bytes_fwd_decl.hpp>

namespace eosio::session {

class key_value;

inline key_value make_kv(shared_bytes key, shared_bytes value);

template <typename Key, typename Value, typename Allocator>
key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length, Allocator& a);

template <typename Key, typename Value>
key_value make_kv(const Key* the_key, size_t key_length, const Value* the_value, size_t value_length);

template <typename Allocator>
key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, Allocator& a);

inline key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length);

}
