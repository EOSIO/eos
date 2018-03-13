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

extern "C" void __cxa_pure_virtual() { eosio_assert(false, "pure virtual function called"); }

namespace bmi = boost::multi_index;
using boost::multi_index::const_mem_fun;

/*
template<class Class,typename Type,Type (Class::*PtrToMemberFunction)()const>
struct const_mem_fun
{
  typedef typename std::remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

  typename std::enable_if<
    !std::is_convertible<const ChainedPtr&,const Class&>::value,Type>::type
  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  Type operator()(const Class& x)const
  {
    return (x.*PtrToMemberFunction)();
  }

  Type operator()(const std::reference_wrapper<const Class>& x)const
  {
    return operator()(x.get());
  }

  Type operator()(const std::reference_wrapper<Class>& x)const
  {
    return operator()(x.get());
  }
};
*/

namespace hana = boost::hana;

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

template<uint64_t IndexName, typename Extractor>
struct indexed_by {
   enum constants { index_name   = IndexName };
   typedef Extractor secondary_extractor_type;
};

class multi_index_base;

class table_row
{
   public:
      table_row()
      : __multidx(nullptr), __primary_itr(-1)
      {}

      virtual ~table_row() {}
      virtual uint64_t primary_key()const = 0;

   protected:
      virtual int32_t* __get_ptr_to_secondary_iterator( uint32_t ) { return nullptr; }

      template<typename SecondaryKeyType>
      friend class _index;

   private:
      const multi_index_base* __multidx;
      int32_t                 __primary_itr;

      friend class multi_index_base;

      template<uint64_t TableName, typename U, typename... Indices>
      friend class multi_index;
};

class multi_index_base
{
   protected:

      struct item
      {
         item( table_row& o, uint64_t pk, int32_t pi ): obj_ptr(&o),primary_key(pk),primary_itr(pi) {}

         table_row* obj_ptr;
         uint64_t   primary_key;
         int32_t    primary_itr;
      };

      uint64_t _code;
      uint64_t _scope;
      uint64_t _table;

      mutable uint64_t _next_primary_key;

      enum next_primary_key_tags : uint64_t {
         no_available_primary_key = static_cast<uint64_t>(-2), // Must be the smallest uint64_t value compared to all other tags
         unset_next_primary_key = static_cast<uint64_t>(-1)
      };

      mutable std::vector<item> _items;

      const table_row& load_object_by_primary_iterator( int32_t iterator )const {
         auto itr = std::find_if(_items.rbegin(), _items.rend(), [&](const item& objitem) {
            return objitem.primary_itr == iterator;
         });
         if( itr != _items.rend() )
            return *itr->obj_ptr;

         auto size = db_get_i64( iterator, nullptr, 0 );
         eosio_assert( size >= 0, "error reading iterator" );
         char tmp[size];
         db_get_i64( iterator, tmp, uint32_t(size) );

         auto ptr = construct_object_from_buffer(tmp, uint32_t(size));
         ptr->__multidx     = this;
         ptr->__primary_itr = iterator;

         _items.emplace_back( *ptr, ptr->primary_key(), ptr->__primary_itr );
         return *ptr;
      }

      struct const_iterator {
         public:
            const table_row& operator*()const { return *_item; }
            const table_row* operator->()const { return _item; }

            void next() {
               eosio_assert( _item != nullptr, "cannot increment end iterator" );

               uint64_t next_pk;
               auto next_itr = db_next_i64( _item->__primary_itr, &next_pk );
               if( next_itr < 0 )
                  _item = nullptr;
               else
                  _item = &_multidx->load_object_by_primary_iterator( next_itr );
            }

            void previous() {
               uint64_t prev_pk;
               int prev_itr = -1;

               if( !_item ) {
                  auto ei = db_end_i64(_multidx->get_code(), _multidx->get_scope(), _multidx->get_table());
                  eosio_assert( ei != -1, "cannot decrement end iterator when the table is empty" );
                  prev_itr = db_previous_i64( ei , &prev_pk );
                  eosio_assert( prev_itr >= 0, "cannot decrement end iterator when the table is empty" );
               } else {
                  prev_itr = db_previous_i64( _item->__primary_itr, &prev_pk );
                  eosio_assert( prev_itr >= 0, "cannot decrement iterator at beginning of table" );
               }

               _item = &_multidx->load_object_by_primary_iterator( prev_itr );
            }

