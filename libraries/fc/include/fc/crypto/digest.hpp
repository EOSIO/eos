#pragma once
#include <fc/crypto/sha256.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/reflect.hpp>

namespace fc {

template <typename T> fc::sha256 digest(const T &value) {
  fc::sha256::encoder enc;
  fc::raw::pack(enc, value);
  return enc.result();
}
}
