/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eos.hpp>
#include <eosiolib/string.hpp>

/* @abi action insert1
 * @abi table
*/
struct record1 {
   uint64_t  key;
   
   uint256   u256;
   uint128_t u128;
   uint64_t  u64;
   uint32_t  u32;
   uint16_t  u16; 
   uint8_t   u8;

   int64_t   i64;
   int32_t   i32;
   int16_t   i16;
   int8_t    i8;

   price     price;
};

/* @abi action insert2
 * @abi table
*/
struct record2 {
   uint128_t key1;
   uint128_t key2;
};

/* @abi action insert3
 * @abi table
*/
struct record3 {
   uint64_t key1;
   uint64_t key2;
   uint64_t key3;
};

/* @abi action insertkv1
 * @abi table
*/
struct key_value1 {
   eosio::string key;
   eosio::string value;
};

struct complex_type {
   eosio::string name;
   uint64_t age;
};


/* @abi action insertkv2
 * @abi table
*/
struct key_value2 {
   eosio::string key;
   complex_type value;
};