            const_iterator( const multi_index_base& mi, const table_row* i = nullptr )
            : _multidx(&mi), _item(i) {}

            const multi_index_base* _multidx;
            const table_row*        _item;

      }; /// struct multi_index_base::const_iterator

      const_iterator _cend()const {
         return { *this };
      }

      const_iterator _lower_bound( uint64_t primary )const {
         auto itr = db_lowerbound_i64( _code, _scope, _table, primary );
         if( itr < 0 ) return _cend();
         auto& obj = load_object_by_primary_iterator( itr );
         return {*this, &obj};
      }

      const_iterator _upper_bound( uint64_t primary )const {
         auto itr = db_upperbound_i64( _code, _scope, _table, primary );
         if( itr < 0 ) return _cend();
         auto& obj = load_object_by_primary_iterator( itr );
         return {*this, &obj};
      }

      const_iterator _emplace( uint64_t payer, table_row& obj, const char* buf, uint32_t buf_len ) {
         auto pk = obj.primary_key();

         obj.__multidx = this;
         obj.__primary_itr = db_store_i64( _scope, _table, payer, pk, buf, buf_len );

         if( pk >= _next_primary_key )
            _next_primary_key = (pk >= no_available_primary_key) ? no_available_primary_key : (pk + 1);

         return {*this, &obj};
      }

      void _modify( uint64_t payer, table_row& obj, const char* buf, uint32_t buf_len ) {
         db_update_i64( obj.__primary_itr, payer, buf, buf_len );

         auto pk = obj.primary_key();
         if( pk >= _next_primary_key )
            _next_primary_key = (pk >= no_available_primary_key) ? no_available_primary_key : (pk + 1);
      }

      bool _erase( const table_row& obj ) {
         //auto& mutableitem = const_cast<item&>(objitem);
         eosio_assert( obj.__multidx == this, "invalid object" );

         db_remove_i64( obj.__primary_itr );

         auto itr = std::find_if(_items.rbegin(), _items.rend(), [&](const item& i) {
            return i.primary_itr == obj.__primary_itr;
         });

         if( itr == _items.rend() ) return true;

         _items.erase( --(itr.base()) );
         return true;
      }

      const_iterator _find( uint64_t primary )const {
         auto itr = std::find_if(_items.rbegin(), _items.rend(), [&](const item& i) {
            return i.primary_key == primary;
         });
         if( itr != _items.rend() )
            return {*this, itr->obj_ptr};

         int32_t iterator = db_find_i64( _code, _scope, _table, primary );
         if( iterator < 0 ) return _cend();

         const auto& obj = load_object_by_primary_iterator( iterator );
         return {*this, &obj};
      }

      template<typename SecondaryKeyType>
      friend class _index;

   public:

      multi_index_base( uint64_t code, uint64_t scope, uint64_t table )
      :_code(code),_scope(scope),_table(table),_next_primary_key(unset_next_primary_key)
      {
         _items.reserve(8);
      }

      virtual ~multi_index_base() {}

      virtual std::unique_ptr<table_row> construct_object_from_buffer( const char* buf, uint32_t buf_len )const = 0;

      uint64_t get_code()const { return _code; }
      uint64_t get_scope()const { return _scope; }
      uint64_t get_table()const { return _table; }

      /**
       * Ideally this method would only be used to determine the appropriate primary key to use within new objects added to a
       * table in which the primary keys of the table are strictly intended from the beginning to be autoincrementing and
       * thus will not ever be set to custom arbitrary values by the contract.
       * Violating this agreement could result in the table appearing full when in reality there is plenty of space left.
       */
      uint64_t available_primary_key()const {
         if( _next_primary_key == unset_next_primary_key ) {
            // This is the first time available_primary_key() is called for this multi_index instance.
            auto itr = _cend(); // end()
            auto begin = _lower_bound( std::numeric_limits<uint64_t>::lowest() );
            if( begin._item == itr._item  ) { // empty table ( begin() == end() )
               _next_primary_key = 0;
            } else {
               itr.previous(); // --itr;
               auto pk = itr._item->primary_key(); // largest primary key currently in table
               if( pk >= no_available_primary_key ) // Reserve the tags
                  _next_primary_key = no_available_primary_key;
               else
                  _next_primary_key = pk + 1;
            }
         }

         eosio_assert( _next_primary_key < no_available_primary_key, "next primary key in table is at autoincrement limit");
         return _next_primary_key;
      }

}; /// class multi_index_base

