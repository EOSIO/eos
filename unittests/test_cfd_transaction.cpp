#include "test_cfd_transaction.hpp"
#include <contracts.hpp>



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