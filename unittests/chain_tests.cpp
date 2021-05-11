#include <eosio/chain/controller.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/transaction.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

#include "fork_test_utilities.hpp"
#include "test_cfd_transaction.hpp"

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(chain_tests)

BOOST_AUTO_TEST_CASE( replace_producer_keys ) try {
   validating_tester tester;

   const auto head_ptr = tester.control->head_block_state();
   BOOST_REQUIRE(head_ptr);

   const auto new_key = get_public_key(name("newkey"), config::active_name.to_string());

   // make sure new keys is not used
   for(const auto& prod : head_ptr->active_schedule.producers) {
      for(const auto& key : std::get<block_signing_authority_v0>(prod.authority).keys){  
         BOOST_REQUIRE(key.key != new_key);
      }
   }

   const auto old_version = head_ptr->pending_schedule.schedule.version;
   BOOST_REQUIRE_NO_THROW(tester.control->replace_producer_keys(new_key));
   const auto new_version = head_ptr->pending_schedule.schedule.version;
   // make sure version not been changed
   BOOST_REQUIRE(old_version == new_version);

   const auto& gpo = tester.control->db().get<global_property_object>();
   BOOST_REQUIRE(!gpo.proposed_schedule_block_num);
   BOOST_REQUIRE(gpo.proposed_schedule.version == 0);
   BOOST_REQUIRE(gpo.proposed_schedule.producers.empty());

   const uint32_t expected_threshold = 1;
   const weight_type expected_key_weight = 1;
   for(const auto& prod : head_ptr->active_schedule.producers) {
      BOOST_REQUIRE_EQUAL(std::get<block_signing_authority_v0>(prod.authority).threshold, expected_threshold);
      for(const auto& key : std::get<block_signing_authority_v0>(prod.authority).keys){
         BOOST_REQUIRE_EQUAL(key.key, new_key);
         BOOST_REQUIRE_EQUAL(key.weight, expected_key_weight);
       }
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( replace_account_keys ) try {
   validating_tester tester;
   const name usr = config::system_account_name;
   const name active_permission = config::active_name;
   const auto& rlm = tester.control->get_resource_limits_manager();
   const auto* perm = tester.control->db().find<permission_object, by_owner>(boost::make_tuple(usr, active_permission));
   BOOST_REQUIRE(perm != NULL);

   const int64_t old_size = (int64_t)(chain::config::billable_size_v<permission_object> + perm->auth.get_billable_size());
   const auto old_usr_auth = perm->auth;
   const auto new_key = get_public_key(name("newkey"), "active");
   const authority expected_authority(new_key);
   BOOST_REQUIRE(old_usr_auth != expected_authority);
   const auto old_ram_usg = rlm.get_account_ram_usage(usr);

   BOOST_REQUIRE_NO_THROW(tester.control->replace_account_keys(usr, active_permission, new_key));
   const int64_t new_size = (int64_t)(chain::config::billable_size_v<permission_object> + perm->auth.get_billable_size());
   const auto new_ram_usg = rlm.get_account_ram_usage(usr);
   BOOST_REQUIRE_EQUAL(old_ram_usg + (new_size - old_size), new_ram_usg);
   const auto new_usr_auth = perm->auth;
   BOOST_REQUIRE(new_usr_auth == expected_authority);

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( decompressed_size_over_limit ) try {
   tester chain;
   const name usr = config::system_account_name;
   const name active_permission = config::active_name;

   // build a transaction, add cf data, sign
   cf_action                        cfa;
   eosio::chain::signed_transaction trx;
   eosio::chain::action             act({}, cfa);
   trx.context_free_actions.push_back(act); 
   // this is a over limit size (4+4)*129*1024 = 1032*1024 > 1M
   for(int i = 0; i < 129*1024; ++i){
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100));
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
   }
   // add a normal action along with cfa
   dummy_action         da = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   eosio::chain::action act1(
       std::vector<eosio::chain::permission_level>{{"testapi"_n, eosio::chain::config::active_name}}, da);
   trx.actions.push_back(act1);
   chain.set_transaction_headers(trx);
   auto sig = trx.sign(chain.get_private_key("testapi"_n, "active"), chain.control->get_chain_id());


   // pack
   packed_transaction_v0 ptv0(trx, packed_transaction_v0::compression_type::zlib);
   // try unpack and throw
   bytes packed_txn = ptv0.get_packed_transaction();
   bytes pcfd = ptv0.get_packed_context_free_data();
   vector<signature_type>  sigs;
   sigs.push_back(sig);
   bool throwed = false;
   bool throw_expect_string = false; 
   try { 
      // will call into decompress zlib 
      packed_transaction_v0 copy( packed_txn, sigs, pcfd, packed_transaction_v0::compression_type::zlib );
   } catch(fc::exception& er){
     throwed = true;
     if ( er.to_detail_string().find("Exceeded maximum decompressed transaction size") != std::string::npos){
       throw_expect_string = true;
     } 
   }
   BOOST_REQUIRE(throwed);
   BOOST_REQUIRE(throw_expect_string); 
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( decompressed_size_under_limit ) try {
   tester chain;

   // build a transaction, add cf data, sign
   cf_action                        cfa;
   eosio::chain::signed_transaction trx;
   eosio::chain::action             act({}, cfa);
   trx.context_free_actions.push_back(act);
   // this is a under limit size  (4+4)*128*1024 = 1024*1024
   for(int i = 0; i < 100*1024; ++i){
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100));
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
   }
   // add a normal action along with cfa
   dummy_action         da = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   eosio::chain::action act1(
       std::vector<eosio::chain::permission_level>{{"testapi"_n, eosio::chain::config::active_name}}, da);
   trx.actions.push_back(act1);
   chain.set_transaction_headers(trx);
   auto sig = trx.sign(chain.get_private_key("testapi"_n, "active"), chain.control->get_chain_id());

   // pack
   packed_transaction_v0 ptv0(trx, packed_transaction_v0::compression_type::zlib);
   // try unpack and throw
   bytes packed_txn = ptv0.get_packed_transaction();
   bytes pcfd = ptv0.get_packed_context_free_data();
   vector<signature_type>  sigs;
   sigs.push_back(sig);
   bool throwed = false;
   bool throw_expect_string = false; 
   try {
      // will call into decompress zlib 
      packed_transaction_v0 copy( packed_txn, sigs, pcfd, packed_transaction_v0::compression_type::zlib );
   } catch(fc::exception& er){
     throwed = true;
     if ( er.to_detail_string().find("Exceeded maximum decompressed transaction size") != std::string::npos){
       throw_expect_string = true;
     } 
   }
   BOOST_REQUIRE(throwed == false);
   BOOST_REQUIRE(throw_expect_string == false); 
} FC_LOG_AND_RETHROW()



BOOST_AUTO_TEST_SUITE_END()
