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
   static int db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary, uint64_t& secondary ) {
      return db_idx64_find_primary( code, scope, table, &secondary, primary );
   }
};

template<>
struct secondary_iterator<uint128_t> {
   static int db_idx_next( int iterator, uint64_t* primary ) { return db_idx128_next( iterator, primary ); }
   static int db_idx_prev( int iterator, uint64_t* primary ) { return db_idx128_previous( iterator, primary ); }
   static void db_idx_remove( int iterator  )                 { db_idx128_remove( iterator ); }
   static int db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, 
                                   uint64_t primary, uint128_t& secondary ) {
      return db_idx128_find_primary( code, scope, table, &secondary, primary );
   }
};

int db_idx_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t primary, const uint64_t& secondary ) {
   return db_idx64_store( scope, table, payer, primary, &secondary );
}
int db_idx_store(  uint64_t scope, uint64_t table, uint64_t payer, uint64_t primary, const uint128_t& secondary ) {
   return db_idx128_store( scope, table, payer, primary, &secondary );
}

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

template<uint64_t IndexName, typename Extractor>
struct indexed_by {
   enum constants { index_name   = IndexName };
   typedef  Extractor                        secondary_extractor_type;
   typedef  decltype( Extractor()(nullptr) ) secondary_type;
};


template<uint64_t IndexName, typename T, typename Extractor, int N = 0>
struct index_by {
   typedef  Extractor                                                    extractor_secondary_type;
   typedef  typename std::decay<decltype( Extractor()(nullptr) )>::type secondary_type;

   index_by(){}

   enum constants {
      index_name   = IndexName,
      index_number = N
   };

   constexpr static int number()    { return N; }
   constexpr static uint64_t name() { return IndexName; }

   private:
      template<uint64_t, typename, typename... >
      friend class multi_index;

      static auto extract_secondary_key(const T& obj) { return extractor_secondary_type()(obj); }

      static int store( uint64_t scope, uint64_t payer, const T& obj ) {
         return db_idx_store( scope, IndexName, payer, obj.primary_key(), extract_secondary_key(obj) );
      }

      static void update( int iterator, uint64_t payer, const secondary_type& secondary ) {
         db_idx_update( iterator, payer, secondary );
      }

      static int find_primary( uint64_t code, uint64_t scope, uint64_t primary, secondary_type& secondary ) {
         return db_idx_find_primary( code, scope, IndexName,  secondary, primary );
      }

      static void remove( int itr ) {
         secondary_iterator<secondary_type>::db_idx_remove( itr );
      }

      static int find_secondary( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary ) {
         return db_idx_find_secondary( code, scope, IndexName, secondary, primary );
      }

      static int lower_bound( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary ) {
         return db_idx_lowerbound( code, scope, IndexName, secondary, primary );
      }
      static int upper_bound( uint64_t code, uint64_t scope, secondary_type& secondary, uint64_t& primary ) {
         return db_idx_upperbound( code, scope, IndexName, secondary, primary );
      }
};





