#pragma once
#include <fc/variant.hpp>
#include <fc/container/flat_fwd.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc {
   namespace raw {
       template<typename Stream, typename T>
       inline void pack( Stream& s, const flat_set<T>& value ) {
         FC_ASSERT( value.size() <= MAX_NUM_ARRAY_ELEMENTS );
         pack( s, unsigned_int((uint32_t)value.size()) );
         auto itr = value.begin();
         auto end = value.end();
         while( itr != end ) {
           fc::raw::pack( s, *itr );
           ++itr;
         }
       }
       template<typename Stream, typename T>
       inline void unpack( Stream& s, flat_set<T>& value ) {
         unsigned_int size; unpack( s, size );
         FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
         value.clear();
         value.reserve(size.value);
         for( uint32_t i = 0; i < size.value; ++i )
         {
             T tmp;
             fc::raw::unpack( s, tmp );
             value.insert( std::move(tmp) );
         }
       }
       template<typename Stream, typename K, typename... V>
       inline void pack( Stream& s, const flat_map<K,V...>& value ) {
         FC_ASSERT( value.size() <= MAX_NUM_ARRAY_ELEMENTS );
         pack( s, unsigned_int((uint32_t)value.size()) );
         auto itr = value.begin();
         auto end = value.end();
         while( itr != end ) {
           fc::raw::pack( s, *itr );
           ++itr;
         }
       }
       template<typename Stream, typename K, typename V, typename... A>
       inline void unpack( Stream& s, flat_map<K,V,A...>& value )
       {
         unsigned_int size; unpack( s, size );
         FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
         value.clear();
         value.reserve(size.value);
         for( uint32_t i = 0; i < size.value; ++i )
         {
             std::pair<K,V> tmp;
             fc::raw::unpack( s, tmp );
             value.insert( std::move(tmp) );
         }
       }

       template<typename Stream, typename T, typename A>
       void pack( Stream& s, const bip::vector<T,A>& value ) {
         FC_ASSERT( value.size() <= MAX_NUM_ARRAY_ELEMENTS );
         pack( s, unsigned_int((uint32_t)value.size()) );
         if( !std::is_fundamental<T>::value ) {
            auto itr = value.begin();
            auto end = value.end();
            while( itr != end ) {
              fc::raw::pack( s, *itr );
              ++itr;
            }
         } else {
             s.write( (const char*)value.data(), value.size() );
         }
       }

       template<typename Stream, typename T, typename A>
       void unpack( Stream& s, bip::vector<T,A>& value ) {
          unsigned_int size;
          unpack( s, size );
          FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
          value.resize( size );
          if( !std::is_fundamental<T>::value ) {
             for( auto& item : value )
                unpack( s, item );
          } else {
             s.read( (char*)value.data(), value.size() );
          }
       }

   } // namespace raw


   template<typename T>
   void to_variant( const flat_set<T>& var,  variant& vo )
   {
       std::vector<variant> vars(var.size());
       size_t i = 0;
       for( auto itr = var.begin(); itr != var.end(); ++itr, ++i )
          vars[i] = variant(*itr);
       vo = vars;
   }
   template<typename T>
   void from_variant( const variant& var,  flat_set<T>& vo )
   {
      const variants& vars = var.get_array();
      vo.clear();
      vo.reserve( vars.size() );
      for( auto itr = vars.begin(); itr != vars.end(); ++itr )
         vo.insert( itr->as<T>() );
   }

   template<typename K, typename... T>
   void to_variant( const flat_map<K, T...>& var,  variant& vo )
   {
       std::vector< variant > vars(var.size());
       size_t i = 0;
       for( auto itr = var.begin(); itr != var.end(); ++itr, ++i )
          vars[i] = fc::variant(*itr);
       vo = vars;
   }
   template<typename K, typename T, typename... A>
   void from_variant( const variant& var,  flat_map<K, T, A...>& vo )
   {
      const variants& vars = var.get_array();
      vo.clear();
      for( auto itr = vars.begin(); itr != vars.end(); ++itr )
         vo.insert( itr->as< std::pair<K,T> >() );

   }

}