template<typename SecondaryKeyType>
class _index
{
   protected:
      const multi_index_base* _multidx;
      uint64_t                _index_table_name;
      uint32_t                _number;

   public:
      typedef SecondaryKeyType secondary_key_type;

      _index(const multi_index_base& multidx, uint64_t index_table_name, uint32_t number)
      : _multidx(&multidx), _index_table_name(index_table_name), _number(number)
      {}

      uint64_t get_code()const  { return _multidx->get_code(); }
      uint64_t get_scope()const { return _multidx->get_scope(); }
      uint64_t name()const      { return _index_table_name; }
      uint32_t number()const    { return _number; }

      static int32_t store( uint64_t scope, uint64_t name, uint64_t payer, uint64_t pk, const secondary_key_type& secondary ) {
         return db_idx_store( scope, name, payer, pk, secondary );
      }

      static void update( int32_t iterator, uint64_t payer, const secondary_key_type& secondary ) {
         db_idx_update( iterator, payer, secondary );
      }

      static int32_t find_primary( uint64_t code, uint64_t scope, uint64_t name, uint64_t primary, secondary_key_type& secondary ) {
         return db_idx_find_primary( code, scope, name, primary,  secondary );
      }

      static void remove( int32_t itr ) {
         secondary_iterator<secondary_key_type>::db_idx_remove( itr );
      }

      static int32_t find_secondary( uint64_t code, uint64_t scope, uint64_t name, secondary_key_type& secondary, uint64_t& primary ) {
         return db_idx_find_secondary( code, scope, name, secondary, primary );
      }

   protected:
      struct const_iterator {
         public:
            const table_row& operator*()const { return *_item; }
            const table_row* operator->()const { return _item; }

            void next() {
               eosio_assert( _item != nullptr, "cannot increment end iterator" );

               auto ptr = const_cast<table_row*>(_item)->__get_ptr_to_secondary_iterator(_idx->number());
               eosio_assert( ptr != nullptr, "invariant failure" );
               auto& i = *ptr;

               if( i == -1 ) {
                  secondary_key_type temp_secondary_key;
                  auto idxitr = db_idx_find_primary(_idx->get_code(), _idx->get_scope(), _idx->name(),
                                                    _item->primary_key(), temp_secondary_key);
                  i = idxitr;
               }

               uint64_t next_pk = 0;
               auto next_itr = secondary_iterator<secondary_key_type>::db_idx_next( i, &next_pk );
               if( next_itr < 0 ) {
                  _item = nullptr;
                  return;
               }

               auto& obj = const_cast<table_row&>(*_idx->_multidx->_find(next_pk));
               auto ptr2 = const_cast<table_row&>(obj).__get_ptr_to_secondary_iterator(_idx->number());
               eosio_assert( ptr2 != nullptr, "invariant failure" );
               *ptr2 = next_itr;
               _item = &obj;
            }

            void previous() {
               uint64_t prev_pk = 0;
               int prev_itr = -1;

               if( !_item ) {
                  auto ei = secondary_iterator<secondary_key_type>::db_idx_end(_idx->get_code(), _idx->get_scope(), _idx->name());
                  eosio_assert( ei != -1, "cannot decrement end iterator when the index is empty" );
                  prev_itr = secondary_iterator<secondary_key_type>::db_idx_previous( ei , &prev_pk );
                  eosio_assert( prev_itr >= 0, "cannot decrement end iterator when the index is empty" );
               } else {
                  auto ptr = const_cast<table_row*>(_item)->__get_ptr_to_secondary_iterator(_idx->number());
                  eosio_assert( ptr != nullptr, "invariant failure" );
                  auto& i = *ptr;

                  if( i == -1 ) {
                     secondary_key_type temp_secondary_key;
                     auto idxitr = db_idx_find_primary(_idx->get_code(), _idx->get_scope(), _idx->name(),
                                                       _item->primary_key(), temp_secondary_key);
                     i = idxitr;
                  }
                  prev_itr = secondary_iterator<secondary_key_type>::db_idx_previous( i, &prev_pk );
                  eosio_assert( prev_itr >= 0, "cannot decrement iterator at beginning of index" );
               }

               const auto& obj = *_idx->_multidx->_find( prev_pk );
               auto ptr2 = const_cast<table_row&>(obj).__get_ptr_to_secondary_iterator(_idx->number());
               eosio_assert( ptr2 != nullptr, "invariant failure" );
               *ptr2 = prev_itr;
               _item = &obj;
            }

