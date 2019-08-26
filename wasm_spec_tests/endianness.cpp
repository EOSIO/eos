#include <wasm_spec_tests.hpp>

const string wasm_str_endianness_0 = base_dir + "/wasm_spec_tests/wasms/endianness.0.wasm";
std::vector<uint8_t> wasm_endianness_0= read_wasm(wasm_str_endianness_0.c_str());

BOOST_DATA_TEST_CASE(endianness_0_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_endianness_0);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

