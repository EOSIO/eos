#pragma once

#include <fc/variant.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc {

   namespace raw {

      namespace detail {

         template<template<typename...> class Set, typename Stream, typename T, typename... U>
         inline void pack_set( Stream& s, const Set<T, U...>& value ) {
            FC_ASSERT( value.size() <= MAX_NUM_ARRAY_ELEMENTS );
            pack( s, unsigned_int((uint32_t)value.size()) );
            for( const auto& item : value ) {
               pack( s, item );
            }
         }

         template<template<typename...> class Set, typename Stream, typename T, typename... U>
         inline void unpack_set( Stream& s, Set<T, U...>& value ) {
            unsigned_int size; unpack( s, size );
            FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
            value.clear();
            for( uint32_t i = 0; i < size.value; ++i ) {
               auto tmp = ::fc::detail::construct_maybe_with_allocator<T>( value.get_allocator() );
               unpack( s, tmp );
               value.insert( value.end(), std::move(tmp) );
            }
         }

         template<template<typename...> class Set, typename Stream, typename T, typename... U>
         inline void unpack_flat_set( Stream& s, Set<T, U...>& value ) {
            unsigned_int size; unpack( s, size );
            FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
            value.clear();
            value.reserve( size.value );
            for( uint32_t i = 0; i < size.value; ++i ) {
               auto tmp = ::fc::detail::construct_maybe_with_allocator<T>( value.get_allocator() );
               unpack( s, tmp );
               value.insert( value.end(), std::move(tmp) );
            }
         }

         template<template<typename...> class Map, typename Stream, typename K, typename V, typename... U>
         inline void pack_map( Stream& s, const Map<K, V, U...>& value ) {
            FC_ASSERT( value.size() <= MAX_NUM_ARRAY_ELEMENTS );
            pack( s, unsigned_int((uint32_t)value.size()) );
            for( const auto& item : value ) {
               pack( s, item );
            }
         }

         template<template<typename...> class Map, typename Stream, typename K, typename V, typename... U>
         inline void unpack_map( Stream& s, Map<K, V, U...>& value ) {
            unsigned_int size; unpack( s, size );
            FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
            value.clear();
            for( uint32_t i = 0; i < size.value; ++i ) {
               auto tmp = ::fc::detail::default_construct_pair_maybe_with_allocator<K, V>( value.get_allocator() );
               unpack( s, tmp );
               value.insert( value.end(), std::move(tmp) );
            }
         }

         template<template<typename...> class Map, typename Stream, typename K, typename V, typename... U>
         inline void unpack_flat_map( Stream& s, Map<K, V, U...>& value ) {
            unsigned_int size; unpack( s, size );
            FC_ASSERT( size.value <= MAX_NUM_ARRAY_ELEMENTS );
            value.clear();
            value.reserve( size.value );
            for( uint32_t i = 0; i < size.value; ++i ) {
               auto tmp = ::fc::detail::default_construct_pair_maybe_with_allocator<K, V>( value.get_allocator() );
               unpack( s, tmp );
               value.insert( value.end(), std::move(tmp) );
            }
         }

      }

   }

   namespace detail {

      template<template<typename...> class Set, typename T, typename... U>
      void to_variant_from_set( const Set<T, U...>& s, fc::variant& vo ) {
         FC_ASSERT( s.size() <= MAX_NUM_ARRAY_ELEMENTS );
         variants vars;
         vars.reserve( s.size() );
         for( const auto& item : s ) {
            vars.emplace_back( item );
         }
         vo = std::move( vars );
      }

      template<template<typename...> class Set, typename T, typename... U>
      void from_variant_to_set( const fc::variant& v, Set< T, U... >& s ) {
         const variants& vars = v.get_array();
         FC_ASSERT( vars.size() <= MAX_NUM_ARRAY_ELEMENTS );
         s.clear();
         for( const auto& var : vars ) {
            auto item = construct_maybe_with_allocator<T>( s.get_allocator() );
            var.as<T>( item );
            s.insert( s.end(), std::move(item) );
         }
      }

      template<template<typename...> class Set, typename T, typename... U>
      void from_variant_to_flat_set( const fc::variant& v, Set< T, U... >& s ) {
         const variants& vars = v.get_array();
         FC_ASSERT( vars.size() <= MAX_NUM_ARRAY_ELEMENTS );
         s.clear();
         s.reserve( vars.size() );
         for( const auto& var : vars ) {
            auto item = construct_maybe_with_allocator<T>( s.get_allocator() );
            var.as<T>( item );
            s.insert( s.end(), std::move(item) );
         }
      }

      template<template<typename...> class Map, typename K, typename V, typename... U >
      void to_variant_from_map( const Map< K, V, U... >& m, fc::variant& vo ) {
         FC_ASSERT( m.size() <= MAX_NUM_ARRAY_ELEMENTS );
         variants vars;
         vars.reserve( m.size() );
         for( const auto& item : m ) {
            vars.emplace_back( item );
         }
         vo = std::move( vars );
      }

      template<template<typename...> class Map, typename K, typename V, typename... U>
      void from_variant_to_map( const variant& v, Map<K, V, U...>& m ) {
         const variants& vars = v.get_array();
         FC_ASSERT( vars.size() <= MAX_NUM_ARRAY_ELEMENTS );
         m.clear();
         for( const auto& var : vars ) {
            auto item = default_construct_pair_maybe_with_allocator<K, V>( m.get_allocator() );
            var.as< std::pair<K,V> >( item );
            m.insert( m.end(), std::move(item) );
         }
      }

      template<template<typename...> class Map, typename K, typename V, typename... U>
      void from_variant_to_flat_map( const variant& v, Map<K, V, U...>& m ) {
         const variants& vars = v.get_array();
         FC_ASSERT( vars.size() <= MAX_NUM_ARRAY_ELEMENTS );
         m.clear();
         m.reserve( vars.size() );
         for( const auto& var : vars ) {
            auto item = default_construct_pair_maybe_with_allocator<K, V>( m.get_allocator() );
            var.as< std::pair<K,V> >( item );
            m.insert( m.end(), std::move(item) );
         }
      }

   }

}
