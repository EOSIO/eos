/** This source adapted from https://github.com/kmicklas/variadic-static_variant
 *
 * Copyright (C) 2013 Kenneth Micklas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **/
#pragma once
#include <stdexcept>
#include <typeinfo>
#include <fc/exception/exception.hpp>
#include <boost/core/typeinfo.hpp>
#include <variant>

namespace fc {

// This will go away.
template<typename Result>
struct visitor {};

template <typename variant, int I = 0>
void from_index(variant& v, int index) 
{
  if constexpr(I >= std::variant_size_v<variant>)
  {
    throw std::runtime_error{"Variant index " + std::to_string(I + index) + " out of bounds"};
  }
  else if (index == 0)
  {
     auto value = variant(std::in_place_index<I>);
     v = std::move(value); 
  }
  else
  {
     from_index<variant, I + 1>(v, index - 1);
  }
}

template<typename VariantType, typename T, std::size_t index = 0>
constexpr std::size_t get_index() 
{
  if constexpr (index == std::variant_size_v<VariantType>) 
  {
    return index;
  } 
  else if constexpr (std::is_same_v<std::variant_alternative_t<index, VariantType>, T>) 
  {
    return index;
  } 
  else 
  {
    return get_index<VariantType, T, index + 1>();
  }
} 

struct from_static_variant
{
  variant& var;
  from_static_variant( variant& dv ):var(dv){}

  template<typename T> void operator()( const T& v )const
  {
      to_variant( v, var );
  }
};

struct to_static_variant
{
  const variant& var;
  to_static_variant( const variant& dv ):var(dv){}

  template<typename T> void operator()( T& v )const
  {
      from_variant( var, v );
  }
};

template<typename... T> void to_variant( const std::variant<T...>& s, fc::variant& v )
{
  variants vars(2);
  vars[0] = s.index();
  visit( from_static_variant(vars[1]), s );
  v = std::move(vars);
}
template<typename... T> void from_variant( const fc::variant& v, std::variant<T...>& s )
{
  auto ar = v.get_array();
  if( ar.size() < 2 ) return;
  from_index(s, ar[0].as_uint64());
  visit( to_static_variant(ar[1]), s );
}

template<typename... T> struct get_typename { static const char* name()   { return BOOST_CORE_TYPEID(std::variant<T...>).name();   } };

}