#pragma once

#include <cstddef>
#include <functional>

namespace eosio::session {

using free_function_type = std::function<void(void* data, size_t length_bytes)>;

class shared_bytes;

template <typename T, typename Allocator>
shared_bytes make_shared_bytes(const T* data, size_t length, Allocator& a);

template <typename Allocator>
shared_bytes make_shared_bytes(const void* data, size_t length, Allocator& a);

template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length);

}

namespace std {

template <>
struct less<eosio::session::shared_bytes>;

template <>
struct greater<eosio::session::shared_bytes>;

template <>
struct hash<eosio::session::shared_bytes>;

template <>
struct equal_to<eosio::session::shared_bytes>;

}
