#pragma once
#include <eoslib/reflect.hpp>
#include <eoslib/datastream.hpp>

namespace eosio {


   template<typename DataStream>
   void pack( DataStream& ds, uint64_t data ) {
      ds.write( (const char*)&data, sizeof(data) );
   }

   template<typename DataStream>
   void unpack( DataStream& ds, uint64_t data ) {
      ds.read( (char*)&data, sizeof(data) );
   }

   template<typename DataStream, typename T>
   void pack( DataStream& ds, const T& t ) {
      reflector<T>::visit( t, [&]( const auto& field ) {
         pack( ds, field );
      });
   }

   template<typename DataStream, typename T>
   void unpack( DataStream& ds, const T& t ) {
      reflector<T>::visit( t, [&]( auto& field ) {
         unpack( ds, field );
      });
   }

   template<typename T>
   T unpack( const char* data, uint32_t size )
   {
      datastream<const char*> ds( data, size );
      T tmp;
      unpack( ds, tmp );
      return tmp;
   }



   template<typename T>
   uint32_t pack_size( const T& v ) {
      datastream<size_t> ds;
      pack( ds, v );
      return ds.tellp();
   }
}
