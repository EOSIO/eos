#pragma once

#include <b1/session/bytes_fwd_decl.hpp>

namespace eosio::session {

class key_value;

inline key_value make_kv(bytes key, bytes value);

template <typename key, typename value, typename allocator>
key_value make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length, allocator& a);

template <typename key, typename value>
key_value make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length);

template <typename allocator>
key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a);

inline key_value make_kv(const void* key, size_t key_length, const void* value, size_t value_length);

}
