#pragma once
namespace eosio {
  typedef decltype(nullptr) nullptr_t;

  struct true_type  { enum _value { value = 1 }; };
  struct false_type { enum _value { value = 0 }; };

  template<typename T, typename U>
  inline T&& forward( U&& u ) { return static_cast<T&&>(u); }
} /// namespace eosio
