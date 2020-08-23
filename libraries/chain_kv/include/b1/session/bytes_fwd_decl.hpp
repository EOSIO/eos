#pragma once

#include <cstddef>
#include <functional>

namespace eosio::session {

using free_function_type = std::function<void(void* data, size_t length_bytes)>;

class bytes;

template <typename T, typename allocator>
bytes make_bytes(const T* data, size_t length, allocator& a);

template <typename allocator>
bytes make_bytes(const void* data, size_t length, allocator& a);

template <typename T>
bytes make_bytes(const T* data, size_t length);

}

namespace std {

template <>
struct less<eosio::session::bytes>;

template <>
struct greater<eosio::session::bytes>;

template <>
struct hash<eosio::session::bytes>;

template <>
struct equal_to<eosio::session::bytes>;

}
