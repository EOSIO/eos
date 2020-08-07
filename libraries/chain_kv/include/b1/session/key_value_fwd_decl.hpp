#ifndef key_value_fwd_decl_h
#define key_value_fwd_decl_h

#include <b1/session/bytes_fwd_decl.hpp>

namespace b1::session
{

struct key_value;

inline auto make_kv(bytes key, bytes value) -> key_value;

template <typename key, typename value, typename allocator>
auto make_kv(const key* the_key, size_t key_length, const value* the_value, size_t value_length, allocator& a) -> key_value;

template <typename allocator>
auto make_kv(const void* key, size_t key_length, const void* value, size_t value_length, allocator& a) -> key_value;

}

#endif /* key_value_fwd_decl_h */
