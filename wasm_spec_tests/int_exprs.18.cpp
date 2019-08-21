#include <wasm_spec_tests.hpp>
BOOST_AUTO_TEST_SUITE(int_exprs_18)

const string wasm_str = base_dir + "/wasm_spec_tests/wasms/int_exprs.18.wasm";
std::vector<uint8_t> wasm = read_wasm(wasm_str.c_str());

BOOST_AUTO_TEST_CASE(int_exprs_18_sub_apply_0) { try {
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

BOOST_AUTO_TEST_CASE(int_exprs_18_sub_apply_1) { try {
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

BOOST_AUTO_TEST_SUITE_END()
