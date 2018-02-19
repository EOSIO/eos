/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/db.h>


namespace eosio {

/**
 *  @defgroup databaseCpp Database C++ API
 *  @brief C++ APIs for interfacing with the database. It is based on pimpl idiom.
 *  @ingroup database
 */


template<typename T>
struct table_impl_obj {};


template<int Primary, int Secondary>
struct table_impl{};


template<>
struct table_impl<sizeof(uint128_t),sizeof(uint128_t)> {

    static int32_t front_primary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return front_primary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t back_primary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return back_primary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t load_primary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return load_primary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t next_primary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return next_primary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t previous_primary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return previous_primary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t upper_bound_primary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return upper_bound_primary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t lower_bound_primary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return lower_bound_primary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t front_secondary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return front_secondary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t back_secondary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return back_secondary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t load_secondary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return load_secondary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t next_secondary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return next_secondary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t previous_secondary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return previous_secondary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t upper_bound_secondary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return upper_bound_secondary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t lower_bound_secondary( uint64_t code, uint64_t scope, uint64_t table_n, void* data, uint32_t len ) {
       return lower_bound_secondary_i128i128( code, scope, table_n, data, len );
    }

    static int32_t remove( uint64_t scope, uint64_t table_n, const void* data ) {
       return remove_i128i128( scope, table_n, data );
    }

    static int32_t store( account_name scope, table_name table_n, account_name bta, const void* data, uint32_t len ) {
       return store_i128i128( scope, table_n, bta, data, len );
    }

    static int32_t update( account_name scope, table_name table_n, account_name bta, const void* data, uint32_t len ) {
       return update_i128i128( scope, table_n, bta, data, len );
    }
};

template<uint64_t code, uint64_t scope, uint64_t table_n, uint64_t bta, typename Record, typename PrimaryType, typename SecondaryType = void>
struct table {
   private:
   typedef table_impl<sizeof( PrimaryType ), sizeof( SecondaryType )> impl;
   static_assert( sizeof(PrimaryType) + sizeof(SecondaryType) <= sizeof(Record), "invalid template parameters" );

   public:
   typedef PrimaryType primary;
   typedef SecondaryType secondary;

    /**
     * @brief Primary Index of the Table
     */
   struct primary_index {
      /**
      *  @param r - reference to a record to store the front record based on primary index.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool front( Record& r, uint64_t s = scope ) {
         return impl::front_primary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to store the back record based on primary index.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool back( Record& r, uint64_t s = scope ) {
         return impl::back_primary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to store next value; must be initialized with current.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool next( Record& r, uint64_t s = scope ) {
         return impl::next_primary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to store previous value; must be initialized with current.
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successful read.
      */
      static bool previous( Record& r, uint64_t s = scope ) {
         return impl::previous_primary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
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
         return impl::load_primary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param p - reference to primary key to get the lower bound of; must be initialized with a value;
       *  @param r - reference to a record to load the value to.
       *
       *  @return true if successful read.
       */
      static bool lower_bound( const PrimaryType& p, Record& r ) {
         return impl::lower_bound_primary( code, scope, table_n, &p &r, sizeof(Record) ) == sizeof(Record);
      }

       /**
       *  @param p - reference to primary key to get the upper bound of; must be initialized with a value;
       *  @param r - reference to a record to load the value to.
       *
       *  @return true if successful read.
       */
      static bool upper_bound( const PrimaryType& p, Record& r ) {
         return impl::upper_bound_primary( code, scope, table_n, &p &r, sizeof(Record) ) == sizeof(Record);
      }

      /**
      *  @param r - reference to a record to remove from table;
      *  @param s - account scope. default is current scope of the class
      *
      *  @return true if successfully removed;
      */
      static bool remove( const Record& r, uint64_t s = scope ) {
         return impl::remove( s, table_n, &r ) != 0;
      }
   };

   /**
     * @brief Secondary Index of the Table
     */

   struct secondary_index {
       /**
       *  @param r - reference to a record to store the front record based on secondary index.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool front( Record& r, uint64_t s = scope ) {
          return impl::front_secondary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param r - reference to a record to store the back record based on secondary index.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool back( Record& r, uint64_t s = scope ) {
          return impl::back_secondary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param r - reference to a record to return the next record .
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool next( Record& r, uint64_t s = scope ) {
          return impl::next_secondary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param r - reference to a record to return the next record.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool previous( Record& r, uint64_t s = scope ) {
          return impl::previous_secondary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param p - reference to secondary index key
       *  @param r - reference to record to hold the value
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */
       static bool get( const SecondaryType& p, Record& r, uint64_t s = scope ) {
          return impl::load_secondary( code, s, table_n, &p &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
       *  @param p - reference to secondary key to get the lower bound of; must be initialized with a value;
       *  @param r - reference to a record to load the value to.
       *  @param s - account scope. default is current scope of the class
       *
       *  @return true if successful read.
       */

       static bool lower_bound( const SecondaryType& p, Record& r, uint64_t s = scope ) {
          return impl::lower_bound_secondary( code, s, table_n, &p &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
        *  @param p - reference to secondary key to get the upper bound of; must be initialized with a value;
        *  @param r - reference to a record to load the value to.
        *  @param s - account scope. default is current scope of the class
        *
        *  @return true if successful read.
        */
       static bool upper_bound( const SecondaryType& p, Record& r, uint64_t s = scope ) {
          return impl::upper_bound_secondary( code, s, table_n, &p &r, sizeof(Record) ) == sizeof(Record);
       }

       /**
        *  @param r - reference to a record to be removed.
        *  @param s - account scope. default is current scope of the class
        *
        *  @return true if successfully removed.
        */
       static bool remove( const Record& r, uint64_t s = scope ) {
          return impl::remove( s, table_n, &r ) != 0;
       }
    };

    /**
    *  @brief Fetches a record from the table.
    *  @details Fetches a record from the table.
    *  @param p - reference to primary key to retrieve
    *  @param r - reference to a record to load the value to.
    *  @param s - account scope. default is current scope of the class
    *
    *  @return true if successful read.
    */

    static bool get( const PrimaryType& p, Record& r, uint64_t s = scope ) {
       *reinterpret_cast<PrimaryType*>(&r) = p;
       return impl::load_primary( code, s, table_n, &r, sizeof(Record) ) == sizeof(Record);
    }

     /**
     *  @brief Store a record in the table.
     *  @details Store a record in the table.
     *  @param r - reference to a record to store.
     *  @param s - account scope. default is current scope of the class
     *
     *  @return true if successful store.
     */
    static bool store( const Record& r, uint64_t s = scope, uint64_t b = bta ) {
       eosio_assert( impl::store( s, table_n, b, &r, sizeof(r) ), "error storing record" );
       return true;
    }

    /**
    *  @brief Update a record in the table.
    *  @details Update a record in the table.
    *  @param r - reference to a record to update.
    *  @param s - account scope. default is current scope of the class
    *
    *  @return true if successful update.
    */
    static bool update( const Record& r, uint64_t s = scope, uint64_t b = bta ) {
       eosio_assert( impl::update( s, table_n, b, &r, sizeof(r) ), "error updating record" );
       return true;
    }

    /**
    *  @brief Remove a record from the table.
    *  @details Remove a record from the table.
    *  @param r - reference to a record to remove.
    *  @param s - account scope. default is current scope of the class
    *
    *  @return true if successful remove.
    */
    static bool remove( const Record& r, uint64_t s = scope ) {
       return impl::remove( s, table_n, &r ) != 0;
    }
 };
/// @}



} // namespace eosio

