#pragma once
/**
 * @brief Used to forward declare raw functions.
 */
#include <eoslib/types.hpp>
#include <eoslib/varint.hpp>

namespace eosio { namespace raw {    
   template<typename Stream, typename Arg0, typename... Args>  void pack( Stream& s, Arg0&& a0, Args&&... args );
   template<typename Stream, typename T>  void pack( Stream& s, T&& v );
   template<typename Stream, typename T>  void unpack( Stream& s, T& v );
   template<typename Stream>  void pack( Stream& s, signed_int v );
   template<typename Stream>  void pack( Stream& s, unsigned_int v );
   template<typename Stream>  void unpack( Stream& s, signed_int& vi );
   template<typename Stream>  void unpack( Stream& s, unsigned_int& vi );
   template<typename Stream>  void pack( Stream& s, const bytes& value );
   template<typename Stream>  void unpack( Stream& s, bytes& value );
   template<typename Stream>  void pack( Stream& s, const public_key& value );
   template<typename Stream>  void unpack( Stream& s, public_key& value );
   template<typename Stream>  void pack( Stream& s, const string& v );
   template<typename Stream>  void unpack( Stream& s, string& v);
   template<typename Stream>  void pack( Stream& s, const fixed_string32& v );
   template<typename Stream>  void unpack( Stream& s, fixed_string32& v);
   template<typename Stream>  void pack( Stream& s, const fixed_string16& v );
   template<typename Stream>  void unpack( Stream& s, fixed_string16& v);
   template<typename Stream>  void pack( Stream& s, const price& v );
   template<typename Stream>  void unpack( Stream& s, price& v);
   template<typename Stream>  void pack( Stream& s, const asset& v );
   template<typename Stream>  void unpack( Stream& s, asset& v);
   template<typename Stream>  void pack( Stream& s, const bool& v );
   template<typename Stream>  void unpack( Stream& s, bool& v );
   template<typename T> size_t pack_size( T&& v );
   template<typename T> bytes pack(  T&& v );
   template<typename T> void pack( char* d, uint32_t s, T&& v );
   template<typename T> T unpack( const char* d, uint32_t s );
   template<typename T>  void unpack( const char* d, uint32_t s, T& v );
} } // namespace eosio::raw

