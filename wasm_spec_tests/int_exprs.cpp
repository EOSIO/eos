#include <wasm_spec_tests.hpp>

const string wasm_str_int_exprs_0 = base_dir + "/wasm_spec_tests/wasms/int_exprs.0.wasm";
std::vector<uint8_t> wasm_int_exprs_0= read_wasm(wasm_str_int_exprs_0.c_str());

BOOST_DATA_TEST_CASE(int_exprs_0_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_0);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_1 = base_dir + "/wasm_spec_tests/wasms/int_exprs.1.wasm";
std::vector<uint8_t> wasm_int_exprs_1= read_wasm(wasm_str_int_exprs_1.c_str());

BOOST_DATA_TEST_CASE(int_exprs_1_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_1);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_10 = base_dir + "/wasm_spec_tests/wasms/int_exprs.10.wasm";
std::vector<uint8_t> wasm_int_exprs_10= read_wasm(wasm_str_int_exprs_10.c_str());

BOOST_DATA_TEST_CASE(int_exprs_10_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_10);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_11 = base_dir + "/wasm_spec_tests/wasms/int_exprs.11.wasm";
std::vector<uint8_t> wasm_int_exprs_11= read_wasm(wasm_str_int_exprs_11.c_str());

BOOST_DATA_TEST_CASE(int_exprs_11_check_throw, boost::unit_test::data::xrange(0,4), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_11);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_12 = base_dir + "/wasm_spec_tests/wasms/int_exprs.12.wasm";
std::vector<uint8_t> wasm_int_exprs_12= read_wasm(wasm_str_int_exprs_12.c_str());

BOOST_DATA_TEST_CASE(int_exprs_12_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_12);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_13 = base_dir + "/wasm_spec_tests/wasms/int_exprs.13.wasm";
std::vector<uint8_t> wasm_int_exprs_13= read_wasm(wasm_str_int_exprs_13.c_str());

BOOST_DATA_TEST_CASE(int_exprs_13_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_13);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_14 = base_dir + "/wasm_spec_tests/wasms/int_exprs.14.wasm";
std::vector<uint8_t> wasm_int_exprs_14= read_wasm(wasm_str_int_exprs_14.c_str());

BOOST_DATA_TEST_CASE(int_exprs_14_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_14);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_15 = base_dir + "/wasm_spec_tests/wasms/int_exprs.15.wasm";
std::vector<uint8_t> wasm_int_exprs_15= read_wasm(wasm_str_int_exprs_15.c_str());

BOOST_DATA_TEST_CASE(int_exprs_15_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_15);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_16 = base_dir + "/wasm_spec_tests/wasms/int_exprs.16.wasm";
std::vector<uint8_t> wasm_int_exprs_16= read_wasm(wasm_str_int_exprs_16.c_str());

BOOST_DATA_TEST_CASE(int_exprs_16_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_16);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_17 = base_dir + "/wasm_spec_tests/wasms/int_exprs.17.wasm";
std::vector<uint8_t> wasm_int_exprs_17= read_wasm(wasm_str_int_exprs_17.c_str());

BOOST_DATA_TEST_CASE(int_exprs_17_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_17);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_18 = base_dir + "/wasm_spec_tests/wasms/int_exprs.18.wasm";
std::vector<uint8_t> wasm_int_exprs_18= read_wasm(wasm_str_int_exprs_18.c_str());

BOOST_DATA_TEST_CASE(int_exprs_18_check_throw, boost::unit_test::data::xrange(0,2), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_18);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_2 = base_dir + "/wasm_spec_tests/wasms/int_exprs.2.wasm";
std::vector<uint8_t> wasm_int_exprs_2= read_wasm(wasm_str_int_exprs_2.c_str());

BOOST_DATA_TEST_CASE(int_exprs_2_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_2);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_3 = base_dir + "/wasm_spec_tests/wasms/int_exprs.3.wasm";
std::vector<uint8_t> wasm_int_exprs_3= read_wasm(wasm_str_int_exprs_3.c_str());

BOOST_DATA_TEST_CASE(int_exprs_3_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_3);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_4 = base_dir + "/wasm_spec_tests/wasms/int_exprs.4.wasm";
std::vector<uint8_t> wasm_int_exprs_4= read_wasm(wasm_str_int_exprs_4.c_str());

BOOST_DATA_TEST_CASE(int_exprs_4_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_4);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_5 = base_dir + "/wasm_spec_tests/wasms/int_exprs.5.wasm";
std::vector<uint8_t> wasm_int_exprs_5= read_wasm(wasm_str_int_exprs_5.c_str());

BOOST_DATA_TEST_CASE(int_exprs_5_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_5);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_6 = base_dir + "/wasm_spec_tests/wasms/int_exprs.6.wasm";
std::vector<uint8_t> wasm_int_exprs_6= read_wasm(wasm_str_int_exprs_6.c_str());

BOOST_DATA_TEST_CASE(int_exprs_6_check_throw, boost::unit_test::data::xrange(0,4), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_6);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_7 = base_dir + "/wasm_spec_tests/wasms/int_exprs.7.wasm";
std::vector<uint8_t> wasm_int_exprs_7= read_wasm(wasm_str_int_exprs_7.c_str());

BOOST_DATA_TEST_CASE(int_exprs_7_check_throw, boost::unit_test::data::xrange(0,4), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_7);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_8 = base_dir + "/wasm_spec_tests/wasms/int_exprs.8.wasm";
std::vector<uint8_t> wasm_int_exprs_8= read_wasm(wasm_str_int_exprs_8.c_str());

BOOST_DATA_TEST_CASE(int_exprs_8_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_8);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_int_exprs_9 = base_dir + "/wasm_spec_tests/wasms/int_exprs.9.wasm";
std::vector<uint8_t> wasm_int_exprs_9= read_wasm(wasm_str_int_exprs_9.c_str());

BOOST_DATA_TEST_CASE(int_exprs_9_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_int_exprs_9);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