            const_iterator( const _index& idx, const table_row* i = nullptr )
            : _idx(&idx), _item(i) {}

            const _index*    _idx;
            const table_row* _item;
      }; /// struct _index::const_iterator

      const_iterator _cend()const {
         return { *this };
      }

      const_iterator _lower_bound( const secondary_key_type& secondary )const {
         uint64_t primary = 0;
         secondary_key_type secondary_copy(secondary);
         auto itr = db_idx_lowerbound( get_code(), get_scope(), name(), secondary_copy, primary );
         if( itr < 0 ) return _cend();

         const auto& obj = *_multidx->_find( primary );
         auto ptr = const_cast<table_row&>(obj).__get_ptr_to_secondary_iterator(_number);
         eosio_assert( ptr != nullptr, "invariant failure" );
         *ptr = itr;

         return {*this, &obj};
      }

      const_iterator _upper_bound( const secondary_key_type& secondary )const {
         uint64_t primary = 0;
         secondary_key_type secondary_copy(secondary);
         auto itr = db_idx_upperbound( get_code(), get_scope(), name(), secondary_copy, primary );
         if( itr < 0 ) return _cend();

         const auto& obj = *_multidx->_find( primary );
         auto ptr = const_cast<table_row&>(obj).__get_ptr_to_secondary_iterator(number());
         eosio_assert( ptr != nullptr, "invariant failure" );
         *ptr = itr;

         return {*this, &obj};
      }

      const_iterator _iterator_to( const table_row& obj )const {
         eosio_assert( obj.__multidx == _multidx, "object passed to iterator_to is not in multi_index" );

         auto ptr = const_cast<table_row&>(obj).__get_ptr_to_secondary_iterator(number());
         eosio_assert( ptr != nullptr, "invariant failure" );
         auto& i = *ptr;

         if( i == -1 ) {
            secondary_key_type temp_secondary_key;
            auto idxitr = db_idx_find_primary(get_code(), get_scope(), name(), obj.primary_key(), temp_secondary_key);
            i = idxitr;
         }

         return {*this, &obj};
      }
}; /// class _index

template<typename T, uint32_t NumSecondaryIndices>
class _multi_index_helper : public multi_index_base
{
   public:
      _multi_index_helper(uint64_t code, uint64_t scope, uint64_t table) : multi_index_base(code, scope, table) {}

   protected:

      struct _item : public T
      {
         public:
            _item() : T() {
               for( uint32_t i = 0; i < NumSecondaryIndices; ++i )
                  __iters[i] = -1;
            }

            int32_t __iters[NumSecondaryIndices + (NumSecondaryIndices == 0)];

         protected:
            virtual int32_t* __get_ptr_to_secondary_iterator(uint32_t number) override {
               eosio_assert( number < NumSecondaryIndices, "secondary index sequence number is out of bounds" );
               return &__iters[number];
            }
      };

      virtual std::unique_ptr<table_row> construct_object_from_buffer( const char* buf, uint32_t buf_len )const override {
         auto ptr = std::make_unique<typename std::conditional<NumSecondaryIndices == 0, T, _item>::type>();
         datastream<const char*> ds(buf, buf_len);
         ds >> *ptr;
         return ptr;
      }
};

template<uint64_t TableName, typename T, typename... Indices>
class multi_index : public _multi_index_helper<T, sizeof...(Indices)>
{
   private:

