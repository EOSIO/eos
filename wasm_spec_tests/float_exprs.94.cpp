#include <wasm_spec_tests.hpp>
BOOST_AUTO_TEST_SUITE(float_exprs_94)

const string wasm_str = base_dir + "/wasm_spec_tests/wasms/float_exprs.94.wasm";
std::vector<uint8_t> wasm = read_wasm(wasm_str.c_str());

BOOST_AUTO_TEST_CASE(float_exprs_94_sub_apply_0) { try {
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

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
