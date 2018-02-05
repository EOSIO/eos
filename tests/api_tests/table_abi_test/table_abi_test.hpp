#include <eosiolib/eos.hpp>
#include <eosiolib/db.hpp>

struct PACKED(table1) {
   uint64_t  key;
   uint128_t value1;
   int64_t   value2;
};

struct PACKED(table2) {
   uint128_t key1;
   uint128_t key2;
   uint64_t  value1;
   int64_t   value2;
};

struct PACKED(table3) {
   uint64_t  key1;
   int64_t   key2;
   uint64_t  key3;
   uint128_t value1;
   int64_t   value2;
};