      static_assert( sizeof...(Indices) <= 16, "multi_index only supports a maximum of 16 secondary indices" );

      static_assert( std::is_base_of<table_row, T>::value, "type T must derive from eosio::table_row" );

      static_assert( std::is_default_constructible<T>::value, "type T must be default constructible" );

      constexpr static bool validate_table_name( uint64_t n ) {
         // Limit table names to 12 characters so that the last character (4 bits) can be used to distinguish between the secondary indices.
         return (n & 0x000000000000000FULL) == 0;
      }

      static_assert( validate_table_name(TableName), "multi_index does not support table names with a length greater than 12");

      template<uint64_t IndexName, typename Extractor, uint32_t Number, bool IsConst>
      class index : protected _index<typename std::decay<decltype( Extractor()(nullptr) )>::type> {
         public:
            typedef Extractor  secondary_extractor_type;
            typedef typename std::decay<decltype( Extractor()(nullptr) )>::type secondary_key_type;

            constexpr static bool validate_index_name( uint64_t n ) {
               return n != 0 && n != N(primary); // Primary is a reserve index name.
            }

            static_assert( validate_index_name(IndexName), "invalid index name used in multi_index" );

            enum constants : uint64_t {
               table_name   = TableName,
               index_name   = IndexName,
               index_number = Number,
               index_table_name = (TableName & 0xFFFFFFFFFFFFFFF0ULL)
                                   | (static_cast<uint64_t>(Number) & 0x000000000000000FULL) // Assuming no more than 16 secondary indices are allowed
            };

            constexpr static uint64_t name()   { return index_table_name; }
            constexpr static uint32_t number() { return Number; }

            struct const_iterator : public std::iterator<std::bidirectional_iterator_tag, const T>,
                                    private _index<secondary_key_type>::const_iterator
            {
               public:
                  friend bool operator == ( const const_iterator& a, const const_iterator& b ) {
                     return a._item == b._item;
                  }
                  friend bool operator != ( const const_iterator& a, const const_iterator& b ) {
                     return a._item != b._item;
                  }

                  const T& operator*()const { return *static_cast<const T*>(_index<secondary_key_type>::const_iterator::_item); }
                  const T* operator->()const { return static_cast<const T*>(_index<secondary_key_type>::const_iterator::_item); }

                  const_iterator operator++(int)const {
                     return ++(const_iterator(*this));
                  }

                  const_iterator operator--(int)const {
                     return --(const_iterator(*this));
                  }

                  const_iterator& operator++() {
                     _index<secondary_key_type>::const_iterator::next();
                     return *this;
                  }

                  const_iterator& operator--() {
                     _index<secondary_key_type>::const_iterator::previous();
                     return *this;
                  }

               private:
                  friend class index;

                  const_iterator( const _index<secondary_key_type>& idx, const T* obj_ptr = nullptr )
                  : _index<secondary_key_type>::const_iterator(idx, obj_ptr) {}

                  const_iterator( const typename _index<secondary_key_type>::const_iterator& itr )
                  : _index<secondary_key_type>::const_iterator(itr) {}
            }; /// struct multi_index::index::const_iterator

            typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

            const_iterator cbegin()const {
               return lower_bound(std::numeric_limits<secondary_key_type>::lowest());
            }
            const_iterator begin()const  { return cbegin(); }

            const_iterator cend()const   { return const_iterator( *this ); }
            const_iterator end()const    { return cend(); }

            const_reverse_iterator crbegin()const { return std::make_reverse_iterator(cend()); }
            const_reverse_iterator rbegin()const  { return crbegin(); }

            const_reverse_iterator crend()const   { return std::make_reverse_iterator(cbegin()); }
            const_reverse_iterator rend()const    { return crend(); }

            const_iterator find( secondary_key_type&& secondary )const {
               auto lb = lower_bound( secondary );
               auto e = end();
               if( lb == e ) return e;

               if( secondary != secondary_extractor_type()(*lb) )
                  return e;
               return lb;
            }

            const_iterator lower_bound( secondary_key_type&& secondary )const {
               return lower_bound( secondary );
            }

            const_iterator lower_bound( const secondary_key_type& secondary )const {
               return {_index<secondary_key_type>::_lower_bound(secondary)};
            }

