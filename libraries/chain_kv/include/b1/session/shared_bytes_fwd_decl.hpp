#pragma once

#include <cstddef>
#include <functional>

namespace eosio::session {

class shared_bytes;

template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length);

inline shared_bytes make_shared_bytes(const int8_t* data, size_t length);

template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length);

} // namespace eosio::session

namespace std {

template <>
struct less<eosio::session::shared_bytes>;

template <>
struct greater<eosio::session::shared_bytes>;

template <>
struct hash<eosio::session::shared_bytes>;

template <>
struct equal_to<eosio::session::shared_bytes>;

} // namespace std
