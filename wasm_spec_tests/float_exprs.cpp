#include <wasm_spec_tests.hpp>

const string wasm_str_float_exprs_0 = base_dir + "/wasm_spec_tests/wasms/float_exprs.0.wasm";
std::vector<uint8_t> wasm_float_exprs_0= read_wasm(wasm_str_float_exprs_0.c_str());

BOOST_DATA_TEST_CASE(float_exprs_0_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_0);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_1 = base_dir + "/wasm_spec_tests/wasms/float_exprs.1.wasm";
std::vector<uint8_t> wasm_float_exprs_1= read_wasm(wasm_str_float_exprs_1.c_str());

BOOST_DATA_TEST_CASE(float_exprs_1_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_1);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_10 = base_dir + "/wasm_spec_tests/wasms/float_exprs.10.wasm";
std::vector<uint8_t> wasm_float_exprs_10= read_wasm(wasm_str_float_exprs_10.c_str());

BOOST_DATA_TEST_CASE(float_exprs_10_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_10);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_11 = base_dir + "/wasm_spec_tests/wasms/float_exprs.11.wasm";
std::vector<uint8_t> wasm_float_exprs_11= read_wasm(wasm_str_float_exprs_11.c_str());

BOOST_DATA_TEST_CASE(float_exprs_11_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_11);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_12 = base_dir + "/wasm_spec_tests/wasms/float_exprs.12.wasm";
std::vector<uint8_t> wasm_float_exprs_12= read_wasm(wasm_str_float_exprs_12.c_str());

BOOST_DATA_TEST_CASE(float_exprs_12_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_12);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_13 = base_dir + "/wasm_spec_tests/wasms/float_exprs.13.wasm";
std::vector<uint8_t> wasm_float_exprs_13= read_wasm(wasm_str_float_exprs_13.c_str());

BOOST_DATA_TEST_CASE(float_exprs_13_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_13);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_14 = base_dir + "/wasm_spec_tests/wasms/float_exprs.14.wasm";
std::vector<uint8_t> wasm_float_exprs_14= read_wasm(wasm_str_float_exprs_14.c_str());

BOOST_DATA_TEST_CASE(float_exprs_14_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_14);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_15 = base_dir + "/wasm_spec_tests/wasms/float_exprs.15.wasm";
std::vector<uint8_t> wasm_float_exprs_15= read_wasm(wasm_str_float_exprs_15.c_str());

BOOST_DATA_TEST_CASE(float_exprs_15_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_15);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_16 = base_dir + "/wasm_spec_tests/wasms/float_exprs.16.wasm";
std::vector<uint8_t> wasm_float_exprs_16= read_wasm(wasm_str_float_exprs_16.c_str());

BOOST_DATA_TEST_CASE(float_exprs_16_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_16);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_17 = base_dir + "/wasm_spec_tests/wasms/float_exprs.17.wasm";
std::vector<uint8_t> wasm_float_exprs_17= read_wasm(wasm_str_float_exprs_17.c_str());

BOOST_DATA_TEST_CASE(float_exprs_17_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_17);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_18 = base_dir + "/wasm_spec_tests/wasms/float_exprs.18.wasm";
std::vector<uint8_t> wasm_float_exprs_18= read_wasm(wasm_str_float_exprs_18.c_str());

BOOST_DATA_TEST_CASE(float_exprs_18_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_18);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_19 = base_dir + "/wasm_spec_tests/wasms/float_exprs.19.wasm";
std::vector<uint8_t> wasm_float_exprs_19= read_wasm(wasm_str_float_exprs_19.c_str());

BOOST_DATA_TEST_CASE(float_exprs_19_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_19);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_2 = base_dir + "/wasm_spec_tests/wasms/float_exprs.2.wasm";
std::vector<uint8_t> wasm_float_exprs_2= read_wasm(wasm_str_float_exprs_2.c_str());

BOOST_DATA_TEST_CASE(float_exprs_2_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_2);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_20 = base_dir + "/wasm_spec_tests/wasms/float_exprs.20.wasm";
std::vector<uint8_t> wasm_float_exprs_20= read_wasm(wasm_str_float_exprs_20.c_str());

