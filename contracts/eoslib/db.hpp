#pragma once

#include <eoslib/db.h>



/**
 *  @ingroup databaseCpp
 *  Cpp implementation of database API. It is based on pimpl idiom.
 *  @see Table class and table_impl class
 */

template<int Primary, int Secondary>
struct table_impl{};

template<>
struct table_impl<sizeof(uint128_t),sizeof(uint128_t)> {

   /**
   *  @param scope - the account scope that will be read, must exist in the transaction scopes list
   *  @param code  - identifies the code that controls write-access to the data
   *  @param table - the ID/name of the table within the scope/code context to query
   *  @param data  - location to copy the front record based on primary index.
   *  @param len - the maximum length of data to read
   *
   *  @return the number of bytes read or -1 if no record found
   */
    static int32_t front_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return front_primary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the back record based on primary index.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t back_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return back_primary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the data; must be initialized with the primary key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t load_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return load_primary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the data; must be initialized with the primary key to fetch next record.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t next_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return next_primary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the record; must be initialized with the primary key to fetch previous record.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t previous_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return previous_primary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the record. must be initialized with the primary key to fetch upper bound of.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t upper_bound_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return upper_bound_primary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the record. must be initialized with the primary key to fetch lower bound of.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t lower_bound_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return lower_bound_primary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the front record based on secondary index.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t front_secondary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return front_secondary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the back record based on secondary index.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t back_secondary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return back_secondary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the record based on secondary index; must be initialized with secondary key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t load_secondary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return load_secondary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the next record based on secondary index; must be initialized with a key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t next_secondary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return next_secondary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the previous record; must be initialized with a key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t previous_secondary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return previous_secondary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the upper bound record; must be initialized with a key to find the upper bound.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t upper_bound_secondary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return upper_bound_secondary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the lower bound record; must be initialized with a key to find the lower bound.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t lower_bound_secondary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return lower_bound_secondary_i128i128( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - must be initialized with the key (primary, secondary) of the record to remove.
    *
    *  @return 1 if  remove successful else -1
    */
    static int32_t remove( uint64_t scope, uint64_t table, const void* data ) {
       return remove_i128i128( scope, table, data );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - must be initialized with the data to be stored.
    *  @param len - the maximum length of data to be be stored.
    *
    *  @return 1 if store successful else -1
    */
    static int32_t store( AccountName scope, TableName table, const void* data, uint32_t len ) {
       return store_i128i128( scope, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - must be initialized with the data to be updated.
    *  @param len   - the maximum length of data to be updated.
    *
    *  @return 1 if update successful else -1
    */
    static int32_t update( AccountName scope, TableName table, const void* data, uint32_t len ) {
       return update_i128i128( scope, table, data, len );
    }
};

template<>
struct table_impl<sizeof(uint64_t),0> {
    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the front record based on primary index (only index).
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t front_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return front_i64( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the back record based on primary index (only index).
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t back_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return back_i64( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the record; must be initialized with a key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t load_primary( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return load_i64( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the next record; must be initialized with a key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t next( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return next_i64( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the previous record; must be initialized with a key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t previous( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return previous_i64( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the lower bound; must be initialized with a key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t lower_bound( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return lower_bound_i64( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param code  - identifies the code that controls write-access to the data
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - location to copy the upper bound; must be initialized with a key.
    *  @param len - the maximum length of data to read
    *
    *  @return the number of bytes read or -1 if no record found
    */
    static int32_t upper_bound( uint64_t scope, uint64_t code, uint64_t table, void* data, uint32_t len ) {
       return upper_bound_i64( scope, code, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - must be initialized with the key of record to remove from table.
    *
    *  @return 1 if successful else -1
    */
    static int32_t remove( uint64_t scope, uint64_t table, const void* data ) {
       return remove_i64( scope, table, (uint64_t*)data);
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - must be initialized with the data to be stored.
    *  @param len - the maximum length of data
    *
    *  @return 1 if successful else -1
    */
    static int32_t store( AccountName scope, TableName table, const void* data, uint32_t len ) {
       return store_i64( scope, table, data, len );
    }

    /**
    *  @param scope - the account scope that will be read, must exist in the transaction scopes list
    *  @param table - the ID/name of the table within the scope/code context to query
    *  @param data  - must be initialized with the data to be updated.
    *  @param len - the maximum length of data
    *
    *  @return 1 if successful else -1
    */
    static int32_t update( AccountName scope, TableName table, const void* data, uint32_t len ) {
       return update_i64( scope, table, data, len );
    }
};


/**
 *  @class Table
 *  @defgroup dualIndexTable Dual Index Table
 *  @brief defines a type-safe C++ wrapper around the @ref databaseC
 *
 *  @tparam scope         - the default account name/scope that this table is located within
 *  @tparam code          - the code account name which has write permission to this table
 *  @tparam table         - a unique identifier (name) for this table
 *  @tparam Record        - the type of data stored in each row
 *  @tparam PrimaryType   - the type of the first field stored in @ref Record
 *  @tparam SecondaryType - the type of the second field stored in @ref Record
 *
 *  The primary and secondary indices are sorted as N-bit unsigned integers from lowest to highest.
 *
 *  @code
 *  struct Model {
 *      uint64_t primary;
 *      uint64_t secondary;
 *      uint64_t value;
 *  };
 *
 *  typedef Table<N(myscope), N(mycode), N(mytable), Model, uint64_t, uint64_t> MyTable;
 *  Model a { 1, 11, N(first) };
 *  Model b { 2, 22, N(second) };
 *  Model c { 3, 33, N(third) };
 *  Model d { 4, 44, N(fourth) };
 *
 *  bool res = MyTable::store(a);
 *  ASSERT(res, "store");
 *
 *  res = MyTable::store(b);
 *  ASSERT(res, "store");
 *
 *  res = MyTable::store(c);
 *  ASSERT(res, "store");
 *
 *  res = MyTable::store(d);
 *  ASSERT(res, "store");
 *
 *  Model query;
 *  res = MyTable::PrimaryIndex::get(1, query);
 *  ASSERT(res && query.primary == 1 && query.value == N(first), "first");
 *
 *  res = MyTable::PrimaryIndex::front(query);
 *  ASSERT(res && query.primary == 4 && query.value == N(fourth), "front");
 *
 *  res = MyTable::PrimaryIndex::back(query);
 *  ASSERT(res && query.primary == 1 && query.value == N(first), "back");
 *
 *  res = MyTable::PrimaryIndex::previous(query);
 *  ASSERT(res && query.primary == 2 && query.value == N(second), "previous");
 *
 *  res = MyTable::PrimaryIndex::next(query);
 *  ASSERT(res && query.primary == 1 && query.value == N(first), "first");
 *
 *  res = MyTable::SecondaryIndex::get(11, query);
 *  ASSERT(res && query.primary == 11 && query.value == N(first), "first");
 *
 *  res = MyTable::SecondaryIndex::front(query);
 *  ASSERT(res && query.secondary == 44 && query.value == N(fourth), "front");
 *
 *  res = MyTable::SecondaryIndex::back(query);
 *  ASSERT(res && query.secondary == 11 && query.value == N(first), "back");
 *
 *  res = MyTable::SecondaryIndex::previous(query);
 *  ASSERT(res && query.secondary == 22 && query.value == N(second), "previous");
 *
 *  res = MyTable::SecondaryIndex::next(query);
 *  ASSERT(res && query.secondary == 11 && query.value == N(first), "first");
 *
 *  res = MyTable::remove(query);
 *  ASSERT(res, "remove");
 *
 *  res = MyTable::get(query);
 *  ASSERT(!res, "not found already removed");
 *
 *  @endcode
 *  @ingroup databaseCpp
  * @{
 */
template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename PrimaryType, typename SecondaryType = void>
struct Table {
   private:
   typedef table_impl<sizeof( PrimaryType ), sizeof( SecondaryType )> impl;
   static_assert( sizeof(PrimaryType) + sizeof(SecondaryType) <= sizeof(Record), "invalid template parameters" );

   public:
   typedef PrimaryType Primary;
   typedef SecondaryType Secondary;

    /**
     * @brief The Primary Index
    */

   struct PrimaryIndex {
      /**
      *  @param r - reference to a record to store the front record based on primary index.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool front( Record& r, uint64_t s = scope ) {
         return impl::front_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to store the back record based on primary index.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool back( Record& r, uint64_t s = scope ) {
         return impl::back_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to store next value; must be initialized with current.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool next( Record& r, uint64_t s = scope ) {
         return impl::next_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to store previous value; must be initialized with current.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool previous( Record& r, uint64_t s = scope ) {
         return impl::previous_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param p - reference to primary key to load; must be initialized with a value;
      *  @param r - reference to a record to load the value to.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
         *reinterpret_cast<PrimaryType*>(&r) = p;
         return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param p - reference to primary key to get the lower bound of; must be initialized with a value;
       *  @param r - reference to a record to load the value to.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
      static bool lower_bound( const PrimaryType& p, Record& r ) {
         return impl::lower_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param p - reference to primary key to get the upper bound of; must be initialized with a value;
       *  @param r - reference to a record to load the value to.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
      static bool upper_bound( const PrimaryType& p, Record& r ) {
         return impl::upper_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to remove from table;
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successfully removed;
      */
      static bool remove( const Record& r, uint64_t s = scope ) {
         return impl::remove( s, table, &r ) != 0;
      }
   };


     /**
     * @brief The Secondary Index
     */

   struct SecondaryIndex {
       /**
       *  @param r - reference to a record to store the front record based on secondary index.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool front( Record& r, uint64_t s = scope ) {
          return impl::front_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param r - reference to a record to store the back record based on secondary index.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool back( Record& r, uint64_t s = scope ) {
          return impl::back_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param r - reference to a record to return the next record .
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool next( Record& r, uint64_t s = scope ) {
          return impl::next_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param r - reference to a record to return the next record.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool previous( Record& r, uint64_t s = scope ) {
          return impl::previous_secondary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param p - reference to secondary index key
       *  @param r - reference to record to hold the value
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool get( const SecondaryType& p, Record& r, uint64_t s = scope ) {
          return impl::load_secondary( s, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param p - reference to secondary key to get the lower bound of; must be initialized with a value;
       *  @param r - reference to a record to load the value to.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */

       static bool lower_bound( const SecondaryType& p, Record& r, uint64_t s = scope ) {
          return impl::lower_bound_secondary( s, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
        *  @param p - reference to secondary key to get the upper bound of; must be initialized with a value;
        *  @param r - reference to a record to load the value to.
        *  @param s - account scope. default is current scope of the class
        *
        *  @return true if successful read.
        */
       static bool upper_bound( const SecondaryType& p, Record& r, uint64_t s = scope ) {
          return impl::upper_bound_secondary( s, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
        *  @param r - reference to a record to be removed.
        *  @param s - account scope. default is current scope of the class
        *
        *  @return true if successfully removed.
        */
       static bool remove( const Record& r, uint64_t s = scope ) {
          return impl::remove( s, table, &r ) != 0;
       }
    };


    /**
    *  @param p - reference to primary key to retrieve
    *  @param r - reference to a record to load the value to.
    *  @param s - account scope. default is current scope of the class
    *
    *  @return true if successful read.
    */

    static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
       *reinterpret_cast<PrimaryType*>(&r) = p;
       return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
    }

     /**
     *  @param r - reference to a record to store.
     *  @param s - account scope. default is current scope of the class
     *
     *  @return true if successful store.
     */
    static bool store( const Record& r, uint64_t s = scope ) {
       assert( impl::store( s, table, &r, sizeof(r) ), "error storing record" );
       return true;
    }

    /**
    *  @param r - reference to a record to update.
    *  @param s - account scope. default is current scope of the class
    *
    *  @return true if successful update.
    */
    static bool update( const Record& r, uint64_t s = scope ) {
       assert( impl::update( s, table, &r, sizeof(r) ), "error updating record" );
       return true;
    }

    /**
    *  @param r - reference to a record to remove.
    *  @param s - account scope. default is current scope of the class
    *
    *  @return true if successful remove.
    */
    static bool remove( const Record& r, uint64_t s = scope ) {
       return impl::remove( s, table, &r ) != 0;
    }
 };
/// @}

 /**
  * @defgroup Singleindextable Single Index Table
  *  @brief this specialization of Table is for single-index tables
  *
  *  @tparam scope - the default account name scope that this table is located within
  *  @tparam code  - the code account name which has write permission to this table
  *  @tparam table - a unique identifier (name) for this table
  *  @tparam Record - the type of data stored in each row
  *  @tparam PrimaryType - the type of the first field stored in @ref Record
  *
  *  Example
  *  @code
  *
  *  struct MyModel {
  *     uint128_t number;
  *     uint64_t  name;
  *  };
  *
  *  typedef Table<N(myscope), N(mycode), N(mytable), MyModel, uint128_t> MyTable;
  *
  *  MyModel a { 1, N(one) };
  *  MyModel b { 2, N(two) };
  *  MyModel c { 3, N(three) };
  *  MyModel d { 4, N(four) };
  *
  *  bool res = MyTable::store(a);
  *  ASSERT(res, "store");

  *  res = MyTable::store(b);
  *  ASSERT(res, "store");
  *
  *  res = MyTable::store(c);
  *  ASSERT(res, "store");
  *
  *  res = MyTable::store(d);
  *  ASSERT(res, "store");
  *
  *  MyModel query;
  *  res = MyTable::front(query);
  *  ASSERT(res && query.number == 4 && query.name == N(four), "front");
  *
  *  res = MyTable::back(query);
  *  ASSERT(res && query.number == 1 && query.name == N(one), "back");
  *
  *  res = MyTable::PrimaryIndex::previous(query);
  *  ASSERT(res && query.number == 2 && query.name == N(two), "previous");
  *
  *  res = MyTable::PrimaryIndex::next(query);
  *  ASSERT(res && query.number == 1 && query.name == N(one), "next");
  *
  *  query.number = 4;
  *  res = MyTable::get(query);
  *  ASSERT(res && query.number == 4 && query.name = N(four), "get");
  *
  *  query.name = N(Four);
  *  res = MyTable.update(query);
  *  ASSERT(res && query.number == 4 && query.name == N(Four), "update");
  *
  *  res = MyTable.remove(query);
  *  ASSERT(res, "remove");
  *
  *  res = MyTable.get(query);
  *  ASSERT(!res, "get of removed record");
  *
  *  @endcode
  *  @ingroup databaseCpp
  * @{
  */

 template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename PrimaryType>
struct Table<scope,code,table,Record,PrimaryType,void> {
   private:
   typedef table_impl<sizeof( PrimaryType ),0> impl;
   static_assert( sizeof(PrimaryType) <= sizeof(Record), "invalid template parameters" );

   public:
   typedef PrimaryType Primary;
    /**
     * @brief Primary Index of Table
     */
   struct PrimaryIndex {
       /**
       *  @param r - reference to a record to store the front.
       *
       *  @return true if successfully retrieved the front of the table.
       */
      static bool front( Record& r ) {
         return impl::front_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param r - reference to a record to store the back.
       *
       *  @return true if successfully retrieved the back of the table.
       */
      static bool back( Record& r ) {
         return impl::back_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param r - reference to store the next record. Must be initialized with a key.
       *
       *  @return true if successfully retrieved the next record.
       */
      static bool next( Record& r ) {
         return impl::next_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param r - reference to store previous record. Must be initialized with a key.
       *
       *  @return true if successfully retrieved the previous record.
       */
      static bool previous( Record& r ) {
         return impl::previous_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param p - reference to the primary key to retrieve the record.
       *  @param r - reference to hold the result of the query.
        * @param s - scope; defaults to scope of the class.
       *  @return true if successfully retrieved the record.
       */
      static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
         *reinterpret_cast<PrimaryType*>(&r) = p;
         return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param p - reference to the primary key to retrieve the lower bound.
       *  @param r - reference to hold the result of the query.
       *  @return true if successfully retrieved the record.
       */
      static bool lower_bound( const PrimaryType& p, Record& r ) {
         return impl::lower_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param p - reference to the primary key to retrieve the upper bound.
       *  @param r - reference to hold the result of the query.
       *  @return true if successfully retrieved the record.
       */
       static bool upper_bound( const PrimaryType& p, Record& r ) {
         return impl::upper_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param r - reference to record to be removed.
       *  @return true if successfully removed.
       */
       static bool remove( const Record& r ) {
         return impl::remove( scope, table, &r ) != 0;
      }
   };

    /**
    * fetches the front of the table
    * @param  r - reference to hold the value
    * @return true if successfully retrieved the front
    */
    static bool front( Record& r ) { return PrimaryIndex::front(r); }

    /**
    * fetches the back of the table
    * @param  r - reference to hold the value
    * @return true if successfully retrieved the back
    */
    static bool back( Record& r )  { return PrimaryIndex::back(r);  }

    /**
     * retrieves the record for the specified primary key
     * @param p - the primary key of the record to fetch
     * @param r - reference of record to hold return value
     * @param s - scope; defaults to scope of the class.
     * @return true if get succeeds.
     */
   static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
      *reinterpret_cast<PrimaryType*>(&r) = p;
      return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }

   /**
    * retrieves a record based on initialized primary key value
    * @param r - reference of a record to hold return value; must be initialized to the primary key to be fetched.
    * @param s - scope; defaults to scope of the class.
    * @return true if get succeeds.
    */
   static bool get( Record& r, uint64_t s = scope ) {
      return impl::load_primary( s, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }

    /**
     * store a record to the table
     * @param r - the record to be stored.
     * @param s - scope; defaults to scope of the class.
     * @return true if store succeeds.
     */
   static bool store( const Record& r, uint64_t s = scope ) {
      return impl::store( s, table, &r, sizeof(r) ) != 0;
   }

    /**
     * update a record in the table.
     * @param r - the record to be updated including the updated values (cannot update the index).
     * @param s - scope; defaults to scope of the class.
     * @return true if update succeeds.
     */
   static bool update( const Record& r, uint64_t s = scope ) {
      return impl::update( s, table, &r, sizeof(r) ) != 0;
   }

    /**
     * remove a record from the table.
     * @param r - the record to be removed.
     * @param s - scope; defaults to scope of the class.
     * @return true if remove succeeds.
     */
   static bool remove( const Record& r, uint64_t s = scope ) {
      return impl::remove( s, table, &r ) != 0;
   }
}; /// @}


#define TABLE2(NAME, SCOPE, CODE, TABLE, TYPE, PRIMARY_NAME, PRIMARY_TYPE, SECONDARY_NAME, SECONDARY_TYPE) \
   using NAME = Table<N(SCOPE),N(CODE),N(TABLE),TYPE,PRIMARY_TYPE,SECONDARY_TYPE>; \
   typedef NAME::PrimaryIndex PRIMARY_NAME; \
   typedef NAME::SecondaryIndex SECONDARY_NAME;
