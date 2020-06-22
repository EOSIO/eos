#pragma once
#include <eosio/testing/tester.hpp>

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

std::vector<eosio::chain::signed_block_ptr> deploy_test_api(eosio::testing::tester& chain);
eosio::chain::transaction_trace_ptr push_test_cfd_transaction(eosio::testing::tester& chain);

struct scoped_temp_path {
   boost::filesystem::path path;
   scoped_temp_path() {
      path = boost::filesystem::unique_path();
      if (boost::unit_test::framework::master_test_suite().argc >= 2) {
         path += boost::unit_test::framework::master_test_suite().argv[1];
      }
   }
   ~scoped_temp_path() {
      boost::filesystem::remove_all(path);
   }
};