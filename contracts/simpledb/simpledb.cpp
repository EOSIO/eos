/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <string>
#include <eosiolib/action.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/db.hpp>
#include <eosiolib/system.h>

using namespace eosio;

namespace simpledb {

   template<uint64_t Val>
   struct dispatchable {
      constexpr static uint64_t action_name = Val;
   };

   //@abi table
   struct record1 {

      uint64_t  key;

      //uint256   u256;
      uint128_t u128;
      uint64_t  u64;
      uint32_t  u32;
      uint16_t  u16;
      uint8_t   u8;

      int64_t   i64;
      int32_t   i32;
      int16_t   i16;
      int8_t    i8;

      EOSLIB_SERIALIZE( record1, (key)(u128)(u64)(u32)(u16)(u8)(i64)(i32)(i16)(i8) )
   };

   //@abi action insert1
   struct insert_record1 : dispatchable<N(insert1)> {
      record1     r1;
      void process() {
         bytes b = eosio::pack(r1);
         store_i64( N(simpledb), N(record1), N(simpledb), b.data(), b.size());
      }

      EOSLIB_SERIALIZE( insert_record1, (r1) )
   };

   //@abi action remove1
   struct remove_record1 : dispatchable<N(remove1)> {
      uint64_t key;
      void process() {
         remove_i64( N(simpledb), N(record1), (char *)this);
      }

      EOSLIB_SERIALIZE( remove_record1, (key) )
   };

   //@abi table
   struct record2 {
      uint128_t key1;
      uint128_t key2;

      EOSLIB_SERIALIZE( record2, (key1)(key2) )
   };

   //@abi action insert2
   struct insert_record2 : dispatchable<N(insert2)> {
      record2 r2;
      void process() {
         bytes b = eosio::pack(r2);
         store_i128i128( N(simpledb), N(record2), N(simpledb), b.data(), b.size());
      }

      EOSLIB_SERIALIZE( insert_record2, (r2) )
   };

   //@abi action remove2
   struct remove_record2 : dispatchable<N(remove2)> {
      record2 key;
      void process() {
         bytes b = eosio::pack(key);
         remove_i128i128( N(simpledb), N(record2), b.data());
      }

      EOSLIB_SERIALIZE( remove_record2, (key) )
   };

   //@abi table
   struct record3 {
      uint64_t key1;
      uint64_t key2;
      uint64_t key3;

      EOSLIB_SERIALIZE( record3, (key1)(key2)(key3) )
   };

   //@abi action insert3
   struct insert_record3 : dispatchable<N(insert3)> {
      record3 r3;
      void process() {
         bytes b = eosio::pack(r3);
         store_i128i128( N(simpledb), N(record3), N(simpledb), b.data(), b.size());
      }

      EOSLIB_SERIALIZE( insert_record3, (r3) )
   };

   //@abi action remove3
   struct remove_record3 : dispatchable<N(remove3)> {
      record3 key;
      void process() {
         bytes b = eosio::pack(key);
         remove_i64i64i64( N(simpledb), N(record2), b.data());
      }

      EOSLIB_SERIALIZE( remove_record3, (key) )
   };

   //@abi table
   struct key_value1 {
      std::string key;
      std::string value;

      EOSLIB_SERIALIZE( key_value1, (key)(value) )
   };

   //@abi action insertkv1
   struct insert_keyvalue1 : dispatchable<N(insertkv1)> {
      key_value1 kv1;
      void process() {
         eosio_assert(false, "not implemented");
         // bytes b = eosio::pack(kv1.value);
         // store_str( N(simpledb), N(keyvalue1), N(simpledb), (char *)kv1.key.data(), kv1.key.size(), b.data(), b.size());
      }

      EOSLIB_SERIALIZE( insert_keyvalue1, (kv1) )
   };

   //@abi action removekv1
   struct remove_keyvalue1 : dispatchable<N(removekv1)> {
      std::string key;
      void process() {
         eosio_assert(false, "not implemented");
         //remove_str( N(simpledb), N(keyvalue1), (char *)key.data(), key.size() );
      }

      EOSLIB_SERIALIZE( remove_keyvalue1, (key) )
   };

   struct complex_type {
      std::string name;
      uint64_t age;

      EOSLIB_SERIALIZE( complex_type, (name)(age) )
   };

   //@abi table
   struct key_value2 {
      std::string key;
      complex_type value;

      EOSLIB_SERIALIZE( key_value2, (key)(value) )
   };

   //@abi action insertkv2
   struct insert_keyvalue2 : dispatchable<N(insertkv2)> {
      key_value2 kv2;
      void process() {
         eosio_assert(false, "not implemented");
         // bytes b = eosio::pack(kv2.value);
         // store_str( N(simpledb), N(keyvalue2), N(simpledb), (char *)kv2.key.data(), kv2.key.size(), b.data(), b.size());
      }

      EOSLIB_SERIALIZE( insert_keyvalue2, (kv2) )
   };

   //@abi action removekv2
   struct remove_keyvalue2 : dispatchable<N(removekv2)> {
      std::string key;
      void process() {
         remove_str( N(simpledb), N(keyvalue2), (char *)key.data(), key.size() );
      }

      EOSLIB_SERIALIZE( remove_keyvalue2, (key) )
   };

   template<typename ...Ts>
   struct dispatcher_impl;

   template<typename T>
   struct dispatcher_impl<T> {
      static bool dispatch(uint64_t action) {
         if (action == T::action_name) {
            unpack_action<T>().process();
            return true;
         }

         return false;
      }
   };

   template<typename T, typename ...Rem>
   struct dispatcher_impl<T, Rem...> {
      static bool dispatch(uint64_t action) {
         return dispatcher_impl<T>::dispatch(action) || dispatcher_impl<Rem...>::dispatch(action);
      }
   };

   using dispatcher = dispatcher_impl<insert_record1, remove_record1, insert_record2, insert_record3, insert_keyvalue1, insert_keyvalue2>;
}

extern "C" {

   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t code, uint64_t act ) {
      if (code == current_receiver()) {
         auto action_processed = simpledb::dispatcher::dispatch(act);
         eosio_assert(action_processed, "unknown action");
      }
   }

}
