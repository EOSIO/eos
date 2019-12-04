#include <eosio/contract.hpp>
#include <eosio/name.hpp>
#include <eosio/privileged.hpp>

extern "C" __attribute__((eosio_wasm_import)) void set_resource_limit(int64_t, int64_t, int64_t);
extern "C" __attribute__((eosio_wasm_import)) uint32_t get_kv_parameters_packed(uint64_t db, void* params, uint32_t size);
extern "C" __attribute__((eosio_wasm_import)) void set_kv_parameters_packed(uint64_t db, const void* params, uint32_t size);

using namespace eosio;

// A simplified system contract for controlling resources
class [[eosio::contract]] resources : eosio::contract {
 public:
   using contract::contract;
   [[eosio::action]] void setdisklimit(name account, int64_t limit) {
      set_resource_limit(account.value, "disk"_n.value, limit);
   }
   [[eosio::action]] void setramlimit(name account, int64_t limit) {
      set_resource_limit(account.value, "ram"_n.value, limit);
   }
   [[eosio::action]] void ramkvlimits(uint32_t k, uint32_t v) {
      uint32_t limits[2];
      limits[0] = k;
      limits[1] = v;
      set_kv_parameters_packed("eosio.kvram"_n.value, limits, sizeof(limits));
      int sz = get_kv_parameters_packed("eosio.kvram"_n.value, nullptr, 0);
      limits[0] = limits[1] = 0xFFFFFFFF;
      check(sz == 8, "wrong kv parameters size");
      sz = get_kv_parameters_packed("eosio.kvram"_n.value, limits, sizeof(limits));
      check(sz == 8, "wrong kv parameters result");
      check(limits[0] == k, "wrong key");
      check(limits[1] == v, "wrong value");
   }
   [[eosio::action]] void diskkvlimits(uint32_t k, uint32_t v) {
      uint32_t limits[2];
      limits[0] = k;
      limits[1] = v;
      set_kv_parameters_packed("eosio.kvdisk"_n.value, limits, sizeof(limits));
      int sz = get_kv_parameters_packed("eosio.kvdisk"_n.value, nullptr, 0);
      limits[0] = limits[1] = 0xFFFFFFFF;
      check(sz == 8, "wrong kv parameters size");
      sz = get_kv_parameters_packed("eosio.kvdisk"_n.value, limits, sizeof(limits));
      check(sz == 8, "wrong kv parameters result");
      check(limits[0] == k, "wrong key");
      check(limits[1] == v, "wrong value");
   }
};
