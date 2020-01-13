#include <eosio/eosio.hpp>

extern "C" {
   namespace eosio { namespace internal_use_do_not_use {
      __attribute__((eosio_wasm_import))
      uint32_t get_trx_cpu_bill();
      __attribute__((eosio_wasm_import))
      uint64_t get_wallclock_time();
      __attribute__((eosio_wasm_import))
      void get_random(char*, uint32_t);
   }}
};

class [[eosio::contract]] subjective_data : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void trxcpu() {
      uint32_t cpu = eosio::internal_use_do_not_use::get_trx_cpu_bill();
      cpu = eosio::internal_use_do_not_use::get_trx_cpu_bill();
   }
   [[eosio::action]]
   void walltime() {
      uint64_t walltime = eosio::internal_use_do_not_use::get_wallclock_time();
      walltime = eosio::internal_use_do_not_use::get_wallclock_time();
   }
};
