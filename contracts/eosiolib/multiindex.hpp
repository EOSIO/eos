
extern "C" {
   /**
    *
    * @return an ID that serves as an iterator to the object stored, -1 for error
    */
   int db_store_i64( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, char* buffer, size_t buffer_size );
   int db_update_i64( int iterator, char* buffer, size_t buffer_size );

   /**
    * max_buffer_size should start out with the space reserved for buffer, but is set to the actual size of buffer
    *
    * if max_buffer_size is greater than actual buffer size, then buffer will be filled with contents, otherwise not
    * 
    * @return an ID that serves as an iterator to the object stored, -1 for error/not found
    */
   int db_find_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id, char* buffer, size_t* max_buffer_size );
   int db_lower_bound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id, char* buffer, size_t* max_buffer_size );
   int db_upper_bound_i64( uint64_t code, uint64_t scope, uint64_t table, uint64_t id, char* buffer, size_t* max_buffer_size );

   /** return an iterator to the next item after @param iterator and optionally fetch data if buffer is not null */
   int db_next( int iterator, uint64_t* id, char* buffer, size_t* max_buffer_size );
   /** return an iterator to the prev item after @param iterator and optionally fetch data if buffer is not null */
   int db_prev( int iterator, uint64_t* id, char* buffer, size_t* max_buffer_size );

   /**
    * @return the number of elements in the table stored at code/scope/table
    */
   int  db_count_i64( uint64_t code, uint64_t scope, uint64_t table );
   void db_remove_i64( int iterator );

   int db_find_primary_index64( uint64_t scope, uint64_t table, uint64_t primary, uint64_t* secondary );
   int db_find_secondary_index64( uint64_t scope, uint64_t table, uint64_t* primary, uint64_t secondary );

   int db_upper_bound_primary_index64( uint64_t scope, uint64_t table, uint64_t primary, uint64_t* secondary );
   int db_upper_bound_secondary_index64( uint64_t scope, uint64_t table, uint64_t* primary, uint64_t secondary );

   int db_lower_bound_primary_index64( uint64_t scope, uint64_t table, uint64_t primary, uint64_t* secondary );
   int db_lower_bound_secondary_index64( uint64_t scope, uint64_t table, uint64_t* primary, uint64_t secondary );

   int db_update_index64( int iterator, uint64_t payer, uint64_t id, uint64_t indexvalue );
   int db_remove_index64( int iterator );
   int db_next_index64( int iterator, uint64_t* value );
   int db_prev_index64( int iterator, uint64_t* value );

   int db_find_primary_index128( uint64_t scope, uint64_t table, uint64_t primary, uint128_t* secondary );
   int db_find_secondary_index128( uint64_t scope, uint64_t table, uint64_t* primary, const uint128_t* secondary );

   int db_upper_bound_primary_index128( uint64_t scope, uint64_t table, uint64_t primary, uint128_t* secondary );
   int db_upper_bound_secondary_index128( uint64_t scope, uint64_t table, uint64_t* primary, const uint128_t* secondary );

   int db_lower_bound_primary_index128( uint64_t scope, uint64_t table, uint64_t primary, uint128_t* secondary );
   int db_lower_bound_secondary_index128( uint64_t scope, uint64_t table, uint64_t* primary, const uint128_t* secondary );

   int db_update_index128( int iterator, uint64_t payer, uint64_t id, const uint128_t* secondary );
   int db_remove_index128( int iterator );

   int db_next_index128( int iterator, uint128_t* value );
   int db_prev_index128( int iterator, uint128_t* value );
} /// extern "C"


namespace eosio {

   namespace detail {
      template<typename T>
      struct id_for{};
   }

   struct limit_order {
      uint64_t     id;
      uint128_t    price;
      uint64_t     expiration;
      account_name owner;

      uint128_t  by_owner_id()const {
         uint128_t result(owner);
         result << 64;
         result |= id;
         return result;
      }

