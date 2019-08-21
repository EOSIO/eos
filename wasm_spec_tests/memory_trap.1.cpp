#include <wasm_spec_tests.hpp>
BOOST_AUTO_TEST_SUITE(memory_trap_1)

const string wasm_str = base_dir + "/wasm_spec_tests/wasms/memory_trap.1.wasm";
std::vector<uint8_t> wasm = read_wasm(wasm_str.c_str());

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_0) { try {
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

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_1) { try {
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

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_2) { try {
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

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_3) { try {
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

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_4) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)4);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_5) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)5);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_6) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)6);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_7) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)7);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_8) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)8);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_9) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)9);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_10) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)10);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_11) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)11);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_12) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)12);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_13) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)13);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_14) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)14);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_15) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)15);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_16) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)16);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_17) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)17);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_18) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)18);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_19) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)19);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_20) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)20);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_21) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)21);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_22) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)22);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_23) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)23);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_24) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)24);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_25) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)25);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_26) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)26);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_27) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)27);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_28) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)28);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_29) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)29);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_30) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)30);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_31) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)31);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_32) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)32);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_33) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)33);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_34) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)34);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_35) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)35);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_36) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)36);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_37) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)37);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_38) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)38);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_39) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)39);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_40) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)40);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_41) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)41);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_42) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)42);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_43) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)43);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_44) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)44);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_45) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)45);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_46) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)46);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_47) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)47);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_48) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)48);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_49) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)49);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_50) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)50);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_51) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)51);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_52) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)52);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_53) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)53);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_54) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)54);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_55) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)55);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_56) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)56);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_57) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)57);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_58) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)58);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_59) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)59);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_60) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)60);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_61) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)61);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_62) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)62);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_63) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)63);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_64) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)64);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_65) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)65);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_66) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)66);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_67) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)67);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_68) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)68);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_69) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)69);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_70) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)70);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_71) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)71);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_72) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)72);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_73) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)73);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_74) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)74);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_75) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)75);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_76) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)76);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_77) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)77);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_78) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)78);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_79) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)79);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_80) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)80);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_81) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)81);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_82) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)82);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_83) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)83);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_84) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)84);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_85) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)85);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_86) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)86);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_87) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)87);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_88) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)88);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_89) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)89);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_90) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)90);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_91) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)91);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_92) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)92);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_93) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)93);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_94) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)94);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_95) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)95);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_96) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)96);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_97) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)97);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_98) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)98);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_99) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)99);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_100) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)100);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_101) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)101);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_102) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)102);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_103) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)103);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_104) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)104);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_105) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)105);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_106) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)106);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_107) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)107);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_108) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)108);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_109) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)109);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_110) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)110);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_111) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)111);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_112) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)112);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_113) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)113);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_114) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)114);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_115) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)115);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_116) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)116);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_117) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)117);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_118) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)118);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_119) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)119);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_120) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)120);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_121) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)121);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_122) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)122);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_123) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)123);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_124) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)124);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_125) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)125);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_126) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)126);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_127) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)127);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_128) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)128);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_129) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)129);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_130) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)130);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_131) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)131);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_132) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)132);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_133) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)133);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_134) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)134);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_135) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)135);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_136) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)136);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_137) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)137);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_138) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)138);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_139) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)139);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_140) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)140);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_141) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)141);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_142) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)142);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_143) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)143);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_144) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)144);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_145) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)145);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_146) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)146);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_147) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)147);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_148) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)148);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_149) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)149);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_150) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)150);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_151) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)151);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_152) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)152);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_153) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)153);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_154) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)154);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_155) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)155);
   test.authorization = {{N(wasmtest), config::active_name}};

   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), wasm_execution_error);
   tester.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(memory_trap_1_sub_apply_156) { try {
   TESTER tester;
   tester.produce_block();
   tester.create_account( N(wasmtest) );
   tester.produce_block();
   tester.set_code(N(wasmtest), wasm);
   tester.produce_block();

   action test;
   test.account = N(wasmtest);
   test.name = account_name((uint64_t)156);
   test.authorization = {{N(wasmtest), config::active_name}};

   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());
   tester.produce_block();
   BOOST_REQUIRE_EQUAL( tester.validate(), true );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