namespace hana = boost::hana;

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

      template<uint64_t I>
      struct intc { enum e{ value = I }; operator uint64_t()const{ return I; }  };

      static constexpr auto transform_indicies( ) {
         typedef decltype( hana::zip_shortest( 
                             hana::make_tuple( intc<0>(), intc<1>(), intc<2>(), intc<3>(), intc<4>(), intc<5>() ), 
                             hana::tuple<Indicies...>() ) ) indicies_input_type;

         return hana::transform( indicies_input_type(), [&]( auto&& idx ){
             typedef typename std::decay<decltype(hana::at_c<0>(idx))>::type num_type;
             typedef typename std::decay<decltype(hana::at_c<1>(idx))>::type idx_type;
             return index_by<idx_type::index_name,
                             T,
                             typename idx_type::secondary_extractor_type,
                             num_type::e::value>();

         });
      }

      typedef decltype( multi_index::transform_indicies() ) indicies_type;

      indicies_type _indicies;

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
         char tmp[size];
         db_get_i64( itr, tmp, size );

         datastream<const char*> ds(tmp,size);

         auto result = _items_index.emplace( *this, [&]( auto& i ) {
            T& val = static_cast<T&>(i);
            ds >> val;

            i.__primary_itr = itr;
            boost::hana::for_each( _indicies, [&]( auto& idx ) {
               i.__iters[ idx.number() ] = -1;
            });
         });

         // eosio_assert( result.second, "failed to insert object, likely a uniqueness constraint was violated" );

         return *result.first;
      } /// load_object_by_iterator

      friend struct const_iterator;

   public:

      multi_index( uint64_t code, uint64_t scope ):_code(code),_scope(scope){}
      ~multi_index() { }

      uint64_t get_code()const { return _code; }
      uint64_t get_scope()const { return _scope; }

      template<typename MultiIndexType, typename IndexType, uint64_t Number>
      struct index {
         private:
            typedef typename MultiIndexType::item item_type;
            typedef typename IndexType::secondary_type secondary_key_type;

         public:
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
                     if( !_item ) return *this;
                     if( _item->__iters[Number] == -1 ) {
                        /// TODO: lookup iter for this item in this index
                        secondary_key_type temp_secondary_key;
                        auto idxitr = secondary_iterator<secondary_key_type>::db_idx_find_primary( 
                                                           _idx.get_code(), 
                                                           _idx.get_scope(), 
                                                           _idx.name(), 
                                                           _item->primary_key(), temp_secondary_key);
                     }


                     uint64_t next_pk = 0;
                     auto next_itr = secondary_iterator<secondary_key_type>::db_idx_next( _item->__iters[Number], &next_pk );
                     if( next_itr == -1 ) {
                        _item = nullptr;
                        return *this;
                     }

                     const T& obj = *_idx._multidx.find( next_pk );
                     auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
                     mi.__iters[Number] = next_itr;
                     _item = &mi;

                     return *this;
                  }

                  const_iterator& operator--() {
                     if( !_item ) {

                     }
                     uint64_t prev_pk = 0;
                     auto prev_itr = secondary_iterator<secondary_key_type>::db_idx_prev( _item->__iters[Number], &prev_pk );
                     if( prev_itr == -1 ) {
                        _item = nullptr;
                        return *this;
                     }

                     const T& obj = *_idx._multidx.find( prev_pk );
                     auto& mi = const_cast<item_type&>( static_cast<const item_type&>(obj) );
                     mi.__iters[Number] = prev_itr;
                     _item = &mi;

                     return *this;
                  }

                  const T& operator*()const { return *static_cast<const T*>(_item); }
                  const T* operator->()const { return *static_cast<const T*>(_item); }

               private:
                  friend struct index;
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
               auto itr = IndexType::lower_bound( _multidx._code, _multidx._scope, secondary, primary );
               if( itr == -1 ) return end();

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

            const MultiIndexType _multidx;
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
            if( next_itr == -1 )
               _item = nullptr;
            else
               _item = &_multidx.load_object_by_primary_iterator( next_itr );
            return *this;
         }
         const_iterator& operator--() {
            //eosio_assert( _item, "null ptr" );
            uint64_t pk;
            auto next_itr = db_previous_i64( _item->__primary_itr, &pk );
            if( next_itr == -1 ) 
               _item = nullptr;
            else
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
        auto idx = boost::hana::find_if( _indicies, []( auto&& in ){ 
                                                /*
            auto& x = hana::at_c<1>(idxp);
            return std::integral_constant<bool,(std::decay<decltype(x)>::type::index_name == IndexName)>(); 
            */
            return std::integral_constant<bool, std::decay<decltype(in)>::type::index_name == IndexName>();
        } ).value();

        return index<multi_index, decltype(idx), idx.number()>( *this );

        /*
        typedef typename std::decay<decltype(hana::at_c<0>(idx))>::type num_type;
        
        return index<multi_index, typename std::decay<decltype(hana::at_c<1>(idx))>::type, num_type::value >( *this, hana::at_c<1>(idx) );
        */
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

         auto secondary_keys = boost::hana::transform( _indicies, [&]( auto&& idx ) {
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
            typedef typename std::decay<decltype(idx)>::type index_type;

            auto secondary = idx.extract_secondary_key( mutableobj );
            if( hana::at_c<index_type::index_number>(secondary_keys) != secondary ) {
               auto indexitr = mutableitem.__iters[idx.number()];

               if( indexitr == -1 )
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
            auto i = objitem.__iters[idx.number()];
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