      uint128_t by_price()const      { return price;      }
      uint64_t  by_expiration()const { return expiration; }

      EOSLIB_SERIALIZE( (id)(price)(expiration)(owner) )
   };

   template<uint64_t Number, uint64_t IndexName, typename Extractor>
   class index_by {
      typedef   std::decay<decltype( ex( *((const T*)(nullptr)) ) )>::type value_type 
      static const uint64_t index_name = IndexName; 
      static const uint64_t number     = Number; 
      Extractor ex;
   };

   template<typename T, typename IndexTuple>
   class multi_index {

      auto indicies = make_tuple( make_index(N(byorderid), []( const T& obj ){ return obj.id; }),
                                  make_index(N(byexpiration), []( const T& obj) { return obj.by_expiration(); } )
                                  make_index(N(byprice), []( const T& obj) { return obj.by_price(); } ) );

      mutable map<uint64_t, item*> _items;

      public:


         template<uint64_t IndexName, typename Key>
         const T* find( const Key& k )const {

         }

         template<typename Existing>
         auto get_keys( const T& obj ) {
            return get_keys<1>( std::make_tuple( 
         }

         template<int I, typename Existing, std::enable_if< I == std::tuple_size(IndexTuple) - 1> = 0 >
         auto get_keys( Existing&& e, const T& obj ) {
            return std::tuple_cat( std::forward<Existing>(e), std::get<I>( indicies ).extract( obj ) );
         }

         template<int I, typename Existing, std::enable_if< (I < std::tuple_size(IndexTuple) - 1)  > = 0 >
         auto get_keys( Existing&& e, const T& obj ) {
            return get_keys<I+1>( std::tuple_cat( std::forward<Existing>(e), std::get<I>( indicies ).extract( obj ) ), obj );
         }


         template<typename Lambda>
         const T& create( Lambda&& constructor, uint64_t payer ) {
            auto i = new item( *this );
            constructor( static_cast<T&>(*i) ); 
            const T& obj = static_cast<const T&>(*i);

            char tmp[ pack_size( obj ) ];
            datastream<char*> ds( tmp, sizeof(tmp) );
            pack( ds, obj );

            auto pk = obj.primary_key();
            i->__itrs[0] = db_store_i64( _code, _scope, _tables[0], payer, pk, tmp, sizeof(tmp) );

            for_each( indicies, [&]( auto& idx ) {
               i->__itrs[idx.number] = idx.store( _code, _scope, idx.index_name, payer, pk, idx.extract( obj ) );   
            });

            items[pk] = i;
            return obj;
         }

         template<typename Lambda>
         void update( const T& obj, uint64_t payer, Lambda&& updater ) {
            T& mobj = const_cast<T&>(obj);
            item& i = static_cast<item&>(mobj);

            auto pk = mobj.primary_key();

            eosio_assert( &i.__mutli_idx == this );

            auto old_idx = std::make_tuple( 
                            obj.primary_key(), obj.expiration(), obj.by_owner_id(), obj.by_price() );
            updater(mobj);

            char tmp[ pack_size( mobj ) ];
            datastream<char*> ds( tmp, sizeof(tmp) );
            pack( ds, mobj );
            db_update_i64( i.__itrs[0], payer, tmp, sizeof(tmp) );

            auto new_idx = std::make_tuple( 
                            obj.primary_key(), obj.expiration(), obj.by_owner_id(), obj.by_price() );

            if( std::get<1>(old_idx) != std::get<1>(new_idx) ) {
               if( i.__itrs[1] == -2 ) i.__itrs[1] = db_idx64_find_primary( pk );
               db_idx64_update( i.__itrs[1], payer, std::get<1>(new_idx) );
            }
            if( std::get<2>(old_idx) != std::get<2>(new_idx) ) {
               if( i.__itrs[2] == -2 ) i.__itrs[2] = db_idx64_find_primary( pk );
               db_idx64_update( i.__itrs[2], payer, std::get<2>(new_idx) );
            }
            if( std::get<3>(old_idx) != std::get<3>(new_idx) ) {
               if( i.__itrs[3] == -2 ) i.__itrs[3] = db_idx64_find_primary( pk );
               db_idx64_update( i.__itrs[3], payer, std::get<3>(new_idx) );
            }
         }

      private:
         struct item : public T 
         {
            item( multi_index& o ):__mutli_idx(o) {
            }
            multi_index& __multi_idx;
            int          __itrs[3];
         };
         
   };

   /*
   multi_index<  N(limitorders), limit_order, 
          index< N(ownerid), &limit_order::by_owner_id >,
          index< N(byprice), &limit_order::by_price >,
          index< N(byexpire), &limit_order::by_expiration> 
   > orderbook;
   */








   /*
   template<uint64_t Code, uint64_t TableName, typename ObjectType>
   class multi_index {
      public:
         struct cache_object : public ObjectType {
            int primary_itr;
         };

         const cache_object* find( uint64_t primary ) {
            auto itr = _cache.find( primary );
            if( itr != cache.end() ) 
               return itr->second; 
            db_find_i64( Code, _scope, TableName, primary );
            
         }

      private:
         std::map<uint64_t, cache_object*> _cache;
   };


   auto order = orderbook.begin();
   auto end   = orderbook.end();
   while( order != end ) {
      auto cur = order;
      ++order;
      orderbook.remove( cur );
   }

   const limit_order& order = *orderbook.begin();


   /// Options:
   //  1. maintain a wasm-side cache of all dereferenced objects 
   //      a. keeps pointers valid
   //      b. minimizes temporary copies
   //      c. eliminates redundant deserialization losses
   //      d. will utilize more heap memory than necessary, potentially hitting sbrk
   //
   //  2. keep API light and return a copy

   


   

   template<typename ObjectType, typename SecondaryKeyType>
   class index
   {
      public:
         index( uint64_t code, uint64_t scope, uint64_t table );

         struct iterator {
            iterator( index& idx, int itr )
            :_idx(idx), _itr(itr){
               if( itr >= 0 )
                  db_index<SecondaryKeyType>::get( itr, _primary, _secondary );
            }

            uint64_t                  primary()const   { return _primary;   }
            const SecondaryKeyType&   secondary()const { return _secondary; }

            private:
               uint64_t          _primary;
               SecondaryKeyType  _secondary;
               index& _idx;              
               int    _itr;
         };

         iterator lower_bound( const SecondaryKeyType& lb ) {
            return iterator( db_index<SecondaryKeyType>::secondary_lower_bound( _code, _scope, _table, lb ) );
         }

      private:
         uint64_t _code;
         uint64_t _scope;
         uint64_t _table;
   };



   template<typename T, typename Indices>
   class multi_index {
      public:
         struct iterator {
            
            private:
               int primary_itr;
         };



         multi_index( code_name code, scope_name scope, table_name table )
         :_code(code),_scope(scope),_table(table){}

         void insert( const T& obj, account_name payer ) {
            uint64_t id = id_for<T>::get(obj);
            auto buf = pack(obj);
            store_i64(  _scope, _table, payer, id, buf.data(), buf.size() );
            Indices::insert( _code, _scope, _table, payer, obj ); 
         }

         template<typename Constructor>
         void emplace( Constructor&& c ) {
            T tmp;
            id_for<T>::get(tmp) = allocate_id();
            c(tmp);
            insert( tmp );
         }


         template<typename Lambda>
         void modify( const T& obj, account_name payer, Lambda&& update ) {
            update(tmp);

            uint64_t id = id_for<T>::get(value);
            auto buf = pack(tmp);
            store_i64( _code, _scope, _table, payer, id, buf.data(), buf.size() );

            Indices::update( _code, _scope, _table, payer, obj ); 
         }



      private:

   };


} // namespace eosio
