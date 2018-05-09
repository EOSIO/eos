/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <vector>
#include <tuple>
#include <boost/hana.hpp>
#include <functional>
#include <utility>
#include <type_traits>
#include <iterator>
#include <limits>
#include <algorithm>
#include <memory>

#include <boost/multi_index/mem_fun.hpp>

#include <eosiolib/action.h>
#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/db.h>
#include <eosiolib/fixed_key.hpp>

namespace eosio {

using boost::multi_index::const_mem_fun;

#define WRAP_SECONDARY_SIMPLE_TYPE(IDX, TYPE)\
template<>\
struct secondary_index_db_functions<TYPE> {\
   static int32_t db_idx_next( int32_t iterator, uint64_t* primary )          { return db_##IDX##_next( iterator, primary ); }\
   static int32_t db_idx_previous( int32_t iterator, uint64_t* primary )      { return db_##IDX##_previous( iterator, primary ); }\
   static void    db_idx_remove( int32_t iterator  )                          { db_##IDX##_remove( iterator ); }\
   static int32_t db_idx_end( uint64_t code, uint64_t scope, uint64_t table ) { return db_##IDX##_end( code, scope, table ); }\
   static int32_t db_idx_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
      return db_##IDX##_store( scope, table, payer, id, &secondary );\
   }\
   static void    db_idx_update( int32_t iterator, uint64_t payer, const TYPE& secondary ) {\
      db_##IDX##_update( iterator, payer, &secondary );\
   }\
   static int32_t db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary, TYPE& secondary ) {\
      return db_##IDX##_find_primary( code, scope, table, &secondary, primary );\
   }\
   static int32_t db_idx_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
      return db_##IDX##_find_secondary( code, scope, table, &secondary, &primary );\
   }\
   static int32_t db_idx_lowerbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
      return db_##IDX##_lowerbound( code, scope, table, &secondary, &primary );\
   }\
   static int32_t db_idx_upperbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
      return db_##IDX##_upperbound( code, scope, table, &secondary, &primary );\
   }\
};

