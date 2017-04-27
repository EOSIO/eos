/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <fc/io/varint.hpp>
#include <fc/reflect/reflect.hpp>

namespace eos { namespace chain {

template< typename T >
struct extension
{
   extension() {}

   T value;
};

template< typename T >
struct eos_extension_pack_count_visitor
{
   eos_extension_pack_count_visitor( const T& v ) : value(v) {}

   template<typename Member, class Class, Member (Class::*member)>
   void operator()( const char* name )const
   {
      count += ((value.*member).valid()) ? 1 : 0;
   }

   const T& value;
   mutable uint32_t count = 0;
};

template< typename Stream, typename T >
struct eos_extension_pack_read_visitor
{
   eos_extension_pack_read_visitor( Stream& s, const T& v ) : stream(s), value(v) {}

   template<typename Member, class Class, Member (Class::*member)>
   void operator()( const char* name )const
   {
      if( (value.*member).valid() )
      {
         fc::raw::pack( stream, unsigned_int( which ) );
         fc::raw::pack( stream, *(value.*member) );
      }
      ++which;
   }

   Stream& stream;
   const T& value;
   mutable uint32_t which = 0;
};

template< typename Stream, class T >
void operator<<( Stream& stream, const eos::chain::extension<T>& value )
{
   eos_extension_pack_count_visitor<T> count_vtor( value.value );
   fc::reflector<T>::visit( count_vtor );
   fc::raw::pack( stream, unsigned_int( count_vtor.count ) );
   eos_extension_pack_read_visitor<Stream,T> read_vtor( stream, value.value );
   fc::reflector<T>::visit( read_vtor );
}



template< typename Stream, typename T >
struct eos_extension_unpack_visitor
{
   eos_extension_unpack_visitor( Stream& s, T& v ) : stream(s), value(v)
   {
      unsigned_int c;
      fc::raw::unpack( stream, c );
      count_left = c.value;
      maybe_read_next_which();
   }

   void maybe_read_next_which()const
   {
      if( count_left > 0 )
      {
         unsigned_int w;
         fc::raw::unpack( stream, w );
         next_which = w.value;
      }
   }

   template< typename Member, class Class, Member (Class::*member)>
   void operator()( const char* name )const
   {
      if( (count_left > 0) && (which == next_which) )
      {
         typename Member::value_type temp;
         fc::raw::unpack( stream, temp );
         (value.*member) = temp;
         --count_left;
         maybe_read_next_which();
      }
      else
         (value.*member).reset();
      ++which;
   }

   mutable uint32_t      which = 0;
   mutable uint32_t next_which = 0;
   mutable uint32_t count_left = 0;

   Stream& stream;
   T& value;
};

template< typename Stream, typename T >
void operator>>( Stream& s, eos::chain::extension<T>& value )
{
   value = eos::chain::extension<T>();
   eos_extension_unpack_visitor<Stream, T> vtor( s, value.value );
   fc::reflector<T>::visit( vtor );
   FC_ASSERT( vtor.count_left == 0 ); // unrecognized extension throws here
}

} } // eos::chain

namespace fc {

template< typename T >
struct eos_extension_from_variant_visitor
{
   eos_extension_from_variant_visitor( const variant_object& v, T& val )
      : vo( v ), value( val )
   {
      count_left = vo.size();
   }

   template<typename Member, class Class, Member (Class::*member)>
   void operator()( const char* name )const
   {
      auto it = vo.find(name);
      if( it != vo.end() )
      {
         from_variant( it->value(), (value.*member) );
         assert( count_left > 0 );    // x.find(k) returns true for n distinct values of k only if x.size() >= n
         --count_left;
      }
   }

   const variant_object& vo;
   T& value;
   mutable uint32_t count_left = 0;
};

template< typename T >
void from_variant( const fc::variant& var, eos::chain::extension<T>& value )
{
   value = eos::chain::extension<T>();
   if( var.is_null() )
      return;
   if( var.is_array() )
   {
      FC_ASSERT( var.size() == 0 );
      return;
   }

   eos_extension_from_variant_visitor<T> vtor( var.get_object(), value.value );
   fc::reflector<T>::visit( vtor );
   FC_ASSERT( vtor.count_left == 0 );    // unrecognized extension throws here
}

template< typename T >
struct eos_extension_to_variant_visitor
{
   eos_extension_to_variant_visitor( const T& v ) : value(v) {}

   template<typename Member, class Class, Member (Class::*member)>
   void operator()( const char* name )const
   {
      if( (value.*member).valid() )
         mvo[ name ] = (value.*member);
   }

   const T& value;
   mutable mutable_variant_object mvo;
};

template< typename T >
void to_variant( const eos::chain::extension<T>& value, fc::variant& var )
{
   eos_extension_to_variant_visitor<T> vtor( value.value );
   fc::reflector<T>::visit( vtor );
   var = vtor.mvo;
}

} // fc
