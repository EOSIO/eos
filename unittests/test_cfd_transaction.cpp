#include "test_cfd_transaction.hpp"
#include <contracts.hpp>

struct dummy_action {
   static eosio::chain::name get_name() { return N(dummyaction); }
   static eosio::chain::name get_account() { return N(testapi); }

   char     a; // 1
   uint64_t b; // 8
   int32_t  c; // 4
};

struct cf_action {
   static eosio::chain::name get_name() { return N(cfaction); }
   static eosio::chain::name get_account() { return N(testapi); }

   uint32_t payload = 100;
   uint32_t cfd_idx = 0; // context free data index
};

FC_REFLECT(dummy_action, (a)(b)(c))
FC_REFLECT(cf_action, (payload)(cfd_idx))

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

std::vector<eosio::chain::signed_block_ptr> deploy_test_api(eosio::testing::tester& chain) {
   std::vector<eosio::chain::signed_block_ptr> result;
   chain.create_account(N(testapi));
   chain.create_account(N(dummy));
   result.push_back(chain.produce_block());
   chain.set_code(N(testapi), eosio::testing::contracts::test_api_wasm());
   result.push_back(chain.produce_block());
   return result;
}

eosio::chain::transaction_trace_ptr push_test_cfd_transaction(eosio::testing::tester& chain) {
   cf_action                        cfa;
   eosio::chain::signed_transaction trx;
   eosio::chain::action             act({}, cfa);
   trx.context_free_actions.push_back(act);
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100));
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
   // add a normal action along with cfa
   dummy_action         da = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   eosio::chain::action act1(
       std::vector<eosio::chain::permission_level>{{N(testapi), eosio::chain::config::active_name}}, da);
   trx.actions.push_back(act1);
   chain.set_transaction_headers(trx);
   // run normal passing case
   auto sigs = trx.sign(chain.get_private_key(N(testapi), "active"), chain.control->get_chain_id());
   return chain.push_transaction(trx);
}