#define WRAP_SECONDARY_ARRAY_TYPE(IDX, TYPE)\
template<>\
struct secondary_index_db_functions<TYPE> {\
   static int32_t db_idx_next( int32_t iterator, uint64_t* primary )          { return db_##IDX##_next( iterator, primary ); }\
   static int32_t db_idx_previous( int32_t iterator, uint64_t* primary )      { return db_##IDX##_previous( iterator, primary ); }\
   static void    db_idx_remove( int32_t iterator )                           { db_##IDX##_remove( iterator ); }\
   static int32_t db_idx_end( uint64_t code, uint64_t scope, uint64_t table ) { return db_##IDX##_end( code, scope, table ); }\
   static int32_t db_idx_store( uint64_t scope, uint64_t table, uint64_t payer, uint64_t id, const TYPE& secondary ) {\
      return db_##IDX##_store( scope, table, payer, id, secondary.data(), TYPE::num_words() );\
   }\
   static void    db_idx_update( int32_t iterator, uint64_t payer, const TYPE& secondary ) {\
      db_##IDX##_update( iterator, payer, secondary.data(), TYPE::num_words() );\
   }\
   static int32_t db_idx_find_primary( uint64_t code, uint64_t scope, uint64_t table, uint64_t primary, TYPE& secondary ) {\
      return db_##IDX##_find_primary( code, scope, table, secondary.data(), TYPE::num_words(), primary );\
   }\
   static int32_t db_idx_find_secondary( uint64_t code, uint64_t scope, uint64_t table, const TYPE& secondary, uint64_t& primary ) {\
      return db_##IDX##_find_secondary( code, scope, table, secondary.data(), TYPE::num_words(), &primary );\
   }\
   static int32_t db_idx_lowerbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
      return db_##IDX##_lowerbound( code, scope, table, secondary.data(), TYPE::num_words(), &primary );\
   }\
   static int32_t db_idx_upperbound( uint64_t code, uint64_t scope, uint64_t table, TYPE& secondary, uint64_t& primary ) {\
      return db_##IDX##_upperbound( code, scope, table, secondary.data(), TYPE::num_words(), &primary );\
   }\
};

#define MAKE_TRAITS_FOR_ARITHMETIC_SECONDARY_KEY(TYPE)\
template<>\
struct secondary_key_traits<TYPE> {\
   static constexpr  TYPE lowest() { return std::numeric_limits<TYPE>::lowest(); }\
};

namespace _multi_index_detail {

   namespace hana = boost::hana;

   template<typename T>
   struct secondary_index_db_functions;

   template<typename T>
   struct secondary_key_traits;

   WRAP_SECONDARY_SIMPLE_TYPE(idx64,  uint64_t)
   MAKE_TRAITS_FOR_ARITHMETIC_SECONDARY_KEY(uint64_t)

   WRAP_SECONDARY_SIMPLE_TYPE(idx128, uint128_t)
   MAKE_TRAITS_FOR_ARITHMETIC_SECONDARY_KEY(uint128_t)

   WRAP_SECONDARY_SIMPLE_TYPE(idx_double, double)
   MAKE_TRAITS_FOR_ARITHMETIC_SECONDARY_KEY(double)

   WRAP_SECONDARY_SIMPLE_TYPE(idx_long_double, long double)
   MAKE_TRAITS_FOR_ARITHMETIC_SECONDARY_KEY(long double)

   WRAP_SECONDARY_ARRAY_TYPE(idx256, key256)
   template<>
   struct secondary_key_traits<key256> {
      static constexpr key256 lowest() { return key256(); }
   };

}

/**
 *  The indexed_by struct is used to instantiate the indices for the Multi-Index table. In EOSIO, up to 16 secondary indices can be specified.
 *  @brief The indexed_by struct is used to instantiate the indices for the Multi-Index table. In EOSIO, up to 16 secondary indices can be specified.
 *
 *  @tparam IndexName - is the name of the index. The name must be provided as an EOSIO base32 encoded 64-bit integer and must conform to the EOSIO naming requirements of a maximum of 13 characters, the first twelve from the lowercase characters a-z, digits 0-5, and ".", and if there is a 13th character, it is restricted to lowercase characters a-p and ".".
 *  @tparam Extractor - is a function call operator that takes a const reference to the table object type and returns either a secondary key type or a reference to a secondary key type. It is recommended to use the `eosio::const_mem_fun` template, which is a type alias to the `boost::multi_index::const_mem_fun`. See the documentation for the Boost `const_mem_fun` key extractor for more details.
 *
 *  Example:
 *  @code
 *  #include <eosiolib/eosio.hpp>
 *  using namespace eosio;
 *  class mycontract: eosio::contract {
 *    struct record {
 *       uint64_t    primary;
 *       uint128_t   secondary;
 *       uint64_t primary_key() const { return primary; }
 *       uint64_t get_secondary() const { return secondary; }
 *       EOSLIB_SERIALIZE( record, (primary)(secondary) )
 *     };
 *    public:
 *      mycontract( account_name self ):contract(self){}
 *      void myaction() {
 *        auto code = _self;
 *        auto scope = _self;
 *        multi_index<N(mytable), record,
 *                   indexed_by< N(bysecondary), const_mem_fun<record, uint128_t, &record::get_secondary> > > table( code, scope);
 *      }
 *  }
 *  EOSIO_ABI( mycontract, (myaction) )
 *  @endcode
 */
template<uint64_t IndexName, typename Extractor>
struct indexed_by {
   enum constants { index_name   = IndexName };
   typedef Extractor secondary_extractor_type;
};

/**
 *  @defgroup multiindex Multi Index Table 
 *  @brief Defines EOSIO Multi Index Table
 *  @ingroup databasecpp
 * 
 *  
 *  
 *  EOSIO Multi-Index API provides a C++ interface to the EOSIO database. It is patterned after Boost Multi Index Container.
 *  EOSIO Multi-Index table requires exactly a uint64_t primary key. For the table to be able to retrieve the primary key,
 *  the object stored inside the table is required to have a const member function called primary_key() that returns uint64_t.
 *  EOSIO Multi-Index table also supports up to 16 secondary indices. The type of the secondary indices could be any of:
 *  - uint64_t
 *  - uint128_t
 *  - uint256_t
 *  - double
 *  - long double
 *  
 *  @tparam TableName - name of the table
 *  @tparam T - type of the data stored inside the table 
 *  @tparam Indices - secondary indices for the table, up to 16 indices is supported here
 *
 *  Example:
 *  @code
 *  #include <eosiolib/eosio.hpp>
 *  using namespace eosio;
 *  class mycontract: contract {
 *    struct record {
 *      uint64_t    primary;
 *      uint64_t    secondary_1;
 *      uint128_t   secondary_2;
 *      uint256_t   secondary_3;
 *      double      secondary_4;
 *      long double secondary_5;
 *      uint64_t primary_key() const { return primary; }
 *      uint64_t get_secondary_1() const { return secondary_1; }
 *      uint128_t get_secondary_2() const { return secondary_2; }
 *      uint256_t get_secondary_3() const { return secondary_3; }
 *      double get_secondary_4() const { return secondary_4; }
 *      long double get_secondary_5() const { return secondary_5; }
 *      EOSLIB_SERIALIZE( record, (primary)(secondary_1)(secondary_2)(secondary_3)(secondary_4)(secondary_5) )
 *    };
 *    public:
 *      mycontract( account_name self ):contract(self){}
 *      void myaction() {
 *        auto code = _self;
 *        auto scope = _self;
 *        multi_index<N(mytable), record,
 *          indexed_by< N(bysecondary1), const_mem_fun<record, uint64_t, &record::get_secondary_1> >,
 *          indexed_by< N(bysecondary2), const_mem_fun<record, uint128_t, &record::get_secondary_2> >,
 *          indexed_by< N(bysecondary3), const_mem_fun<record, uint256_t, &record::get_secondary_3> >,
 *          indexed_by< N(bysecondary4), const_mem_fun<record, double, &record::get_secondary_4> >,
 *          indexed_by< N(bysecondary5), const_mem_fun<record, long double, &record::get_secondary_5> >
 *        > table( code, scope);
 *      }
 *  }
 *  EOSIO_ABI( mycontract, (myaction) )
 *  @endcode
 *  @{
 */

template<uint64_t TableName, typename T, typename... Indices>
class multi_index
{
   private:

      static_assert( sizeof...(Indices) <= 16, "multi_index only supports a maximum of 16 secondary indices" );

      constexpr static bool validate_table_name( uint64_t n ) {
         // Limit table names to 12 characters so that the last character (4 bits) can be used to distinguish between the secondary indices.
         return (n & 0x000000000000000FULL) == 0;
      }

      constexpr static size_t max_stack_buffer_size = 512;

      static_assert( validate_table_name(TableName), "multi_index does not support table names with a length greater than 12");

      uint64_t _code;
      uint64_t _scope;

      mutable uint64_t _next_primary_key;

      enum next_primary_key_tags : uint64_t {
         no_available_primary_key = static_cast<uint64_t>(-2), // Must be the smallest uint64_t value compared to all other tags
         unset_next_primary_key = static_cast<uint64_t>(-1)
      };

      struct item : public T
      {
         template<typename Constructor>
         item( const multi_index* idx, Constructor&& c )
         :__idx(idx){
            c(*this);
         }

         const multi_index* __idx;
         int32_t            __primary_itr;
         int32_t            __iters[sizeof...(Indices)+(sizeof...(Indices)==0)];
      };

      struct item_ptr
      {
         item_ptr(std::unique_ptr<item>&& i, uint64_t pk, int32_t pitr)
         : _item(std::move(i)), _primary_key(pk), _primary_itr(pitr) {}

         std::unique_ptr<item> _item;
         uint64_t              _primary_key;
         int32_t               _primary_itr;
      };

      mutable std::vector<item_ptr> _items_vector;

      template<uint64_t IndexName, typename Extractor, uint64_t Number, bool IsConst>
      struct index {
         public:
            typedef Extractor  secondary_extractor_type;
            typedef typename std::decay<decltype( Extractor()(nullptr) )>::type secondary_key_type;

            constexpr static bool validate_index_name( uint64_t n ) {
               return n != 0 && n != N(primary); // Primary is a reserve index name.
            }

            static_assert( validate_index_name(IndexName), "invalid index name used in multi_index" );

            enum constants {
               table_name   = TableName,
               index_name   = IndexName,
               index_number = Number,
               index_table_name = (TableName & 0xFFFFFFFFFFFFFFF0ULL) | (Number & 0x000000000000000FULL) // Assuming no more than 16 secondary indices are allowed
            };

            constexpr static uint64_t name()   { return index_table_name; }
            constexpr static uint64_t number() { return Number; }

            struct const_iterator : public std::iterator<std::bidirectional_iterator_tag, const T> {
               public:
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
                     using namespace _multi_index_detail;

                     eosio_assert( _item != nullptr, "cannot increment end iterator" );

                     if( _item->__iters[Number] == -1 ) {
                        secondary_key_type temp_secondary_key;
                        auto idxitr = secondary_index_db_functions<secondary_key_type>::db_idx_find_primary(_idx->get_code(), _idx->get_scope(), _idx->name(), _item->primary_key(), temp_secondary_key);
                        auto& mi = const_cast<item&>( *_item );
                        mi.__iters[Number] = idxitr;
                     }

                     uint64_t next_pk = 0;
                     auto next_itr = secondary_index_db_functions<secondary_key_type>::db_idx_next( _item->__iters[Number], &next_pk );
                     if( next_itr < 0 ) {
                        _item = nullptr;
                        return *this;
                     }

                     const T& obj = *_idx->_multidx->find( next_pk );
                     auto& mi = const_cast<item&>( static_cast<const item&>(obj) );
                     mi.__iters[Number] = next_itr;
                     _item = &mi;

                     return *this;
                  }

                  const_iterator& operator--() {
                     using namespace _multi_index_detail;

                     uint64_t prev_pk = 0;
                     int32_t  prev_itr = -1;

                     if( !_item ) {
                        auto ei = secondary_index_db_functions<secondary_key_type>::db_idx_end(_idx->get_code(), _idx->get_scope(), _idx->name());
                        eosio_assert( ei != -1, "cannot decrement end iterator when the index is empty" );
                        prev_itr = secondary_index_db_functions<secondary_key_type>::db_idx_previous( ei , &prev_pk );
                        eosio_assert( prev_itr >= 0, "cannot decrement end iterator when the index is empty" );
                     } else {
                        if( _item->__iters[Number] == -1 ) {
                           secondary_key_type temp_secondary_key;
                           auto idxitr = secondary_index_db_functions<secondary_key_type>::db_idx_find_primary(_idx->get_code(), _idx->get_scope(), _idx->name(), _item->primary_key(), temp_secondary_key);
                           auto& mi = const_cast<item&>( *_item );
                           mi.__iters[Number] = idxitr;
                        }
                        prev_itr = secondary_index_db_functions<secondary_key_type>::db_idx_previous( _item->__iters[Number], &prev_pk );
                        eosio_assert( prev_itr >= 0, "cannot decrement iterator at beginning of index" );
                     }

                     const T& obj = *_idx->_multidx->find( prev_pk );
                     auto& mi = const_cast<item&>( static_cast<const item&>(obj) );
                     mi.__iters[Number] = prev_itr;
                     _item = &mi;

                     return *this;
                  }

                  const_iterator():_item(nullptr){}
               private:
                  friend struct index;
                  const_iterator( const index* idx, const item* i = nullptr )
                  : _idx(idx), _item(i) {}

                  const index* _idx;
                  const item*  _item;
            }; /// struct multi_index::index::const_iterator

            typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

            const_iterator cbegin()const {
               using namespace _multi_index_detail;
               return lower_bound( secondary_key_traits<secondary_key_type>::lowest() );
            }
            const_iterator begin()const  { return cbegin(); }

            const_iterator cend()const   { return const_iterator( this ); }
            const_iterator end()const    { return cend(); }

            const_reverse_iterator crbegin()const { return std::make_reverse_iterator(cend()); }
            const_reverse_iterator rbegin()const  { return crbegin(); }

            const_reverse_iterator crend()const   { return std::make_reverse_iterator(cbegin()); }
            const_reverse_iterator rend()const    { return crend(); }

            const_iterator find( secondary_key_type&& secondary )const {
               return find( secondary );
            }

            const_iterator find( const secondary_key_type& secondary )const {
               auto lb = lower_bound( secondary );
               auto e = cend();
               if( lb == e ) return e;

               if( secondary != secondary_extractor_type()(*lb) )
                  return e;
               return lb;
            }

            const T& get( secondary_key_type&& secondary )const {
               return get( secondary );
            }

            // Gets the object with the smallest primary key in the case where the secondary key is not unique.
            const T& get( const secondary_key_type& secondary )const {
               auto result = find( secondary );
               eosio_assert( result != cend(), "unable to find secondary key" );
               return *result;
            }

            const_iterator lower_bound( secondary_key_type&& secondary )const {
               return lower_bound( secondary );
            }
            const_iterator lower_bound( const secondary_key_type& secondary )const {
               using namespace _multi_index_detail;

               uint64_t primary = 0;
               secondary_key_type secondary_copy(secondary);
               auto itr = secondary_index_db_functions<secondary_key_type>::db_idx_lowerbound( get_code(), get_scope(), name(), secondary_copy, primary );
               if( itr < 0 ) return cend();

               const T& obj = *_multidx->find( primary );
               auto& mi = const_cast<item&>( static_cast<const item&>(obj) );
               mi.__iters[Number] = itr;

               return {this, &mi};
            }

            const_iterator upper_bound( secondary_key_type&& secondary )const {
               return upper_bound( secondary );
            }
            const_iterator upper_bound( const secondary_key_type& secondary )const {
               using namespace _multi_index_detail;

               uint64_t primary = 0;
               secondary_key_type secondary_copy(secondary);
               auto itr = secondary_index_db_functions<secondary_key_type>::db_idx_upperbound( get_code(), get_scope(), name(), secondary_copy, primary );
               if( itr < 0 ) return cend();

               const T& obj = *_multidx->find( primary );
               auto& mi = const_cast<item&>( static_cast<const item&>(obj) );
               mi.__iters[Number] = itr;

               return {this, &mi};
            }

            const_iterator iterator_to( const T& obj ) {
               using namespace _multi_index_detail;

               const auto& objitem = static_cast<const item&>(obj);
               eosio_assert( objitem.__idx == _multidx, "object passed to iterator_to is not in multi_index" );

               if( objitem.__iters[Number] == -1 ) {
                  secondary_key_type temp_secondary_key;
                  auto idxitr = secondary_index_db_functions<secondary_key_type>::db_idx_find_primary(get_code(), get_scope(), name(), objitem.primary_key(), temp_secondary_key);
                  auto& mi = const_cast<item&>( objitem );
                  mi.__iters[Number] = idxitr;
               }

               return {this, &objitem};
            }

            template<typename Lambda>
            void modify( const_iterator itr, uint64_t payer, Lambda&& updater ) {
               eosio_assert( itr != cend(), "cannot pass end iterator to modify" );

               _multidx->modify( *itr, payer, std::forward<Lambda&&>(updater) );
            }

            const_iterator erase( const_iterator itr ) {
               eosio_assert( itr != cend(), "cannot pass end iterator to erase" );

               const auto& obj = *itr;
               ++itr;

               _multidx->erase(obj);

               return itr;
            }

            uint64_t get_code()const  { return _multidx->get_code(); }
            uint64_t get_scope()const { return _multidx->get_scope(); }

            static auto extract_secondary_key(const T& obj) { return secondary_extractor_type()(obj); }

         private:
            friend class multi_index;

            index( typename std::conditional<IsConst, const multi_index*, multi_index*>::type midx )
            :_multidx(midx){}

            typename std::conditional<IsConst, const multi_index*, multi_index*>::type _multidx;
      }; /// struct multi_index::index

      template<uint64_t I>
      struct intc { enum e{ value = I }; operator uint64_t()const{ return I; }  };

      static constexpr auto transform_indices( ) {
         using namespace _multi_index_detail;

         typedef decltype( hana::zip_shortest(
                             hana::make_tuple( intc<0>(), intc<1>(), intc<2>(), intc<3>(), intc<4>(), intc<5>(),
                                               intc<6>(), intc<7>(), intc<8>(), intc<9>(), intc<10>(), intc<11>(),
                                               intc<12>(), intc<13>(), intc<14>(), intc<15>() ),
                             hana::tuple<Indices...>() ) ) indices_input_type;

         return hana::transform( indices_input_type(), [&]( auto&& idx ){
             typedef typename std::decay<decltype(hana::at_c<0>(idx))>::type num_type;
             typedef typename std::decay<decltype(hana::at_c<1>(idx))>::type idx_type;
             return hana::make_tuple( hana::type_c<index<idx_type::index_name,
                                                         typename idx_type::secondary_extractor_type,
                                                         num_type::e::value, false> >,
                                      hana::type_c<index<idx_type::index_name,
                                                         typename idx_type::secondary_extractor_type,
                                                         num_type::e::value, true> > );

         });
      }

      typedef decltype( multi_index::transform_indices() ) indices_type;

      indices_type _indices;

      const item& load_object_by_primary_iterator( int32_t itr )const {
         using namespace _multi_index_detail;

         auto itr2 = std::find_if(_items_vector.rbegin(), _items_vector.rend(), [&](const item_ptr& ptr) {
            return ptr._primary_itr == itr;
         });
         if( itr2 != _items_vector.rend() )
            return *itr2->_item;

         auto size = db_get_i64( itr, nullptr, 0 );
         eosio_assert( size >= 0, "error reading iterator" );

         //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
         void* buffer = max_stack_buffer_size < size_t(size) ? malloc(size_t(size)) : alloca(size_t(size));

         db_get_i64( itr, buffer, uint32_t(size) );

         datastream<const char*> ds( (char*)buffer, uint32_t(size) );

         if ( max_stack_buffer_size < size_t(size) ) {
            free(buffer);
         }

         auto itm = std::make_unique<item>( this, [&]( auto& i ) {
            T& val = static_cast<T&>(i);
            ds >> val;

            i.__primary_itr = itr;
            hana::for_each( _indices, [&]( auto& idx ) {
               typedef typename decltype(+hana::at_c<1>(idx))::type index_type;

               i.__iters[ index_type::number() ] = -1;
            });
         });

         const item* ptr = itm.get();
         auto pk   = itm->primary_key();
         auto pitr = itm->__primary_itr;

         _items_vector.emplace_back( std::move(itm), pk, pitr );

         return *ptr;
      } /// load_object_by_primary_iterator

   public:
      /**
       *  Constructs an instance of a Multi-Index table.
       *  @brief Constructs an instance of a Multi-Index table.
       *
       *  @param code - Account that owns table
       *  @param scope - Scope identifier within the code hierarchy
       *
       *  @pre code and scope member properties are initialized
       *  @post each secondary index table initialized
       *  @post Secondary indices are updated to refer to the newly added object. If the secondary index tables do not exist, they are created.
       *  @post The payer is charged for the storage usage of the new object and, if the table (and secondary index tables) must be created, for the overhead of the table creation.
       * 
       *  Notes
       *  The `eosio::multi_index` template has template parameters `<uint64_t TableName, typename T, typename... Indices>`, where:
       *  - `TableName` is the name of the table, maximum 12 characters long, characters in the name from the set of lowercase letters, digits 1 to 5, and the "." (period) character;
       *  - `T` is the object type (i.e., row definition);
       *  - `Indices` is a list of up to 16 secondary indices.
       *  - Each must be a default constructable class or struct
       *  - Each must have a function call operator that takes a const reference to the table object type and returns either a secondary key type or a reference to a secondary key type
       *  - It is recommended to use the eosio::const_mem_fun template, which is a type alias to the boost::multi_index::const_mem_fun.  See the documentation for the Boost const_mem_fun key extractor for more details.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      multi_index( uint64_t code, uint64_t scope )
      :_code(code),_scope(scope),_next_primary_key(unset_next_primary_key)
      {}

      /**
       *  Returns the `code` member property.
       *  @brief Returns the `code` member property.
       * 
       *  @return Account name of the Code that owns the Primary Table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(N(dan), N(dan)); // code, scope
       *        eosio_assert(addresses.get_code() == N(dan), "Codes don't match.");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      uint64_t get_code()const  { return _code; }

      /**
       *  Returns the `scope` member property.
       *  @brief Returns the `scope` member property.
       * 
       *  @return Scope id of the Scope within the Code of the Current Receiver under which the desired Primary Table instance can be found.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(N(dan), N(dan)); // code, scope
       *        eosio_assert(addresses.get_code() == N(dan), "Scopes don't match");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      uint64_t get_scope()const { return _scope; }

      struct const_iterator : public std::iterator<std::bidirectional_iterator_tag, const T> {
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

            uint64_t next_pk;
            auto next_itr = db_next_i64( _item->__primary_itr, &next_pk );
            if( next_itr < 0 )
               _item = nullptr;
            else
               _item = &_multidx->load_object_by_primary_iterator( next_itr );
            return *this;
         }
         const_iterator& operator--() {
            uint64_t prev_pk;
            int32_t  prev_itr = -1;

            if( !_item ) {
               auto ei = db_end_i64(_multidx->get_code(), _multidx->get_scope(), TableName);
               eosio_assert( ei != -1, "cannot decrement end iterator when the table is empty" );
               prev_itr = db_previous_i64( ei , &prev_pk );
               eosio_assert( prev_itr >= 0, "cannot decrement end iterator when the table is empty" );
            } else {
               prev_itr = db_previous_i64( _item->__primary_itr, &prev_pk );
               eosio_assert( prev_itr >= 0, "cannot decrement iterator at beginning of table" );
            }

            _item = &_multidx->load_object_by_primary_iterator( prev_itr );
            return *this;
         }

         private:
            const_iterator( const multi_index* mi, const item* i = nullptr )
            :_multidx(mi),_item(i){}

            const multi_index* _multidx;
            const item*        _item;
            friend class multi_index;
      }; /// struct multi_index::const_iterator

      typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

      /**
       *  Returns an iterator pointing to the object_type with the lowest primary key value in the Multi-Index table.
       *  @brief Returns an iterator pointing to the object_type with the lowest primary key value in the Multi-Index table.
       * 
       *  @return An iterator pointing to the object_type with the lowest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr == addresses.cbegin(), "Only address is not at front.");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator cbegin()const {
         return lower_bound(std::numeric_limits<uint64_t>::lowest());
      }

      /**
       *  Returns an iterator pointing to the object_type with the lowest primary key value in the Multi-Index table.
       *  @brief Returns an iterator pointing to the object_type with the lowest primary key value in the Multi-Index table.
       * 
       *  @return An iterator pointing to the object_type with the lowest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr == addresses.begin(), "Only address is not at front.");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator begin()const  { return cbegin(); }

      /**
       *  Returns an iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       *  @brief Returns an iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  @return An iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr != addresses.cend(), "Address for account doesn't exist");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator cend()const   { return const_iterator( this ); }

      /**
       *  Returns an iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       *  @brief Returns an iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  @return An iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr != addresses.end(), "Address for account doesn't exist");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator end()const    { return cend(); }

      /**
       *  Returns a reverse iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       *  @brief Returns a reverse iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  @return A reverse iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *        });
       *        auto itr = addresses.crbegin();
       *        eosio_assert(itr->account_name == N(dan), "Incorrect Last Record ");
       *        itr++;
       *        eosio_assert(itr->account_name == N(brendan), "Incorrect Second Last Record");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_reverse_iterator crbegin()const { return std::make_reverse_iterator(cend()); }
      
      /**
       *  Returns a reverse iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       *  @brief Returns a reverse iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  @return A reverse iterator pointing to the `object_type` with the highest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *        });
       *        auto itr = addresses.rbegin();
       *        eosio_assert(itr->account_name == N(dan), "Incorrect Last Record ");
       *        itr++;
       *        eosio_assert(itr->account_name == N(brendan), "Incorrect Second Last Record");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_reverse_iterator rbegin()const  { return crbegin(); }

      /**
       *  Returns an iterator pointing to the `object_type` with the lowest primary key value in the Multi-Index table.
       *  @brief Returns an iterator pointing to the `object_type` with the lowest primary key value in the Multi-Index table.
       * 
       *  @return An iterator pointing to the `object_type` with the lowest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *        });
       *        auto itr = addresses.crend();
       *        itr--;
       *        eosio_assert(itr->account_name == N(brendan), "Incorrect First Record ");
       *        itr--;
       *        eosio_assert(itr->account_name == N(dan), "Incorrect Second Record");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_reverse_iterator crend()const   { return std::make_reverse_iterator(cbegin()); }
      
      /**
       *  Returns an iterator pointing to the `object_type` with the lowest primary key value in the Multi-Index table.
       *  @brief Returns an iterator pointing to the `object_type` with the lowest primary key value in the Multi-Index table.
       * 
       *  @return An iterator pointing to the `object_type` with the lowest primary key value in the Multi-Index table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *        });
       *        auto itr = addresses.rend();
       *        itr--;
       *        eosio_assert(itr->account_name == N(brendan), "Incorrect First Record ");
       *        itr--;
       *        eosio_assert(itr->account_name == N(dan), "Incorrect Second Record");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_reverse_iterator rend()const    { return crend(); }

      /**
       *  Searches for the `object_type` with the lowest primary key that is greater than or equal to a given primary key.
       *  @brief Searches for the `object_type` with the lowest primary key that is greater than or equal to a given primary key.
       *
       *  @param primary - Primary key that establishes the target value for the lower bound search.
       * 
       *  @return An iterator pointing to the `object_type` that has the lowest primary key that is greater than or equal to `primary`. If an object could not be found, it will return the `end` iterator. If the table does not exist** it will return `-1`.
       *
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint32_t zip = 0;
       *       uint64_t primary_key() const { return account_name; }
       *       uint64_t by_zip() const { return zip; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state)(zip) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address, indexed_by< N(zip), const_mem_fun<address, uint64_t, &address::by_zip> > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *          address.zip = 93446;
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *          address.zip = 93445;
       *        });
       *        uint32_t zipnumb = 93445;
       *        auto zip_index = addresses.get_index<N(zip)>();
       *        auto itr = zip_index.lower_bound(zipnumb);
       *        eosio_assert(itr->account_name == N(brendan), "Incorrect First Lower Bound Record ");
       *        itr++;
       *        eosio_assert(itr->account_name == N(dan), "Incorrect Second Lower Bound Record");
       *        itr++;
       *        eosio_assert(itr == zip_index.end(), "Incorrect End of Iterator");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator lower_bound( uint64_t primary )const {
         auto itr = db_lowerbound_i64( _code, _scope, TableName, primary );
         if( itr < 0 ) return end();
         const auto& obj = load_object_by_primary_iterator( itr );
         return {this, &obj};
      }

      /**
       *  Searches for the `object_type` with the highest primary key that is less than or equal to a given primary key.
       *  @brief Searches for the `object_type` with the highest primary key that is less than or equal to a given primary key.
       *
       *  @param primary - Primary key that establishes the target value for the upper bound search
       * 
       *  @return An iterator pointing to the `object_type` that has the highest primary key that is less than or equal to `primary`. If an object could not be found, it will return the `end` iterator. If the table does not exist** it will return `-1`.
       *
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint32_t zip = 0;
       *       uint64_t liked = 0;
       *       uint64_t primary_key() const { return account_name; }
       *       uint64_t by_zip() const { return zip; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state)(zip) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address, indexed_by< N(zip), const_mem_fun<address, uint64_t, &address::by_zip> > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *          address.zip = 93446;
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *          address.zip = 93445;
       *        });
       *        uint32_t zipnumb = 93445;
       *        auto zip_index = addresses.get_index<N(zip)>();
       *        auto itr = zip_index.upper_bound(zipnumb);
       *        eosio_assert(itr->account_name == N(dan), "Incorrect First Upper Bound Record ");
       *        itr++;
       *        eosio_assert(itr == zip_index.end(), "Incorrect End of Iterator");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator upper_bound( uint64_t primary )const {
         auto itr = db_upperbound_i64( _code, _scope, TableName, primary );
         if( itr < 0 ) return end();
         const auto& obj = load_object_by_primary_iterator( itr );
         return {this, &obj};
      }

      /**
       *  Returns an available primary key.
       *  @brief Returns an available primary key.
       * 
       *  @return An available (unused) primary key value.
       * 
       *  Notes:
       *  Intended to be used in tables in which the primary keys of the table are strictly intended to be auto-incrementing, and thus will never be set to custom values by the contract.  Violating this expectation could result in the table appearing to be full due to inability to allocate an available primary key.
       *  Ideally this method would only be used to determine the appropriate primary key to use within new objects added to a table in which the primary keys of the table are strictly intended from the beginning to be autoincrementing and thus will not ever be set to custom arbitrary values by the contract. Violating this agreement could result in the table appearing full when in reality there is plenty of space left.
       *
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t key;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return key; }
       *       EOSLIB_SERIALIZE( address, (key)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.key = addresses.available_primary_key();
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
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

      /**
       *  Returns an appropriately typed Secondary Index.
       *  @brief Returns an appropriately typed Secondary Index.
       *
       *  @tparam IndexName - the ID of the desired secondary index
       * 
       *  @return An index of the appropriate type: Primitive 64-bit unsigned integer key (idx64), Primitive 128-bit unsigned integer key (idx128), 128-bit fixed-size lexicographical key (idx128), 256-bit fixed-size lexicographical key (idx256), Floating point key, Double precision floating point key, Long Double (quadruple) precision floating point key
       *
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint32_t zip = 0;
       *       uint64_t primary_key() const { return account_name; }
       *       uint64_t by_zip() const { return zip; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state)(zip) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address, indexed_by< N(zip), const_mem_fun<address, uint64_t, &address::by_zip> > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self);  // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *          address.zip = 93446;
       *        });
       *        uint32_t zipnumb = 93446;
       *        auto zip_index = addresses.get_index<N(zip)>();
       *        auto itr = zip_index.find(zipnumb);
       *        eosio_assert(itr->account_name == N(dan), "Incorrect Record ");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      template<uint64_t IndexName>
      auto get_index() {
         using namespace _multi_index_detail;

         auto res = hana::find_if( _indices, []( auto&& in ) {
            return std::integral_constant<bool, std::decay<typename decltype(+hana::at_c<0>(in))::type>::type::index_name == IndexName>();
         });

         static_assert( res != hana::nothing, "name provided is not the name of any secondary index within multi_index" );

         return typename decltype(+hana::at_c<0>(res.value()))::type(this);
      }

      /**
       *  Returns an appropriately typed Secondary Index.
       *  @brief Returns an appropriately typed Secondary Index.
       *
       *  @tparam IndexName - the ID of the desired secondary index
       * 
       *  @return An index of the appropriate type: Primitive 64-bit unsigned integer key (idx64), Primitive 128-bit unsigned integer key (idx128), 128-bit fixed-size lexicographical key (idx128), 256-bit fixed-size lexicographical key (idx256), Floating point key, Double precision floating point key, Long Double (quadruple) precision floating point key
       *
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint32_t zip = 0;
       *       uint64_t primary_key() const { return account_name; }
       *       uint64_t by_zip() const { return zip; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state)(zip) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address, indexed_by< N(zip), const_mem_fun<address, uint64_t, &address::by_zip> > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *          address.zip = 93446;
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *          address.zip = 93445;
       *        });
       *        uint32_t zipnumb = 93445;
       *        auto zip_index = addresses.get_index<N(zip)>();
       *        auto itr = zip_index.upper_bound(zipnumb);
       *        eosio_assert(itr->account_name == N(dan), "Incorrect First Upper Bound Record ");
       *        itr++;
       *        eosio_assert(itr == zip_index.end(), "Incorrect End of Iterator");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      template<uint64_t IndexName>
      auto get_index()const {
         using namespace _multi_index_detail;

         auto res = hana::find_if( _indices, []( auto&& in ) {
            return std::integral_constant<bool, std::decay<typename decltype(+hana::at_c<1>(in))::type>::type::index_name == IndexName>();
         });

         static_assert( res != hana::nothing, "name provided is not the name of any secondary index within multi_index" );

         return typename decltype(+hana::at_c<1>(res.value()))::type(this);
      }

      /**
       *  Returns an iterator to the given object in a Multi-Index table.
       *  @brief Returns an iterator to the given object in a Multi-Index table.
       *
       *  @param obj - A reference to the desired object
       * 
       *  @return An iterator to the given object
       *
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint32_t zip = 0;
       *       uint64_t primary_key() const { return account_name; }
       *       uint64_t by_zip() const { return zip; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state)(zip) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address, indexed_by< N(zip), const_mem_fun<address, uint64_t, &address::by_zip> > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *          address.zip = 93446;
       *        });
       *        addresses.emplace(payer, [&](auto& address) {
       *          address.account_name = N(brendan);
       *          address.first_name = "Brendan";
       *          address.last_name = "Blumer";
       *          address.street = "1 EOS Way";
       *          address.city = "Hong Kong";
       *          address.state = "HK";
       *          address.zip = 93445;
       *        });
       *        auto user = addresses.get(N(dan));
       *        auto itr = address.find(N(dan));
       *        eosio_assert(iterator_to(user) == itr, "Invalid iterator");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator iterator_to( const T& obj )const {
         const auto& objitem = static_cast<const item&>(obj);
         eosio_assert( objitem.__idx == this, "object passed to iterator_to is not in multi_index" );
         return {this, &objitem};
      }
      /**
       *  Adds a new object (i.e., row) to the table.
       *  @brief Adds a new object (i.e., row) to the table.
       *
       *  @param payer - Account name of the payer for the Storage usage of the new object
       *  @param constructor - Lambda function that does an in-place initialization of the object to be created in the table
       *
       *  @pre A multi index table has been instantiated
       *  @post A new object is created in the Multi-Index table, with a unique primary key (as specified in the object).  The object is serialized and written to the table. If the table does not exist, it is created.
       *  @post Secondary indices are updated to refer to the newly added object. If the secondary index tables do not exist, they are created.
       *  @post The payer is charged for the storage usage of the new object and, if the table (and secondary index tables) must be created, for the overhead of the table creation.
       * 
       *  @return A primary key iterator to the newly created object
       * 
       *  Exception - The account is not authorized to write to the table.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      template<typename Lambda>
      const_iterator emplace( uint64_t payer, Lambda&& constructor ) {
         using namespace _multi_index_detail;

         eosio_assert( _code == current_receiver(), "cannot create objects in table of another contract" ); // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.

         auto itm = std::make_unique<item>( this, [&]( auto& i ){
            T& obj = static_cast<T&>(i);
            constructor( obj );

            size_t size = pack_size( obj );

            //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
            void* buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);

            datastream<char*> ds( (char*)buffer, size );
            ds << obj;

            auto pk = obj.primary_key();

            i.__primary_itr = db_store_i64( _scope, TableName, payer, pk, buffer, size );

            if ( max_stack_buffer_size < size ) {
               free(buffer);
            }

            if( pk >= _next_primary_key )
               _next_primary_key = (pk >= no_available_primary_key) ? no_available_primary_key : (pk + 1);

            hana::for_each( _indices, [&]( auto& idx ) {
               typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

               i.__iters[index_type::number()] = secondary_index_db_functions<typename index_type::secondary_key_type>::db_idx_store( _scope, index_type::name(), payer, obj.primary_key(), index_type::extract_secondary_key(obj) );
            });
         });

         const item* ptr = itm.get();
         auto pk   = itm->primary_key();
         auto pitr = itm->__primary_itr;

         _items_vector.emplace_back( std::move(itm), pk, pitr );

         return {this, ptr};
      }

      /**
       *  Modifies an existing object in a table.
       *  @brief Modifies an existing object in a table.
       *
       *  @param itr - an iterator pointing to the object to be updated
       *  @param payer - account name of the payer for the Storage usage of the updated row
       *  @param updater - lambda function that updates the target object
       * 
       *  @pre itr points to an existing element
       *  @pre payer is a valid account that is authorized to execute the action and be billed for storage usage.
       *  
       *  @post The modified object is serialized, then replaces the existing object in the table.
       *  @post Secondary indices are updated; the primary key of the updated object is not changed.
       *  @post The payer is charged for the storage usage of the updated object.
       *  @post If payer is the same as the existing payer, payer only pays for the usage difference between existing and updated object (and is refunded if this difference is negative).
       *  @post If payer is different from the existing payer, the existing payer is refunded for the storage usage of the existing object.
       * 
       *  Exceptions:
       *  If called with an invalid precondition, execution is aborted.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr != addresses.end(), "Address for account not found");
       *        addresses.modify( itr, account payer, [&]( auto& address ) {
       *          address.city = "San Luis Obispo";
       *          address.state = "CA";
       *        });
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      template<typename Lambda>
      void modify( const_iterator itr, uint64_t payer, Lambda&& updater ) {
         eosio_assert( itr != end(), "cannot pass end iterator to modify" );

         modify( *itr, payer, std::forward<Lambda&&>(updater) );
      }

      /**
       *  Modifies an existing object in a table.
       *  @brief Modifies an existing object in a table.
       *
       *  @param obj - a reference to the object to be updated
       *  @param payer - account name of the payer for the Storage usage of the updated row
       *  @param updater - lambda function that updates the target object
       * 
       *  @pre obj is an existing object in the table
       *  @pre payer is a valid account that is authorized to execute the action and be billed for storage usage.
       *  
       *  @post The modified object is serialized, then replaces the existing object in the table.
       *  @post Secondary indices are updated; the primary key of the updated object is not changed.
       *  @post The payer is charged for the storage usage of the updated object.
       *  @post If payer is the same as the existing payer, payer only pays for the usage difference between existing and updated object (and is refunded if this difference is negative).
       *  @post If payer is different from the existing payer, the existing payer is refunded for the storage usage of the existing object.
       * 
       *  Exceptions:
       *  If called with an invalid precondition, execution is aborted.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr != addresses.end(), "Address for account not found");
       *        addresses.modify( *itr, payer, [&]( auto& address ) {
       *          address.city = "San Luis Obispo";
       *          address.state = "CA";
       *        });
       *        eosio_assert(itr->city == "San Luis Obispo", "Address not modified");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      template<typename Lambda>
      void modify( const T& obj, uint64_t payer, Lambda&& updater ) {
         using namespace _multi_index_detail;

         const auto& objitem = static_cast<const item&>(obj);
         eosio_assert( objitem.__idx == this, "object passed to modify is not in multi_index" );
         auto& mutableitem = const_cast<item&>(objitem);
         eosio_assert( _code == current_receiver(), "cannot modify objects in table of another contract" ); // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.

         auto secondary_keys = hana::transform( _indices, [&]( auto&& idx ) {
            typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

            return index_type::extract_secondary_key( obj );
         });

         auto pk = obj.primary_key();

         auto& mutableobj = const_cast<T&>(obj); // Do not forget the auto& otherwise it would make a copy and thus not update at all.
         updater( mutableobj );

         eosio_assert( pk == obj.primary_key(), "updater cannot change primary key when modifying an object" );

         size_t size = pack_size( obj );
         //using malloc/free here potentially is not exception-safe, although WASM doesn't support exceptions
         void* buffer = max_stack_buffer_size < size ? malloc(size) : alloca(size);

         datastream<char*> ds( (char*)buffer, size );
         ds << obj;

         db_update_i64( objitem.__primary_itr, payer, buffer, size );

         if ( max_stack_buffer_size < size ) {
            free( buffer );
         }

         if( pk >= _next_primary_key )
            _next_primary_key = (pk >= no_available_primary_key) ? no_available_primary_key : (pk + 1);

         hana::for_each( _indices, [&]( auto& idx ) {
            typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

            auto secondary = index_type::extract_secondary_key( obj );
            if( hana::at_c<index_type::index_number>(secondary_keys) != secondary ) {
               auto indexitr = mutableitem.__iters[index_type::number()];

               if( indexitr < 0 ) {
                  typename index_type::secondary_key_type temp_secondary_key;
                  indexitr = mutableitem.__iters[index_type::number()]
                           = secondary_index_db_functions<typename index_type::secondary_key_type>::db_idx_find_primary( _code, _scope, index_type::name(), pk,  temp_secondary_key );
               }

               secondary_index_db_functions<typename index_type::secondary_key_type>::db_idx_update( indexitr, payer, secondary );
            }
         });
      }

      /**
       *  Retrieves an existing object from a table using its primary key.
       *  @brief Retrieves an existing object from a table using its primary key.
       *
       *  @param primary - Primary key value of the object
       *  @return A constant reference to the object containing the specified primary key.
       * 
       *  Exception - No object matches the given key
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto user = addresses.get(N(dan));
       *        eosio_assert(user.first_name == "Daniel", "Couldn't get him.");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const T& get( uint64_t primary )const {
         auto result = find( primary );
         eosio_assert( result != cend(), "unable to find key" );
         return *result;
      }

      /**
       *  Search for an existing object in a table using its primary key.
       *  @brief Search for an existing object in a table using its primary key.
       *
       *  @param primary - Primary key value of the object
       *  @return An iterator to the found object which has a primary key equal to `primary` OR the `end` iterator of the referenced table if an object with primary key `primary` is not found.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr != addresses.end(), "Couldn't get him.");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator find( uint64_t primary )const {
         auto itr2 = std::find_if(_items_vector.rbegin(), _items_vector.rend(), [&](const item_ptr& ptr) {
            return ptr._item->primary_key() == primary;
         });
         if( itr2 != _items_vector.rend() )
            return iterator_to(*(itr2->_item));

         auto itr = db_find_i64( _code, _scope, TableName, primary );
         if( itr < 0 ) return end();

         const item& i = load_object_by_primary_iterator( itr );
         return iterator_to(static_cast<const T&>(i));
      }

      /**
       *  Remove an existing object from a table using its primary key.
       *  @brief Remove an existing object from a table using its primary key.
       *
       *  @param itr - An iterator pointing to the object to be removed
       *  
       *  @pre itr points to an existing element
       *  @post The object is removed from the table and all associated storage is reclaimed.
       *  @post Secondary indices associated with the table are updated.
       *  @post The existing payer for storage usage of the object is refunded for the table and secondary indices usage of the removed object, and if the table and indices are removed, for the associated overhead.
       * 
       *  @return For the signature with `const_iterator`, returns a pointer to the object following the removed object.
       * 
       *  Exceptions:
       *  The object to be removed is not in the table.
       *  The action is not authorized to modify the table.
       *  The given iterator is invalid.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr != addresses.end(), "Address for account not found");
       *        addresses.erase( itr );
       *        eosio_assert(itr != addresses.end(), "Address not erased properly");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      const_iterator erase( const_iterator itr ) {
         eosio_assert( itr != end(), "cannot pass end iterator to erase" );

         const auto& obj = *itr;
         ++itr;

         erase(obj);

         return itr;
      }

      /**
       *  Remove an existing object from a table using its primary key.
       *  @brief Remove an existing object from a table using its primary key.
       *
       *  @param obj - Object to be removed
       *  
       *  @pre obj is an existing object in the table
       *  @post The object is removed from the table and all associated storage is reclaimed.
       *  @post Secondary indices associated with the table are updated.
       *  @post The existing payer for storage usage of the object is refunded for the table and secondary indices usage of the removed object, and if the table and indices are removed, for the associated overhead.
       * 
       *  Exceptions:
       *  The object to be removed is not in the table.
       *  The action is not authorized to modify the table.
       *  The given iterator is invalid.
       * 
       *  Example:
       *  @code
       *  #include <eosiolib/eosio.hpp>
       *  using namespace eosio;
       *  using namespace std;
       *  class addressbook: contract {
       *    struct address {
       *       uint64_t account_name;
       *       string first_name;
       *       string last_name;
       *       string street;
       *       string city;
       *       string state;
       *       uint64_t primary_key() const { return account_name; }
       *       EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state) )
       *    };
       *    public:
       *      addressbook(account_name self):contract(self) {}
       *      typedef eosio::multi_index< N(address), address > address_index;
       *      void myaction() {
       *        address_index addresses(_self, _self); // code, scope
       *        // add to table, first argument is account to bill for storage
       *        addresses.emplace(_self, [&](auto& address) {
       *          address.account_name = N(dan);
       *          address.first_name = "Daniel";
       *          address.last_name = "Larimer";
       *          address.street = "1 EOS Way";
       *          address.city = "Blacksburg";
       *          address.state = "VA";
       *        });
       *        auto itr = addresses.find(N(dan));
       *        eosio_assert(itr != addresses.end(), "Record is not found");
       *        addresses.erase(*itr);
       *        itr = addresses.find(N(dan));   
       *        eosio_assert(itr == addresses.end(), "Record is not deleted");
       *      } 
       *  }
       *  EOSIO_ABI( addressbook, (myaction) )
       *  @endcode
       */
      void erase( const T& obj ) {
         using namespace _multi_index_detail;

         const auto& objitem = static_cast<const item&>(obj);
         eosio_assert( objitem.__idx == this, "object passed to erase is not in multi_index" );
         eosio_assert( _code == current_receiver(), "cannot erase objects in table of another contract" ); // Quick fix for mutating db using multi_index that shouldn't allow mutation. Real fix can come in RC2.

         auto pk = objitem.primary_key();
         auto itr2 = std::find_if(_items_vector.rbegin(), _items_vector.rend(), [&](const item_ptr& ptr) {
            return ptr._item->primary_key() == pk;
         });

         eosio_assert( itr2 != _items_vector.rend(), "attempt to remove object that was not in multi_index" );

         _items_vector.erase(--(itr2.base()));

         db_remove_i64( objitem.__primary_itr );

         hana::for_each( _indices, [&]( auto& idx ) {
            typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

            auto i = objitem.__iters[index_type::number()];
            if( i < 0 ) {
              typename index_type::secondary_key_type secondary;
              i = secondary_index_db_functions<typename index_type::secondary_key_type>::db_idx_find_primary( _code, _scope, index_type::name(), objitem.primary_key(),  secondary );
            }
            if( i >= 0 )
               secondary_index_db_functions<typename index_type::secondary_key_type>::db_idx_remove( i );
         });
      }

};
  /// @}
}  /// eosio
