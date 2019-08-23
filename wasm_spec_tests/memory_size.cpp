#include <wasm_spec_tests.hpp>

const string wasm_str_memory_size_0 = base_dir + "/wasm_spec_tests/wasms/memory_size.0.wasm";
std::vector<uint8_t> wasm_memory_size_0= read_wasm(wasm_str_memory_size_0.c_str());

BOOST_DATA_TEST_CASE(memory_size_0_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_memory_size_0);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_memory_size_1 = base_dir + "/wasm_spec_tests/wasms/memory_size.1.wasm";
std::vector<uint8_t> wasm_memory_size_1= read_wasm(wasm_str_memory_size_1.c_str());

BOOST_DATA_TEST_CASE(memory_size_1_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_memory_size_1);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_memory_size_2 = base_dir + "/wasm_spec_tests/wasms/memory_size.2.wasm";
std::vector<uint8_t> wasm_memory_size_2= read_wasm(wasm_str_memory_size_2.c_str());

BOOST_DATA_TEST_CASE(memory_size_2_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_memory_size_2);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_memory_size_3 = base_dir + "/wasm_spec_tests/wasms/memory_size.3.wasm";
std::vector<uint8_t> wasm_memory_size_3= read_wasm(wasm_str_memory_size_3.c_str());

BOOST_DATA_TEST_CASE(memory_size_3_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_memory_size_3);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

