#pragma once
#include <tuple>
#include <boost/hana.hpp>
#include <functional>
#include <type_traits>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/db.h>
#include <eosiolib/fixed_key.hpp>

namespace eosio {

namespace bmi = boost::multi_index;
using boost::multi_index::const_mem_fun;

template<typename T>
struct secondary_iterator;

#define WRAP_SECONDARY_SIMPLE_TYPE(IDX, TYPE)\
template<>\
struct secondary_iterator<TYPE> {\
   static int db_idx_next( int iterator, uint64_t* primary ) { return db_##IDX##_next( iterator, primary ); }\
   static int db_idx_previous( int iterator, uint64_t* primary ) { return db_##IDX##_previous( iterator, primary ); }\
   static void db_idx_remove( int iterator  )                { db_##IDX##_remove( iterator ); }\
   static int db_idx_end( uint64_t code, uint64_t scope, uint64_t table ) { return db_##IDX##_end( code, scope, table ); }\
};\
int db_idx_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
   return db_##IDX##_store( scope, table, payer, id, &secondary );\
}\
void db_idx_update( int iterator, uint64_t payer, const TYPE& secondary ) {\
   db_##IDX##_update( iterator, payer, &secondary );\
}\
int db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary, TYPE& secondary ) {\
   return db_##IDX##_find_primary( code, scope, table, &secondary, primary );\
}\
int db_idx_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
   return db_##IDX##_find_secondary( code, scope, table, &secondary, &primary );\
}\
int db_idx_lowerbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
   return db_##IDX##_lowerbound( code, scope, table, &secondary, &primary );\
}\
int db_idx_upperbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
   return db_##IDX##_upperbound( code, scope, table, &secondary, &primary );\
}

#define WRAP_SECONDARY_ARRAY_TYPE(IDX, TYPE)\
template<>\
struct secondary_iterator<TYPE> {\
   static int db_idx_next( int iterator, uint64_t* primary ) { return db_##IDX##_next( iterator, primary ); }\
   static int db_idx_previous( int iterator, uint64_t* primary ) { return db_##IDX##_previous( iterator, primary ); }\
   static void db_idx_remove( int iterator  )                { db_##IDX##_remove( iterator ); }\
   static int db_idx_end( uint64_t code, uint64_t scope, uint64_t table ) { return db_##IDX##_end( code, scope, table ); }\
};\
int db_idx_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
   return db_##IDX##_store( scope, table, payer, id, secondary.data(), TYPE::num_words() );\
}\
void db_idx_update( int iterator, uint64_t payer, const TYPE& secondary ) {\
   db_##IDX##_update( iterator, payer, secondary.data(), TYPE::num_words() );\
}\
int db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary, TYPE& secondary ) {\
   return db_##IDX##_find_primary( code, scope, table, secondary.data(), TYPE::num_words(), primary );\
}\
int db_idx_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
   return db_##IDX##_find_secondary( code, scope, table, secondary.data(), TYPE::num_words(), &primary );\
}\
int db_idx_lowerbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
   return db_##IDX##_lowerbound( code, scope, table, secondary.data(), TYPE::num_words(), &primary );\
}\
int db_idx_upperbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
   return db_##IDX##_upperbound( code, scope, table, secondary.data(), TYPE::num_words(), &primary );\
}

WRAP_SECONDARY_SIMPLE_TYPE(idx64,  uint64_t)
WRAP_SECONDARY_SIMPLE_TYPE(idx128, uint128_t)
WRAP_SECONDARY_ARRAY_TYPE(idx256, key256)


template<uint64_t TableName, typename T, typename... Indices>
class multi_index;

template<uint64_t IndexName, typename Extractor>
struct indexed_by {
   enum constants { index_name   = IndexName };
   typedef  Extractor                        secondary_extractor_type;
   typedef  decltype( Extractor()(nullptr) ) secondary_type;
};


