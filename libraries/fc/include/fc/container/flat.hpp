#pragma once

#include <fc/container/flat_fwd.hpp>
#include <fc/container/container_detail.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <fc/crypto/hex.hpp>

namespace fc {

   namespace raw {

      template<typename Stream, typename T, typename A>
      void pack( Stream& s, const boost::container::vector<T, A>& value ) {
         FC_ASSERT( value.size() <= MAX_NUM_ARRAY_ELEMENTS );
         pack( s, unsigned_int((uint32_t)value.size()) );
         if( !std::is_fundamental<T>::value ) {
            for( const auto& item : value ) {
               pack( s, item );
            }
         } else if( value.size() ) {
            s.write( (const char*)value.data(), value.size() );
         }
      }

      template<typename Stream, typename T, typename A>
      void unpack( Stream& s, boost::container::vector<T, A>& value ) {
         unsigned_int size;
         unpack( s, size );
         FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
         value.clear();
         value.resize( size.value );
         if( !std::is_fundamental<T>::value ) {
            for( auto& item : value ) {
               unpack( s, item );
            }
         } else if( value.size() ) {
            s.read( (char*)value.data(), value.size() );
         }
      }

      template<typename Stream, typename A>
      void pack( Stream& s, const boost::container::vector<char, A>& value ) {
         FC_ASSERT( value.size() <= MAX_SIZE_OF_BYTE_ARRAYS );
         pack( s, unsigned_int((uint32_t)value.size()) );
         if( value.size() )
            s.write( (const char*)value.data(), value.size() );
      }

      template<typename Stream, typename A>
      void unpack( Stream& s, boost::container::vector<char, A>& value ) {
         unsigned_int size;
         unpack( s, size );
         FC_ASSERT( size.value <= MAX_SIZE_OF_BYTE_ARRAYS );
         value.clear();
         value.resize( size.value );
         if( value.size() )
            s.read( (char*)value.data(), value.size() );
      }

      template<typename Stream, typename T, typename... U>
      void pack( Stream& s, const flat_set<T, U...>& value ) {
         detail::pack_set( s, value );
      }

      template<typename Stream, typename T, typename... U>
      void unpack( Stream& s, flat_set<T, U...>& value ) {
         detail::unpack_flat_set( s, value );
      }

      template<typename Stream, typename T, typename... U>
      void pack( Stream& s, const flat_multiset<T, U...>& value ) {
         detail::pack_set( s, value );
      }

      template<typename Stream, typename T, typename... U>
      void unpack( Stream& s, flat_multiset<T, U...>& value ) {
         detail::unpack_flat_set( s, value );
      }

      template<typename Stream, typename K, typename V, typename... U>
      void pack( Stream& s, const flat_map<K, V, U...>& value ) {
         detail::pack_map( s, value );
      }

      template<typename Stream, typename K, typename V, typename... U>
      void unpack( Stream& s, flat_map<K, V, U...>& value ) {
         detail::unpack_flat_map( s, value );
      }

      template<typename Stream, typename K, typename V, typename... U>
      void pack( Stream& s, const flat_multimap<K, V, U...>& value ) {
         detail::pack_map( s, value );
      }

      template<typename Stream, typename K, typename V, typename... U>
      void unpack( Stream& s, flat_multimap<K, V, U...>& value ) {
         detail::unpack_flat_map( s, value );
      }

   } // namespace raw

   template<typename T, typename... U>
   void to_variant( const boost::container::vector<T, U...>& vec, fc::variant& vo ) {
      FC_ASSERT( vec.size() <= MAX_NUM_ARRAY_ELEMENTS );
      variants vars;
      vars.reserve( vec.size() );
      for( const auto& item : vec ) {
         vars.emplace_back( item );
      }
      vo = std::move(vars);
   }

   template<typename T, typename... U>
   void from_variant( const fc::variant& v, boost::container::vector<T, U...>& vec ) {
      const variants& vars = v.get_array();
      FC_ASSERT( vars.size() <= MAX_NUM_ARRAY_ELEMENTS );
      vec.clear();
      vec.resize( vars.size() );
      for( uint32_t i = 0; i < vars.size(); ++i ) {
         from_variant( vars[i], vec[i] );
      }
   }

   template<typename... U>
   void to_variant( const boost::container::vector<char, U...>& vec, fc::variant& vo ) {
      FC_ASSERT( vec.size() <= MAX_SIZE_OF_BYTE_ARRAYS );
      if( vec.size() )
         vo = variant( fc::to_hex( vec.data(), vec.size() ) );
      else
         vo = "";
   }

   template<typename... U>
   void from_variant( const fc::variant& v, boost::container::vector<char, U...>& vec )
   {
      const auto& str = v.get_string();
      FC_ASSERT( str.size() <= 2*MAX_SIZE_OF_BYTE_ARRAYS ); // Doubled because hex strings needs two characters per byte
      vec.resize( str.size() / 2 );
      if( vec.size() ) {
         size_t r = fc::from_hex( str, vec.data(), vec.size() );
         FC_ASSERT( r == vec.size() );
      }
   }

   template<typename T, typename... U>
   void to_variant( const flat_set< T, U... >& s, fc::variant& vo ) {
      detail::to_variant_from_set( s, vo );
   }

   template<typename T, typename... U>
   void from_variant( const fc::variant& v, flat_set< T, U... >& s ) {
      detail::from_variant_to_flat_set( v, s );
   }

   template<typename T, typename... U>
   void to_variant( const flat_multiset< T, U... >& s, fc::variant& vo ) {
      detail::to_variant_from_set( s, vo );
   }

   template<typename T, typename... U>
   void from_variant( const fc::variant& v, flat_multiset< T, U... >& s ) {
      detail::from_variant_to_flat_set( v, s );
   }

   template<typename K, typename V, typename... U >
   void to_variant( const flat_map< K, V, U... >& m, fc::variant& vo ) {
      detail::to_variant_from_map( m, vo );
   }

   template<typename K, typename V, typename... U>
   void from_variant( const variant& v,  flat_map<K, V, U...>& m ) {
      detail::from_variant_to_flat_map( v, m );
   }

   template<typename K, typename V, typename... U >
   void to_variant( const flat_multimap< K, V, U... >& m, fc::variant& vo ) {
      detail::to_variant_from_map( m, vo );
   }

   template<typename K, typename V, typename... U>
   void from_variant( const variant& v,  flat_multimap<K, V, U...>& m ) {
      detail::from_variant_to_flat_map( v, m );
   }

}
