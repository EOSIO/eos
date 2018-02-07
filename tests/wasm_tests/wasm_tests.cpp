#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/exceptions.hpp>
#include <asserter/asserter.wast.hpp>
#include <asserter/asserter.abi.hpp>

#include <test_api/test_api.wast.hpp>

#include <proxy/proxy.wast.hpp>
#include <proxy/proxy.abi.hpp>

#include <noop/noop.wast.hpp>
#include <noop/noop.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include "test_wasts.hpp"

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

constexpr uint32_t DJBH(const char* cp)
{
   uint32_t hash = 5381;
   while (*cp)
      hash = 33 * hash ^ (unsigned char) *cp++;
   return hash;
}

constexpr uint64_t TEST_METHOD(const char* CLASS, const char *METHOD) {
   return ( (uint64_t(DJBH(CLASS))<<32) | uint32_t(DJBH(METHOD)) );
}


template<uint64_t NAME>
struct test_api_action {
   static account_name get_account() {
      return N(tester);
   }

   static action_name get_name() {
      return action_name(NAME);
   }
};
FC_REFLECT_TEMPLATE((uint64_t T), test_api_action<T>, BOOST_PP_SEQ_NIL);


BOOST_AUTO_TEST_SUITE(wasm_tests)

/**
 * Prove that action reading and assertions are working
 */
BOOST_FIXTURE_TEST_CASE( basic_test, tester ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(asserter), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(asserter), asserter_wast);
   produce_blocks(1);

   transaction_id_type no_assert_id;
   {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                assertdef {1, "Should Not Assert!"} );

      set_tapos( trx );
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

      set_tapos( trx );
      trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
      yes_assert_id = trx.id();

      BOOST_CHECK_THROW(push_transaction( trx ), fc::assert_exception);
   }

   produce_blocks(1);

   auto has_tx = chain_has_transaction(yes_assert_id);
   BOOST_REQUIRE_EQUAL(false, has_tx);

} FC_LOG_AND_RETHROW() /// basic_test

/**
 * Prove the modifications to global variables are wiped between runs
 */
BOOST_FIXTURE_TEST_CASE( prove_mem_reset, tester ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(asserter), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(asserter), asserter_wast);
   produce_blocks(1);

   // repeat the action multiple times, each time the action handler checks for the expected
   // default value then modifies the value which should not survive until the next invoction
   for (int i = 0; i < 5; i++) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{N(asserter),config::active_name}},
                                provereset {} );

      set_tapos( trx );
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
BOOST_FIXTURE_TEST_CASE( abi_from_variant, tester ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(asserter), "10.0000 EOS", "memo" );
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
   set_tapos( trx );
   trx.sign( get_private_key( N(asserter), "active" ), chain_id_type() );
   push_transaction( trx );
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   const auto& receipt = get_transaction_receipt(trx.id());
   BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);

} FC_LOG_AND_RETHROW() /// prove_mem_reset

BOOST_FIXTURE_TEST_CASE( test_api_bootstrap, tester ) try {
   produce_blocks(2);

   create_accounts( {N(tester)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(tester), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(tester), test_api_wast);
   produce_blocks(1);

   // make sure asserts function as we are predicated on them
   {
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                               test_api_action<TEST_METHOD("test_action", "assert_false")> {});

      set_tapos(trx);
      trx.sign(get_private_key(N(tester), "active"), chain_id_type());
      BOOST_CHECK_EXCEPTION(push_transaction(trx), fc::assert_exception, assert_message_is("test_action::assert_false"));
      produce_block();

      BOOST_REQUIRE_EQUAL(false, chain_has_transaction(trx.id()));
   }

   {
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(tester), config::active_name}},
                               test_api_action<TEST_METHOD("test_action", "assert_true")> {});

      set_tapos(trx);
      trx.sign(get_private_key(N(tester), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      const auto& receipt = get_transaction_receipt(trx.id());
      BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);
   }
} FC_LOG_AND_RETHROW() /// test_api_bootstrap


