#pragma once
#include <tuple>
#include <boost/hana.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/db.h>




namespace eosio {

namespace bmi = boost::multi_index;
using boost::multi_index::const_mem_fun;

template<typename T>
struct secondary_iterator;

template<>
struct secondary_iterator<uint64_t> {
   static int db_idx_next( int iterator, uint64_t* primary ) { return db_idx64_next( iterator, primary ); }
   static int db_idx_prev( int iterator, uint64_t* primary ) { return db_idx64_previous( iterator, primary ); }
   static void db_idx_remove( int iterator  )                 { db_idx64_remove( iterator ); }
};

template<>
struct secondary_iterator<uint128_t> {
   static int db_idx_next( int iterator, uint64_t* primary ) { return db_idx128_next( iterator, primary ); }
   static int db_idx_prev( int iterator, uint64_t* primary ) { return db_idx128_previous( iterator, primary ); }
   static void db_idx_remove( int iterator  )                 { db_idx128_remove( iterator ); }
};

void db_idx_update( int iterator, uint64_t payer, const uint64_t& secondary ) {
   db_idx64_update( iterator, payer, &secondary );
}
void db_idx_update( int iterator, uint64_t payer, const uint128_t& secondary ) {
   db_idx128_update( iterator, payer, &secondary );
}

int db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary, uint64_t primary ) { 
   return db_idx64_find_primary( code, scope, table, &secondary, primary );
}
int db_idx_find_secondary( uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary, uint64_t& primary ) { 
   return db_idx64_find_secondary( code, scope, table, &secondary, &primary );
}
int db_idx_lowerbound( uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary, uint64_t& primary ) { 
   return db_idx64_lowerbound( code, scope, table, &secondary, &primary );
}
int db_idx_upperbound( uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary, uint64_t& primary ) { 
   return db_idx64_lowerbound( code, scope, table, &secondary, &primary );
}


int db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t primary ) { 
   return db_idx128_find_primary( code, scope, table, &secondary, primary );
}
int db_idx_find_secondary( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) { 
   return db_idx128_find_secondary( code, scope, table, &secondary, &primary );
}
int db_idx_lowerbound( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) { 
   return db_idx128_lowerbound( code, scope, table, &secondary, &primary );
}
int db_idx_upperbound( uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary, uint64_t& primary ) { 
   return db_idx128_lowerbound( code, scope, table, &secondary, &primary );
}



template<uint64_t TableName, typename T, typename... Indicies>
class multi_index;



template<int IndexNumber, uint64_t IndexName, typename T, typename Extractor>
struct index_by {
   //typedef  typename std::decay<decltype( (Extractor())( *((const T*)(nullptr)) ) )>::type value_type;
   typedef  Extractor                        extractor_secondary_type;
   typedef  decltype( Extractor()(nullptr) ) secondary_type;

   index_by(){}

   static const int index_number = IndexNumber;
   static const uint64_t index_name   = IndexName;

   private:
      template<uint64_t, typename, typename... >
      friend class multi_index;

      Extractor extract_secondary_key;

      int store( uint64_t scope, uint64_t payer, const T& obj ) {
         // fetch primary key and secondary key here..
         return -1;
      }
      void update( int iterator, uint64_t payer, const secondary_type& secondary ) {
         db_idx_update( iterator, payer, secondary );
      }

      int find_primary( uint64_t code, uint64_t scope, uint64_t primary, secondary_type& secondary )const {
         return db_idx_find_primary( code, scope, IndexName,  secondary, primary );
      }

      void remove( int itr ) {
         secondary_iterator<secondary_type>::db_idx_remove( itr );
      }

      int find_secondary( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary )const {
         return db_idx_find_secondary( code, scope, IndexName, secondary, primary );
      }

      int lower_bound( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary )const {
         return db_idx_lowerbound( code, scope, IndexName, secondary, primary );
      }
      int upper_bound( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary )const {
         return db_idx_upperbound( code, scope, IndexName, secondary, primary );
      }
};


/*
template<int IndexNumber, uint64_t IndexName, typename T, typename Extractor>
auto make_index_by( Extractor&& l ) {
   return index_by<IndexNumber, IndexName, T, Extractor>{ std::forward<Extractor>(l) };
}
*/


template<uint64_t TableName, typename T, typename... Indicies>
class multi_index 
{
   private:

      struct item : public T 
      {
         template<typename Constructor>
         item( const multi_index& idx, Constructor&& c )
         :__idx(idx){
            c(*this);
         }

         const multi_index& __idx;
         int                __primary_itr;
         int                __iters[sizeof...(Indicies)];
      };

      uint64_t _code;
      uint64_t _scope;