template<uint64_t TableName, uint64_t IndexName, typename T, typename Extractor, int N = 0>
struct index_by {
   typedef  Extractor  extractor_secondary_type;
   typedef  Extractor  secondary_extractor_type;
   typedef  typename std::decay<decltype( Extractor()(nullptr) )>::type secondary_type;

   index_by(){}

   enum constants {
      table_name   = TableName,
      index_name   = IndexName,
      index_number = N,
      index_table_name = (TableName & 0xFFFFFFFFFFFFFFF0ULL) | (N & 0x000000000000000FULL) // Assuming no more than 16 secondary indices are allowed
   };

   constexpr static int number()    { return N; }
   constexpr static uint64_t name() {
      // return IndexName;
      return index_table_name;
   }

   private:
      template<uint64_t, typename, typename... >
      friend class multi_index;

      static auto extract_secondary_key(const T& obj) { return extractor_secondary_type()(obj); }

      static int store( uint64_t scope, uint64_t payer, const T& obj ) {
         return db_idx_store( scope, name(), payer, obj.primary_key(), extract_secondary_key(obj) );
      }

      static void update( int iterator, uint64_t payer, const secondary_type& secondary ) {
         db_idx_update( iterator, payer, secondary );
      }

      static int find_primary( uint64_t code, uint64_t scope, uint64_t primary, secondary_type& secondary ) {
         return db_idx_find_primary( code, scope, name(), primary,  secondary );
      }

      static void remove( int itr ) {
         secondary_iterator<secondary_type>::db_idx_remove( itr );
      }

      static int find_secondary( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary ) {
         return db_idx_find_secondary( code, scope, name(), secondary, primary );
      }

      static int lower_bound( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary ) {
         return db_idx_lowerbound( code, scope, name(), secondary, primary );
      }

      static int upper_bound( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary ) {
         return db_idx_upperbound( code, scope, name(), secondary, primary );
      }
};


namespace hana = boost::hana;

template<uint64_t TableName, typename T, typename... Indices>
class multi_index
{
   private:

      static_assert( sizeof...(Indices) <= 16, "multi_index only supports a maximum of 16 secondary indices" );

      constexpr static bool validate_table_name(uint64_t n) {
         // Limit table names to 12 characters so that the last character (4 bits) can be used to distinguish between the secondary indices.
         return (n & 0x000000000000000FULL) == 0;
      }

      static_assert( validate_table_name(TableName), "multi_index does not support table names with a length greater than 12");

      struct item : public T
      {
         template<typename Constructor>
         item( const multi_index& idx, Constructor&& c )
         :__idx(idx){
            c(*this);
         }

         const multi_index& __idx;
         int                __primary_itr;
         int                __iters[sizeof...(Indices)+(sizeof...(Indices)==0)];
      };

      uint64_t _code;
      uint64_t _scope;

      mutable uint64_t _next_primary_key;

      enum next_primary_key_tags : uint64_t {
         no_available_primary_key = static_cast<uint64_t>(-2), // Must be the smallest uint64_t value compared to all other tags
         unset_next_primary_key = static_cast<uint64_t>(-1)
      };

      template<uint64_t I>
      struct intc { enum e{ value = I }; operator uint64_t()const{ return I; }  };

      static constexpr auto transform_indices( ) {
         typedef decltype( hana::zip_shortest(
                             hana::make_tuple( intc<0>(), intc<1>(), intc<2>(), intc<3>(), intc<4>(), intc<5>(),
                                               intc<6>(), intc<7>(), intc<8>(), intc<9>(), intc<10>(), intc<11>(),
                                               intc<12>(), intc<13>(), intc<14>(), intc<15>() ),
                             hana::tuple<Indices...>() ) ) indices_input_type;

         return hana::transform( indices_input_type(), [&]( auto&& idx ){
             typedef typename std::decay<decltype(hana::at_c<0>(idx))>::type num_type;
             typedef typename std::decay<decltype(hana::at_c<1>(idx))>::type idx_type;
             return index_by<TableName,
                             idx_type::index_name,
                             T,
                             typename idx_type::secondary_extractor_type,
                             num_type::e::value>();

         });
      }

