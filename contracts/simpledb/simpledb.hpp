/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include <eoslib/string.hpp>

/* @abi action insert1
 * @abi table
*/
struct record1 {
   uint64_t key;
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
   eos::string key;
   eos::string value;
};

struct complex_type {
   eosio::string name;
   uint64_t age;
};


/* @abi action insertkv2
 * @abi table
*/
struct key_value2 {
   eos::string key;
   ComplexType value;

};
