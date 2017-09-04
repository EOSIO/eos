/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include <eoslib/string.hpp>

/* @abi simpledb action insert1
 * @abi simpledb table
*/
struct PACKED(record1) {
   uint64_t key;
};

/* @abi simpledb action insert2
 * @abi simpledb table
*/
struct PACKED(record2) {
   uint128_t key1;
   uint128_t key2;
};

/* @abi simpledb action insert3
 * @abi simpledb table
*/
struct PACKED(record3) {
   uint64_t key1;
   uint64_t key2;
   uint64_t key3;
};

struct key_value1 {
   eosio::string key;
   eosio::string value;
};

struct complex_type {
   eosio::string name;
   uint64_t age;
};

struct key_value2 {
   eosio::string key;
   complex_type value;
};
