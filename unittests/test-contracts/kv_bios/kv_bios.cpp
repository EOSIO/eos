#include <eosio/contract.hpp>
#include <eosio/name.hpp>
#include <eosio/privileged.hpp>

extern "C" __attribute__((eosio_wasm_import)) void set_resource_limit(int64_t, int64_t, int64_t);
extern "C" __attribute__((eosio_wasm_import)) uint32_t get_kv_parameters_packed(uint64_t db, void* params, uint32_t size, uint32_t max_version);
extern "C" __attribute__((eosio_wasm_import)) void set_kv_parameters_packed(uint64_t db, const void* params, uint32_t size);

using namespace eosio;

// Manages resources used by the kv-store
class [[eosio::contract]] kv_bios : eosio::contract {
 public:
   using contract::contract;
   [[eosio::action]] void setdisklimit(name account, int64_t limit) {
      set_resource_limit(account.value, "disk"_n.value, limit);
   }
   [[eosio::action]] void setramlimit(name account, int64_t limit) {
      set_resource_limit(account.value, "ram"_n.value, limit);
   }
   [[eosio::action]] void ramkvlimits(uint32_t k, uint32_t v, uint32_t i) {
      kvlimits_impl("eosio."_n "k"_n "vram"_n, k, v, i);
   }
   [[eosio::action]] void diskkvlimits(uint32_t k, uint32_t v, uint32_t i) {
      kvlimits_impl("eosio.kvdisk"_n, k, v, i);
   }
   void kvlimits_impl(name db, uint32_t k, uint32_t v, uint32_t i) {
      uint32_t limits[4];
      limits[0] = 0;
      limits[1] = k;
      limits[2] = v;
      limits[3] = i;
      set_kv_parameters_packed(db.value, limits, sizeof(limits));
      int sz = get_kv_parameters_packed(db.value, nullptr, 0, 0);
      limits[0] = limits[1] = 0xFFFFFFFF;
      check(sz == 16, "wrong kv parameters size");
      sz = get_kv_parameters_packed(db.value, limits, sizeof(limits), 0);
      check(sz == 16, "wrong kv parameters result");
      check(limits[0] == 0, "wrong version");
      check(limits[1] == k, "wrong key");
      check(limits[2] == v, "wrong value");
      check(limits[3] == i, "wrong iter limit");
   }
};
