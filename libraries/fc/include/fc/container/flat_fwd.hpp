#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/vector.hpp>

#include <boost/interprocess/containers/vector.hpp> // only added to maintain backwards compatibility with existing consumers of fc (can eventually remove)

namespace fc {

   using boost::container::flat_set;
   using boost::container::flat_multiset;
   using boost::container::flat_map;
   using boost::container::flat_multimap;

   namespace raw {

      template<typename Stream, typename T, typename... U>
      void pack( Stream& s, const boost::container::vector<T, U...>& value );
      template<typename Stream, typename T, typename... U>
      void unpack( Stream& s, boost::container::vector<T, U...>& value );

      template<typename Stream, typename T, typename... U>
      void pack( Stream& s, const flat_set<T, U...>& value );
      template<typename Stream, typename T, typename... U>
      void unpack( Stream& s, flat_set<T, U...>& value );

      template<typename Stream, typename T, typename... U>
      void pack( Stream& s, const flat_multiset<T, U...>& value );
      template<typename Stream, typename T, typename... U>
      void unpack( Stream& s, flat_multiset<T, U...>& value );

      template<typename Stream, typename K, typename V, typename... U>
      void pack( Stream& s, const flat_map<K, V, U...>& value );
      template<typename Stream, typename K, typename V, typename... U>
      void unpack( Stream& s, flat_map<K, V, U...>& value ) ;

      template<typename Stream, typename K, typename V, typename... U>
      void pack( Stream& s, const flat_multimap<K, V, U...>& value );
      template<typename Stream, typename K, typename V, typename... U>
      void unpack( Stream& s, flat_multimap<K, V, U...>& value ) ;

   } // namespace raw

} // fc
