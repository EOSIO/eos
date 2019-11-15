#include <eosio/eosio.hpp>

using namespace eosio;

enum it_stat : int32_t {
   iterator_ok     = 0,
   iterator_erased = -1,
   iterator_oob    = -2,
};

#define IMPORT extern "C" __attribute__((eosio_wasm_import))

// clang-format off
IMPORT void     kv_erase(uint64_t db, uint64_t contract, const char* key, uint32_t key_size);
IMPORT void     kv_set(uint64_t db, uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size);
IMPORT bool     kv_get(uint64_t db, uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size);
IMPORT uint32_t kv_get_data(uint64_t db, uint32_t offset, char* data, uint32_t data_size);
IMPORT uint32_t kv_it_create(uint64_t db, uint64_t contract, const char* prefix, uint32_t size);
IMPORT void     kv_it_destroy(uint32_t itr);
IMPORT it_stat  kv_it_status(uint32_t itr);
IMPORT int      kv_it_compare(uint32_t itr_a, uint32_t itr_b);
IMPORT int      kv_it_key_compare(uint32_t itr, const char* key, uint32_t size);
IMPORT it_stat  kv_it_move_to_oob(uint32_t itr);
IMPORT it_stat  kv_it_increment(uint32_t itr);
IMPORT it_stat  kv_it_decrement(uint32_t itr);
IMPORT it_stat  kv_it_lower_bound(uint32_t itr, const char* key, uint32_t size);
IMPORT it_stat  kv_it_key(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
IMPORT it_stat  kv_it_value(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
// clang-format on

class [[eosio::contract("kv_test")]] kvtest : public eosio::contract {
 public:
   using eosio::contract::contract;

   [[eosio::action]] void itlifetime(uint64_t db) {
      check(1 == kv_it_create(db, 0, nullptr, 0), "itlifetime a");
      check(2 == kv_it_create(db, 0, nullptr, 0), "itlifetime b");
      check(3 == kv_it_create(db, 0, nullptr, 0), "itlifetime c");
      check(4 == kv_it_create(db, 0, nullptr, 0), "itlifetime d");
      check(5 == kv_it_create(db, 0, nullptr, 0), "itlifetime e");
      check(6 == kv_it_create(db, 0, nullptr, 0), "itlifetime f");
      kv_it_destroy(4);
      kv_it_destroy(2);
      kv_it_destroy(5);
      check(5 == kv_it_create(db, 0, nullptr, 0), "itlifetime g");
      check(2 == kv_it_create(db, 0, nullptr, 0), "itlifetime h");
      kv_it_destroy(5);
      check(5 == kv_it_create(db, 0, nullptr, 0), "itlifetime i");
      check(4 == kv_it_create(db, 0, nullptr, 0), "itlifetime j");
      check(7 == kv_it_create(db, 0, nullptr, 0), "itlifetime k");
      // check(kv_it_status(1) == iterator_oob, "itlifetime l");
      // check(kv_it_status(2) == iterator_oob, "itlifetime m");
      // check(kv_it_status(3) == iterator_oob, "itlifetime n");
      // check(kv_it_status(4) == iterator_oob, "itlifetime o");
      // check(kv_it_status(5) == iterator_oob, "itlifetime p");
      // check(kv_it_status(6) == iterator_oob, "itlifetime q");
      // check(kv_it_status(7) == iterator_oob, "itlifetime r");
      check(false, "itlifetime passed");
   }
};