            const_iterator upper_bound( secondary_key_type&& secondary )const {
               return upper_bound( secondary );
            }

            const_iterator upper_bound( const secondary_key_type& secondary )const {
               return {_index<secondary_key_type>::_upper_bound(secondary)};
            }

            const_iterator iterator_to( const T& obj )const {
               return {_index<secondary_key_type>::_iterator_to(obj)};
            }

            template<typename Lambda>
            typename std::enable_if<!IsConst, void>::type
            modify( const_iterator itr, uint64_t payer, Lambda&& updater ) {
               eosio_assert( itr != end(), "cannot pass end iterator to modify" );

               static_cast< multi_index<TableName, T, Indices...>* >(const_cast<multi_index_base*>(_index<secondary_key_type>::_multidx))->
                  modify( *itr, payer, std::forward<Lambda&&>(updater) );
            }

            const_iterator erase( const_iterator itr, typename std::enable_if<!IsConst>::type* = nullptr ) {
               eosio_assert( itr != end(), "cannot pass end iterator to erase" );

               const auto& obj = *itr;
               ++itr;

               static_cast<  multi_index<TableName, T, Indices...>* >(const_cast<multi_index_base*>(_index<secondary_key_type>::_multidx))->erase(obj);

               return itr;
            }

            static auto extract_secondary_key(const T& obj) { return secondary_extractor_type()(obj); }

         private:
            friend class multi_index;

            index( typename std::conditional<IsConst, const multi_index&, multi_index&>::type midx )
            : _index<secondary_key_type>(midx, index_table_name, Number) {}

      }; /// struct multi_index::index

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

   public:

      multi_index( uint64_t code, uint64_t scope ) : _multi_index_helper<T, sizeof...(Indices)>(code, scope, TableName) {}

      struct const_iterator : public std::iterator<std::bidirectional_iterator_tag, const T>,
                              private multi_index_base::const_iterator
      {
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
            next();
            return *this;
         }
         const_iterator& operator--() {
            previous();
            return *this;
         }

         private:
            friend class multi_index;

            const_iterator( const multi_index& mi, const T* obj_ptr = nullptr )
            : multi_index_base::const_iterator(mi, obj_ptr) {}