      boost::hana::tuple<Indicies...>   _indicies;


      struct by_primary_key;
      struct by_primary_itr;

      mutable boost::multi_index_container< item, 
         bmi::indexed_by< 
            bmi::ordered_unique< bmi::tag<by_primary_key>, bmi::const_mem_fun<T, uint64_t, &T::primary_key> >,
            bmi::ordered_unique< bmi::tag<by_primary_itr>, bmi::member<item, int, &item::__primary_itr> >
         >
      > _items_index;

//      mutable std::map<uint64_t, item*> _items; /// by_primary_key
//      mutable std::map<int, item*>      _items_by_itr;

      const item& load_object_by_primary_iterator( int itr )const {
         const auto& by_pitr = _items_index.template get<by_primary_itr>();
         auto cacheitr = by_pitr.find( itr );
         if( cacheitr != by_pitr.end() )
            return *cacheitr;

         auto size = db_get_i64( itr, nullptr, 0 );
         char tmp[size];
         db_get_i64( itr, tmp, size );

         datastream<const char*> ds(tmp,size);

         auto result = _items_index.emplace( *this, [&]( auto& i ) {
            T& val = static_cast<T&>(i);
            ds >> val;

            i.__primary_itr = itr;
            boost::hana::for_each( _indicies, [&]( auto& idx ) {
               i.__iters[idx.index_number] = -1;
            });
         });

         // eosio_assert( result.second, "failed to insert object, likely a uniqueness constraint was violated" );

         return *result.first;
      } /// load_object_by_iterator

      friend struct const_iterator;

   public:

      multi_index( uint64_t code, uint64_t scope ):_code(code),_scope(scope){}
      ~multi_index() {
      }

      template<typename MultiIndexType, typename IndexType>
      struct index {
            typedef typename MultiIndexType::item item_type;
            typedef typename IndexType::secondary_type secondary_key_type;
         public:

            struct const_iterator {
               private:

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
                     uint64_t next_pk = 0;
                     auto next_itr = secondary_iterator<secondary_key_type>::db_idx_next( _item->__iters[IndexType::index_number], &next_pk );
                     if( next_itr == -1 ) {
                        _item = nullptr;
                        return *this;
                     }

                     const T& obj = *_idx._multidx.find( next_pk );
                     auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
                     mi.__iters[IndexType::index_number] = next_itr;
                     _item = &mi;

                     return *this;
                  }

                  const_iterator& operator--() {
                     uint64_t prev_pk = 0;
                     auto prev_itr = secondary_iterator<secondary_key_type>::db_idx_prev( _item->__iters[IndexType::index_number], &prev_pk );
                     if( prev_itr == -1 ) {
                        _item = nullptr;
                        return *this;
                     }

                     const T& obj = *_idx._multidx.find( prev_pk );
                     auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
                     mi.__iters[IndexType::index_number] = prev_itr;
                     _item = &mi;

                     return *this;
                  }

                  const T& operator*()const { return *static_cast<const T*>(_item); }
                  const T* operator->()const { return *static_cast<const T*>(_item); }

               private:
                  friend class index;
                  const_iterator( const index& idx, const typename MultiIndexType::item* i = nullptr )
                  :_idx(idx), _item(i){}

                  const index& _idx;
                  const typename MultiIndexType::item* _item;
            };

            const_iterator end()const { return const_iterator( *this ); }
            const_iterator begin()const { return const_iterator( *this, nullptr ); }
            const_iterator lower_bound( typename IndexType::secondary_type&& secondary ) {
               return lower_bound( secondary );
            }
            const_iterator lower_bound( typename IndexType::secondary_type& secondary ) {
               uint64_t primary = 0;
               auto itr = _idx.lower_bound( _multidx._code, _multidx._scope, secondary, primary );
               if( itr == -1 ) return end();

               const T& obj = *_multidx.find( primary );
               auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
               mi.__iters[IndexType::index_number] = itr;

               return const_iterator( *this, &mi );
            }


         private:
            friend class multi_index;
            index( const MultiIndexType& midx, const IndexType& idx )
            :_multidx(midx),_idx(idx){}