BOOST_DATA_TEST_CASE(float_exprs_20_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_20);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_21 = base_dir + "/wasm_spec_tests/wasms/float_exprs.21.wasm";
std::vector<uint8_t> wasm_float_exprs_21= read_wasm(wasm_str_float_exprs_21.c_str());

BOOST_DATA_TEST_CASE(float_exprs_21_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_21);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_22 = base_dir + "/wasm_spec_tests/wasms/float_exprs.22.wasm";
std::vector<uint8_t> wasm_float_exprs_22= read_wasm(wasm_str_float_exprs_22.c_str());

BOOST_DATA_TEST_CASE(float_exprs_22_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_22);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_23 = base_dir + "/wasm_spec_tests/wasms/float_exprs.23.wasm";
std::vector<uint8_t> wasm_float_exprs_23= read_wasm(wasm_str_float_exprs_23.c_str());

BOOST_DATA_TEST_CASE(float_exprs_23_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_23);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_24 = base_dir + "/wasm_spec_tests/wasms/float_exprs.24.wasm";
std::vector<uint8_t> wasm_float_exprs_24= read_wasm(wasm_str_float_exprs_24.c_str());

BOOST_DATA_TEST_CASE(float_exprs_24_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_24);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_25 = base_dir + "/wasm_spec_tests/wasms/float_exprs.25.wasm";
std::vector<uint8_t> wasm_float_exprs_25= read_wasm(wasm_str_float_exprs_25.c_str());

BOOST_DATA_TEST_CASE(float_exprs_25_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_25);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_26 = base_dir + "/wasm_spec_tests/wasms/float_exprs.26.wasm";
std::vector<uint8_t> wasm_float_exprs_26= read_wasm(wasm_str_float_exprs_26.c_str());

BOOST_DATA_TEST_CASE(float_exprs_26_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_26);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_27 = base_dir + "/wasm_spec_tests/wasms/float_exprs.27.wasm";
std::vector<uint8_t> wasm_float_exprs_27= read_wasm(wasm_str_float_exprs_27.c_str());

BOOST_DATA_TEST_CASE(float_exprs_27_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_27);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_28 = base_dir + "/wasm_spec_tests/wasms/float_exprs.28.wasm";
std::vector<uint8_t> wasm_float_exprs_28= read_wasm(wasm_str_float_exprs_28.c_str());

BOOST_DATA_TEST_CASE(float_exprs_28_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_28);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_29 = base_dir + "/wasm_spec_tests/wasms/float_exprs.29.wasm";
std::vector<uint8_t> wasm_float_exprs_29= read_wasm(wasm_str_float_exprs_29.c_str());

BOOST_DATA_TEST_CASE(float_exprs_29_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_29);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_3 = base_dir + "/wasm_spec_tests/wasms/float_exprs.3.wasm";
std::vector<uint8_t> wasm_float_exprs_3= read_wasm(wasm_str_float_exprs_3.c_str());

BOOST_DATA_TEST_CASE(float_exprs_3_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_3);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_30 = base_dir + "/wasm_spec_tests/wasms/float_exprs.30.wasm";
std::vector<uint8_t> wasm_float_exprs_30= read_wasm(wasm_str_float_exprs_30.c_str());

BOOST_DATA_TEST_CASE(float_exprs_30_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_30);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_31 = base_dir + "/wasm_spec_tests/wasms/float_exprs.31.wasm";
std::vector<uint8_t> wasm_float_exprs_31= read_wasm(wasm_str_float_exprs_31.c_str());

BOOST_DATA_TEST_CASE(float_exprs_31_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_31);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_32 = base_dir + "/wasm_spec_tests/wasms/float_exprs.32.wasm";
std::vector<uint8_t> wasm_float_exprs_32= read_wasm(wasm_str_float_exprs_32.c_str());