BOOST_FIXTURE_TEST_CASE( test_proxy, tester ) try {
   produce_blocks(2);

   create_account( N(proxy), asset::from_string("0.0000 EOS") );
   create_accounts( {N(alice), N(bob)}, asset::from_string("0.0000 EOS") );
   transfer( N(inita), N(alice), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(proxy), proxy_wast);
   set_abi(N(proxy), proxy_abi);
   produce_blocks(1);

   const auto& accnt  = control->get_database().get<account_object,by_name>( N(proxy) );
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);

   // set up proxy owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(proxy);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(proxy), config::active_name}};
      setowner_act.data = abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "bob")
         ("delay", 10)
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(proxy), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   // for now wasm "time" is in seconds, so we have to truncate off any parts of a second that may have applied
   fc::time_point expected_delivery(fc::seconds(control->head_block_time().sec_since_epoch()) + fc::seconds(10));
   transfer(N(alice), N(proxy), "5.0000 EOS");

   while(control->head_block_time() < expected_delivery) {
      control->push_deferred_transactions(true);
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("5.0000 EOS").amount);
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 EOS").amount);
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 EOS").amount);
   }

   control->push_deferred_transactions(true);
   BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("5.0000 EOS").amount);
   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 EOS").amount);
   BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("5.0000 EOS").amount);

} FC_LOG_AND_RETHROW() /// test_currency

BOOST_FIXTURE_TEST_CASE( test_deferred_failure, tester ) try {
   produce_blocks(2);

   create_accounts( {N(proxy), N(bob)}, asset::from_string("0.0000 EOS") );
   create_account( N(alice), asset::from_string("0.0000 EOS") );
   transfer( N(inita), N(alice), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(proxy), proxy_wast);
   set_abi(N(proxy), proxy_abi);
   set_code(N(bob), proxy_wast);
   produce_blocks(1);

   const auto& accnt  = control->get_database().get<account_object,by_name>( N(proxy) );
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);

   // set up proxy owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(proxy);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(proxy), config::active_name}};
      setowner_act.data = abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "bob")
         ("delay", 10)
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(proxy), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   // for now wasm "time" is in seconds, so we have to truncate off any parts of a second that may have applied
   fc::time_point expected_delivery(fc::seconds(control->head_block_time().sec_since_epoch()) + fc::seconds(10));
   auto trace = transfer(N(alice), N(proxy), "5.0000 EOS");
   BOOST_REQUIRE_EQUAL(trace.deferred_transactions.size(), 1);
   auto deferred_id = trace.deferred_transactions.back().id();

   while(control->head_block_time() < expected_delivery) {
      control->push_deferred_transactions(true);
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("5.0000 EOS").amount);
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 EOS").amount);
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 EOS").amount);
      BOOST_REQUIRE_EQUAL(chain_has_transaction(deferred_id), false);
   }

   fc::time_point expected_redelivery(fc::seconds(control->head_block_time().sec_since_epoch()) + fc::seconds(10));
   control->push_deferred_transactions(true);
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(deferred_id), true);
   BOOST_REQUIRE_EQUAL(get_transaction_receipt(deferred_id).status, transaction_receipt::soft_fail);

   // set up bob owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(bob);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(bob), config::active_name}};
      setowner_act.data = abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "alice")
         ("delay", 0)
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(bob), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   while(control->head_block_time() < expected_redelivery) {
      control->push_deferred_transactions(true);
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("5.0000 EOS").amount);
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 EOS").amount);
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 EOS").amount);
   }

   control->push_deferred_transactions(true);
   BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("5.0000 EOS").amount);
   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 EOS").amount);
   BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("5.0000 EOS").amount);

   control->push_deferred_transactions(true);

   BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("10.0000 EOS").amount);
   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 EOS").amount);
   BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 EOS").amount);

} FC_LOG_AND_RETHROW() /// test_currency

/**
 * Make sure WASM "start" method is used correctly
 */
