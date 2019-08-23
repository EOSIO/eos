#include <wasm_spec_tests.hpp>

const string wasm_str_func_ptrs_0 = base_dir + "/wasm_spec_tests/wasms/func_ptrs.0.wasm";
std::vector<uint8_t> wasm_func_ptrs_0= read_wasm(wasm_str_func_ptrs_0.c_str());

BOOST_DATA_TEST_CASE(func_ptrs_0_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_func_ptrs_0);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_func_ptrs_8 = base_dir + "/wasm_spec_tests/wasms/func_ptrs.8.wasm";
std::vector<uint8_t> wasm_func_ptrs_8= read_wasm(wasm_str_func_ptrs_8.c_str());

BOOST_DATA_TEST_CASE(func_ptrs_8_check_throw, boost::unit_test::data::xrange(0,6), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_func_ptrs_8);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_DATA_TEST_CASE(func_ptrs_8_pass, boost::unit_test::data::xrange(6,7), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_func_ptrs_8);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_func_ptrs_9 = base_dir + "/wasm_spec_tests/wasms/func_ptrs.9.wasm";
std::vector<uint8_t> wasm_func_ptrs_9= read_wasm(wasm_str_func_ptrs_9.c_str());

BOOST_DATA_TEST_CASE(func_ptrs_9_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_func_ptrs_9);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

