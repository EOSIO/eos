#ifndef bytes_fwd_decl_h
#define bytes_fwd_decl_h

#include <cstddef>
#include <functional>

namespace b1::session
{

struct bytes;

template <typename T, typename allocator>
auto make_bytes(const T* data, size_t length, allocator& a) -> bytes;

template <typename T>
auto make_bytes(const T* data, size_t length) -> bytes;

}

namespace std
{
template <>
class less<b1::session::bytes>;

template <>
class greater<b1::session::bytes>;

template <>
class hash<b1::session::bytes>;

template <>
class equal_to<b1::session::bytes>;
}

#endif /* bytes_fwd_decl_h */
