#pragma once
#include <eosio/chain/contract_table_objects.hpp>
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>
#include <set>

namespace eosio { namespace chain { namespace backing_store {

template<typename T>
class db_chainbase_iter_store {
   public:
      db_chainbase_iter_store(){
         _end_iterator_to_table.reserve(8);
         _iterator_to_object.reserve(32);
      }

      /// Returns end iterator of the table.
      int cache_table( const table_id_object& tobj ) {
         auto itr = _table_cache.find(tobj.id);
         if( itr != _table_cache.end() )
            return itr->second.second;

         auto ei = index_to_end_iterator(_end_iterator_to_table.size());
         _end_iterator_to_table.push_back( &tobj );
         _table_cache.emplace( tobj.id, make_pair(&tobj, ei) );
         return ei;
      }

      const table_id_object& get_table( table_id_object::id_type i )const {
         auto itr = _table_cache.find(i);
         EOS_ASSERT( itr != _table_cache.end(), table_not_in_cache, "an invariant was broken, table should be in cache" );
         return *itr->second.first;
      }

      int get_end_iterator_by_table_id( table_id_object::id_type i )const {
         auto itr = _table_cache.find(i);
         EOS_ASSERT( itr != _table_cache.end(), table_not_in_cache, "an invariant was broken, table should be in cache" );
         return itr->second.second;
      }

      const table_id_object* find_table_by_end_iterator( int ei )const {
         EOS_ASSERT( ei < -1, invalid_table_iterator, "not an end iterator" );
         auto indx = end_iterator_to_index(ei);
         if( indx >= _end_iterator_to_table.size() ) return nullptr;
         return _end_iterator_to_table[indx];
      }

      const T& get( int iterator ) {
         EOS_ASSERT( iterator != -1, invalid_table_iterator, "invalid iterator" );
         EOS_ASSERT( iterator >= 0, table_operation_not_permitted, "dereference of end iterator" );
         EOS_ASSERT( (size_t)iterator < _iterator_to_object.size(), invalid_table_iterator, "iterator out of range" );
         auto result = _iterator_to_object[iterator];
         EOS_ASSERT( result, table_operation_not_permitted, "dereference of deleted object" );
         return *result;
      }

      void remove( int iterator ) {
         EOS_ASSERT( iterator != -1, invalid_table_iterator, "invalid iterator" );
         EOS_ASSERT( iterator >= 0, table_operation_not_permitted, "cannot call remove on end iterators" );
         EOS_ASSERT( (size_t)iterator < _iterator_to_object.size(), invalid_table_iterator, "iterator out of range" );

         auto obj_ptr = _iterator_to_object[iterator];
         if( !obj_ptr ) return;
         _iterator_to_object[iterator] = nullptr;
         _object_to_iterator.erase( obj_ptr );
      }

      int add( const T& obj ) {
         auto itr = _object_to_iterator.find( &obj );
         if( itr != _object_to_iterator.end() )
              return itr->second;

         _iterator_to_object.push_back( &obj );
         _object_to_iterator[&obj] = _iterator_to_object.size() - 1;

         return _iterator_to_object.size() - 1;
      }

   private:
      map<table_id_object::id_type, pair<const table_id_object*, int>> _table_cache;
      vector<const table_id_object*>                                   _end_iterator_to_table;
      vector<const T*>                                                 _iterator_to_object;
      map<const T*,int>                                                _object_to_iterator;

      /// Precondition: std::numeric_limits<int>::min() < ei < -1
      /// Iterator of -1 is reserved for invalid iterators (i.e. when the appropriate table has not yet been created).
      inline size_t end_iterator_to_index( int ei )const { return (-ei - 2); }
      /// Precondition: indx < _end_iterator_to_table.size() <= std::numeric_limits<int>::max()
      inline int index_to_end_iterator( size_t indx )const { return -(indx + 2); }
}; /// class db_chainbase_iter_store

}}} // namespace eosio::chain::backing_store