            const_iterator( const multi_index_base::const_iterator& itr )
            : multi_index_base::const_iterator(itr) {}
      }; /// struct multi_index::const_iterator

      typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

      const_iterator cbegin()const {
         return lower_bound(std::numeric_limits<uint64_t>::lowest());
      }
      const_iterator begin()const  { return cbegin(); }

      const_iterator cend()const   { return const_iterator( *this ); }
      const_iterator end()const    { return cend(); }

      const_reverse_iterator crbegin()const { return std::make_reverse_iterator(cend()); }
      const_reverse_iterator rbegin()const  { return crbegin(); }

      const_reverse_iterator crend()const   { return std::make_reverse_iterator(cbegin()); }
      const_reverse_iterator rend()const    { return crend(); }

      const_iterator lower_bound( uint64_t primary )const {
         return {multi_index_base::_lower_bound(primary)};
      }

      const_iterator upper_bound( uint64_t primary )const {
         return {multi_index_base::_upper_bound(primary)};
      }

      template<uint64_t IndexName>
      auto get_index() {
        auto res = boost::hana::find_if( _indices, []( auto&& in ) {
            return std::integral_constant<bool, std::decay<typename decltype(+hana::at_c<0>(in))::type>::type::index_name == IndexName>();
        });

        static_assert( res != hana::nothing, "name provided is not the name of any secondary index within multi_index" );

        return typename decltype(+hana::at_c<0>(res.value()))::type(*this);
      }

      template<uint64_t IndexName>
      auto get_index()const {
        auto res = boost::hana::find_if( _indices, []( auto&& in ) {
            return std::integral_constant<bool, std::decay<typename decltype(+hana::at_c<1>(in))::type>::type::index_name == IndexName>();
        });

        static_assert( res != hana::nothing, "name provided is not the name of any secondary index within multi_index" );

        return typename decltype(+hana::at_c<1>(res.value()))::type(*this);
      }

      const_iterator iterator_to( const T& obj )const {
         eosio_assert( obj.__multidx == this, "object passed to iterator_to is not in multi_index" );
         return {*this, &obj};
      }

      template<typename Lambda>
      const_iterator emplace( uint64_t payer, Lambda&& constructor ) {
         auto ptr = std::make_unique<T>();
         constructor( *ptr );

         char tmp[ pack_size( *ptr ) ];
         datastream<char*> ds( tmp, sizeof(tmp) );
         ds << *ptr;

         auto itr = multi_index_base::_emplace( payer, *ptr, tmp, sizeof(tmp) );

         auto& i = static_cast<typename _multi_index_helper<T, sizeof...(Indices)>::_item&>(const_cast<table_row&>(*itr._item));

         boost::hana::for_each( _indices, [&]( auto& idx ) {
            typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

             i.__iters[index_type::number()] = index_type::store( multi_index_base::get_scope(), index_type::name(),
                                                                  payer, ptr->primary_key(),
                                                                  index_type::extract_secondary_key(*ptr) );
         });

         return {itr};
      }

      template<typename Lambda>
      void modify( const_iterator itr, uint64_t payer, Lambda&& updater ) {
         eosio_assert( itr != end(), "cannot pass end iterator to update" );

         modify( *itr, payer, std::forward<Lambda&&>(updater) );
      }

      template<typename Lambda>
      void modify( const T& obj, uint64_t payer, Lambda&& updater ) {
         eosio_assert( obj.__multidx == this, "invalid object" );

         auto secondary_keys = boost::hana::transform( _indices, [&]( auto&& idx ) {
            typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

            return index_type::extract_secondary_key( obj );
         });

         auto pk = obj.primary_key();

         auto& mutableobj = const_cast<T&>(obj); // Do not forget the auto& otherwise it would make a copy and thus not update at all.
         updater( mutableobj );

         eosio_assert( pk == obj.primary_key(), "updater cannot change primary key when modifying an object" );

         char tmp[ pack_size( obj ) ];
         datastream<char*> ds( tmp, sizeof(tmp) );
         ds << obj;

         multi_index_base::_modify( payer, mutableobj, tmp, sizeof(tmp) );

         auto& i = static_cast<typename _multi_index_helper<T, sizeof...(Indices)>::_item&>(mutableobj);

         boost::hana::for_each( _indices, [&]( auto& idx ) {
            typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

            auto secondary = index_type::extract_secondary_key( obj );
            if( hana::at_c<index_type::index_number>(secondary_keys) != secondary ) {
               auto indexitr = i.__iters[index_type::number()];

               if( indexitr < 0 )
                  indexitr = i.__iters[index_type::number()]
                           = index_type::find_primary( multi_index_base::get_code(), multi_index_base::get_scope(),
                                                       index_type::name(), pk, secondary );

               index_type::update( indexitr, payer, secondary );
            }
         });
      }

      const T& get( uint64_t primary )const {
         auto result = find( primary );
         eosio_assert( result != cend(), "unable to find key" );
         return *result;
      }

      const_iterator find( uint64_t primary )const {
         return {multi_index_base::_find(primary)};
      }

      const_iterator erase( const_iterator itr ) {
         eosio_assert( itr != cend(), "cannot pass end iterator to erase" );

         const auto& obj = *itr;
         ++itr;

         erase(obj);

         return itr;
      }

      void erase( const T& obj ) {
         if( !multi_index_base::_erase(obj) ) return;

         auto pk = obj.primary_key();

         const auto& i = static_cast<const typename _multi_index_helper<T, sizeof...(Indices)>::_item&>(obj);

         boost::hana::for_each( _indices, [&]( auto& idx ) {
            typedef typename decltype(+hana::at_c<0>(idx))::type index_type;

            auto itr = i.__iters[index_type::number()];
            if( itr < 0 ) {
              typename index_type::secondary_key_type second;
              itr = index_type::find_primary( multi_index_base::get_code(), multi_index_base::get_scope(),
                                              index_type::name(), pk, second );
            }
            if( itr >= 0 )
              index_type::remove( itr );
         });
      }

}; /// class multi_index

}  /// eosio
