#pragma once
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>
#include <set>

namespace eosio { namespace chain { namespace backing_store {
struct unique_table {
   name contract;
   name scope;
   name table;
};

template<typename T>
struct secondary_key {
   int       table_ei;
   T         secondary;
   uint64_t  primary;
   account_name payer;
};

inline bool operator<(const unique_table& lhs, const unique_table& rhs) {
   return std::tie(lhs.contract, lhs.scope, lhs.table) < std::tie(rhs.contract, rhs.scope, rhs.table);
}

template<typename T>
bool operator<(const secondary_key<T>& lhs, const secondary_key<T>& rhs) {
   // checking primary second to optimize the search since a given primary key
   // cannot have more than one secondary key with the same value (but keeping
   // remainder of algorithm for consistency)
   return std::tie(lhs.table_ei, lhs.primary, lhs.secondary) < std::tie(rhs.table_ei, rhs.primary, rhs.secondary);
   // ignoring payer, it does not contribute to uniqueness
}

template<typename SecondaryKey>
class db_key_value_iter_store {
   public:
      using secondary_obj_type = secondary_key<SecondaryKey>;
      using secondary_key_type = SecondaryKey;

      db_key_value_iter_store(){
         _end_iterator_to_table.reserve(8);
         _iterator_to_object.reserve(32);
      }

      constexpr int invalid_iterator() const { return -1; }

      /// Returns end iterator of the table.
      int cache_table( const unique_table& tobj ) {
         auto itr = _table_cache.find(tobj);
         if( itr != _table_cache.end() )
            return itr->second;

         auto ei = index_to_end_iterator(_end_iterator_to_table.size());
         _end_iterator_to_table.push_back( tobj );
         _table_cache.emplace( tobj, ei );
         return ei;
      }

      int get_end_iterator_by_table( const unique_table& tobj )const {
         auto itr = _table_cache.find(tobj);
         EOS_ASSERT( itr != _table_cache.end(), table_not_in_cache, "an invariant was broken, table should be in cache" );
         return itr->second;
      }

      const unique_table* find_table_by_end_iterator( int ei )const {
         EOS_ASSERT( ei < invalid_iterator(), invalid_table_iterator, "not an end iterator" );
         auto indx = end_iterator_to_index(ei);
         if( indx >= _end_iterator_to_table.size() ) return nullptr;
         return &_end_iterator_to_table[indx];
      }

      const unique_table& get_table( const secondary_obj_type& obj )const {
         EOS_ASSERT( obj.table_ei < invalid_iterator(), invalid_table_iterator, "not an end iterator" );
         auto indx = end_iterator_to_index(obj.table_ei);
         EOS_ASSERT( indx < _end_iterator_to_table.size(), table_not_in_cache,
                     "an invariant was broken, secondary object references table that is not in cache" );
         return _end_iterator_to_table[indx];
      }

      const secondary_obj_type& get( int iterator ) const {
         validate_object_iterator(iterator, "dereference of end iterator");
         // grab a const ref, to ensure that we are returning a reference to the actual object in the vector
         const auto& result = _iterator_to_object[iterator];
         EOS_ASSERT( result, table_operation_not_permitted, "dereference of deleted object" );
         return *result;
      }

      void remove( int iterator ) {
         validate_object_iterator(iterator, "cannot call remove on end iterators");

         auto optional_obj = _iterator_to_object[iterator];
         if( !optional_obj ) return;
         _object_to_iterator.erase( *optional_obj );

         _iterator_to_object[iterator].reset();
      }

      void swap( int iterator, const secondary_key_type& secondary, account_name payer ) {
         validate_object_iterator(iterator, "cannot call swap on end iterators");

         auto& optional_obj = _iterator_to_object[iterator];
         if( !optional_obj ) return;
         // check to see if the object will actually change
         if(optional_obj->payer == payer && optional_obj->secondary == secondary) return;
         const bool map_key_change = optional_obj->secondary != secondary;
         if (map_key_change) {
            // edit object and swap out the map entry
            _object_to_iterator.erase( *optional_obj );
            optional_obj->secondary = secondary;
         }
         optional_obj->payer = payer;

         if (map_key_change) {
            _object_to_iterator.insert({ *optional_obj, iterator });
         }
      }

      int find( const secondary_obj_type& obj ) const {
         auto itr = _object_to_iterator.find( obj );
         if( itr != _object_to_iterator.end() )
            return itr->second;

         return invalid_iterator();
      }

      int add( const secondary_obj_type& obj ) {
         auto itr = find( obj );
         if( itr != invalid_iterator() )
              return itr;

         EOS_ASSERT( obj.table_ei < invalid_iterator(), invalid_table_iterator, "not an end iterator" );
         const auto indx = end_iterator_to_index(obj.table_ei);
         EOS_ASSERT( indx < _end_iterator_to_table.size(), invalid_table_iterator, "an invariant was broken, table should be in cache" );
         _object_to_iterator.insert({ obj, _iterator_to_object.size() });
         _iterator_to_object.emplace_back( obj );

         return _iterator_to_object.size() - 1;
      }

   private:
      void validate_object_iterator(int iterator, const char* explanation_for_no_end_iterators) const {
         EOS_ASSERT( iterator != invalid_iterator(), invalid_table_iterator, "invalid iterator" );
         EOS_ASSERT( iterator >= 0, table_operation_not_permitted, explanation_for_no_end_iterators );
         EOS_ASSERT( (size_t)iterator < _iterator_to_object.size(), invalid_table_iterator, "iterator out of range" );
      }

      map<unique_table, int>                    _table_cache;
      vector<unique_table>                      _end_iterator_to_table;
      vector<std::optional<secondary_obj_type>> _iterator_to_object;
      map<secondary_obj_type, int>              _object_to_iterator;

      /// Precondition: std::numeric_limits<int>::min() < ei < -1
      /// Iterator of -1 is reserved for invalid iterators (i.e. when the appropriate table has not yet been created).
      inline size_t end_iterator_to_index( int ei )const { return (-ei - 2); }
      /// Precondition: indx < _end_iterator_to_table.size() <= std::numeric_limits<int>::max()
      inline int index_to_end_iterator( size_t indx )const { return -(indx + 2); }
}; /// class db_key_value_iter_store

}}} // namespace eosio::chain::backing_store
