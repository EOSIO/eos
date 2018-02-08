#include <map>
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


using namespace eosio;
namespace bmi = boost::multi_index;


extern "C" {
   int db_store_i64( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, char* buffer, size_t buffer_size );
   int db_update_i64( int iterator, uint64_t payer, char* buffer, size_t buffer_size );
   int db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id );
   int db_get_i64( int iterator, char* buffer, size_t buffer_size );
   int db_remove_i64( int iterator );
}

template<uint64_t TableName, typename T, typename... Indicies>
class multi_index;


template<int IndexNumber, uint64_t IndexName, typename T, typename Extractor>
struct index_by {
   //typedef  typename std::decay<decltype( (Extractor())( *((const T*)(nullptr)) ) )>::type value_type;
   typedef  Extractor extractor_secondary_type;

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
      template<typename Secondary>
      void update( int iterator, uint64_t payer, const Secondary& secondary ) {
         // fetch primary key and secondary key here..
      }

      int find_primary( uint64_t primarykey )const {
         return 0;
      }

      void remove( int itr ) {
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

      mutable std::map<uint64_t, item*> _items; /// by_primary_key
      mutable std::map<int, item*>      _items_by_itr;

      const item& load_object_by_primary_iterator( int itr )const {
         auto cacheitr = _items_by_itr.find( itr );
         if( cacheitr != _items_by_itr.end() )
            return *cacheitr->second;

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
         for( const auto& item : _items ) delete item.second;
      }

      struct const_iterator {
         friend bool operator == ( const const_iterator& a, const const_iterator& b ) {
            return a._item == b._item;
         }
         friend bool operator != ( const const_iterator& a, const const_iterator& b ) {
            return a._item != b._item;
         }

         const T& operator*()const { return *static_cast<const T*>(_item); }

         const_iterator& operator++() {
            //eosio_assert( _item, "null ptr" );
            auto next_itr = db_next_i64( _item->__primary_itr );
            _item = _multidx.load_object_by_iterator( next_itr );
            return *this;
         }

         private:
            const_iterator( const multi_index& mi, const item* i = nullptr )
            :_multidx(mi),_item(i){}

            const multi_index& _multidx;
            const item*  _item;
      };

      const_iterator end()const   { return iterator( *this ); }
      const_iterator begin()const { return lower_bound();     }

      const_iterator lower_bound( uint64_t primary = 0 )const { 
         return end(); //iterator( *this, lower_bound()  ); 
      }

      template<uint64_t IndexName>
      const auto& get_index()const  {
        return boost::hana::find_if( _indicies, []( auto x ){ return std::integral_constant<bool,(decltype(x)::index_name == IndexName)>(); } ).value();
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
         boost::hana::at_c<0>(secondary_keys);

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
                  indexitr = mutableitem.__iters[idx.index_number] = idx.find_primary( pk );

               idx.update( indexitr, payer, secondary );
            }
         });
      }

      const T* find( uint64_t primary )const {
         auto cacheitr = _items.find(primary);
         if( cacheitr != _items.end() ) 
            return cacheitr->second;

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
              i = idx.find_primary( objitem.primary_key() );
            }
            if( i != -1 )
              idx.remove( i );
         });
         
         auto cacheitr = _items.find(obj.primary_key());
         if( cacheitr != _items.end() )  {
            delete cacheitr->second;
            _items.erase(cacheitr);
         }
      }

};

/*
template<typename T, uint64_t Table, typename PrimaryLambda
auto make_multi_index( PrimaryLambda


using limit_order_index_type = decltype(make_multi_index<N(orders), limit_order>( 
                                     N(id), [](auto& o) { return o.id; }
                                     N(expiration), []( auto& o ){ return o.expiration; },
                                     N(price), [](auto& o){ return o.price; },
                                   ));
*/



struct limit_order {
   uint64_t     id;
   uint128_t    price;
   uint64_t     expiration;
   account_name owner;

   auto primary_key()const { return id; }
   uint64_t get_expiration()const { return expiration; }

   EOSLIB_SERIALIZE( limit_order, (id)(price)(expiration)(owner) )
};

auto by_expiration = []( const limit_order& order ) { return order.expiration; };
typedef decltype(by_expiration) by_expiration_type;

using boost::multi_index::const_mem_fun;

extern "C" {
   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t code, uint64_t action ) {
      multi_index<N(orders), limit_order,
         index_by<0, N(byexp), limit_order, const_mem_fun<limit_order, uint64_t, &limit_order::get_expiration> > 
         > orders( N(exchange), N(exchange) );

      auto payer = code;
      const auto& order = orders.emplace( payer, [&]( auto& o ) {
        o.id = 1;
        o.expiration = 3;
        o.owner = N(dan);
      });

      orders.update( order, payer, [&]( auto& o ) {
      });

      const auto* o = orders.find( 1 );

      orders.remove( order );

      orders.get_index<N(byexp)>();

   }
}
