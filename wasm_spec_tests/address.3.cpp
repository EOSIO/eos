#include <wasm_spec_tests.hpp>
BOOST_AUTO_TEST_SUITE(address_3)

const string wasm_str = base_dir + "/wasm_spec_tests/wasms/address.3.wasm";
std::vector<uint8_t> wasm = read_wasm(wasm_str.c_str());

BOOST_AUTO_TEST_CASE(address_3_sub_apply_0) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)0);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(address_3_sub_apply_1) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)1);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(address_3_sub_apply_2) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)2);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(address_3_sub_apply_3) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)3);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
