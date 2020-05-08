#pragma once
#include <eosio/testing/tester.hpp>

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