      typedef decltype( multi_index::transform_indices() ) indices_type;

      indices_type _indices;

      struct by_primary_key;
      struct by_primary_itr;

      mutable boost::multi_index_container< item,
         bmi::indexed_by<
            bmi::ordered_unique< bmi::tag<by_primary_key>, bmi::const_mem_fun<T, uint64_t, &T::primary_key> >,
            bmi::ordered_unique< bmi::tag<by_primary_itr>, bmi::member<item, int, &item::__primary_itr> >
         >
      > _items_index;

      const item& load_object_by_primary_iterator( int itr )const {
         const auto& by_pitr = _items_index.template get<by_primary_itr>();
         auto cacheitr = by_pitr.find( itr );
         if( cacheitr != by_pitr.end() )
            return *cacheitr;

         auto size = db_get_i64( itr, nullptr, 0 );
         eosio_assert( size >= 0, "error reading iterator" );
         char tmp[size];
         db_get_i64( itr, tmp, uint32_t(size) );

         datastream<const char*> ds(tmp,uint32_t(size));

         auto result = _items_index.emplace( *this, [&]( auto& i ) {
            T& val = static_cast<T&>(i);
            ds >> val;

            i.__primary_itr = itr;
            boost::hana::for_each( _indices, [&]( auto& idx ) {
               i.__iters[ idx.number() ] = -1;
            });
         });

         // eosio_assert( result.second, "failed to insert object, likely a uniqueness constraint was violated" );

         return *result.first;
      } /// load_object_by_iterator

      friend struct const_iterator;

   public:

      multi_index( uint64_t code, uint64_t scope )
      :_code(code),_scope(scope),_next_primary_key(unset_next_primary_key)
      {}

      uint64_t get_code()const { return _code; }
      uint64_t get_scope()const { return _scope; }

      template<typename MultiIndexType, typename IndexType, uint64_t Number>
      struct index {
         private:
            typedef typename MultiIndexType::item item_type;
            typedef typename IndexType::secondary_type secondary_key_type;

         public:
            typedef typename IndexType::secondary_extractor_type  secondary_extractor_type;
            static constexpr uint64_t name() { return IndexType::name(); }

            struct const_iterator {
               public:
                  friend bool operator == ( const const_iterator& a, const const_iterator& b ) {
                     return a._item == b._item;
                  }
                  friend bool operator != ( const const_iterator& a, const const_iterator& b ) {
                     return a._item != b._item;
                  }

                  const_iterator operator++(int)const {
                     return ++(const_iterator(*this));
                  }

                  const_iterator operator--(int)const {
                     return --(const_iterator(*this));
                  }

                  const_iterator& operator++() {
                     eosio_assert( _item != nullptr, "cannot increment end iterator" );
                     const auto& idx = _idx.get();

                     if( _item->__iters[Number] == -1 ) {
                        secondary_key_type temp_secondary_key;
                        auto idxitr = db_idx_find_primary(idx.get_code(),
                                                          idx.get_scope(),
                                                          idx.name(),
                                                          _item->primary_key(), temp_secondary_key);
                        auto& mi = const_cast<item_type&>( *_item );
                        mi.__iters[Number] = idxitr;
                     }

                     uint64_t next_pk = 0;
                     auto next_itr = secondary_iterator<secondary_key_type>::db_idx_next( _item->__iters[Number], &next_pk );
                     if( next_itr < 0 ) {
                        _item = nullptr;
                        return *this;
                     }

                     const T& obj = *idx._multidx.find( next_pk );
                     auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
                     mi.__iters[Number] = next_itr;
                     _item = &mi;

                     return *this;
                  }