BOOST_DATA_TEST_CASE(float_exprs_32_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_32);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_33 = base_dir + "/wasm_spec_tests/wasms/float_exprs.33.wasm";
std::vector<uint8_t> wasm_float_exprs_33= read_wasm(wasm_str_float_exprs_33.c_str());

BOOST_DATA_TEST_CASE(float_exprs_33_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_33);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_34 = base_dir + "/wasm_spec_tests/wasms/float_exprs.34.wasm";
std::vector<uint8_t> wasm_float_exprs_34= read_wasm(wasm_str_float_exprs_34.c_str());

BOOST_DATA_TEST_CASE(float_exprs_34_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_34);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_35 = base_dir + "/wasm_spec_tests/wasms/float_exprs.35.wasm";
std::vector<uint8_t> wasm_float_exprs_35= read_wasm(wasm_str_float_exprs_35.c_str());

BOOST_DATA_TEST_CASE(float_exprs_35_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_35);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_36 = base_dir + "/wasm_spec_tests/wasms/float_exprs.36.wasm";
std::vector<uint8_t> wasm_float_exprs_36= read_wasm(wasm_str_float_exprs_36.c_str());

BOOST_DATA_TEST_CASE(float_exprs_36_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_36);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_37 = base_dir + "/wasm_spec_tests/wasms/float_exprs.37.wasm";
std::vector<uint8_t> wasm_float_exprs_37= read_wasm(wasm_str_float_exprs_37.c_str());

BOOST_DATA_TEST_CASE(float_exprs_37_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_37);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_38 = base_dir + "/wasm_spec_tests/wasms/float_exprs.38.wasm";
std::vector<uint8_t> wasm_float_exprs_38= read_wasm(wasm_str_float_exprs_38.c_str());

BOOST_DATA_TEST_CASE(float_exprs_38_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_38);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_39 = base_dir + "/wasm_spec_tests/wasms/float_exprs.39.wasm";
std::vector<uint8_t> wasm_float_exprs_39= read_wasm(wasm_str_float_exprs_39.c_str());

BOOST_DATA_TEST_CASE(float_exprs_39_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_39);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_4 = base_dir + "/wasm_spec_tests/wasms/float_exprs.4.wasm";
std::vector<uint8_t> wasm_float_exprs_4= read_wasm(wasm_str_float_exprs_4.c_str());

BOOST_DATA_TEST_CASE(float_exprs_4_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_4);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_40 = base_dir + "/wasm_spec_tests/wasms/float_exprs.40.wasm";
std::vector<uint8_t> wasm_float_exprs_40= read_wasm(wasm_str_float_exprs_40.c_str());

BOOST_DATA_TEST_CASE(float_exprs_40_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_40);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_41 = base_dir + "/wasm_spec_tests/wasms/float_exprs.41.wasm";
std::vector<uint8_t> wasm_float_exprs_41= read_wasm(wasm_str_float_exprs_41.c_str());

BOOST_DATA_TEST_CASE(float_exprs_41_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_41);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_42 = base_dir + "/wasm_spec_tests/wasms/float_exprs.42.wasm";
std::vector<uint8_t> wasm_float_exprs_42= read_wasm(wasm_str_float_exprs_42.c_str());

BOOST_DATA_TEST_CASE(float_exprs_42_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_42);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_43 = base_dir + "/wasm_spec_tests/wasms/float_exprs.43.wasm";
std::vector<uint8_t> wasm_float_exprs_43= read_wasm(wasm_str_float_exprs_43.c_str());

BOOST_DATA_TEST_CASE(float_exprs_43_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_43);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_44 = base_dir + "/wasm_spec_tests/wasms/float_exprs.44.wasm";
std::vector<uint8_t> wasm_float_exprs_44= read_wasm(wasm_str_float_exprs_44.c_str());

BOOST_DATA_TEST_CASE(float_exprs_44_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_44);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_45 = base_dir + "/wasm_spec_tests/wasms/float_exprs.45.wasm";
std::vector<uint8_t> wasm_float_exprs_45= read_wasm(wasm_str_float_exprs_45.c_str());

