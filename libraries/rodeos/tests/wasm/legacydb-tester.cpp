#include "legacydb-test-contract.hpp"
#include <eosio/tester.hpp>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

using namespace eosio;
using namespace legacydb;
using namespace legacydb::actions;

TEST_CASE("xx", "") {
   test_chain  chain;
   test_rodeos rodeos;
   rodeos.connect(chain);
   rodeos.enable_queries(1024 * 1024, 10, 1000, "");

   chain.create_code_account(account);
   chain.set_code(account, "legacydb.wasm");

   chain.as(account).act<write>();
   chain.as(account).act<read>();

   chain.start_block();
   rodeos.sync_blocks();

   expect(rodeos.as().trace<write>(), "unimplemented: db_store_i64");
   rodeos.as().act<read>();
}
