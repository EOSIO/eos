#pragma once
/**
 * @brief Used to forward declare raw functions.
 */
#include <eoslib/types.hpp>
#include <eoslib/varint.hpp>

namespace eosio { namespace raw {    
   template<typename Stream, typename Arg0, typename... Args> inline void pack( Stream& s, const Arg0& a0, Args... args );
   template<typename Stream, typename T> inline void pack( Stream& s, const T& v );
   template<typename Stream, typename T> inline void unpack( Stream& s, T& v );
   template<typename Stream> inline void pack( Stream& s, const signed_int& v );
   template<typename Stream> inline void pack( Stream& s, const unsigned_int& v );
   template<typename Stream> inline void unpack( Stream& s, signed_int& vi );
   template<typename Stream> inline void unpack( Stream& s, unsigned_int& vi );
   template<typename Stream> inline void pack( Stream& s, const bytes& value );
   template<typename Stream> inline void unpack( Stream& s, bytes& value );
   template<typename Stream> inline void pack( Stream& s, const public_key& value );
   template<typename Stream> inline void unpack( Stream& s, public_key& value );
   template<typename Stream> inline void pack( Stream& s, const string& v );
   template<typename Stream> inline void unpack( Stream& s, string& v);
   template<typename Stream> inline void pack( Stream& s, const fixed_string32& v );
   template<typename Stream> inline void unpack( Stream& s, fixed_string32& v);
   template<typename Stream> inline void pack( Stream& s, const fixed_string16& v );
   template<typename Stream> inline void unpack( Stream& s, fixed_string16& v);
   template<typename Stream> inline void pack( Stream& s, const price& v );
   template<typename Stream> inline void unpack( Stream& s, price& v);
   template<typename Stream> inline void pack( Stream& s, const asset& v );
   template<typename Stream> inline void unpack( Stream& s, asset& v);
   template<typename Stream> inline void pack( Stream& s, const bool& v );
   template<typename Stream> inline void unpack( Stream& s, bool& v );
   template<typename T> inline size_t pack_size( const T& v );
   template<typename T> inline bytes pack(  const T& v );
   template<typename T> inline void pack( char* d, uint32_t s, const T& v );
   template<typename T> inline T unpack( const char* d, uint32_t s );
   template<typename T> inline void unpack( const char* d, uint32_t s, T& v );
} } // namespace eosio::raw

