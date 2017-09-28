/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include <eoslib/db.hpp>

struct PACKED(record1) {
   uint64_t key;
};

struct PACKED(record2) {
   uint128_t key1;
   uint128_t key2;
};

struct PACKED(record3) {
   uint64_t key1;
   uint64_t key2;
   uint64_t key3;
};

struct KeyValue1 {
   String key;
   String value;
};

struct ComplexType {
   String name;
   UInt64 age;
};

struct KeyValue2 {
   String key;
   ComplexType value;
};
