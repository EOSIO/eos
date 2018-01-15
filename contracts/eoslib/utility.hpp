#pragma once
#include <eoslib/types.hpp>

namespace eosio {

  template<typename T, typename U>
  inline T&& forward( U&& u ) { return static_cast<T&&>(u); }

  template<typename T>
  inline typename remove_reference<T>::type&& move( T&& arg ) { return (typename remove_reference<T>::type&&)arg; }

} /// namespace eosio