BOOST_DATA_TEST_CASE(float_exprs_45_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_45);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_46 = base_dir + "/wasm_spec_tests/wasms/float_exprs.46.wasm";
std::vector<uint8_t> wasm_float_exprs_46= read_wasm(wasm_str_float_exprs_46.c_str());

BOOST_DATA_TEST_CASE(float_exprs_46_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_46);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_47 = base_dir + "/wasm_spec_tests/wasms/float_exprs.47.wasm";
std::vector<uint8_t> wasm_float_exprs_47= read_wasm(wasm_str_float_exprs_47.c_str());

BOOST_DATA_TEST_CASE(float_exprs_47_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_47);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_48 = base_dir + "/wasm_spec_tests/wasms/float_exprs.48.wasm";
std::vector<uint8_t> wasm_float_exprs_48= read_wasm(wasm_str_float_exprs_48.c_str());

BOOST_DATA_TEST_CASE(float_exprs_48_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_48);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_49 = base_dir + "/wasm_spec_tests/wasms/float_exprs.49.wasm";
std::vector<uint8_t> wasm_float_exprs_49= read_wasm(wasm_str_float_exprs_49.c_str());

BOOST_DATA_TEST_CASE(float_exprs_49_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_49);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_5 = base_dir + "/wasm_spec_tests/wasms/float_exprs.5.wasm";
std::vector<uint8_t> wasm_float_exprs_5= read_wasm(wasm_str_float_exprs_5.c_str());

BOOST_DATA_TEST_CASE(float_exprs_5_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_5);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_50 = base_dir + "/wasm_spec_tests/wasms/float_exprs.50.wasm";
std::vector<uint8_t> wasm_float_exprs_50= read_wasm(wasm_str_float_exprs_50.c_str());

BOOST_DATA_TEST_CASE(float_exprs_50_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_50);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_51 = base_dir + "/wasm_spec_tests/wasms/float_exprs.51.wasm";
std::vector<uint8_t> wasm_float_exprs_51= read_wasm(wasm_str_float_exprs_51.c_str());

BOOST_DATA_TEST_CASE(float_exprs_51_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_51);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_52 = base_dir + "/wasm_spec_tests/wasms/float_exprs.52.wasm";
std::vector<uint8_t> wasm_float_exprs_52= read_wasm(wasm_str_float_exprs_52.c_str());

BOOST_DATA_TEST_CASE(float_exprs_52_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_52);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_53 = base_dir + "/wasm_spec_tests/wasms/float_exprs.53.wasm";
std::vector<uint8_t> wasm_float_exprs_53= read_wasm(wasm_str_float_exprs_53.c_str());

BOOST_DATA_TEST_CASE(float_exprs_53_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_53);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_54 = base_dir + "/wasm_spec_tests/wasms/float_exprs.54.wasm";
std::vector<uint8_t> wasm_float_exprs_54= read_wasm(wasm_str_float_exprs_54.c_str());

BOOST_DATA_TEST_CASE(float_exprs_54_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_54);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_55 = base_dir + "/wasm_spec_tests/wasms/float_exprs.55.wasm";
std::vector<uint8_t> wasm_float_exprs_55= read_wasm(wasm_str_float_exprs_55.c_str());

BOOST_DATA_TEST_CASE(float_exprs_55_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_55);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_56 = base_dir + "/wasm_spec_tests/wasms/float_exprs.56.wasm";
std::vector<uint8_t> wasm_float_exprs_56= read_wasm(wasm_str_float_exprs_56.c_str());

BOOST_DATA_TEST_CASE(float_exprs_56_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_56);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_57 = base_dir + "/wasm_spec_tests/wasms/float_exprs.57.wasm";
std::vector<uint8_t> wasm_float_exprs_57= read_wasm(wasm_str_float_exprs_57.c_str());

BOOST_DATA_TEST_CASE(float_exprs_57_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_57);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_58 = base_dir + "/wasm_spec_tests/wasms/float_exprs.58.wasm";
std::vector<uint8_t> wasm_float_exprs_58= read_wasm(wasm_str_float_exprs_58.c_str());

