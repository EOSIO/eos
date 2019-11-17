#include <eosio/eosio.hpp>

extern "C" {
   namespace eosio { namespace internal_use_do_not_use {
      __attribute__((eosio_wasm_import))
      bool accept_charges(uint32_t, uint32_t);
      __attribute__((eosio_wasm_import))
      void get_accepted_charges(uint64_t&, uint32_t&, uint32_t&);
      __attribute__((eosio_wasm_import))
      void get_num_actions(uint32_t&, uint32_t&);
   }}
};

class [[eosio::contract]] contract_pays : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void accept(uint32_t net, uint32_t cpu, uint64_t acnt) {
      eosio::check(eosio::internal_use_do_not_use::accept_charges(net, cpu), "failed to accept charges");
      uint64_t _acnt = 0;
      uint32_t _net = 0, _cpu = 0;
      eosio::internal_use_do_not_use::get_accepted_charges(_acnt, _net, _cpu);
      eosio::check(acnt == _acnt, "accepted account not matching expected");
      eosio::check(net == _net && cpu == _cpu, "accepted charges do not match");
   }

   [[eosio::action]]
   void eatcpu(uint32_t iters) {
      for (uint32_t i=0; i < iters; i++) {
         eosio::print_f("eating up cpu\n");
      }
   }

};
