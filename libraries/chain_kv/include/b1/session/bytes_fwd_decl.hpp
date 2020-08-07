#pragma once

#include <cstddef>
#include <functional>

namespace b1::session
{
using free_function_type = std::function<void(void* data, size_t length_bytes)>;

class bytes;

template <typename T, typename allocator>
auto make_bytes(const T* data, size_t length, allocator& a) -> bytes;

template <typename allocator>
auto make_bytes(const void* data, size_t length, allocator& a) -> bytes;

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