                  const_iterator& operator--() {
                     uint64_t prev_pk = 0;
                     int prev_itr = -1;
                     const auto& idx = _idx.get();

                     if( !_item ) {
                        auto ei = secondary_iterator<secondary_key_type>::db_idx_end(idx.get_code(), idx.get_scope(), idx.name());
                        eosio_assert( ei != -1, "cannot decrement end iterator when the index is empty" );
                        prev_itr = secondary_iterator<secondary_key_type>::db_idx_previous( ei , &prev_pk );
                        eosio_assert( prev_itr != -1, "cannot decrement end iterator when the index is empty" );
                     } else {
                        if( _item->__iters[Number] == -1 ) {
                           secondary_key_type temp_secondary_key;
                           auto idxitr = db_idx_find_primary(idx.get_code(),
                                                             idx.get_scope(),
                                                             idx.name(),
                                                             _item->primary_key(), temp_secondary_key);
                           auto& mi = const_cast<item_type&>( *_item );
                           mi.__iters[Number] = idxitr;
                        }
                        prev_itr = secondary_iterator<secondary_key_type>::db_idx_previous( _item->__iters[Number], &prev_pk );
                        eosio_assert( prev_itr >= 0, "cannot decrement iterator at beginning of index" );
                     }

                     const T& obj = *idx._multidx.find( prev_pk );
                     auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
                     mi.__iters[Number] = prev_itr;
                     _item = &mi;

                     return *this;
                  }

                  const T& operator*()const { return *static_cast<const T*>(_item); }
                  const T* operator->()const { return static_cast<const T*>(_item); }

               private:
                  friend struct index;
                  const_iterator( const index& idx, const typename MultiIndexType::item* i = nullptr )
                  :_idx(std::cref(idx)), _item(i){}

                  std::reference_wrapper<const index> _idx;
                  const typename MultiIndexType::item* _item;
            };

            const_iterator end()const { return const_iterator( *this ); }
            const_iterator begin()const {
               return lower_bound(typename IndexType::secondary_type());
            }
            const_iterator find( typename IndexType::secondary_type&& secondary )const {
               auto lb = lower_bound( secondary );
               auto e = end();
               if( lb == e ) return e;

               if( secondary != typename IndexType::extractor_secondary_type()(*lb) )
                  return e;
               return lb;
            }

            const_iterator lower_bound( typename IndexType::secondary_type&& secondary )const {
               return lower_bound( secondary );
            }
            const_iterator lower_bound( const typename IndexType::secondary_type& secondary )const {
               uint64_t primary = 0;
               typename IndexType::secondary_type secondary_copy(secondary);
               auto itr = IndexType::lower_bound( get_code(), get_scope(), secondary_copy, primary );
               if( itr < 0 ) return end();

               const T& obj = *_multidx.find( primary );
               auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
               mi.__iters[Number] = itr;

               return const_iterator( *this, &mi );
            }
            const_iterator upper_bound( typename IndexType::secondary_type&& secondary )const {
               return upper_bound( secondary );
            }
            const_iterator upper_bound( const typename IndexType::secondary_type& secondary )const {
               uint64_t primary = 0;
               typename IndexType::secondary_type secondary_copy(secondary);
               auto itr = IndexType::upper_bound( get_code(), get_scope(), secondary_copy, primary );
               if( itr < 0 ) return end();

               const T& obj = *_multidx.find( primary );
               auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
               mi.__iters[Number] = itr;

               return const_iterator( *this, &mi );
            }

            uint64_t get_code()const  { return _multidx.get_code(); }
            uint64_t get_scope()const { return _multidx.get_scope(); }

         private:
            friend class multi_index;
            index( const MultiIndexType& midx ) //, const IndexType& idx )
            :_multidx(midx){}

            const MultiIndexType& _multidx;
      };


      struct const_iterator {
         friend bool operator == ( const const_iterator& a, const const_iterator& b ) {
            return a._item == b._item;
         }
         friend bool operator != ( const const_iterator& a, const const_iterator& b ) {
            return a._item != b._item;
         }

