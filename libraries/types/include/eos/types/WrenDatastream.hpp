/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/io/datastream.hpp>

namespace eosio { namespace types {
   using fc::datastream;

   struct InputDatastream {
      datastream<const char*>  ds;
      vector<char>             value;

      InputDatastream( const std::string& data )
      :ds(nullptr,0) {
         if( data.size() ) {
            value.resize(data.size());
            memcpy( value.data(), data.c_str(), data.size() );
            ds = fc::datastream<const char*>( value.data(), value.size() );
         }
      }

      std::string read( uint32_t bytes ) {
         FC_ASSERT( bytes <= 1024*1024 );
         vector<char> result; result.resize(bytes);
         ds.read( result.data(), bytes );
         return string(result.data(),bytes);
      }

      void read( char* data, uint32_t len ) {
         ds.read( data, len );
      }

      uint32_t remaining()const { return ds.remaining(); }
   };

   struct OutputDatastream {
      vector<char>       value;
      datastream<char*>   ds;

      OutputDatastream( uint64_t bytes )
      :ds(nullptr,0) {
         FC_ASSERT( bytes <= 1024*1024 );
         if( bytes ) {
            value.resize(bytes);
            ds = fc::datastream<char*>( value.data(), value.size() );
         }
      }

      void write( const char* d, uint32_t size ) {
         ds.write( d, size );
      }

      void write( const std::string& data, uint32_t size ) {
         FC_ASSERT( size <= data.size() );
         ds.write( data.c_str(), size );
      }

      std::string data()const { 
         if( value.size() )
            return std::string( value.data(), value.size() ); 
         else 
            return std::string();
      }
      uint32_t tellp()const     { return ds.tellp();     }
      uint32_t remaining()const { return ds.remaining(); }
   };

   struct SizeDatastream {
      datastream<size_t> ds;
      SizeDatastream(){}

      void write( const char* d, uint32_t size ) {
         ds.write( d, size );
      }

      void write( const std::string& data, uint32_t size ) {
         ds.write( nullptr, size );
      }

      uint32_t tellp()const { return ds.tellp(); }
   };

}} // namespace eosio::types
