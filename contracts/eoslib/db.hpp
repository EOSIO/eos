#pragma once

extern "C" {
#include <eoslib/db.h>
}



/**
 *  @ingroup databaseCpp
struct Db
{

   template<typename T>
   static bool get( TableName table, uint64_t key, T& value ){
      return get( currentCode(), table, key, value );
   }

   template<typename T>
   static bool get( AccountName scope, TableName table, uint64_t key, T& value ){
      return get( scope, currentCode(), table, key, value );
   }

   template<typename T>
   static bool get( AccountName scope, AccountName code, TableName table, uint64_t key, T& result ) {
      auto read = load_i64( scope, code, table, key, &result, sizeof(result) );
      return read > 0;
   }

   template<typename T>
   static int32_t store( TableName table, uint64_t key, const T& value ) {
      return store( currentCode(), key, value );
   }

   template<typename T>
   static int32_t store( Name scope, TableName table, const T& value ) {
      return store_i64( scope, table,  &value, sizeof(value) );
   }

   static bool remove( Name scope, TableName table, uint64_t key ) {
      return remove_i64( scope, table, key );
   }
};
 */


template<int Primary, int Secondary>
struct table_impl{};

template<>
struct table_impl<sizeof(uint128_t),sizeof(uint128_t)> {
    static int32_t front_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return front_primary_i128i128( scope, code, table, data, len );
    }
    static int32_t back_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return back_primary_i128i128( scope, code, table, data, len );
    }
    static int32_t load_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return load_primary_i128i128( scope, code, table, data, len );
    }
    static int32_t front_secondary( AccountName scope, AccountName code, TableName table, void* data, uint32_t len ) {
       return front_secondary_i128i128( scope, code, table,  data, len );
    }
    static int32_t back_secondary( AccountName scope, AccountName code, TableName table, void* data, uint32_t len ) {
       return back_secondary_i128i128( scope, code, table,  data, len );
    }

    static bool remove( uint64_t scope, uint64_t table, const void* data ) {
       return remove_i128i128( scope, table, data );
    }
    static bool store( AccountName scope, TableName table, const void* data, uint32_t len ) {
       return store_i128i128( scope, table, data, len );
    }
};

template<>
struct table_impl<sizeof(uint64_t),0> {
    static int32_t front_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return front_i64( scope, code, table, data, len );
    }
    static int32_t back_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return back_i64( scope, code, table, data, len );
    }
    static bool remove( uint64_t scope, uint64_t table, const void* data ) {
       return remove_i64( scope, table, *((uint64_t*)data) );
    }
    static int32_t load_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return load_i64( scope, code, table, data, len );
    }
    static bool store( AccountName scope, TableName table, const void* data, uint32_t len ) {
       return store_i64( scope, table, data, len );
    }
};


/**
 *  @class Table
 *  @brief defines a type-safe C++ wrapper around the @ref databaseC 
 *
 *  @tparam scope         - the default account name/scope that this table is located within
 *  @tparam code          - the code account name which has write permission to this table
 *  @tparam table         - a unique identifier (name) for this table
 *  @tparam Record        - the type of data stored in each row
 *  @tparam PrimaryType   - the type of the first field stored in @ref Record
 *  @tparam SecondaryType - the type of the second field stored in @ref Record
 *
 *  The primary and secondary indicies are sorted as N-bit unsigned integers from lowest to highest.
 *
 *  @ingroup databaseCpp
 */
template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename PrimaryType, typename SecondaryType = void>
struct Table {
   private:
   typedef table_impl<sizeof( PrimaryType ), sizeof( SecondaryType )> impl;
   static_assert( sizeof(PrimaryType) + sizeof(SecondaryType) <= sizeof(Record), "invalid template parameters" );

   public:
   typedef PrimaryType Primary;
   typedef SecondaryType Secondary;


   struct PrimaryIndex {