BOOST_DATA_TEST_CASE(float_exprs_58_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_58);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_59 = base_dir + "/wasm_spec_tests/wasms/float_exprs.59.wasm";
std::vector<uint8_t> wasm_float_exprs_59= read_wasm(wasm_str_float_exprs_59.c_str());

BOOST_DATA_TEST_CASE(float_exprs_59_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_59);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_6 = base_dir + "/wasm_spec_tests/wasms/float_exprs.6.wasm";
std::vector<uint8_t> wasm_float_exprs_6= read_wasm(wasm_str_float_exprs_6.c_str());

BOOST_DATA_TEST_CASE(float_exprs_6_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_6);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_60 = base_dir + "/wasm_spec_tests/wasms/float_exprs.60.wasm";
std::vector<uint8_t> wasm_float_exprs_60= read_wasm(wasm_str_float_exprs_60.c_str());

BOOST_DATA_TEST_CASE(float_exprs_60_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_60);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_61 = base_dir + "/wasm_spec_tests/wasms/float_exprs.61.wasm";
std::vector<uint8_t> wasm_float_exprs_61= read_wasm(wasm_str_float_exprs_61.c_str());

BOOST_DATA_TEST_CASE(float_exprs_61_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_61);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_62 = base_dir + "/wasm_spec_tests/wasms/float_exprs.62.wasm";
std::vector<uint8_t> wasm_float_exprs_62= read_wasm(wasm_str_float_exprs_62.c_str());

BOOST_DATA_TEST_CASE(float_exprs_62_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_62);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_63 = base_dir + "/wasm_spec_tests/wasms/float_exprs.63.wasm";
std::vector<uint8_t> wasm_float_exprs_63= read_wasm(wasm_str_float_exprs_63.c_str());

BOOST_DATA_TEST_CASE(float_exprs_63_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_63);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_64 = base_dir + "/wasm_spec_tests/wasms/float_exprs.64.wasm";
std::vector<uint8_t> wasm_float_exprs_64= read_wasm(wasm_str_float_exprs_64.c_str());

BOOST_DATA_TEST_CASE(float_exprs_64_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_64);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_65 = base_dir + "/wasm_spec_tests/wasms/float_exprs.65.wasm";
std::vector<uint8_t> wasm_float_exprs_65= read_wasm(wasm_str_float_exprs_65.c_str());

BOOST_DATA_TEST_CASE(float_exprs_65_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_65);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_66 = base_dir + "/wasm_spec_tests/wasms/float_exprs.66.wasm";
std::vector<uint8_t> wasm_float_exprs_66= read_wasm(wasm_str_float_exprs_66.c_str());

BOOST_DATA_TEST_CASE(float_exprs_66_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_66);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_67 = base_dir + "/wasm_spec_tests/wasms/float_exprs.67.wasm";
std::vector<uint8_t> wasm_float_exprs_67= read_wasm(wasm_str_float_exprs_67.c_str());

BOOST_DATA_TEST_CASE(float_exprs_67_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_67);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_68 = base_dir + "/wasm_spec_tests/wasms/float_exprs.68.wasm";
std::vector<uint8_t> wasm_float_exprs_68= read_wasm(wasm_str_float_exprs_68.c_str());

BOOST_DATA_TEST_CASE(float_exprs_68_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_68);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_69 = base_dir + "/wasm_spec_tests/wasms/float_exprs.69.wasm";
std::vector<uint8_t> wasm_float_exprs_69= read_wasm(wasm_str_float_exprs_69.c_str());

BOOST_DATA_TEST_CASE(float_exprs_69_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_69);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_7 = base_dir + "/wasm_spec_tests/wasms/float_exprs.7.wasm";
std::vector<uint8_t> wasm_float_exprs_7= read_wasm(wasm_str_float_exprs_7.c_str());

BOOST_DATA_TEST_CASE(float_exprs_7_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_7);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_70 = base_dir + "/wasm_spec_tests/wasms/float_exprs.70.wasm";
std::vector<uint8_t> wasm_float_exprs_70= read_wasm(wasm_str_float_exprs_70.c_str());

