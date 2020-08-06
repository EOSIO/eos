#ifndef key_value_fwd_decl_h
#define key_value_fwd_decl_h

#include <b1/session/bytes_fwd_decl.hpp>

namespace b1::session
{

struct key_value;

inline auto make_kv(bytes key, bytes value) -> key_value;

template <typename Key, typename Value, typename allocator>
auto make_kv(const Key* key, size_t key_length, const Value* value, size_t value_length, allocator& a) -> key_value;

template <typename allocator>
auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a) -> key_value;

}

#endif /* key_value_fwd_decl_h */