            const MultiIndexType _multidx;
            const IndexType&     _idx;
      };


      struct const_iterator {
         friend bool operator == ( const const_iterator& a, const const_iterator& b ) {
            return a._item == b._item;
         }
         friend bool operator != ( const const_iterator& a, const const_iterator& b ) {
            return a._item != b._item;
         }

         const T& operator*()const { return *static_cast<const T*>(_item); }

         const_iterator operator++(int)const {
            return ++(const_iterator(*this));
         }
         const_iterator operator--(int)const {
            return --(const_iterator(*this));
         }

         const_iterator& operator++() {
            //eosio_assert( _item, "null ptr" );
            uint64_t pk;
            auto next_itr = db_next_i64( _item->__primary_itr, &pk );
            _item = &_multidx.load_object_by_primary_iterator( next_itr );
            return *this;
         }
         const_iterator& operator--() {
            //eosio_assert( _item, "null ptr" );
            uint64_t pk;
            auto next_itr = db_previous_i64( _item->__primary_itr, &pk );
            _item = &_multidx.load_object_by_primary_iterator( next_itr );
            return *this;
         }

         private:
            const_iterator( const multi_index& mi, const item* i = nullptr )
            :_multidx(mi),_item(i){}

            const multi_index& _multidx;
            const item*  _item;
            friend class multi_index;
      };

      const_iterator end()const   { return const_iterator( *this ); }
      const_iterator begin()const { return lower_bound();     }

      const_iterator lower_bound( uint64_t primary = 0 )const { 
         auto itr = db_lowerbound_i64( _code, _scope, TableName, primary );
         auto& obj = load_object_by_primary_iterator( itr );
         return const_iterator( *this, &obj );
      }

      const_iterator upper_bound( uint64_t primary = 0 )const { 
         auto itr = db_upperbound_i64( _code, _scope, TableName, primary );
         auto& obj = load_object_by_primary_iterator( itr );
         return const_iterator( *this, &obj );
      }

      void new_id( uint64_t payer )const {
        uint64_t val = 1;
        auto itr = db_find_i64( _code, _scope, TableName+1, 0 );
        if( itr != -1 ) {
           auto s = db_get_i64( itr, (char*)&val, sizeof(val) );
           ++val;
           db_update_i64( itr, 0, (const char*)&val, sizeof(val) );
        }
        else {
           db_store_i64( _scope, TableName+1, payer, 0, (char*)&val, sizeof(val) );
        }
      }
      template<uint64_t IndexName>
      auto get_index()const  {
        const auto& idx = boost::hana::find_if( _indicies, []( auto x ){ 
                                     return std::integral_constant<bool,(decltype(x)::index_name == IndexName)>(); } ).value();
        return index<multi_index, typename std::decay<decltype(idx)>::type>( *this, idx );
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

             boost::hana::for_each( _indicies, [&]( auto& idx ) {
                i.__iters[idx.index_number] = idx.store( _scope, payer, obj );
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

         auto secondary_keys = boost::hana::transform( _indicies, [&]( auto& idx ) {
             return idx.extract_secondary_key( obj );
         });

         auto mutableobj = const_cast<T&>(obj);
         updater( mutableobj );

         char tmp[ pack_size( obj ) ];
         datastream<char*> ds( tmp, sizeof(tmp) );
         ds << obj;

         auto pk = obj.primary_key();
         db_update_i64( objitem.__primary_itr, payer, tmp, sizeof(tmp) );

         boost::hana::for_each( _indicies, [&]( auto& idx ) {
            auto secondary = idx.extract_secondary_key( mutableobj );
            if( boost::hana::at_c<std::decay<decltype(idx)>::type::index_number>(secondary_keys) != secondary ) {
               auto indexitr = mutableitem.__iters[idx.index_number];

               if( indexitr == -1 )
                  indexitr = mutableitem.__iters[idx.index_number] = idx.find_primary( _code, _scope, pk, secondary );

               idx.update( indexitr, payer, secondary );
            }
         });
      }

      const T* find( uint64_t primary )const {
         auto cacheitr = _items_index.find(primary);
         if( cacheitr != _items_index.end() ) 
            return &*cacheitr;

         int itr = db_find_i64( _code, _scope, TableName, primary );
         if( itr == -1 ) return nullptr;

         const item& i = load_object_by_primary_iterator( itr );
         return &static_cast<const T&>(i);
      }

      void remove( const T& obj ) {
         const auto& objitem = static_cast<const item&>(obj);
         auto& mutableitem = const_cast<item&>(objitem);
         // eosio_assert( &objitem.__idx == this, "invalid object" );

         db_remove_i64( objitem.__primary_itr );

         boost::hana::for_each( _indicies, [&]( auto& idx ) {
            auto i = objitem.__iters[idx.index_number];
            if( i == -1 ) {
              typename std::decay<decltype(idx)>::type::secondary_type second;
              i = idx.find_primary( _code, _scope, objitem.primary_key(), second );
            }
            if( i != -1 )
              idx.remove( i );
         });
         
         auto cacheitr = _items_index.find(obj.primary_key());
         if( cacheitr != _items_index.end() )  {
            _items_index.erase(cacheitr);
         }
      }

};

}  /// eosio 