      static bool front( Record& r, uint64_t s = scope ) {
         return impl::front_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool back( Record& r, uint64_t s = scope ) {
         return impl::back_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool next( Record& r, uint64_t s = scope ) {
         return impl::next_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool previous( Record& r, uint64_t s = scope ) {
         return impl::previous_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
         *reinterpret_cast<PrimaryType*>(&r) = p;
         return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool lower_bound( const PrimaryType& p, Record& r ) {
         return impl::lower_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool upper_bound( const PrimaryType& p, Record& r ) {
         return impl::upper_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool remove( const Record& r, uint64_t s = scope ) {
         return impl::remove( s, table, &r );
      }
   };

   struct SecondaryIndex {
      static bool front( Record& r, uint64_t s = scope ) {
         return impl::front_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool back( Record& r, uint64_t s = scope ) {
         return impl::back_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool next( Record& r, uint64_t s = scope ) {
         return impl::next_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool previous( Record& r, uint64_t s = scope ) {
         return impl::previous_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool get( const SecondaryType& p, Record& r, uint64_t s = scope ) {
         return impl::load_secondary( s, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool lower_bound( const SecondaryType& p, Record& r, uint64_t s = scope ) {
         return impl::lower_bound_secondary( s, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool upper_bound( const SecondaryType& p, Record& r, uint64_t s = scope ) {
         return impl::upper_bound_secondary( s, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool remove( const Record& r, uint64_t s = scope ) {
         return impl::remove( s, table, &r );
      }
   };

   static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
      *reinterpret_cast<PrimaryType*>(&r) = p;
      return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }

   static bool store( const Record& r, uint64_t s = scope ) {
      assert( impl::store( s, table, &r, sizeof(r) ), "error storing record" );
      return true;
   }
   static bool remove( const Record& r, uint64_t s = scope ) {
      return impl::remove( s, table, &r );
   }
};


/** 
 *  @brief this specialization of Table is for single-index tables
 *
 *  @tparam scope - the default account name scope that this table is located within
 *  @tparam code  - the code account name which has write permission to this table
 *  @tparam table - a unique identifier (name) for this table
 *  @tparam Record - the type of data stored in each row
 *  @tparam PrimaryType - the type of the first field stored in @ref Record
 *
 *  @ingroup databaseCpp
 */
template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename PrimaryType>
struct Table<scope,code,table,Record,PrimaryType,void> {
   private:
   typedef table_impl<sizeof( PrimaryType ),0> impl;
   static_assert( sizeof(PrimaryType) <= sizeof(Record), "invalid template parameters" );

   public:
   typedef PrimaryType Primary;

   struct PrimaryIndex {
      static bool front( Record& r ) {
         return impl::front_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool back( Record& r ) {
         return impl::back_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool next( Record& r ) {
         return impl::next_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool previous( Record& r ) {
         return impl::previous_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
         *reinterpret_cast<PrimaryType*>(&r) = p;
         return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool lower_bound( const PrimaryType& p, Record& r ) {
         return impl::lower_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool upper_bound( const PrimaryType& p, Record& r ) {
         return impl::upper_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }
      static bool remove( const Record& r ) {
         return impl::remove( scope, table, &r );
      }
   };

   static bool front( Record& r ) { return PrimaryIndex::front(r); }
   static bool back( Record& r )  { return PrimaryIndex::back(r);  }

   static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
      *reinterpret_cast<PrimaryType*>(&r) = p;
      return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }

   static bool get( Record& r, uint64_t s = scope ) {
      return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }

   static bool store( const Record& r, uint64_t s = scope ) {
      return impl::store( s, table, &r, sizeof(r) );
   }

   static bool remove( const Record& r, uint64_t s = scope ) {
      return impl::remove( s, table, &r );
   }

};


/**
 *  @ingroup databaseCpp
 */
#define TABLE2(NAME, SCOPE, CODE, TABLE, TYPE, PRIMARY_NAME, PRIMARY_TYPE, SECONDARY_NAME, SECONDARY_TYPE) \
   using NAME = Table<N(SCOPE),N(CODE),N(TABLE),TYPE,PRIMARY_TYPE,SECONDARY_TYPE>; \
   typedef NAME::PrimaryIndex PRIMARY_NAME; \
   typedef NAME::SecondaryIndex SECONDARY_NAME; 

