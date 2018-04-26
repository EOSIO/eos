#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <asserter/asserter.wast.hpp>
#include <asserter/asserter.abi.hpp>

#include <stltest/stltest.wast.hpp>
#include <stltest/stltest.abi.hpp>

#include <noop/noop.wast.hpp>
#include <noop/noop.abi.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include "test_wasts.hpp"
#include "test_softfloat_wasts.hpp"

#include <array>
#include <utility>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;


struct assertdef {
   int8_t      condition;
   string      message;

   static account_name get_account() {
      return N(asserter);
   }

   static action_name get_name() {
      return N(procassert);
   }
};

FC_REFLECT(assertdef, (condition)(message));

struct provereset {
   static account_name get_account() {
      return N(asserter);
   }

   static action_name get_name() {
      return N(provereset);
   }
};

FC_REFLECT_EMPTY(provereset);

BOOST_AUTO_TEST_SUITE(wasm_tests)

/**
 * Prove that action reading and assertions are working
 */
BOOST_FIXTURE_TEST_CASE( basic_test, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)} );
   produce_block();

   set_code(N(asserter), asserter_wast);
   produce_blocks(1);

   transaction_id_type no_assert_id;
   {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                assertdef {1, "Should Not Assert!"} );
      trx.actions[0].authorization = {{N(asserter),config::active_name}};

      set_transaction_headers(trx);
      trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
      auto result = push_transaction( trx );
      BOOST_CHECK_EQUAL(result.status, transaction_receipt::executed);
      BOOST_CHECK_EQUAL(result.action_traces.size(), 1);
      BOOST_CHECK_EQUAL(result.action_traces.at(0).receiver.to_string(),  name(N(asserter)).to_string() );
      BOOST_CHECK_EQUAL(result.action_traces.at(0).act.account.to_string(), name(N(asserter)).to_string() );
      BOOST_CHECK_EQUAL(result.action_traces.at(0).act.name.to_string(),  name(N(procassert)).to_string() );
      BOOST_CHECK_EQUAL(result.action_traces.at(0).act.authorization.size(),  1 );
      BOOST_CHECK_EQUAL(result.action_traces.at(0).act.authorization.at(0).actor.to_string(),  name(N(asserter)).to_string() );
      BOOST_CHECK_EQUAL(result.action_traces.at(0).act.authorization.at(0).permission.to_string(),  name(config::active_name).to_string() );
      no_assert_id = trx.id();
   }

   produce_blocks(1);

   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(no_assert_id));
   const auto& receipt = get_transaction_receipt(no_assert_id);
   BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);

   transaction_id_type yes_assert_id;
   {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                assertdef {0, "Should Assert!"} );

      set_transaction_headers(trx);
      trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
      yes_assert_id = trx.id();

      BOOST_CHECK_THROW(push_transaction( trx ), transaction_exception);
   }

   produce_blocks(1);

   auto has_tx = chain_has_transaction(yes_assert_id);
   BOOST_REQUIRE_EQUAL(false, has_tx);

} FC_LOG_AND_RETHROW() /// basic_test

/**
 * Make sure WASM doesn't allow function call depths greater than 250 
 */
BOOST_FIXTURE_TEST_CASE( call_depth_test, TESTER ) try {
   produce_blocks(2);
   create_accounts( {N(check)} );
   produce_block();
   // test that we can call to 249
   {
      set_code(N(check), call_depth_almost_limit_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(check);
      act.name = N();
      act.authorization = vector<permission_level>{{N(check),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(check), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }
   // should fail at 250
   {
      set_code(N(check), call_depth_limit_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(check);
      act.name = N();
      act.authorization = vector<permission_level>{{N(check),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(check), "active" ), chain_id_type());
      BOOST_CHECK_THROW( push_transaction(trx), wasm_execution_error );
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(false, chain_has_transaction(trx.id()));
   }
} FC_LOG_AND_RETHROW()

/**
 * Prove the modifications to global variables are wiped between runs
 */
BOOST_FIXTURE_TEST_CASE( prove_mem_reset, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)} );
   produce_block();

   set_code(N(asserter), asserter_wast);
   produce_blocks(1);

   // repeat the action multiple times, each time the action handler checks for the expected
   // default value then modifies the value which should not survive until the next invoction
   for (int i = 0; i < 5; i++) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                provereset {} );

      set_transaction_headers(trx);
      trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
      push_transaction( trx );
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
      BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);
   }

} FC_LOG_AND_RETHROW() /// prove_mem_reset

/**
 * Prove the modifications to global variables are wiped between runs
 */
BOOST_FIXTURE_TEST_CASE( abi_from_variant, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)} );
   produce_block();

   set_code(N(asserter), asserter_wast);
   set_abi(N(asserter), asserter_abi);
   produce_blocks(1);

   auto resolver = [&,this]( const account_name& name ) -> optional<abi_serializer> {
      try {
         const auto& accnt  = this->control->get_database().get<account_object,by_name>( name );
         abi_def abi;
         if (abi_serializer::to_abi(accnt.abi, abi)) {
            return abi_serializer(abi);
         }
         return optional<abi_serializer>();
      } FC_RETHROW_EXCEPTIONS(error, "Failed to find or parse ABI for ${name}", ("name", name))
   };

   variant pretty_trx = mutable_variant_object()
      ("actions", variants({
         mutable_variant_object()
            ("account", "asserter")
            ("name", "procassert")
            ("authorization", variants({
               mutable_variant_object()
                  ("actor", "asserter")
                  ("permission", name(config::active_name).to_string())
            }))
            ("data", mutable_variant_object()
               ("condition", 1)
               ("message", "Should Not Assert!")
            )
         })
      );

   signed_transaction trx;
   abi_serializer::from_variant(pretty_trx, trx, resolver);
   set_transaction_headers(trx);
   trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
   push_transaction( trx );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   const auto& receipt = get_transaction_receipt(trx.id());
   BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);

} FC_LOG_AND_RETHROW() /// prove_mem_reset