BOOST_DATA_TEST_CASE(float_exprs_70_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_70);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_71 = base_dir + "/wasm_spec_tests/wasms/float_exprs.71.wasm";
std::vector<uint8_t> wasm_float_exprs_71= read_wasm(wasm_str_float_exprs_71.c_str());

BOOST_DATA_TEST_CASE(float_exprs_71_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_71);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_72 = base_dir + "/wasm_spec_tests/wasms/float_exprs.72.wasm";
std::vector<uint8_t> wasm_float_exprs_72= read_wasm(wasm_str_float_exprs_72.c_str());

BOOST_DATA_TEST_CASE(float_exprs_72_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_72);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_73 = base_dir + "/wasm_spec_tests/wasms/float_exprs.73.wasm";
std::vector<uint8_t> wasm_float_exprs_73= read_wasm(wasm_str_float_exprs_73.c_str());

BOOST_DATA_TEST_CASE(float_exprs_73_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_73);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_74 = base_dir + "/wasm_spec_tests/wasms/float_exprs.74.wasm";
std::vector<uint8_t> wasm_float_exprs_74= read_wasm(wasm_str_float_exprs_74.c_str());

BOOST_DATA_TEST_CASE(float_exprs_74_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_74);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_75 = base_dir + "/wasm_spec_tests/wasms/float_exprs.75.wasm";
std::vector<uint8_t> wasm_float_exprs_75= read_wasm(wasm_str_float_exprs_75.c_str());

BOOST_DATA_TEST_CASE(float_exprs_75_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_75);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_76 = base_dir + "/wasm_spec_tests/wasms/float_exprs.76.wasm";
std::vector<uint8_t> wasm_float_exprs_76= read_wasm(wasm_str_float_exprs_76.c_str());

BOOST_DATA_TEST_CASE(float_exprs_76_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_76);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_77 = base_dir + "/wasm_spec_tests/wasms/float_exprs.77.wasm";
std::vector<uint8_t> wasm_float_exprs_77= read_wasm(wasm_str_float_exprs_77.c_str());

BOOST_DATA_TEST_CASE(float_exprs_77_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_77);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_78 = base_dir + "/wasm_spec_tests/wasms/float_exprs.78.wasm";
std::vector<uint8_t> wasm_float_exprs_78= read_wasm(wasm_str_float_exprs_78.c_str());

BOOST_DATA_TEST_CASE(float_exprs_78_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_78);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_79 = base_dir + "/wasm_spec_tests/wasms/float_exprs.79.wasm";
std::vector<uint8_t> wasm_float_exprs_79= read_wasm(wasm_str_float_exprs_79.c_str());

BOOST_DATA_TEST_CASE(float_exprs_79_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_79);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_8 = base_dir + "/wasm_spec_tests/wasms/float_exprs.8.wasm";
std::vector<uint8_t> wasm_float_exprs_8= read_wasm(wasm_str_float_exprs_8.c_str());

BOOST_DATA_TEST_CASE(float_exprs_8_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_8);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_80 = base_dir + "/wasm_spec_tests/wasms/float_exprs.80.wasm";
std::vector<uint8_t> wasm_float_exprs_80= read_wasm(wasm_str_float_exprs_80.c_str());

BOOST_DATA_TEST_CASE(float_exprs_80_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_80);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_81 = base_dir + "/wasm_spec_tests/wasms/float_exprs.81.wasm";
std::vector<uint8_t> wasm_float_exprs_81= read_wasm(wasm_str_float_exprs_81.c_str());

BOOST_DATA_TEST_CASE(float_exprs_81_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_81);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_82 = base_dir + "/wasm_spec_tests/wasms/float_exprs.82.wasm";
std::vector<uint8_t> wasm_float_exprs_82= read_wasm(wasm_str_float_exprs_82.c_str());

BOOST_DATA_TEST_CASE(float_exprs_82_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_82);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_83 = base_dir + "/wasm_spec_tests/wasms/float_exprs.83.wasm";
std::vector<uint8_t> wasm_float_exprs_83= read_wasm(wasm_str_float_exprs_83.c_str());