         const T& operator*()const { return *static_cast<const T*>(_item); }
         const T* operator->()const { return static_cast<const T*>(_item); }

         const_iterator operator++(int)const {
            return ++(const_iterator(*this));
         }
         const_iterator operator--(int)const {
            return --(const_iterator(*this));
         }

         const_iterator& operator++() {
            eosio_assert( _item != nullptr, "cannot increment end iterator" );
            const auto& multidx = _multidx.get();

            uint64_t next_pk;
            auto next_itr = db_next_i64( _item->__primary_itr, &next_pk );
            if( next_itr < 0 )
               _item = nullptr;
            else
               _item = &multidx.load_object_by_primary_iterator( next_itr );
            return *this;
         }
         const_iterator& operator--() {
            uint64_t prev_pk;
            int prev_itr = -1;
            const auto& multidx = _multidx.get();

            if( !_item ) {
               auto ei = db_end_i64(multidx.get_code(), multidx.get_scope(), TableName);
               eosio_assert( ei != -1, "cannot decrement end iterator when the table is empty" );
               prev_itr = db_previous_i64( ei , &prev_pk );
               eosio_assert( prev_itr != -1, "cannot decrement end iterator when the table is empty" );
            } else {
               prev_itr = db_previous_i64( _item->__primary_itr, &prev_pk );
               eosio_assert( prev_itr >= 0, "cannot decrement iterator at beginning of table" );
            }

            _item = &multidx.load_object_by_primary_iterator( prev_itr );
            return *this;
         }

         private:
            const_iterator( const multi_index& mi, const item* i = nullptr )
            :_multidx(std::cref(mi)),_item(i){}

            std::reference_wrapper<const multi_index> _multidx;
            const item*  _item;
            friend class multi_index;
      };

      const_iterator end()const   { return const_iterator( *this ); }
      const_iterator begin()const { return lower_bound();     }


      const_iterator lower_bound( uint64_t primary = 0 )const { 
         auto itr = db_lowerbound_i64( _code, _scope, TableName, primary );
         if( itr < 0 ) return end();
         auto& obj = load_object_by_primary_iterator( itr );
         return const_iterator( *this, &obj );
      }

      const_iterator upper_bound( uint64_t primary = 0 )const {
         auto itr = db_upperbound_i64( _code, _scope, TableName, primary );
         if( itr < 0 ) return end();
         auto& obj = load_object_by_primary_iterator( itr );
         return const_iterator( *this, &obj );
      }

      /** Ideally this method would only be used to determine the appropriate primary key to use within new objects added to a
       *  table in which the primary keys of the table are strictly intended from the beginning to be autoincrementing and
       *  thus will not ever be set to custom arbitrary values by the contract.
       *  Violating this agreement could result in the table appearing full when in reality there is plenty of space left.
       */
      uint64_t available_primary_key()const {
         if( _next_primary_key == unset_next_primary_key ) {
            // This is the first time available_primary_key() is called for this multi_index instance.
            if( begin() == end() ) { // empty table
               _next_primary_key = 0;
            } else {
               auto itr = --end(); // last row of table sorted by primary key
               auto pk = itr->primary_key(); // largest primary key currently in table
               if( pk >= no_available_primary_key ) // Reserve the tags
                  _next_primary_key = no_available_primary_key;
               else
                  _next_primary_key = pk + 1;
            }
         }

         eosio_assert( _next_primary_key < no_available_primary_key, "next primary key in table is at autoincrement limit");
         return _next_primary_key;
      }

      template<uint64_t IndexName>
      auto get_index()const  {
        auto idx = boost::hana::find_if( _indices, []( auto&& in ) {
            return std::integral_constant<bool, std::decay<decltype(in)>::type::index_name == IndexName>();
        }).value();

        return index<multi_index, decltype(idx), idx.number()>( *this );
      }