// test softfloat 32 bit operations
BOOST_FIXTURE_TEST_CASE( f32_tests, TESTER ) try {
   produce_blocks(2);
   account_name an = N(f_tests);
   create_accounts( {N(f32_tests)} );
   produce_block();
   {
      set_code(N(f32_tests), f32_test_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f32_tests);
      act.name = N();
      act.authorization = vector<permission_level>{{N(f32_tests),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f32_tests), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
   }
   {
      set_code(N(f32_tests), f32_bitwise_test_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f32_tests);
      act.name = N();
      act.authorization = vector<permission_level>{{N(f32_tests),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f32_tests), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
   }
   {
      set_code(N(f32_tests), f32_cmp_test_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f32_tests);
      act.name = N();
      act.authorization = vector<permission_level>{{N(f32_tests),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f32_tests), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
   }
} FC_LOG_AND_RETHROW()

// test softfloat 64 bit operations
BOOST_FIXTURE_TEST_CASE( f64_tests, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(f_tests)} );
   produce_block();
   {
      set_code(N(f_tests), f64_test_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f_tests);
      act.name = N();
      act.authorization = vector<permission_level>{{N(f_tests),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f_tests), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
   }
   {
      set_code(N(f_tests), f64_bitwise_test_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f_tests);
      act.name = N();
      act.authorization = vector<permission_level>{{N(f_tests),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f_tests), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
   }
   {
      set_code(N(f_tests), f64_cmp_test_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f_tests);
      act.name = N();
      act.authorization = vector<permission_level>{{N(f_tests),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f_tests), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
   }
} FC_LOG_AND_RETHROW()

// test softfloat conversion operations
BOOST_FIXTURE_TEST_CASE( f32_f64_conversion_tests, tester ) try {
   produce_blocks(2);

   create_accounts( {N(f_tests)} );
   produce_block();
   {
      set_code(N(f_tests), f32_f64_conv_wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f_tests);
      act.name = N();
      act.authorization = vector<permission_level>{{N(f_tests),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f_tests), "active" ), chain_id_type());
      push_transaction(trx);
      produce_blocks(1);
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
   }
} FC_LOG_AND_RETHROW()

// test softfloat conversion operations
BOOST_FIXTURE_TEST_CASE( f32_f64_overflow_tests, tester ) try {
   int count = 0;
   auto check = [&](const char *wast_template, const char *op, const char *param) -> bool {
      count+=16;
      create_accounts( {N(f_tests)+count} );
      produce_blocks(1);
      std::vector<char> wast;
      wast.resize(strlen(wast_template) + 128);
      sprintf(&(wast[0]), wast_template, op, param);
      set_code(N(f_tests)+count, &(wast[0]));
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(f_tests)+count;
      act.name = N();
      act.authorization = vector<permission_level>{{N(f_tests)+count,config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f_tests)+count, "active" ), chain_id_type());

      try {
         push_transaction(trx);
         produce_blocks(1);
         BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
         const auto& receipt = get_transaction_receipt(trx.id());
         return true;
      } catch (eosio::chain::wasm_execution_error &) {
         return false;
      }
   };
   //
   //// float32 => int32
   // 2^31
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_s_f32", "f32.const 2147483648"));
   // the maximum value below 2^31 representable in IEEE float32
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_s_f32", "f32.const 2147483520"));
   // -2^31
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_s_f32", "f32.const -2147483648"));  
   // the maximum value below -2^31 in IEEE float32
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_s_f32", "f32.const -2147483904"));   

   //
   //// float32 => uint32
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_u_f32", "f32.const 0")); 
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_u_f32", "f32.const -1")); 
   // max value below 2^32 in IEEE float32
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_u_f32", "f32.const 4294967040"));
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_u_f32", "f32.const 4294967296"));

   //
   //// double => int32
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_s_f64", "f64.const 2147483648"));
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_s_f64", "f64.const 2147483647"));
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_s_f64", "f64.const -2147483648"));  
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_s_f64", "f64.const -2147483649"));   

   //
   //// double => uint32
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_u_f64", "f64.const 0")); 
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_u_f64", "f64.const -1")); 
   BOOST_REQUIRE_EQUAL(true, check(i32_overflow_wast, "i32_trunc_u_f64", "f64.const 4294967295"));
   BOOST_REQUIRE_EQUAL(false, check(i32_overflow_wast, "i32_trunc_u_f64", "f64.const 4294967296"));


   //// float32 => int64
   // 2^63
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_s_f32", "f32.const 9223372036854775808"));
   // the maximum value below 2^63 representable in IEEE float32
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_s_f32", "f32.const 9223371487098961920"));
   // -2^63
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_s_f32", "f32.const -9223372036854775808"));  
   // the maximum value below -2^63 in IEEE float32
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_s_f32", "f32.const -9223373136366403584"));  

   //// float32 => uint64
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_u_f32", "f32.const -1"));
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_u_f32", "f32.const 0"));
   // max value below 2^64 in IEEE float32
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_u_f32", "f32.const 18446742974197923840"));  
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_u_f32", "f32.const 18446744073709551616")); 

   //// double => int64
   // 2^63
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_s_f64", "f64.const 9223372036854775808"));
   // the maximum value below 2^63 representable in IEEE float64
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_s_f64", "f64.const 9223372036854774784"));
   // -2^63
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_s_f64", "f64.const -9223372036854775808"));  
   // the maximum value below -2^63 in IEEE float64
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_s_f64", "f64.const -9223372036854777856"));  

   //// double => uint64
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_u_f64", "f64.const -1"));
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_u_f64", "f64.const 0"));
   // max value below 2^64 in IEEE float64
   BOOST_REQUIRE_EQUAL(true, check(i64_overflow_wast, "i64_trunc_u_f64", "f64.const 18446744073709549568"));  
   BOOST_REQUIRE_EQUAL(false, check(i64_overflow_wast, "i64_trunc_u_f64", "f64.const 18446744073709551616")); 
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(misaligned_tests, tester ) try {
   produce_blocks(2);
   create_accounts( {N(aligncheck)} );
   produce_block();
   
   auto check_aligned = [&]( auto wast ) { 
      set_code(N(aligncheck), wast);
      produce_blocks(10);

      signed_transaction trx;
      action act;
      act.account = N(aligncheck);
      act.name = N();
      act.authorization = vector<permission_level>{{N(aligncheck),config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(aligncheck), "active" ), chain_id_type());
      push_transaction(trx);
      auto sb = produce_block();
      block_trace trace(sb);
      
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   };

   check_aligned(aligned_ref_wast);
   check_aligned(misaligned_ref_wast);
   check_aligned(aligned_const_ref_wast);
   check_aligned(misaligned_const_ref_wast);
   check_aligned(aligned_ptr_wast);
   check_aligned(misaligned_ptr_wast);
   check_aligned(misaligned_const_ptr_wast);
} FC_LOG_AND_RETHROW()

// test cpu usage
BOOST_FIXTURE_TEST_CASE(cpu_usage_tests, tester ) try {

   create_accounts( {N(f_tests)} );
   bool pass = false;

   std::string code = R"=====(
(module
  (import "env" "require_auth" (func $require_auth (param i64)))
  (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
   (table 0 anyfunc)
   (memory $0 1)
   (export "apply" (func $apply))
   (func $i64_trunc_u_f64 (param $0 f64) (result i64) (i64.trunc_u/f64 (get_local $0)))
   (func $test (param $0 i64))
   (func $apply (param $0 i64)(param $1 i64)(param $2 i64)
   )=====";
   for (int i = 0; i < 1024; ++i) {
      code += "(call $test (call $i64_trunc_u_f64 (f64.const 1)))\n";
   }
   code += "))";

   produce_blocks(1);
   set_code(N(f_tests), code.c_str());
   produce_blocks(10);

   int limit = 190;
   while (!pass && limit < 200) {
      signed_transaction trx;

      for (int i = 0; i < 100; ++i) {
         action act;
         act.account = N(f_tests);
         act.name = N() + (i * 16);
         act.authorization = vector<permission_level>{{N(f_tests),config::active_name}};
         trx.actions.push_back(act);
      }

      set_transaction_headers(trx);
      trx.max_kcpu_usage = limit++;
      trx.sign(get_private_key( N(f_tests), "active" ), chain_id_type());

      try {
         push_transaction(trx);
         produce_blocks(1);
         BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
         pass = true;
      } catch (eosio::chain::tx_resource_exhausted &) {
         produce_blocks(1);
      }

      BOOST_REQUIRE_EQUAL(true, validate());
   }
// NOTE: limit is 197
   BOOST_REQUIRE_EQUAL(true, limit > 190 && limit < 200);
} FC_LOG_AND_RETHROW()


// test weighted cpu limit
BOOST_FIXTURE_TEST_CASE(weighted_cpu_limit_tests, tester ) try {

   resource_limits_manager mgr = control->get_mutable_resource_limits_manager();
   create_accounts( {N(f_tests)} );
   create_accounts( {N(acc2)} );
   bool pass = false;

   std::string code = R"=====(
(module
  (import "env" "require_auth" (func $require_auth (param i64)))
  (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
   (table 0 anyfunc)
   (memory $0 1)
   (export "apply" (func $apply))
   (func $i64_trunc_u_f64 (param $0 f64) (result i64) (i64.trunc_u/f64 (get_local $0)))
   (func $test (param $0 i64))
   (func $apply (param $0 i64)(param $1 i64)(param $2 i64)
   )=====";
   for (int i = 0; i < 1024; ++i) {
      code += "(call $test (call $i64_trunc_u_f64 (f64.const 1)))\n";
   }
   code += "))";

   produce_blocks(1);
   set_code(N(f_tests), code.c_str());
   produce_blocks(10);

   mgr.set_account_limits(N(f_tests), -1, -1, 1);
   int count = 0;
   while (count < 4) {
      signed_transaction trx;

      for (int i = 0; i < 100; ++i) {
         action act;
         act.account = N(f_tests);
         act.name = N() + (i * 16);
         act.authorization = vector<permission_level>{{N(f_tests),config::active_name}};
         trx.actions.push_back(act);
      }

      set_transaction_headers(trx);
      trx.sign(get_private_key( N(f_tests), "active" ), chain_id_type());

      try {
         push_transaction(trx);
         produce_blocks(1);
         BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
         pass = true;
         count++;
      } catch (eosio::chain::tx_resource_exhausted &) {
         BOOST_REQUIRE_EQUAL(count, 3);
         break;
      } 
      BOOST_REQUIRE_EQUAL(true, validate());

      if (count == 2) { // add a big weight on acc2, making f_tests out of resource
        mgr.set_account_limits(N(acc2), -1, -1, 1000);
      }
   }
   BOOST_REQUIRE_EQUAL(count, 3);
} FC_LOG_AND_RETHROW()

/**
 * Make sure WASM "start" method is used correctly
 */
BOOST_FIXTURE_TEST_CASE( check_entry_behavior, TESTER ) try {
   produce_blocks(2);
   create_accounts( {N(entrycheck)} );
   produce_block();

   set_code(N(entrycheck), entry_wast);
   produce_blocks(10);

   signed_transaction trx;
   action act;
   act.account = N(entrycheck);
   act.name = N();
   act.authorization = vector<permission_level>{{N(entrycheck),config::active_name}};
   trx.actions.push_back(act);

   set_transaction_headers(trx);
   trx.sign(get_private_key( N(entrycheck), "active" ), chain_id_type());
   push_transaction(trx);
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   const auto& receipt = get_transaction_receipt(trx.id());
   BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);
} FC_LOG_AND_RETHROW()

/**
 * Ensure we can load a wasm w/o memory
 */
BOOST_FIXTURE_TEST_CASE( simple_no_memory_check, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(nomem)} );
   produce_block();

   set_code(N(nomem), simple_no_memory_wast);
   produce_blocks(1);

   //the apply func of simple_no_memory_wast tries to call a native func with linear memory pointer
   signed_transaction trx;
   action act;
   act.account = N(nomem);
   act.name = N();
   act.authorization = vector<permission_level>{{N(nomem),config::active_name}};
   trx.actions.push_back(act);
   trx.expiration = control->head_block_time();
   set_transaction_headers(trx);
   trx.sign(get_private_key( N(nomem), "active" ), chain_id_type());
   BOOST_CHECK_THROW(push_transaction( trx ), wasm_execution_error);
} FC_LOG_AND_RETHROW()

//Make sure globals are all reset to their inital values
BOOST_FIXTURE_TEST_CASE( check_global_reset, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(globalreset)} );
   produce_block();

   set_code(N(globalreset), mutable_global_wast);
   produce_blocks(1);

   signed_transaction trx;
   {
   action act;
   act.account = N(globalreset);
   act.name = name(0ULL);
   act.authorization = vector<permission_level>{{N(globalreset),config::active_name}};
   trx.actions.push_back(act);
   }
   {
   action act;
   act.account = N(globalreset);
   act.name = 1ULL;
   act.authorization = vector<permission_level>{{N(globalreset),config::active_name}};
   trx.actions.push_back(act);
   }

   set_transaction_headers(trx);
   trx.sign(get_private_key( N(globalreset), "active" ), chain_id_type());
   push_transaction(trx);
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   const auto& receipt = get_transaction_receipt(trx.id());
   BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stl_test, TESTER ) try {
    produce_blocks(2);

    create_accounts( {N(stltest), N(alice), N(bob)} );
    produce_block();

    set_code(N(stltest), stltest_wast);
    set_abi(N(stltest), stltest_abi);
    produce_blocks(1);

    const auto& accnt  = control->get_database().get<account_object,by_name>( N(stltest) );
    abi_def abi;
    BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
    abi_serializer abi_ser(abi);

    //send message
    {
        signed_transaction trx;
        action msg_act;
        msg_act.account = N(stltest);
        msg_act.name = N(message);
        msg_act.authorization = {{N(stltest), config::active_name}};
        msg_act.data = abi_ser.variant_to_binary("message", mutable_variant_object()
                                             ("from", "bob")
                                             ("to", "alice")
                                             ("message","Hi Alice!")
                                             );
        trx.actions.push_back(std::move(msg_act));

        set_transaction_headers(trx);
        trx.sign(get_private_key(N(stltest), "active"), chain_id_type());
        push_transaction(trx);
        produce_block();

        BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
    }
} FC_LOG_AND_RETHROW() /// stltest

//Make sure we can create a wasm with maximum pages, but not grow it any
BOOST_FIXTURE_TEST_CASE( big_memory, TESTER ) try {
   produce_blocks(2);


   create_accounts( {N(bigmem)} );
   produce_block();

   string biggest_memory_wast_f = fc::format_string(biggest_memory_wast, fc::mutable_variant_object(
                                          "MAX_WASM_PAGES", eosio::chain::wasm_constraints::maximum_linear_memory/(64*1024)));

   set_code(N(bigmem), biggest_memory_wast_f.c_str());
   produce_blocks(1);

   signed_transaction trx;
   action act;
   act.account = N(bigmem);
   act.name = N();
   act.authorization = vector<permission_level>{{N(bigmem),config::active_name}};
   trx.actions.push_back(act);

   set_transaction_headers(trx);
   trx.sign(get_private_key( N(bigmem), "active" ), chain_id_type());
   //but should not be able to grow beyond largest page
   push_transaction(trx);

   produce_blocks(1);

   string too_big_memory_wast_f = fc::format_string(too_big_memory_wast, fc::mutable_variant_object(
                                          "MAX_WASM_PAGES_PLUS_ONE", eosio::chain::wasm_constraints::maximum_linear_memory/(64*1024)+1));
   BOOST_CHECK_THROW(set_code(N(bigmem), too_big_memory_wast_f.c_str()), eosio::chain::wasm_execution_error);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( table_init_tests, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(tableinit)} );
   produce_block();

   set_code(N(tableinit), valid_sparse_table);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(tableinit), too_big_table), eosio::chain::wasm_execution_error);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( memory_init_border, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(memoryborder)} );
   produce_block();

   set_code(N(memoryborder), memory_init_borderline);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(memoryborder), memory_init_toolong), eosio::chain::wasm_execution_error);
   BOOST_CHECK_THROW(set_code(N(memoryborder), memory_init_negative), eosio::chain::wasm_execution_error);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( imports, TESTER ) try {
   try {
      produce_blocks(2);

      create_accounts( {N(imports)} );
      produce_block();

      //this will fail to link but that's okay; mainly looking to make sure that the constraint
      // system doesn't choke when memories and tables exist only as imports
      BOOST_CHECK_THROW(set_code(N(imports), memory_table_import), fc::exception);
   } catch ( const fc::exception& e ) {

        edump((e.to_detail_string()));
        throw;
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( lotso_globals, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(globals)} );
   produce_block();

   std::stringstream ss;
   ss << "(module (export \"apply\" (func $apply)) (func $apply (param $0 i64) (param $1 i64) (param $2 i64))";
   for(unsigned int i = 0; i < 85; ++i)
      ss << "(global $g" << i << " (mut i32) (i32.const 0))" << "(global $g" << i+100 << " (mut i64) (i64.const 0))";
   //that gives us 1020 bytes of mutable globals
   //add a few immutable ones for good measure
   for(unsigned int i = 0; i < 10; ++i)
      ss << "(global $g" << i+200 << " i32 (i32.const 0))";

   set_code(N(globals),
      string(ss.str() + ")")
   .c_str());
   //1024 should pass
   set_code(N(globals),
      string(ss.str() + "(global $z (mut i32) (i32.const -12)))")
   .c_str());
   //1028 should fail
   BOOST_CHECK_THROW(set_code(N(globals),
      string(ss.str() + "(global $z (mut i64) (i64.const -12)))")
   .c_str()), eosio::chain::wasm_execution_error);;

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( offset_check, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(offsets)} );
   produce_block();

   vector<string> loadops = {
      "i32.load", "i64.load", "f32.load", "f64.load", "i32.load8_s", "i32.load8_u",
      "i32.load16_s", "i32.load16_u", "i64.load8_s", "i64.load8_u", "i64.load16_s",
      "i64.load16_u", "i64.load32_s", "i64.load32_u"
   };
   vector<vector<string>> storeops = {
      {"i32.store",   "i32"},
      {"i64.store",   "i64"},
      {"f32.store",   "f32"},
      {"f64.store",   "f64"},
      {"i32.store8",  "i32"},
      {"i32.store16", "i32"},
      {"i64.store8",  "i64"},
      {"i64.store16", "i64"},
      {"i64.store32", "i64"},
   };

   for(const string& s : loadops) {
      std::stringstream ss;
      ss << "(module (memory $0 " << eosio::chain::wasm_constraints::maximum_linear_memory/(64*1024) << ") (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      ss << "(drop (" << s << " offset=" << eosio::chain::wasm_constraints::maximum_linear_memory-2 << " (i32.const 0)))";
      ss << ") (export \"apply\" (func $apply)) )";

      set_code(N(offsets), ss.str().c_str());
      produce_block();
   }
   for(const vector<string>& o : storeops) {
      std::stringstream ss;
      ss << "(module (memory $0 " << eosio::chain::wasm_constraints::maximum_linear_memory/(64*1024) << ") (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      ss << "(" << o[0] << " offset=" << eosio::chain::wasm_constraints::maximum_linear_memory-2 << " (i32.const 0) (" << o[1] << ".const 0))";
      ss << ") (export \"apply\" (func $apply)) )";

      set_code(N(offsets), ss.str().c_str());
      produce_block();
   }

   for(const string& s : loadops) {
      std::stringstream ss;
      ss << "(module (memory $0 " << eosio::chain::wasm_constraints::maximum_linear_memory/(64*1024) << ") (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      ss << "(drop (" << s << " offset=" << eosio::chain::wasm_constraints::maximum_linear_memory+4 << " (i32.const 0)))";
      ss << ") (export \"apply\" (func $apply)) )";

      BOOST_CHECK_THROW(set_code(N(offsets), ss.str().c_str()), eosio::chain::wasm_execution_error);
      produce_block();
   }
   for(const vector<string>& o : storeops) {
      std::stringstream ss;
      ss << "(module (memory $0 " << eosio::chain::wasm_constraints::maximum_linear_memory/(64*1024) << ") (func $apply (param $0 i64) (param $1 i64) (param $2 i64)";
      ss << "(" << o[0] << " offset=" << eosio::chain::wasm_constraints::maximum_linear_memory+4 << " (i32.const 0) (" << o[1] << ".const 0))";
      ss << ") (export \"apply\" (func $apply)) )";

      BOOST_CHECK_THROW(set_code(N(offsets), ss.str().c_str()), eosio::chain::wasm_execution_error);
      produce_block();
   }

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(noop, TESTER) try {
   produce_blocks(2);
   create_accounts( {N(noop), N(alice)} );
   produce_block();

   set_code(N(noop), noop_wast);

   set_abi(N(noop), noop_abi);
   const auto& accnt  = control->get_database().get<account_object,by_name>(N(noop));
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);

   {
      produce_blocks(5);
      signed_transaction trx;
      action act;
      act.account = N(noop);
      act.name = N(anyaction);
      act.authorization = vector<permission_level>{{N(noop), config::active_name}};

      act.data = abi_ser.variant_to_binary("anyaction", mutable_variant_object()
                                           ("from", "noop")
                                           ("type", "some type")
                                           ("data", "some data goes here")
                                           );

      trx.actions.emplace_back(std::move(act));

      set_transaction_headers(trx);
      trx.sign(get_private_key(N(noop), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   {
      produce_blocks(5);
      signed_transaction trx;
      action act;
      act.account = N(noop);
      act.name = N(anyaction);
      act.authorization = vector<permission_level>{{N(alice), config::active_name}};

      act.data = abi_ser.variant_to_binary("anyaction", mutable_variant_object()
                                           ("from", "alice")
                                           ("type", "some type")
                                           ("data", "some data goes here")
                                           );

      trx.actions.emplace_back(std::move(act));

      set_transaction_headers(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

 } FC_LOG_AND_RETHROW()

// abi_serializer::to_variant failed because eosio_system_abi modified via set_abi.
// This test also verifies that chain_initializer::eos_contract_abi() does not conflict
// with eosio_system_abi as they are not allowed to contain duplicates.
BOOST_FIXTURE_TEST_CASE(eosio_abi, TESTER) try {
   produce_blocks(2);

   set_code(config::system_account_name, eosio_system_wast);
   set_abi(config::system_account_name, eosio_system_abi);
   produce_block();

   const auto& accnt  = control->get_database().get<account_object,by_name>(config::system_account_name);
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);
   abi_ser.validate();

   signed_transaction trx;
   name a = N(alice);
   authority owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{config::system_account_name,config::active_name}},
                             contracts::newaccount{
                                   .creator  = config::system_account_name,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) ),
                                   .recovery = authority( get_public_key( a, "recovery" ) ),
                             });
   set_transaction_headers(trx);
   trx.sign( get_private_key( config::system_account_name, "active" ), chain_id_type()  );
   auto result = push_transaction( trx );

   fc::variant pretty_output;
   // verify to_variant works on eos native contract type: newaccount
   // see abi_serializer::to_abi()
   abi_serializer::to_variant(result, pretty_output, get_resolver());

   BOOST_TEST(fc::json::to_string(pretty_output).find("newaccount") != std::string::npos);

   produce_block();
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_table_key_validation, TESTER ) try {
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( check_table_maximum, TESTER ) try {
   produce_blocks(2);
   create_accounts( {N(tbl)} );
   produce_block();

   set_code(N(tbl), table_checker_wast);
   produce_blocks(1);
   {
   signed_transaction trx;
   action act;
   act.name = 555ULL<<32 | 0ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
      set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());
   push_transaction(trx);
   }

   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 555ULL<<32 | 1022ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
      set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());
   push_transaction(trx);
   }

   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 7777ULL<<32 | 1023ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
      set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());
   push_transaction(trx);
   }

   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 7778ULL<<32 | 1023ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
      set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //should fail, a check to make sure assert() in wasm is being evaluated correctly
   BOOST_CHECK_THROW(push_transaction(trx), transaction_exception);
   }

   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 133ULL<<32 | 5ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
      set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //should fail, this element index (5) does not exist
   BOOST_CHECK_THROW(push_transaction(trx), eosio::chain::wasm_execution_error);
   }

   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = eosio::chain::wasm_constraints::maximum_table_elements+54334;
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
      set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //should fail, this element index is out of range
   BOOST_CHECK_THROW(push_transaction(trx), eosio::chain::wasm_execution_error);
   }

   produce_blocks(1);

   //run a few tests with new, proper syntax, call_indirect
   set_code(N(tbl), table_checker_proper_syntax_wast);
   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 555ULL<<32 | 1022ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
      set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());
   push_transaction(trx);
   }

   produce_blocks(1);
   {
   signed_transaction trx;
   action act;
   act.name = 7777ULL<<32 | 1023ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
   set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());
   push_transaction(trx);
   }
   set_code(N(tbl), table_checker_small_wast);
   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 888ULL;
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
   set_transaction_headers(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //an element that is out of range and has no mmap access permission either (should be a trapped segv)
   BOOST_CHECK_EXCEPTION(push_transaction(trx), eosio::chain::wasm_execution_error, [](const eosio::chain::wasm_execution_error &e) {return true;});
   }
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( protected_globals, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(gob)} );
   produce_block();

   BOOST_CHECK_THROW(set_code(N(gob), global_protection_none_get_wast), fc::exception);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(gob), global_protection_some_get_wast), fc::exception);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(gob), global_protection_none_set_wast), fc::exception);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(gob), global_protection_some_set_wast), fc::exception);
   produce_blocks(1);

   //sanity to make sure I got general binary construction okay
   set_code(N(gob), global_protection_okay_get_wasm);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(gob), global_protection_none_get_wasm), fc::exception);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(gob), global_protection_some_get_wasm), fc::exception);
   produce_blocks(1);

   set_code(N(gob), global_protection_okay_set_wasm);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(gob), global_protection_some_set_wasm), fc::exception);
   produce_blocks(1);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( lotso_stack, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(stackz)} );
   produce_block();

   {
   std::stringstream ss;
   ss << "(module ";
   ss << "(export \"apply\" (func $apply))";
   ss << "  (func $apply  (param $0 i64)(param $1 i64)(param $2 i64))";
   ss << "  (func ";
   for(unsigned int i = 0; i < wasm_constraints::maximum_func_local_bytes; i+=4)
      ss << "(local i32)";
   ss << "  )";
   ss << ")";
   set_code(N(stackz), ss.str().c_str());
   produce_blocks(1);
   }
   {
   std::stringstream ss;
   ss << "(module ";
   ss << "(import \"env\" \"require_auth\" (func $require_auth (param i64)))";
   ss << "(export \"apply\" (func $apply))";
   ss << "  (func $apply  (param $0 i64)(param $1 i64)(param $2 i64) (call $require_auth (i64.const 14288945783897063424)))";
   ss << "  (func ";
   for(unsigned int i = 0; i < wasm_constraints::maximum_func_local_bytes; i+=8)
      ss << "(local f64)";
   ss << "  )";
   ss << ")";
   set_code(N(stackz), ss.str().c_str());
   produce_blocks(1);
   }

   //try to use contract with this many locals (so that it actually gets compiled). Note that
   //at this time not having an apply() is an acceptable non-error.
   {
   signed_transaction trx;
   action act;
   act.account = N(stackz);
   act.name = N();
   act.authorization = vector<permission_level>{{N(stackz),config::active_name}};
   trx.actions.push_back(act);

      set_transaction_headers(trx);
   trx.sign(get_private_key( N(stackz), "active" ), chain_id_type());
   push_transaction(trx);
   }


   //too many locals! should fail validation
   {
   std::stringstream ss;
   ss << "(module ";
   ss << "(export \"apply\" (func $apply))";
   ss << "  (func $apply  (param $0 i64) (param $1 i64) (param $2 i64))";
   ss << "  (func ";
   for(unsigned int i = 0; i < wasm_constraints::maximum_func_local_bytes+4; i+=4)
      ss << "(local i32)";
   ss << "  )";
   ss << ")";
   BOOST_CHECK_THROW(set_code(N(stackz), ss.str().c_str()), fc::exception);
   produce_blocks(1);
   }

   //try again but with parameters
   {
   std::stringstream ss;
   ss << "(module ";
   ss << "(import \"env\" \"require_auth\" (func $require_auth (param i64)))";
   ss << "(export \"apply\" (func $apply))";
   ss << "  (func $apply  (param $0 i64)(param $1 i64)(param $2 i64) (call $require_auth (i64.const 14288945783897063424)))";
   ss << "  (func ";
   for(unsigned int i = 0; i < wasm_constraints::maximum_func_local_bytes; i+=4)
      ss << "(param i32)";
   ss << "  )";
   ss << ")";
   set_code(N(stackz), ss.str().c_str());
   produce_blocks(1);
   }
   //try to use contract with this many params
   {
   signed_transaction trx;
   action act;
   act.account = N(stackz);
   act.name = N();
   act.authorization = vector<permission_level>{{N(stackz),config::active_name}};
   trx.actions.push_back(act);

      set_transaction_headers(trx);
   trx.sign(get_private_key( N(stackz), "active" ), chain_id_type());
   push_transaction(trx);
   }

   //too many params!
   {
   std::stringstream ss;
   ss << "(module ";
   ss << "(export \"apply\" (func $apply))";
   ss << "  (func $apply  (param $0 i64) (param $1 i64) (param $2 i64))";
   ss << "  (func ";
   for(unsigned int i = 0; i < wasm_constraints::maximum_func_local_bytes+4; i+=4)
      ss << "(param i32)";
   ss << "  )";
   ss << ")";
   BOOST_CHECK_THROW(set_code(N(stackz), ss.str().c_str()), fc::exception);
   produce_blocks(1);
   }

   //let's mix params and locals are make sure it's counted correctly in mixed case
   {
   std::stringstream ss;
   ss << "(module ";
   ss << "(export \"apply\" (func $apply))";
   ss << "  (func $apply  (param $0 i64) (param $1 i64) (param $2 i64))";
   ss << "  (func (param i64) (param f32) ";
   for(unsigned int i = 12; i < wasm_constraints::maximum_func_local_bytes; i+=4)
      ss << "(local i32)";
   ss << "  )";
   ss << ")";
   set_code(N(stackz), ss.str().c_str());
   produce_blocks(1);
   }
   {
   std::stringstream ss;
   ss << "(module ";
   ss << "(export \"apply\" (func $apply))";
   ss << "  (func $apply  (param $0 i64) (param $1 i64) (param $2 i64))";
   ss << "  (func (param i64) (param f32) ";
   for(unsigned int i = 12; i < wasm_constraints::maximum_func_local_bytes+4; i+=4)
      ss << "(local f32)";
   ss << "  )";
   ss << ")";
   BOOST_CHECK_THROW(set_code(N(stackz), ss.str().c_str()), fc::exception);
   produce_blocks(1);
   }


} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( apply_export_and_signature, TESTER ) try {
   produce_blocks(2);
   create_accounts( {N(bbb)} );
   produce_block();

   BOOST_CHECK_THROW(set_code(N(bbb), no_apply_wast), fc::exception);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(bbb), apply_wrong_signature_wast), fc::exception);
   produce_blocks(1);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( trigger_serialization_errors, TESTER) try {
   produce_blocks(2);
   const vector<uint8_t> proper_wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x13, 0x04, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x60, 
                                       0x00, 0x00, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00, 0x02, 0x1e, 0x02, 0x03, 0x65, 0x6e, 0x76, 0x0c, 0x65, 0x6f, 0x73, 0x69, 0x6f, 
                                       0x5f, 0x61, 0x73, 0x73, 0x65, 0x72, 0x74, 0x00, 0x00, 0x03, 0x65, 0x6e, 0x76, 0x03, 0x6e, 0x6f, 0x77, 0x00, 0x01, 0x03, 0x03,
                                       0x02, 0x02, 0x03, 0x04, 0x04, 0x01, 0x70, 0x00, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x1a, 0x03, 0x06, 0x6d, 0x65, 0x6d, 
                                       0x6f, 0x72, 0x79, 0x02, 0x00, 0x05, 0x65, 0x6e, 0x74, 0x72, 0x79, 0x00, 0x02, 0x05, 0x61, 0x70, 0x70, 0x6c, 0x79, 0x00, 0x03, 
                                       0x08, 0x01, 0x02, 0x0a, 0x1a, 0x02, 0x09, 0x00, 0x41, 0x00, 0x10, 0x01, 0x36, 0x02, 0x04, 0x0b, 0x0e, 0x00, 0x41, 0x00, 0x28, 
                                       0x02, 0x04, 0x10, 0x01, 0x46, 0x41, 0x00, 0x10, 0x00, 0x0b};

   const vector<uint8_t> malformed_wasm = {0x00, 0x61, 0x73, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x04, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x60, 
                                       0x00, 0x00, 0x60, 0x03, 0x7e, 0x7e, 0x7e, 0x00, 0x02, 0x1e, 0x02, 0x03, 0x65, 0x6e, 0x76, 0x0c, 0x65, 0x6f, 0x73, 0x69, 0x6f, 
                                       0x5f, 0x61, 0x73, 0x73, 0x65, 0x72, 0x74, 0x00, 0x00, 0x03, 0x65, 0x6e, 0x76, 0x03, 0x6e, 0x6f, 0x77, 0x00, 0x01, 0x03, 0x03,
                                       0x02, 0x02, 0x03, 0x04, 0x04, 0x01, 0x70, 0x00, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07, 0x1a, 0x03, 0x06, 0x0d, 0x65, 0x0d, 
                                       0x6f, 0x72, 0x79, 0x02, 0x00, 0x05, 0x65, 0x6e, 0x74, 0x72, 0x79, 0x00, 0x02, 0x05, 0x61, 0x70, 0x70, 0x6c, 0x79, 0x00, 0x03, 
                                       0x08, 0x01, 0x02, 0x0a, 0x1a, 0x02, 0x09, 0x00, 0x41, 0x00, 0x10, 0x01, 0x36, 0x02, 0x04, 0x0b, 0x0e, 0x00, 0x41, 0x00, 0x28, 
                                       0x02, 0x04, 0x10, 0x01, 0x46, 0x41, 0x00, 0x10, 0x00, 0x0b};


   create_accounts( {N(bbb)} );
   produce_block();

   set_code(N(bbb), proper_wasm);
   BOOST_CHECK_THROW(set_code(N(bbb), malformed_wasm), wasm_serialization_error);
   produce_blocks(1);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( protect_injected, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(inj)} );
   produce_block();

   BOOST_CHECK_THROW(set_code(N(inj), import_injected_wast), fc::exception);
   produce_blocks(1);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(net_usage_tests, tester ) try {
   int count = 0;
   auto check = [&](int coderepeat, int max_net_usage)-> bool {
      account_name account = N(f_tests) + (count++) * 16;
      create_accounts({account});

      std::string code = R"=====(
   (module
   (import "env" "require_auth" (func $require_auth (param i64)))
   (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
      (table 0 anyfunc)
      (memory $0 1)
      (export "apply" (func $apply))
      (func $i64_trunc_u_f64 (param $0 f64) (result i64) (i64.trunc_u/f64 (get_local $0)))
      (func $test (param $0 i64))
      (func $apply (param $0 i64)(param $1 i64)(param $2 i64)
      )=====";
      for (int i = 0; i < coderepeat; ++i) {
         code += "(call $test (call $i64_trunc_u_f64 (f64.const 1)))\n";
      }
      code += "))";
      produce_blocks(1);
      signed_transaction trx;
      auto wasm = ::eosio::chain::wast_to_wasm(code);
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                              contracts::setcode{
                                 .account    = account,
                                 .vmtype     = 0,
                                 .vmversion  = 0,
                                 .code       = bytes(wasm.begin(), wasm.end())
                              });
      set_transaction_headers(trx);
      if (max_net_usage) trx.max_net_usage_words = max_net_usage;
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
      try {
         push_transaction(trx);
         produce_blocks(1);
         return true;
      } catch (tx_resource_exhausted &) {
         return false;
      } catch (transaction_exception &) {
         return false;
      }
   };
   BOOST_REQUIRE_EQUAL(true, check(1024, 0)); // default behavior
   BOOST_REQUIRE_EQUAL(false, check(1024, 1000)); // transaction max_net_usage too small
   BOOST_REQUIRE_EQUAL(false, check(10240, 0)); // larger than global maximum

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(weighted_net_usage_tests, tester ) try {
   account_name account = N(f_tests);
   account_name acc2 = N(acc2);
   create_accounts({account, acc2});
   int ver = 0;
   auto check = [&](int coderepeat)-> bool {
      std::string code = R"=====(
   (module
   (import "env" "require_auth" (func $require_auth (param i64)))
   (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
      (table 0 anyfunc)
      (memory $0 1)
      (export "apply" (func $apply))
      (func $i64_trunc_u_f64 (param $0 f64) (result i64) (i64.trunc_u/f64 (get_local $0)))
      (func $test (param $0 i64))
      (func $apply (param $0 i64)(param $1 i64)(param $2 i64)
      )=====";
      for (int i = 0; i < coderepeat; ++i) {
         code += "(call $test (call $i64_trunc_u_f64 (f64.const ";
         code += (char)('0' + ver);
         code += ")))\n";
      }
      code += "))"; ver++;
      produce_blocks(1);
      signed_transaction trx;
      auto wasm = ::eosio::chain::wast_to_wasm(code);
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                              contracts::setcode{
                                 .account    = account,
                                 .vmtype     = 0,
                                 .vmversion  = 0,
                                 .code       = bytes(wasm.begin(), wasm.end())
                              });
      set_transaction_headers(trx);
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
      try {
         push_transaction(trx);
         produce_blocks(1);
         return true;
      } catch (tx_resource_exhausted &) {
         return false;
      }
   };
   BOOST_REQUIRE_EQUAL(true, check(128)); // no limits, should pass

   resource_limits_manager mgr = control->get_mutable_resource_limits_manager();
   mgr.set_account_limits(account, -1, 1, -1); // set weight = 1 for account

   BOOST_REQUIRE_EQUAL(true, check(128)); 

   mgr.set_account_limits(acc2, -1, 1000, -1); // set a big weight for other account
   BOOST_REQUIRE_EQUAL(false, check(128));

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