BOOST_DATA_TEST_CASE(float_exprs_83_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_83);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_84 = base_dir + "/wasm_spec_tests/wasms/float_exprs.84.wasm";
std::vector<uint8_t> wasm_float_exprs_84= read_wasm(wasm_str_float_exprs_84.c_str());

BOOST_DATA_TEST_CASE(float_exprs_84_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_84);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_85 = base_dir + "/wasm_spec_tests/wasms/float_exprs.85.wasm";
std::vector<uint8_t> wasm_float_exprs_85= read_wasm(wasm_str_float_exprs_85.c_str());

BOOST_DATA_TEST_CASE(float_exprs_85_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_85);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_86 = base_dir + "/wasm_spec_tests/wasms/float_exprs.86.wasm";
std::vector<uint8_t> wasm_float_exprs_86= read_wasm(wasm_str_float_exprs_86.c_str());

BOOST_DATA_TEST_CASE(float_exprs_86_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_86);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_87 = base_dir + "/wasm_spec_tests/wasms/float_exprs.87.wasm";
std::vector<uint8_t> wasm_float_exprs_87= read_wasm(wasm_str_float_exprs_87.c_str());

BOOST_DATA_TEST_CASE(float_exprs_87_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_87);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_88 = base_dir + "/wasm_spec_tests/wasms/float_exprs.88.wasm";
std::vector<uint8_t> wasm_float_exprs_88= read_wasm(wasm_str_float_exprs_88.c_str());

BOOST_DATA_TEST_CASE(float_exprs_88_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_88);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_89 = base_dir + "/wasm_spec_tests/wasms/float_exprs.89.wasm";
std::vector<uint8_t> wasm_float_exprs_89= read_wasm(wasm_str_float_exprs_89.c_str());

BOOST_DATA_TEST_CASE(float_exprs_89_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_89);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_9 = base_dir + "/wasm_spec_tests/wasms/float_exprs.9.wasm";
std::vector<uint8_t> wasm_float_exprs_9= read_wasm(wasm_str_float_exprs_9.c_str());

BOOST_DATA_TEST_CASE(float_exprs_9_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_9);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_90 = base_dir + "/wasm_spec_tests/wasms/float_exprs.90.wasm";
std::vector<uint8_t> wasm_float_exprs_90= read_wasm(wasm_str_float_exprs_90.c_str());

BOOST_DATA_TEST_CASE(float_exprs_90_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_90);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_91 = base_dir + "/wasm_spec_tests/wasms/float_exprs.91.wasm";
std::vector<uint8_t> wasm_float_exprs_91= read_wasm(wasm_str_float_exprs_91.c_str());

BOOST_DATA_TEST_CASE(float_exprs_91_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_91);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_92 = base_dir + "/wasm_spec_tests/wasms/float_exprs.92.wasm";
std::vector<uint8_t> wasm_float_exprs_92= read_wasm(wasm_str_float_exprs_92.c_str());

BOOST_DATA_TEST_CASE(float_exprs_92_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_92);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_93 = base_dir + "/wasm_spec_tests/wasms/float_exprs.93.wasm";
std::vector<uint8_t> wasm_float_exprs_93= read_wasm(wasm_str_float_exprs_93.c_str());

BOOST_DATA_TEST_CASE(float_exprs_93_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_93);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_94 = base_dir + "/wasm_spec_tests/wasms/float_exprs.94.wasm";
std::vector<uint8_t> wasm_float_exprs_94= read_wasm(wasm_str_float_exprs_94.c_str());

BOOST_DATA_TEST_CASE(float_exprs_94_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_94);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

const string wasm_str_float_exprs_95 = base_dir + "/wasm_spec_tests/wasms/float_exprs.95.wasm";
std::vector<uint8_t> wasm_float_exprs_95= read_wasm(wasm_str_float_exprs_95.c_str());

BOOST_DATA_TEST_CASE(float_exprs_95_pass, boost::unit_test::data::xrange(0,1), index) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm_float_exprs_95);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)index);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