BOOST_FIXTURE_TEST_CASE( check_entry_behavior, tester ) try {
   produce_blocks(2);

   create_accounts( {N(entrycheck)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(entrycheck), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(entrycheck), entry_wast);
   produce_blocks(10);

   signed_transaction trx;
   action act;
   act.account = N(entrycheck);
   act.name = N();
   act.authorization = vector<permission_level>{{N(entrycheck),config::active_name}};
   trx.actions.push_back(act);

   set_tapos(trx);
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
BOOST_FIXTURE_TEST_CASE( simple_no_memory_check, tester ) try {
   produce_blocks(2);

   create_accounts( {N(nomem)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(nomem), "10.0000 EOS", "memo" );
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

   trx.sign(get_private_key( N(nomem), "active" ), chain_id_type());
   BOOST_CHECK_THROW(push_transaction( trx ), wasm_execution_error);
} FC_LOG_AND_RETHROW()

//Make sure globals are all reset to their inital values
BOOST_FIXTURE_TEST_CASE( check_global_reset, tester ) try {
   produce_blocks(2);

   create_accounts( {N(globalreset)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(globalreset), "10.0000 EOS", "memo" );
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

   set_tapos(trx);
   trx.sign(get_private_key( N(globalreset), "active" ), chain_id_type());
   push_transaction(trx);
   produce_blocks(1);
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   const auto& receipt = get_transaction_receipt(trx.id());
   BOOST_CHECK_EQUAL(transaction_receipt::executed, receipt.status);
} FC_LOG_AND_RETHROW()

//Make sure current_memory/grow_memory is not allowed
BOOST_FIXTURE_TEST_CASE( memory_operators, tester ) try {
   produce_blocks(2);

   create_accounts( {N(current_memory)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(current_memory), "10.0000 EOS", "memo" );
   produce_block();

   BOOST_CHECK_THROW(set_code(N(current_memory), current_memory_wast), eosio::chain::wasm_execution_error);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(current_memory), grow_memory_wast), eosio::chain::wasm_execution_error);
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

//Make sure we can create a wasm with 16 pages, but not grow it any
BOOST_FIXTURE_TEST_CASE( big_memory, tester ) try {
   produce_blocks(2);

   create_accounts( {N(bigmem)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(bigmem), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(bigmem), biggest_memory_wast);  //should pass, 16 pages is fine
   produce_blocks(1);

   signed_transaction trx;
   action act;
   act.account = N(bigmem);
   act.name = N();
   act.authorization = vector<permission_level>{{N(bigmem),config::active_name}};
   trx.actions.push_back(act);

   set_tapos(trx);
   trx.sign(get_private_key( N(bigmem), "active" ), chain_id_type());
   //but should not be able to grow beyond 16th page
   BOOST_CHECK_THROW(push_transaction(trx), fc::exception);

   produce_blocks(1);

   //should fail, 17 blocks is no no
   BOOST_CHECK_THROW(set_code(N(bigmem), too_big_memory_wast), eosio::chain::wasm_execution_error);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( table_init_tests, tester ) try {
   produce_blocks(2);

   create_accounts( {N(tableinit)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(tableinit), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(tableinit), valid_sparse_table);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(tableinit), too_big_table), eosio::chain::wasm_execution_error);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( memory_init_border, tester ) try {
   produce_blocks(2);

   create_accounts( {N(memoryborder)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(memoryborder), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(memoryborder), memory_init_borderline);
   produce_blocks(1);

   BOOST_CHECK_THROW(set_code(N(memoryborder), memory_init_toolong), eosio::chain::wasm_execution_error);
   BOOST_CHECK_THROW(set_code(N(memoryborder), memory_init_negative), eosio::chain::wasm_execution_error);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( imports, tester ) try {
   produce_blocks(2);

   create_accounts( {N(imports)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(imports), "10.0000 EOS", "memo" );
   produce_block();

   //this will fail to link but that's okay; mainly looking to make sure that the constraint
   // system doesn't choke when memories and tables exist only as imports
   BOOST_CHECK_THROW(set_code(N(imports), memory_table_import), fc::exception);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( lotso_globals, tester ) try {
   produce_blocks(2);

   create_accounts( {N(globals)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(globals), "10.0000 EOS", "memo" );
   produce_block();

   std::stringstream ss;
   ss << "(module ";
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

BOOST_FIXTURE_TEST_CASE( offset_check, tester ) try {
   produce_blocks(2);

   create_accounts( {N(offsets)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(offsets), "10.0000 EOS", "memo" );
   produce_block();

   //floats not tested since they are blocked in the serializer before eosio_constraints
   vector<string> loadops = {
      "i32.load", "i64.load", /* "f32.load", "f64.load",*/ "i32.load8_s", "i32.load8_u",
      "i32.load16_s", "i32.load16_u", "i64.load8_s", "i64.load8_u", "i64.load16_s",
      "i64.load16_u", "i64.load32_s", "i64.load32_u"
   };
   vector<vector<string>> storeops = {
      {"i32.store",   "i32"},
      {"i64.store",   "i64"},
    /*{"f32.store",   "f32"},
      {"f64.store",   "f64"},*/
      {"i32.store8",  "i32"},
      {"i32.store16", "i32"},
      {"i64.store8",  "i64"},
      {"i64.store16", "i64"},
      {"i64.store32", "i64"},
   };

   for(const string& s : loadops) {
      std::stringstream ss;
      ss << "(module (memory $0 16) (func $apply (param $0 i64) (param $1 i64) ";
      ss << "(drop (" << s << " offset=1048574 (i32.const 0)))";
      ss << "))";

      set_code(N(offsets), ss.str().c_str());
      produce_block();
   }
   for(const vector<string>& o : storeops) {
      std::stringstream ss;
      ss << "(module (memory $0 16) (func $apply (param $0 i64) (param $1 i64) ";
      ss << "(" << o[0] << " offset=1048574 (i32.const 0) (" << o[1] << ".const 0))";
      ss << "))";

      set_code(N(offsets), ss.str().c_str());
      produce_block();
   }

   for(const string& s : loadops) {
      std::stringstream ss;
      ss << "(module (memory $0 16) (func $apply (param $0 i64) (param $1 i64) ";
      ss << "(drop (" << s << " offset=1048580 (i32.const 0)))";
      ss << "))";

      BOOST_CHECK_THROW(set_code(N(offsets), ss.str().c_str()), eosio::chain::wasm_execution_error);
      produce_block();
   }
   for(const vector<string>& o : storeops) {
      std::stringstream ss;
      ss << "(module (memory $0 16) (func $apply (param $0 i64) (param $1 i64) ";
      ss << "(" << o[0] << " offset=1048580 (i32.const 0) (" << o[1] << ".const 0))";
      ss << "))";

      BOOST_CHECK_THROW(set_code(N(offsets), ss.str().c_str()), eosio::chain::wasm_execution_error);
      produce_block();
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(noop, tester) try {
   produce_blocks(2);
   create_accounts( {N(noop), N(alice)}, asset::from_string("1000.0000 EOS") );
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

      set_tapos(trx);
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

      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

 } FC_LOG_AND_RETHROW()

//busted because of checktime, disable for now
#if 0
BOOST_FIXTURE_TEST_CASE( check_table_maximum, tester ) try {
   produce_blocks(2);

   create_accounts( {N(tbl)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(tbl), "10.0000 EOS", "memo" );
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
   set_tapos(trx);
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
   set_tapos(trx);
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
   set_tapos(trx);
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
   set_tapos(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //should fail, a check to make sure assert() in wasm is being evaluated correctly
   BOOST_CHECK_THROW(push_transaction(trx), fc::assert_exception);
   }

   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 133ULL<<32 | 5ULL;       //top 32 is what we assert against, bottom 32 is indirect call index
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
   set_tapos(trx);
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
   set_tapos(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //should fail, this element index is out of range
   BOOST_CHECK_THROW(push_transaction(trx), eosio::chain::wasm_execution_error);
   }

   produce_blocks(1);

   set_code(N(tbl), table_checker_small_wast);
   produce_blocks(1);

   {
   signed_transaction trx;
   action act;
   act.name = 888ULL;
   act.account = N(tbl);
   act.authorization = vector<permission_level>{{N(tbl),config::active_name}};
   trx.actions.push_back(act);
   set_tapos(trx);
   trx.sign(get_private_key( N(tbl), "active" ), chain_id_type());

   //an element that is out of range and has no mmap access permission either (should be a trapped segv)
   BOOST_CHECK_THROW(push_transaction(trx), eosio::chain::wasm_execution_error);
   }

} FC_LOG_AND_RETHROW()
#endif

BOOST_AUTO_TEST_SUITE_END()
