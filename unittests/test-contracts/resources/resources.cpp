#include <eosio/contract.hpp>
#include <eosio/name.hpp>
#include <eosio/privileged.hpp>

extern "C" __attribute__((eosio_wasm_import)) void set_disk_limit(int64_t, int64_t);

using namespace eosio;

// A simplified system contract for controlling resources
class [[eosio::contract]] resources : eosio::contract {
 public:
   using contract::contract;
   [[eosio::action]] void setdisklimit(name account, int64_t limit) {
      ::set_disk_limit(account.value, limit);
   }
   [[eosio::action]] void setramlimit(name account, int64_t limit) {
      int64_t ram_bytes, net_weight, cpu_weight;
      get_resource_limits(account, ram_bytes, net_weight, cpu_weight);
      set_resource_limits(account, limit, net_weight, cpu_weight);
   }
};
