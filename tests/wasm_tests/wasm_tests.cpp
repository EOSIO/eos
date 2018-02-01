#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <asserter/asserter.wast.hpp>
#include <asserter/asserter.abi.hpp>

#include <test_api/test_api.wast.hpp>

#include <proxy/proxy.wast.hpp>
#include <proxy/proxy.abi.hpp>

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

   create_account( N(proxy), asset::from_string("10000.0000 EOS") );
   create_accounts( {N(alice), N(bob)}, asset::from_string("1000.0000 EOS") );
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

   create_accounts( {N(proxy), N(bob)}, asset::from_string("10000.0000 EOS") );
   create_account( N(alice), asset::from_string("1000.0000 EOS") );
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

   set_code(N(current_memory), current_memory_wast);
   produce_blocks(1);
   {
      signed_transaction trx;
      action act;
      act.account = N(current_memory);
      act.authorization = vector<permission_level>{{N(current_memory),config::active_name}};
      trx.actions.push_back(act);
      set_tapos(trx);
      trx.sign(get_private_key( N(current_memory), "active" ), chain_id_type());

      BOOST_CHECK_THROW(push_transaction(trx), fc::unhandled_exception);
   }

   produce_blocks(1);
   set_code(N(current_memory), grow_memory_wast);
   produce_blocks(1);
   {
      signed_transaction trx;
      action act;
      act.account = N(current_memory);
      act.authorization = vector<permission_level>{{N(current_memory),config::active_name}};
      trx.actions.push_back(act);
      set_tapos(trx);
      trx.sign(get_private_key( N(current_memory), "active" ), chain_id_type());

      BOOST_CHECK_THROW(push_transaction(trx), fc::unhandled_exception);
      produce_blocks(1);
   }

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