      template<typename Lambda>
      const T& emplace( uint64_t payer, Lambda&& constructor ) {
         auto  result  = _items_index.emplace( *this, [&]( auto& i ){
            T& obj = static_cast<T&>(i);
            constructor( obj );

            char tmp[ pack_size( obj ) ];
            datastream<char*> ds( tmp, sizeof(tmp) );
            ds << obj;

            auto pk = obj.primary_key();

            i.__primary_itr = db_store_i64( _scope, TableName, payer, pk, tmp, sizeof(tmp) );

            if( pk >= _next_primary_key )
               _next_primary_key = (pk >= no_available_primary_key) ? no_available_primary_key : (pk + 1);

            boost::hana::for_each( _indices, [&]( auto& idx ) {
               i.__iters[idx.number()] = idx.store( _scope, payer, obj );
            });
         });

         /// eosio_assert( result.second, "could not insert object, uniqueness constraint in cache violated" )
         return static_cast<const T&>(*result.first);
      }

      template<typename Lambda>
      void update( const T& obj, uint64_t payer, Lambda&& updater ) {
         const auto& objitem = static_cast<const item&>(obj);
         auto& mutableitem = const_cast<item&>(objitem);

         // eosio_assert( &objitem.__idx == this, "invalid object" );

         auto secondary_keys = boost::hana::transform( _indices, [&]( auto&& idx ) {
             return idx.extract_secondary_key( obj );
         });

         auto& mutableobj = const_cast<T&>(obj); // Do not forget the auto& otherwise it would make a copy and thus not update at all.
         updater( mutableobj );

         char tmp[ pack_size( obj ) ];
         datastream<char*> ds( tmp, sizeof(tmp) );
         ds << obj;

         auto pk = obj.primary_key();

         db_update_i64( objitem.__primary_itr, payer, tmp, sizeof(tmp) );

         if( pk >= _next_primary_key )
            _next_primary_key = (pk >= no_available_primary_key) ? no_available_primary_key : (pk + 1);

         boost::hana::for_each( _indices, [&]( auto& idx ) {
            typedef typename std::decay<decltype(idx)>::type index_type;

            auto secondary = idx.extract_secondary_key( obj );
            if( hana::at_c<index_type::index_number>(secondary_keys) != secondary ) {
               auto indexitr = mutableitem.__iters[idx.number()];

               if( indexitr < 0 )
                  indexitr = mutableitem.__iters[idx.number()] = idx.find_primary( _code, _scope, pk, secondary );

               idx.update( indexitr, payer, secondary );
            }
         });
      }

      const T& get( uint64_t primary )const {
         auto result = find( primary );
         eosio_assert( result != nullptr, "unable to find key" );
         return *result;
      }

      const T* find( uint64_t primary )const {
         auto cacheitr = _items_index.find(primary);
         if( cacheitr != _items_index.end() )
            return &*cacheitr;

         int itr = db_find_i64( _code, _scope, TableName, primary );
         if( itr < 0 ) return nullptr;

         const item& i = load_object_by_primary_iterator( itr );
         return &static_cast<const T&>(i);
      }

      void remove( const T& obj ) {
         const auto& objitem = static_cast<const item&>(obj);
         //auto& mutableitem = const_cast<item&>(objitem);
         // eosio_assert( &objitem.__idx == this, "invalid object" );

         db_remove_i64( objitem.__primary_itr );

         boost::hana::for_each( _indices, [&]( auto& idx ) {
            auto i = objitem.__iters[idx.number()];
            if( i < 0 ) {
              typename std::decay<decltype(idx)>::type::secondary_type second;
              i = idx.find_primary( _code, _scope, objitem.primary_key(), second );
            }
            if( i >= 0 )
              idx.remove( i );
         });

         auto cacheitr = _items_index.find(obj.primary_key());
         if( cacheitr != _items_index.end() )  {
            _items_index.erase(cacheitr);
         }
      }

};

}  /// eosio
