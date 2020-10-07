#include <algorithm>
#include <random>
#include <iostream>
#include <vector>
#include <iterator>
#include <numeric>
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

#include <Inline/BasicTypes.h>
#include <Runtime/Runtime.h>

#include <contracts.hpp>
#include "test_cfd_transaction.hpp"

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

static constexpr unsigned int DJBH(const char* cp)
{
  unsigned int hash = 5381;
  while (*cp)
      hash = 33 * hash ^ (unsigned char) *cp++;
  return hash;
}

static constexpr unsigned long long WASM_TEST_ACTION(const char* cls, const char* method)
{
  return static_cast<unsigned long long>(DJBH(cls)) << 32 | static_cast<unsigned long long>(DJBH(method));
}

struct u128_action {
  unsigned __int128  values[3]; //16*3
};

using namespace eosio::chain::literals;

// Deferred Transaction Trigger Action
struct dtt_action {
   static uint64_t get_name() {
      return WASM_TEST_ACTION("test_transaction", "send_deferred_tx_with_dtt_action");
   }
   static uint64_t get_account() {
      return "testapi"_n.to_uint64_t();
   }

   uint64_t       payer = "testapi"_n.to_uint64_t();
   uint64_t       deferred_account = "testapi"_n.to_uint64_t();
   uint64_t       deferred_action = WASM_TEST_ACTION("test_transaction", "deferred_print");
   uint64_t       permission_name = "active"_n.to_uint64_t();
   uint32_t       delay_sec = 2;
};

struct invalid_access_action {
   uint64_t code;
   uint64_t val;
   uint32_t index;
   bool store;
};

FC_REFLECT( u128_action, (values) )
FC_REFLECT( dtt_action, (payer)(deferred_account)(deferred_action)(permission_name)(delay_sec) )
FC_REFLECT( invalid_access_action, (code)(val)(index)(store) )

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#define ROCKSDB_TESTER rocksdb_tester
#else
#define TESTER validating_tester
#define ROCKSDB_TESTER rocksdb_validating_tester
#endif

using namespace eosio;
using namespace eosio::testing;
using namespace chain;
using namespace fc;

namespace bio = boost::iostreams;

template<uint64_t NAME>
struct test_api_action {
	static account_name get_account() {
		return "testapi"_n;
	}

	static action_name get_name() {
		return action_name(NAME);
	}
};

FC_REFLECT_TEMPLATE((uint64_t T), test_api_action<T>, BOOST_PP_SEQ_NIL)

template<uint64_t NAME>
struct test_chain_action {
	static account_name get_account() {
		return account_name(config::system_account_name);
	}

	static action_name get_name() {
		return action_name(NAME);
	}
};

FC_REFLECT_TEMPLATE((uint64_t T), test_chain_action<T>, BOOST_PP_SEQ_NIL)

struct check_auth {
   account_name            account;
   permission_name         permission;
   vector<public_key_type> pubkeys;
};

FC_REFLECT(check_auth, (account)(permission)(pubkeys) )

struct test_permission_last_used_action {
   account_name     account;
   permission_name  permission;
   fc::time_point   last_used_time;
};

FC_REFLECT( test_permission_last_used_action, (account)(permission)(last_used_time) )

constexpr uint64_t TEST_METHOD(const char* CLASS, const char *METHOD) {
  return ( (uint64_t(DJBH(CLASS))<<32) | uint32_t(DJBH(METHOD)) );
}

string I64Str(int64_t i)
{
	std::stringstream ss;
	ss << i;
	return ss.str();
}

string U64Str(uint64_t i)
{
   std::stringstream ss;
   ss << i;
   return ss.str();
}

string U128Str(unsigned __int128 i)
{
   return fc::variant(fc::uint128(i)).get_string();
}

template <typename T>
transaction_trace_ptr CallAction(TESTER& test, T ac, const vector<account_name>& scope = {"testapi"_n}) {
   signed_transaction trx;


   auto pl = vector<permission_level>{{scope[0], config::active_name}};
   if (scope.size() > 1)
      for (size_t i = 1; i < scope.size(); i++)
         pl.push_back({scope[i], config::active_name});

   action act(pl, ac);
   trx.actions.push_back(act);

   test.set_transaction_headers(trx);
   auto sigs = trx.sign(test.get_private_key(scope[0], "active"), test.control->get_chain_id());
   flat_set<public_key_type> keys;
   trx.get_signature_keys(test.control->get_chain_id(), fc::time_point::maximum(), keys);
   auto res = test.push_transaction(trx);
   BOOST_CHECK_EQUAL(res->receipt->status, transaction_receipt::executed);
   test.produce_block();
   return res;
}

template <typename T, typename Tester>
std::pair<transaction_trace_ptr, signed_block_ptr> _CallFunction(Tester& test, T ac, const vector<char>& data, const vector<account_name>& scope = {"testapi"_n}, bool no_throw = false) {
   {
      signed_transaction trx;

      auto pl = vector<permission_level>{{scope[0], config::active_name}};
      if (scope.size() > 1)
         for (unsigned int i = 1; i < scope.size(); i++)
            pl.push_back({scope[i], config::active_name});

      action act(pl, ac);
      act.data = data;
      act.authorization = {{"testapi"_n, config::active_name}};
      trx.actions.push_back(act);

      test.set_transaction_headers(trx, test.DEFAULT_EXPIRATION_DELTA);
      auto sigs = trx.sign(test.get_private_key(scope[0], "active"), test.control->get_chain_id());

      flat_set<public_key_type> keys;
      trx.get_signature_keys(test.control->get_chain_id(), fc::time_point::maximum(), keys);

      auto res = test.push_transaction(trx, fc::time_point::maximum(), Tester::DEFAULT_BILLED_CPU_TIME_US, no_throw);
      if (!no_throw) {
         BOOST_CHECK_EQUAL(res->receipt->status, transaction_receipt::executed);
      }
      auto block = test.produce_block();
      return { res, block };
   }
}

template <typename T, typename Tester>
transaction_trace_ptr CallFunction(Tester& test, T ac, const vector<char>& data, const vector<account_name>& scope = {"testapi"_n}, bool no_throw = false) {
   {
      return _CallFunction(test, ac, data, scope, no_throw).first;
   }
}

#define CALL_TEST_FUNCTION(_TESTER, CLS, MTH, DATA) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)
#define CALL_TEST_FUNCTION_WITH_BLOCK(_TESTER, CLS, MTH, DATA) _CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)
#define CALL_TEST_FUNCTION_SYSTEM(_TESTER, CLS, MTH, DATA) CallFunction(_TESTER, test_chain_action<TEST_METHOD(CLS, MTH)>{}, DATA, {config::system_account_name} )
#define CALL_TEST_FUNCTION_SCOPE(_TESTER, CLS, MTH, DATA, ACCOUNT) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA, ACCOUNT)
#define CALL_TEST_FUNCTION_NO_THROW(_TESTER, CLS, MTH, DATA) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA, {"testapi"_n}, true)
#define CALL_TEST_FUNCTION_AND_CHECK_EXCEPTION(_TESTER, CLS, MTH, DATA, EXC, EXC_MESSAGE) \
BOOST_CHECK_EXCEPTION( \
   CALL_TEST_FUNCTION( _TESTER, CLS, MTH, DATA), \
                       EXC, \
                       [](const EXC& e) { \
                          return expect_assert_message(e, EXC_MESSAGE); \
                     } \
);

bool is_access_violation(fc::unhandled_exception const & e) {
   try {
      std::rethrow_exception(e.get_inner_exception());
    }
    catch (const eosio::chain::wasm_execution_error& e) {
       return true;
    } catch (...) {

    }
   return false;
}
bool is_access_violation(const Runtime::Exception& e) { return true; }

bool is_assert_exception(fc::assert_exception const & e) { return true; }
bool is_page_memory_error(page_memory_error const &e) { return true; }
bool is_unsatisfied_authorization(unsatisfied_authorization const & e) { return true;}
bool is_wasm_execution_error(eosio::chain::wasm_execution_error const& e) {return true;}
bool is_tx_net_usage_exceeded(const tx_net_usage_exceeded& e) { return true; }
bool is_block_net_usage_exceeded(const tx_cpu_usage_exceeded& e) { return true; }
bool is_tx_cpu_usage_exceeded(const tx_cpu_usage_exceeded& e) { return true; }
bool is_block_cpu_usage_exceeded(const tx_cpu_usage_exceeded& e) { return true; }
bool is_deadline_exception(const deadline_exception& e) { return true; }

/*
 * register test suite `api_tests`
 */
BOOST_AUTO_TEST_SUITE(api_tests)

/*
 * Print capturing stuff
 */
std::vector<std::string> capture;

struct MySink : public bio::sink
{

   std::streamsize write(const char* s, std::streamsize n)
   {
      std::string tmp;
      tmp.assign(s, n);
      capture.push_back(tmp);
      std::cout << "stream : [" << tmp << "]" << std::endl;
      return n;
   }
};
uint32_t last_fnc_err = 0;

BOOST_FIXTURE_TEST_CASE(action_receipt_tests, TESTER) { try {
   produce_blocks(2);
   create_account( "test"_n );
   set_code( "test"_n, contracts::payloadless_wasm() );
   produce_blocks(1);

   auto call_doit_and_check = [&]( account_name contract, account_name signer, auto&& checker ) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{signer, config::active_name}}, contract, "doit"_n, bytes{} );
      this->set_transaction_headers( trx, this->DEFAULT_EXPIRATION_DELTA );
      trx.sign( this->get_private_key(signer, "active"), control->get_chain_id() );
      auto res = this->push_transaction(trx);
      checker( res );
   };

   auto call_provereset_and_check = [&]( account_name contract, account_name signer, auto&& checker ) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{signer, config::active_name}}, contract, "provereset"_n, bytes{} );
      this->set_transaction_headers( trx, this->DEFAULT_EXPIRATION_DELTA );
      trx.sign( this->get_private_key(signer, "active"), control->get_chain_id() );
      auto res = this->push_transaction(trx);
      checker( res );
   };

   auto result = push_reqauth( config::system_account_name, "active" );
   BOOST_REQUIRE_EQUAL( result->receipt->status, transaction_receipt::executed );
   BOOST_REQUIRE( result->action_traces[0].receipt->auth_sequence.find( config::system_account_name )
                     != result->action_traces[0].receipt->auth_sequence.end() );
   auto base_global_sequence_num = result->action_traces[0].receipt->global_sequence;
   auto base_system_recv_seq_num = result->action_traces[0].receipt->recv_sequence;
   auto base_system_auth_seq_num = result->action_traces[0].receipt->auth_sequence[config::system_account_name];
   auto base_system_code_seq_num = result->action_traces[0].receipt->code_sequence.value;
   auto base_system_abi_seq_num  = result->action_traces[0].receipt->abi_sequence.value;

   uint64_t base_test_recv_seq_num = 0;
   uint64_t base_test_auth_seq_num = 0;
   call_doit_and_check( "test"_n, "test"_n, [&]( const transaction_trace_ptr& res ) {
      BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->global_sequence, base_global_sequence_num + 1 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->code_sequence.value, 1 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->abi_sequence.value, 0 );
      base_test_recv_seq_num = res->action_traces[0].receipt->recv_sequence;
      BOOST_CHECK( base_test_recv_seq_num > 0 );
      base_test_recv_seq_num--;
      const auto& m = res->action_traces[0].receipt->auth_sequence;
      BOOST_CHECK_EQUAL( m.size(), 1 );
      BOOST_CHECK_EQUAL( m.begin()->first.to_string(), "test" );
      base_test_auth_seq_num = m.begin()->second;
      BOOST_CHECK( base_test_auth_seq_num > 0 );
      --base_test_auth_seq_num;
   } );

   set_code( "test"_n, contracts::asserter_wasm() );
   set_code( config::system_account_name, contracts::payloadless_wasm() );

   call_provereset_and_check( "test"_n, "test"_n, [&]( const transaction_trace_ptr& res ) {
      BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->global_sequence, base_global_sequence_num + 4 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->recv_sequence, base_test_recv_seq_num + 2 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->code_sequence.value, 2 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->abi_sequence.value, 0 );
      const auto& m = res->action_traces[0].receipt->auth_sequence;
      BOOST_CHECK_EQUAL( m.size(), 1 );
      BOOST_CHECK_EQUAL( m.begin()->first.to_string(), "test" );
      BOOST_CHECK_EQUAL( m.begin()->second, base_test_auth_seq_num + 3 );
   } );

   produce_blocks(1); // Added to avoid the last doit transaction from being considered a duplicate.
   // Adding a block also retires an onblock action which increments both the global sequence number
   // and the recv and auth sequences numbers for the system account.

   call_doit_and_check( config::system_account_name, "test"_n, [&]( const transaction_trace_ptr& res ) {
      BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->global_sequence, base_global_sequence_num + 6 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->recv_sequence, base_system_recv_seq_num + 4 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->code_sequence.value, base_system_code_seq_num + 1 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->abi_sequence.value, base_system_abi_seq_num );
      const auto& m = res->action_traces[0].receipt->auth_sequence;
      BOOST_CHECK_EQUAL( m.size(), 1 );
      BOOST_CHECK_EQUAL( m.begin()->first.to_string(), "test" );
      BOOST_CHECK_EQUAL( m.begin()->second, base_test_auth_seq_num + 4 );
   } );

   set_code( config::system_account_name, contracts::eosio_bios_wasm() );

   set_code( "test"_n, contracts::eosio_bios_wasm() );
   set_abi( "test"_n, contracts::eosio_bios_abi().data() );
	set_code( "test"_n, contracts::payloadless_wasm() );

   call_doit_and_check( "test"_n, "test"_n, [&]( const transaction_trace_ptr& res ) {
      BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed);
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->global_sequence, base_global_sequence_num + 11 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->recv_sequence, base_test_recv_seq_num + 3 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->code_sequence.value, 4 );
      BOOST_CHECK_EQUAL( res->action_traces[0].receipt->abi_sequence.value, 1 );
      const auto& m = res->action_traces[0].receipt->auth_sequence;
      BOOST_CHECK_EQUAL( m.size(), 1 );
      BOOST_CHECK_EQUAL( m.begin()->first.to_string(), "test" );
      BOOST_CHECK_EQUAL( m.begin()->second, base_test_auth_seq_num + 8 );
   } );

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * action_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_tests, TESTER) { try {
	produce_blocks(2);
	create_account( "testapi"_n );
	create_account( "acc1"_n );
	create_account( "acc2"_n );
	create_account( "acc3"_n );
	create_account( "acc4"_n );
	produce_blocks(10);
	set_code( "testapi"_n, contracts::test_api_wasm() );
	produce_blocks(1);

   // test assert_true
	CALL_TEST_FUNCTION( *this, "test_action", "assert_true", {});

   //test assert_false
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_action", "assert_false", {} ),
                          eosio_assert_message_exception, eosio_assert_message_is("test_action::assert_false") );

   // test read_action_normal
   dummy_action dummy13{DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   CALL_TEST_FUNCTION( *this, "test_action", "read_action_normal", fc::raw::pack(dummy13));

   // test read_action_to_0
   std::vector<char> raw_bytes((1<<16));
   CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_0", raw_bytes );

   // test read_action_to_0
   raw_bytes.resize((1<<16)+1);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_0", raw_bytes), eosio::chain::wasm_execution_error,
         [](const eosio::chain::wasm_execution_error& e) {
            return expect_assert_message(e, "access violation");
         }
      );

   // test read_action_to_64k
   raw_bytes.resize(1);
	CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_64k", raw_bytes );

   // test read_action_to_64k
   raw_bytes.resize(3);
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "read_action_to_64k", raw_bytes ), eosio::chain::wasm_execution_error,
         [](const eosio::chain::wasm_execution_error& e) {
            return expect_assert_message(e, "access violation");
         }
      );

   // test require_notice
   auto scope = std::vector<account_name>{"testapi"_n};
   auto test_require_notice = [this](auto& test, std::vector<char>& data, std::vector<account_name>& scope){
      signed_transaction trx;
      auto tm = test_api_action<TEST_METHOD("test_action", "require_notice")>{};

      action act(std::vector<permission_level>{{"testapi"_n, config::active_name}}, tm);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(data.begin(), data.end(), std::back_inserter(dest));
      trx.actions.push_back(act);

      test.set_transaction_headers(trx);
      trx.sign(test.get_private_key("inita"_n, "active"), control->get_chain_id());
      auto res = test.push_transaction(trx);
      BOOST_CHECK_EQUAL(res->receipt->status, transaction_receipt::executed);
   };

   BOOST_CHECK_EXCEPTION(test_require_notice(*this, raw_bytes, scope), unsatisfied_authorization,
         [](const unsatisfied_authorization& e) {
            return expect_assert_message(e, "transaction declares authority");
         }
      );

   // test require_auth
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", {}), missing_auth_exception,
         [](const missing_auth_exception& e) {
            return expect_assert_message(e, "missing authority of");
         }
      );

   // test require_auth
   auto a3only = std::vector<permission_level>{{"acc3"_n, config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a3only)), missing_auth_exception,
         [](const missing_auth_exception& e) {
            return expect_assert_message(e, "missing authority of");
         }
      );

   // test require_auth
   auto a4only = std::vector<permission_level>{{"acc4"_n, config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a4only)), missing_auth_exception,
         [](const missing_auth_exception& e) {
            return expect_assert_message(e, "missing authority of");
         }
      );

   // test require_auth
   auto a3a4 = std::vector<permission_level>{{"acc3"_n, config::active_name}, {"acc4"_n, config::active_name}};
   auto a3a4_scope = std::vector<account_name>{"acc3"_n, "acc4"_n};
   {
      signed_transaction trx;
      auto tm = test_api_action<TEST_METHOD("test_action", "require_auth")>{};
      auto pl = a3a4;
      if (a3a4_scope.size() > 1)
         for (unsigned int i=1; i < a3a4_scope.size(); i++)
            pl.push_back({a3a4_scope[i], config::active_name});

      action act(pl, tm);
      auto dat = fc::raw::pack(a3a4);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(dat.begin(), dat.end(), std::back_inserter(dest));
      act.authorization = {{"testapi"_n, config::active_name}, {"acc3"_n, config::active_name}, {"acc4"_n, config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key("testapi"_n, "active"), control->get_chain_id());
      trx.sign(get_private_key("acc3"_n, "active"), control->get_chain_id());
      trx.sign(get_private_key("acc4"_n, "active"), control->get_chain_id());
      auto res = push_transaction(trx);
      BOOST_CHECK_EQUAL(res->receipt->status, transaction_receipt::executed);
   }

   uint64_t now = static_cast<uint64_t>( control->head_block_time().time_since_epoch().count() );
   now += config::block_interval_us;
   CALL_TEST_FUNCTION( *this, "test_action", "test_current_time", fc::raw::pack(now));

   // test current_time
   produce_block();
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_action", "test_current_time", fc::raw::pack(now) ),
                          eosio_assert_message_exception, eosio_assert_message_is("tmp == current_time()")     );

   // test test_current_receiver
   CALL_TEST_FUNCTION( *this, "test_action", "test_current_receiver", fc::raw::pack("testapi"_n));

   // test send_action_sender
   CALL_TEST_FUNCTION( *this, "test_transaction", "send_action_sender", fc::raw::pack("testapi"_n));

   produce_block();

   // test_publication_time
   uint64_t pub_time = static_cast<uint64_t>( control->head_block_time().time_since_epoch().count() );
   pub_time += config::block_interval_us;
   CALL_TEST_FUNCTION( *this, "test_action", "test_publication_time", fc::raw::pack(pub_time) );

   // test test_abort
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "test_abort", {} ), abort_called,
         [](const fc::exception& e) {
            return expect_assert_message(e, "abort() called");
         }
      );

   dummy_action da = { DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C };
   CallAction(*this, da);
   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

// test require_recipient loop (doesn't cause infinite loop)
BOOST_FIXTURE_TEST_CASE(require_notice_tests, TESTER) { try {
      produce_blocks(2);
      create_account( "testapi"_n );
      create_account( "acc5"_n );
      produce_blocks(1);
      set_code( "testapi"_n, contracts::test_api_wasm() );
      set_code( "acc5"_n, contracts::test_api_wasm() );
      produce_blocks(1);

      // test require_notice
      signed_transaction trx;
      auto tm = test_api_action<TEST_METHOD( "test_action", "require_notice_tests" )>{};

      action act( std::vector<permission_level>{{"testapi"_n, config::active_name}}, tm );
      trx.actions.push_back( act );

      set_transaction_headers( trx );
      trx.sign( get_private_key( "testapi"_n, "active" ), control->get_chain_id() );
      auto res = push_transaction( trx );
      BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed );

   } FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(ram_billing_in_notify_tests) { try {
   fc::temp_directory tempdir;
   validating_tester chain( tempdir, true );
   chain.execute_setup_policy( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = chain.control->get_protocol_feature_manager();
   const auto& d = pfm.get_builtin_digest(builtin_protocol_feature_t::action_return_value); // testapi requires this
   BOOST_REQUIRE(d);
   chain.preactivate_protocol_features( {*d} );

   chain.produce_blocks(2);
   chain.create_account( "testapi"_n );
   chain.create_account( "testapi2"_n );
   chain.produce_blocks(10);
   chain.set_code( "testapi"_n, contracts::test_api_wasm() );
   chain.produce_blocks(1);
   chain.set_code( "testapi2"_n, contracts::test_api_wasm() );
   chain.produce_blocks(1);

   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( chain, "test_action", "test_ram_billing_in_notify",
                                              fc::raw::pack( ((unsigned __int128)"testapi2"_n.to_uint64_t() << 64) | "testapi"_n.to_uint64_t() ) ),
                          subjective_block_production_exception,
                          fc_exception_message_is("Cannot charge RAM to other accounts during notify.")
   );


   CALL_TEST_FUNCTION( chain, "test_action", "test_ram_billing_in_notify", fc::raw::pack( ((unsigned __int128)"testapi2"_n.to_uint64_t() << 64) | 0 ) );

   CALL_TEST_FUNCTION( chain, "test_action", "test_ram_billing_in_notify", fc::raw::pack( ((unsigned __int128)"testapi2"_n.to_uint64_t() << 64) | "testapi2"_n.to_uint64_t() ) );

   BOOST_REQUIRE_EQUAL( chain.validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * context free action tests
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(cf_action_tests, TESTER) { try {
      produce_blocks(2);
      create_account( "testapi"_n );
      create_account( "dummy"_n );
      produce_blocks(10);
      set_code( "testapi"_n, contracts::test_api_wasm() );
      produce_blocks(1);
      cf_action cfa;
      signed_transaction trx;
      set_transaction_headers(trx);
      // need at least one normal action
      BOOST_CHECK_EXCEPTION(push_transaction(trx), tx_no_auths,
                            [](const fc::assert_exception& e) {
                               return expect_assert_message(e, "transaction must have at least one authorization");
                            }
      );

      action act({}, cfa);
      trx.context_free_actions.push_back(act);
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100)); // verify payload matches context free data
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
      set_transaction_headers(trx);

      BOOST_CHECK_EXCEPTION(push_transaction(trx), tx_no_auths,
                            [](const fc::exception& e) {
                               return expect_assert_message(e, "transaction must have at least one authorization");
                            }
      );

      trx.signatures.clear();

      // add a normal action along with cfa
      dummy_action da = { DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C };
      auto pl = vector<permission_level>{{"testapi"_n, config::active_name}};
      action act1(pl, da);
      trx.actions.push_back(act1);
      set_transaction_headers(trx);
      // run normal passing case
      auto sigs = trx.sign(get_private_key("testapi"_n, "active"), control->get_chain_id());
      auto res = push_transaction(trx);

      BOOST_CHECK_EQUAL(res->receipt->status, transaction_receipt::executed);

      // attempt to access context free api in non context free action

      da = { DUMMY_ACTION_DEFAULT_A, 200, DUMMY_ACTION_DEFAULT_C };
      action act2(pl, da);
      trx.signatures.clear();
      trx.actions.clear();
      trx.actions.push_back(act2);
      set_transaction_headers(trx);
      // run (dummy_action.b = 200) case looking for invalid use of context_free api
      sigs = trx.sign(get_private_key("testapi"_n, "active"), control->get_chain_id());
      BOOST_CHECK_EXCEPTION(push_transaction(trx), unaccessible_api,
                            [](const fc::exception& e) {
                               return expect_assert_message(e, "this API may only be called from context_free apply");
                            }
      );
      {
         // back to normal action
         action act1(pl, da);
         signed_transaction trx;
         trx.context_free_actions.push_back(act);
         trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100)); // verify payload matches context free data
         trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));

         trx.actions.push_back(act1);
         // attempt to access non context free api
         for (uint32_t i = 200; i <= 212; ++i) {
            trx.context_free_actions.clear();
            trx.context_free_data.clear();
            cfa.payload = i;
            cfa.cfd_idx = 1;
            action cfa_act({}, cfa);
            trx.context_free_actions.emplace_back(cfa_act);
            trx.signatures.clear();
            set_transaction_headers(trx);
            sigs = trx.sign(get_private_key("testapi"_n, "active"), control->get_chain_id());
            BOOST_CHECK_EXCEPTION(push_transaction(trx), unaccessible_api,
                 [](const fc::exception& e) {
                    return expect_assert_message(e, "only context free api's can be used in this context" );
                 }
            );
         }

      }
      produce_block();

      // test send context free action
      auto ttrace = CALL_TEST_FUNCTION( *this, "test_transaction", "send_cf_action", {} );

      BOOST_REQUIRE_EQUAL(ttrace->action_traces.size(), 2);
      BOOST_CHECK_EQUAL((int)(ttrace->action_traces[1].creator_action_ordinal), 1);
      BOOST_CHECK_EQUAL(ttrace->action_traces[1].receiver, account_name("dummy"));
      BOOST_CHECK_EQUAL(ttrace->action_traces[1].act.account, account_name("dummy"));
      BOOST_CHECK_EQUAL(ttrace->action_traces[1].act.name, account_name("event1"));
      BOOST_CHECK_EQUAL(ttrace->action_traces[1].act.authorization.size(), 0);

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_transaction", "send_cf_action_fail", {} ),
                             eosio_assert_message_exception,
                             eosio_assert_message_is("context free actions cannot have authorizations") );

      BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE(cfa_tx_signature, TESTER)  try {

   action cfa({}, cf_action());

   signed_transaction tx1;
   tx1.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100));
   tx1.context_free_actions.push_back(cfa);
   set_transaction_headers(tx1);

   signed_transaction tx2;
   tx2.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
   tx2.context_free_actions.push_back(cfa);
   set_transaction_headers(tx2);

   const private_key_type& priv_key = get_private_key(name("dummy"), "active");
   BOOST_TEST(tx1.sign(priv_key, control->get_chain_id()).to_string() != tx2.sign(priv_key, control->get_chain_id()).to_string());

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(cfa_stateful_api, TESTER)  try {

   create_account( "testapi"_n );
	produce_blocks(1);
	set_code( "testapi"_n, contracts::test_api_wasm() );

   account_name a = "testapi2"_n;
   account_name creator = config::system_account_name;

   signed_transaction trx;

   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                 newaccount{
                                 .creator  = creator,
                                 .name     = a,
                                 .owner    = authority( get_public_key( a, "owner" ) ),
                                 .active   = authority( get_public_key( a, "active" ) )
                                 });
   action act({}, test_api_action<TEST_METHOD("test_transaction", "stateful_api")>{});
   trx.context_free_actions.push_back(act);
   set_transaction_headers(trx);
   trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
   BOOST_CHECK_EXCEPTION(push_transaction( trx ), fc::exception,
      [&](const fc::exception &e) {
         return expect_assert_message(e, "only context free api's can be used in this context");
      });

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(deferred_cfa_failed, TESTER)  try {

   create_account( "testapi"_n );
	produce_blocks(1);
	set_code( "testapi"_n, contracts::test_api_wasm() );

   account_name a = "testapi2"_n;
   account_name creator = config::system_account_name;

   signed_transaction trx;

   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                 newaccount{
                                 .creator  = creator,
                                 .name     = a,
                                 .owner    = authority( get_public_key( a, "owner" ) ),
                                 .active   = authority( get_public_key( a, "active" ) )
                                 });
   action act({}, test_api_action<TEST_METHOD("test_transaction", "stateful_api")>{});
   trx.context_free_actions.push_back(act);
   set_transaction_headers(trx, 10, 2);
   trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );

   BOOST_CHECK_EXCEPTION(push_transaction( trx ), fc::exception,
      [&](const fc::exception &e) {
         return expect_assert_message(e, "only context free api's can be used in this context");
      });

   produce_blocks(10);

   // CFA failed, testapi2 not created
   create_account( "testapi2"_n );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(deferred_cfa_success, TESTER)  try {

   create_account( "testapi"_n );
	produce_blocks(1);
	set_code( "testapi"_n, contracts::test_api_wasm() );

   account_name a = "testapi2"_n;
   account_name creator = config::system_account_name;

   signed_transaction trx;

   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                 newaccount{
                                 .creator  = creator,
                                 .name     = a,
                                 .owner    = authority( get_public_key( a, "owner" ) ),
                                 .active   = authority( get_public_key( a, "active" ) )
                                 });
   action act({}, test_api_action<TEST_METHOD("test_transaction", "context_free_api")>{});
   trx.context_free_actions.push_back(act);
   set_transaction_headers(trx, 10, 2);
   trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
   auto trace = push_transaction( trx );
   BOOST_REQUIRE(trace != nullptr);
   if (trace) {
      BOOST_REQUIRE_EQUAL(transaction_receipt_header::status_enum::delayed, trace->receipt->status);
      BOOST_REQUIRE_EQUAL(1, trace->action_traces.size());
   }
   produce_blocks(10);

   // CFA success, testapi2 created
   BOOST_CHECK_EXCEPTION(create_account( "testapi2"_n ), fc::exception,
      [&](const fc::exception &e) {
         return expect_assert_message(e, "Cannot create account named testapi2, as that name is already taken");
      });
   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE(light_validation_skip_cfa) try {
   tester chain(setup_policy::full);

   std::vector<signed_block_ptr> blocks;
   blocks.push_back(chain.produce_block());

   chain.create_account( "testapi"_n );
   chain.create_account( "dummy"_n );
   blocks.push_back(chain.produce_block());
   chain.set_code( "testapi"_n, contracts::test_api_wasm() );
   blocks.push_back(chain.produce_block());

   cf_action cfa;
   signed_transaction trx;
   action act({}, cfa);
   trx.context_free_actions.push_back(act);
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100)); // verify payload matches context free data
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
   // add a normal action along with cfa
   dummy_action da = { DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C };
   action act1(vector<permission_level>{{"testapi"_n, config::active_name}}, da);
   trx.actions.push_back(act1);
   chain.set_transaction_headers(trx);
   // run normal passing case
   auto sigs = trx.sign(chain.get_private_key("testapi"_n, "active"), chain.control->get_chain_id());
   auto trace = chain.push_transaction(trx);
   blocks.push_back(chain.produce_block());

   BOOST_REQUIRE(trace->receipt);
   BOOST_CHECK_EQUAL(trace->receipt->status, transaction_receipt::executed);
   BOOST_CHECK_EQUAL(2, trace->action_traces.size());

   BOOST_CHECK(trace->action_traces.at(0).context_free); // cfa
   BOOST_CHECK_EQUAL("test\n", trace->action_traces.at(0).console); // cfa executed

   BOOST_CHECK(!trace->action_traces.at(1).context_free); // non-cfa
   BOOST_CHECK_EQUAL("", trace->action_traces.at(1).console);


   fc::temp_directory tempdir;
   auto conf_genesis = tester::default_config( tempdir );

   auto& cfg = conf_genesis.first;
   cfg.trusted_producers = { "eosio"_n }; // light validation

   tester other( conf_genesis.first, conf_genesis.second );
   other.execute_setup_policy( setup_policy::full );


   transaction_trace_ptr other_trace;
   auto cc = other.control->applied_transaction.connect( [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
      auto& t = std::get<0>(x);
      if( t && t->id == trace->id ) {
         other_trace = t;
      }
   } );

   for (auto& new_block : blocks) {
      other.push_block(new_block);
   }
   blocks.clear();

   BOOST_REQUIRE(other_trace);
   BOOST_REQUIRE(other_trace->receipt);
   BOOST_CHECK_EQUAL(other_trace->receipt->status, transaction_receipt::executed);
   BOOST_CHECK(*trace->receipt == *other_trace->receipt);
   BOOST_CHECK_EQUAL(2, other_trace->action_traces.size());

   BOOST_CHECK(other_trace->action_traces.at(0).context_free); // cfa
   BOOST_CHECK_EQUAL("", other_trace->action_traces.at(0).console); // cfa not executed for light validation (trusted producer)
   BOOST_CHECK_EQUAL(trace->action_traces.at(0).receipt->global_sequence, other_trace->action_traces.at(0).receipt->global_sequence);
   BOOST_CHECK_EQUAL(trace->action_traces.at(0).receipt->digest(), other_trace->action_traces.at(0).receipt->digest());

   BOOST_CHECK(!other_trace->action_traces.at(1).context_free); // non-cfa
   BOOST_CHECK_EQUAL("", other_trace->action_traces.at(1).console);
   BOOST_CHECK_EQUAL(trace->action_traces.at(1).receipt->global_sequence, other_trace->action_traces.at(1).receipt->global_sequence);
   BOOST_CHECK_EQUAL(trace->action_traces.at(1).receipt->digest(), other_trace->action_traces.at(1).receipt->digest());


   other.close();

} FC_LOG_AND_RETHROW()

/*************************************************************************************
 * checktime_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(checktime_pass_tests, TESTER) { try {
	produce_blocks(2);
	create_account( "testapi"_n );
	produce_blocks(10);
	set_code( "testapi"_n, contracts::test_api_wasm() );
	produce_blocks(1);

   // test checktime_pass
   CALL_TEST_FUNCTION( *this, "test_checktime", "checktime_pass", {});

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

template<class T>
void call_test(TESTER& test, T ac, uint32_t billed_cpu_time_us , uint32_t max_cpu_usage_ms = 200, std::vector<char> payload = {} ) {
   signed_transaction trx;

   auto pl = vector<permission_level>{{"testapi"_n, config::active_name}};
   action act(pl, ac);
   act.data = payload;

   trx.actions.push_back(act);
   test.set_transaction_headers(trx);
   auto sigs = trx.sign(test.get_private_key("testapi"_n, "active"), test.control->get_chain_id());
   flat_set<public_key_type> keys;
   trx.get_signature_keys(test.control->get_chain_id(), fc::time_point::maximum(), keys);
   auto res = test.push_transaction( trx, fc::time_point::now() + fc::milliseconds(max_cpu_usage_ms), billed_cpu_time_us );
   BOOST_CHECK_EQUAL(res->receipt->status, transaction_receipt::executed);
   test.produce_block();
};

BOOST_AUTO_TEST_CASE(checktime_fail_tests) { try {
   TESTER t;
   t.produce_blocks(2);

   ilog( "create account" );
   t.create_account( "testapi"_n );
   ilog( "set code" );
   t.set_code( "testapi"_n, contracts::test_api_wasm() );
   ilog( "produce block" );
   t.produce_blocks(1);

   int64_t x; int64_t net; int64_t cpu;
   t.control->get_resource_limits_manager().get_account_limits( "testapi"_n, x, net, cpu );
   wdump((net)(cpu));

#warning TODO call the contract before testing to cache it, and validate that it was cached

   BOOST_CHECK_EXCEPTION( call_test( t, test_api_action<TEST_METHOD("test_checktime", "checktime_failure")>{},
                                     5000, 200, fc::raw::pack(10000000000000000000ULL) ),
                          deadline_exception, is_deadline_exception );

   BOOST_CHECK_EXCEPTION( call_test( t, test_api_action<TEST_METHOD("test_checktime", "checktime_failure")>{},
                                     0, 200, fc::raw::pack(10000000000000000000ULL) ),
                          tx_cpu_usage_exceeded, is_tx_cpu_usage_exceeded );

   uint32_t time_left_in_block_us = config::default_max_block_cpu_usage - config::default_min_transaction_cpu_usage;
   std::string dummy_string = "nonce";
   uint32_t increment = config::default_max_transaction_cpu_usage / 3;
   for( auto i = 0; time_left_in_block_us > 2*increment; ++i ) {
      t.push_dummy( "testapi"_n, dummy_string + std::to_string(i), increment );
      time_left_in_block_us -= increment;
   }
   BOOST_CHECK_EXCEPTION( call_test( t, test_api_action<TEST_METHOD("test_checktime", "checktime_failure")>{},
                                    0, 200, fc::raw::pack(10000000000000000000ULL) ),
                          block_cpu_usage_exceeded, is_block_cpu_usage_exceeded );

   BOOST_REQUIRE_EQUAL( t.validate(), true );
} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE(checktime_intrinsic, TESTER) { try {
	produce_blocks(2);
	create_account( "testapi"_n );
	produce_blocks(10);

        std::stringstream ss;
        ss << R"CONTRACT(
(module
  (type $FUNCSIG$vij (func (param i32 i64)))
  (type $FUNCSIG$j (func  (result i64)))
  (type $FUNCSIG$vjj (func (param i64 i64)))
  (type $FUNCSIG$vii (func (param i32 i32)))
  (type $FUNCSIG$i (func  (result i32)))
  (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
  (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
  (type $FUNCSIG$vi (func (param i32)))
  (type $FUNCSIG$v (func ))
  (type $_1 (func (param i64 i64 i64)))
  (export "apply" (func $apply))
   (import "env" "memmove" (func $memmove (param i32 i32 i32) (result i32)))
   (import "env" "printui" (func $printui (param i64)))
  (memory $0 1)

  (func $apply (type $_1)
    (param $0 i64)
    (param $1 i64)
    (param $2 i64)
    (drop (grow_memory (i32.const 527)))

    (call $printui (i64.const 11))
)CONTRACT";

        for(unsigned int i = 0; i < 5000; ++i) {
           ss << R"CONTRACT(
(drop (call $memmove
    (i32.const 1)
    (i32.const 9)
    (i32.const 33554432)
    ))

)CONTRACT";
        }
        ss<< "))";
	set_code( "testapi"_n, ss.str().c_str() );
	produce_blocks(1);

        //initialize cache
        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("doesn't matter", "doesn't matter")>{},
                                          5000, 10 ),
                               deadline_exception, is_deadline_exception );

#warning TODO validate that the contract was successfully cached

        //it will always call
        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("doesn't matter", "doesn't matter")>{},
                                          5000, 10 ),
                               deadline_exception, is_deadline_exception );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(checktime_hashing_fail, TESTER) { try {
	produce_blocks(2);
	create_account( "testapi"_n );
	produce_blocks(10);
	set_code( "testapi"_n, contracts::test_api_wasm() );
	produce_blocks(1);

        //hit deadline exception, but cache the contract
        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_sha1_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

#warning TODO validate that the contract was successfully cached

        //the contract should be cached, now we should get deadline_exception because of calls to checktime() from hashing function
        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_sha1_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_assert_sha1_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_sha256_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_assert_sha256_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_sha512_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_assert_sha512_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_ripemd160_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

        BOOST_CHECK_EXCEPTION( call_test( *this, test_api_action<TEST_METHOD("test_checktime", "checktime_assert_ripemd160_failure")>{},
                                          5000, 3 ),
                               deadline_exception, is_deadline_exception );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * transaction_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(transaction_tests, TESTER) { try {
   produce_blocks(2);
   create_account( "testapi"_n );
   produce_blocks(100);
   set_code( "testapi"_n, contracts::test_api_wasm() );
   produce_blocks(1);

   // test for zero auth
   {
      signed_transaction trx;
      auto tm = test_api_action<TEST_METHOD("test_action", "require_auth")>{};
      action act({}, tm);
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      BOOST_CHECK_EXCEPTION(push_transaction(trx), transaction_exception,
         [](const fc::exception& e) {
            return expect_assert_message(e, "transaction must have at least one authorization");
         }
      );
   }

   // test send_action
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action", {});

   // test send_action_empty
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_empty", {});

   // test send_action_large
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_large", {}), inline_action_too_big_nonprivileged,
         [](const fc::exception& e) {
            return expect_assert_message(e, "inline action too big for nonprivileged account");
         }
      );

   // test send_action_inline_fail
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_inline_fail", {}),
                          eosio_assert_message_exception,
                          eosio_assert_message_is("test_action::assert_false")                          );

   //   test send_transaction
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_transaction", {});

   // test send_transaction_empty
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_transaction_empty", {}), tx_no_auths,
         [](const fc::exception& e) {
            return expect_assert_message(e, "transaction must have at least one authorization");
         }
      );

   {
      produce_blocks(10);
      transaction_trace_ptr trace;
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<0>(x);
         if (t && t->receipt && t->receipt->status != transaction_receipt::executed) { trace = t; }
      } );
      block_state_ptr bsp;
      auto c2 = control->accepted_block.connect([&](const block_state_ptr& b) { bsp = b; });

      // test error handling on deferred transaction failure
      auto test_trace = CALL_TEST_FUNCTION(*this, "test_transaction", "send_transaction_trigger_error_handler", {});

      BOOST_REQUIRE(trace);
      BOOST_CHECK_EQUAL(trace->receipt->status, transaction_receipt::soft_fail);

      std::set<transaction_id_type> block_ids;
      for( const auto& receipt : bsp->block->transactions ) {
         transaction_id_type id;
         if( std::holds_alternative<packed_transaction>(receipt.trx) ) {
            const auto& pt = std::get<packed_transaction>(receipt.trx);
            id = pt.id();
         } else {
            id = std::get<transaction_id_type>(receipt.trx);
         }
         block_ids.insert( id );
      }

      BOOST_CHECK_EQUAL(2, block_ids.size() ); // originating trx and deferred
      BOOST_CHECK_EQUAL(1, block_ids.count( test_trace->id ) ); // originating
      BOOST_CHECK( !test_trace->failed_dtrx_trace );
      BOOST_CHECK_EQUAL(0, block_ids.count( trace->id ) ); // onerror id, not in block
      BOOST_CHECK_EQUAL(1, block_ids.count( trace->failed_dtrx_trace->id ) ); // deferred id since trace moved to failed_dtrx_trace
      BOOST_CHECK( trace->action_traces.at(0).act.name == "onerror"_n );

      c.disconnect();
      c2.disconnect();
   }

   // test test_transaction_size
   CALL_TEST_FUNCTION(*this, "test_transaction", "test_transaction_size", fc::raw::pack(54) ); // TODO: Need a better way to test this.

   // test test_read_transaction
   // this is a bit rough, but I couldn't figure out a better way to compare the hashes
   auto tx_trace = CALL_TEST_FUNCTION( *this, "test_transaction", "test_read_transaction", {} );
   string sha_expect = tx_trace->id;
   BOOST_TEST_MESSAGE( "tx_trace->action_traces.front().console: = " << tx_trace->action_traces.front().console );
   BOOST_TEST_MESSAGE( "sha_expect = " << sha_expect );
   BOOST_CHECK_EQUAL(tx_trace->action_traces.front().console == sha_expect, true);
   // test test_tapos_block_num
   CALL_TEST_FUNCTION(*this, "test_transaction", "test_tapos_block_num", fc::raw::pack(control->head_block_num()) );

   // test test_tapos_block_prefix
   CALL_TEST_FUNCTION(*this, "test_transaction", "test_tapos_block_prefix", fc::raw::pack(control->head_block_id()._hash[1]) );

   // test send_action_recurse
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_recurse", {}), eosio::chain::transaction_exception,
         [](const eosio::chain::transaction_exception& e) {
            return expect_assert_message(e, "max inline action depth per transaction reached");
         }
      );

   BOOST_REQUIRE_EQUAL( validate(), true );

   // test read_transaction only returns packed transaction
   {
      signed_transaction trx;

      auto pl = vector<permission_level>{{"testapi"_n, config::active_name}};
      action act( pl, test_api_action<TEST_METHOD( "test_transaction", "test_read_transaction" )>{} );
      act.data = {};
      act.authorization = {{"testapi"_n, config::active_name}};
      trx.actions.push_back( act );

      set_transaction_headers( trx, DEFAULT_EXPIRATION_DELTA );
      auto sigs = trx.sign( get_private_key( "testapi"_n, "active" ), control->get_chain_id() );

      auto time_limit = fc::microseconds::maximum();
      auto ptrx = std::make_shared<packed_transaction>( signed_transaction(trx), true, packed_transaction::compression_type::none );

      string sha_expect = ptrx->id();
      auto packed = fc::raw::pack( static_cast<const transaction&>(ptrx->get_transaction()) );
      packed.push_back('7'); packed.push_back('7'); // extra ignored
      packed_transaction_v0 pkt( packed, *ptrx->get_signatures(), ptrx->to_packed_transaction_v0()->get_packed_context_free_data(),
                                 packed_transaction_v0::compression_type::none );
      BOOST_CHECK(pkt.get_packed_transaction() == packed);
      ptrx = std::make_shared<packed_transaction>( pkt, true );

      auto fut = transaction_metadata::start_recover_keys( std::move( ptrx ), control->get_thread_pool(), control->get_chain_id(), time_limit );
      auto r = control->push_transaction( fut.get(), fc::time_point::maximum(), DEFAULT_BILLED_CPU_TIME_US, true );
      if( r->except_ptr ) std::rethrow_exception( r->except_ptr );
      if( r->except) throw *r->except;
      tx_trace = r;
      produce_block();
      BOOST_CHECK(tx_trace->action_traces.front().console == sha_expect);
   }


} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * verify subjective limit test case
 *************************************************************************************/
BOOST_AUTO_TEST_CASE(inline_action_subjective_limit) { try {
   const uint32_t _4k = 4 * 1024;
   tester chain(setup_policy::full, db_read_mode::SPECULATIVE, {_4k + 100}, {_4k + 1});
   tester chain2(setup_policy::full, db_read_mode::SPECULATIVE, {_4k + 100}, {_4k});
   signed_block_ptr block;
   for (int n=0; n < 2; ++n) {
      block = chain.produce_block();
      chain2.push_block(block);
   }
   chain.create_account( "testapi"_n );
   for (int n=0; n < 2; ++n) {
      block = chain.produce_block();
      chain2.push_block(block);
   }
   chain.set_code( "testapi"_n, contracts::test_api_wasm() );
   block = chain.produce_block();
   chain2.push_block(block);

   block = CALL_TEST_FUNCTION_WITH_BLOCK(chain, "test_transaction", "send_action_4k", {}).second;
   chain2.push_block(block);
   block = chain.produce_block();
   chain2.push_block(block);

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * verify objective limit test case
 *************************************************************************************/
BOOST_AUTO_TEST_CASE(inline_action_objective_limit) { try {
   const uint32_t _4k = 4 * 1024;
   tester chain(setup_policy::full, db_read_mode::SPECULATIVE, {_4k}, {_4k - 1});
   chain.produce_blocks(2);
   chain.create_account( "testapi"_n );
   chain.produce_blocks(100);
   chain.set_code( "testapi"_n, contracts::test_api_wasm() );
   chain.produce_block();

   chain.push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  mutable_variant_object()
         ("account", "testapi")
         ("is_priv", 1));
   chain.produce_block();

   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(chain, "test_transaction", "send_action_4k", {}), inline_action_too_big,
                         [](const fc::exception& e) {
                            return expect_assert_message(e, "inline action too big");
                         }
   );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(deferred_inline_action_subjective_limit_failure) { try {
   const uint32_t _4k = 4 * 1024;
   tester chain(setup_policy::full, db_read_mode::SPECULATIVE, {_4k + 100}, {_4k});
   chain.produce_blocks(2);
   chain.create_accounts( {"testapi"_n, "testapi2"_n, "alice"_n} );
   chain.set_code( "testapi"_n, contracts::test_api_wasm() );
   chain.set_code( "testapi2"_n, contracts::test_api_wasm() );
   chain.produce_block();

   transaction_trace_ptr trace;
   auto c = chain.control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<0>(x);
         if (t->scheduled) { trace = t; }
      } );
   CALL_TEST_FUNCTION(chain, "test_transaction", "send_deferred_transaction_4k_action", {} );
   BOOST_CHECK(!trace);
   BOOST_CHECK_EXCEPTION(chain.produce_block( fc::seconds(2) ), fc::exception,
                         [](const fc::exception& e) {
                            return expect_assert_message(e, "inline action too big for nonprivileged account");
                         }
   );

   //check that it populates exception fields
   BOOST_REQUIRE(trace);
   BOOST_REQUIRE(trace->except);
   BOOST_REQUIRE(trace->error_code);

   BOOST_REQUIRE_EQUAL(1, trace->action_traces.size());
   c.disconnect();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(deferred_inline_action_subjective_limit) { try {
   const uint32_t _4k = 4 * 1024;
   tester chain(setup_policy::full, db_read_mode::SPECULATIVE, {_4k + 100}, {_4k + 1});
   tester chain2(setup_policy::full, db_read_mode::SPECULATIVE, {_4k + 100}, {_4k});
   signed_block_ptr block;
   for (int n=0; n < 2; ++n) {
      block = chain.produce_block();
      chain2.push_block(block);
   }
   chain.create_accounts( {"testapi"_n, "testapi2"_n, "alice"_n} );
   chain.set_code( "testapi"_n, contracts::test_api_wasm() );
   chain.set_code( "testapi2"_n, contracts::test_api_wasm() );
   block = chain.produce_block();
   chain2.push_block(block);

   transaction_trace_ptr trace;
   auto c = chain.control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
      auto& t = std::get<0>(x);
      if (t->scheduled) { trace = t; }
   } );
   block = CALL_TEST_FUNCTION_WITH_BLOCK(chain, "test_transaction", "send_deferred_transaction_4k_action", {} ).second;
   chain2.push_block(block);
   BOOST_CHECK(!trace);
   block = chain.produce_block( fc::seconds(2) );
   chain2.push_block(block);

   //check that it gets executed afterwards
   BOOST_REQUIRE(trace);

   //confirm printed message
   BOOST_TEST(!trace->action_traces.empty());
   BOOST_TEST(trace->action_traces.back().console == "exec 8");
   c.disconnect();

   for (int n=0; n < 10; ++n) {
      block = chain.produce_block();
      chain2.push_block(block);
   }

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(deferred_transaction_tests, TESTER) { try {
   produce_blocks(2);
   create_accounts( {"testapi"_n, "testapi2"_n, "alice"_n} );
   set_code( "testapi"_n, contracts::test_api_wasm() );
   set_code( "testapi2"_n, contracts::test_api_wasm() );
   produce_blocks(1);

   //schedule
   {
      transaction_trace_ptr trace;
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<0>(x);
         if (t->scheduled) { trace = t; }
      } );
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction", {} );
      BOOST_CHECK(!trace);
      produce_block( fc::seconds(2) );

      //check that it gets executed afterwards
      BOOST_REQUIRE(trace);

      //confirm printed message
      BOOST_TEST(!trace->action_traces.empty());
      BOOST_TEST(trace->action_traces.back().console == "deferred executed\n");
      c.disconnect();
   }

   produce_blocks(10);

   //schedule twice without replace_existing flag (second deferred transaction should replace first one)
   {
      transaction_trace_ptr trace;
      uint32_t count = 0;
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<0>(x);
         if (t && t->scheduled) { trace = t; ++count; }
      } );
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction", {});
      BOOST_CHECK_THROW(CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction", {}), deferred_tx_duplicate);
      produce_blocks( 3 );

      //check that only one deferred transaction executed
      auto dtrxs = get_scheduled_transactions();
      BOOST_CHECK_EQUAL(dtrxs.size(), 1);
      for (const auto& trx: dtrxs) {
         control->push_scheduled_transaction(trx, fc::time_point::maximum(), 0, false);
      }
      BOOST_CHECK_EQUAL(1, count);
      BOOST_REQUIRE(trace);
      BOOST_CHECK_EQUAL( 1, trace->action_traces.size() );
      c.disconnect();
   }

   produce_blocks(10);

   //schedule twice with replace_existing flag (second deferred transaction should replace first one)
   {
      transaction_trace_ptr trace;
      uint32_t count = 0;
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<0>(x);
         if (t && t->scheduled) { trace = t; ++count; }
      } );
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction_replace", {});
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction_replace", {});
      produce_blocks( 3 );

      //check that only one deferred transaction executed
      auto billed_cpu_time_us = control->get_global_properties().configuration.min_transaction_cpu_usage;
      auto dtrxs = get_scheduled_transactions();
      BOOST_CHECK_EQUAL(dtrxs.size(), 1);
      for (const auto& trx: dtrxs) {
         control->push_scheduled_transaction(trx, fc::time_point::maximum(), billed_cpu_time_us, true);
      }
      BOOST_CHECK_EQUAL(1, count);
      BOOST_CHECK(trace);
      BOOST_CHECK_EQUAL( 1, trace->action_traces.size() );
      c.disconnect();
   }

   produce_blocks(10);

   //schedule and cancel
   {
      transaction_trace_ptr trace;
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<0>(x);
         if (t && t->scheduled) { trace = t; }
      } );
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction", {});
      CALL_TEST_FUNCTION(*this, "test_transaction", "cancel_deferred_transaction_success", {});
      produce_block( fc::seconds(2) );
      BOOST_CHECK(!trace);
      c.disconnect();
   }

   produce_blocks(10);

   //cancel_deferred() return zero if no scheduled transaction found
   {
      CALL_TEST_FUNCTION(*this, "test_transaction", "cancel_deferred_transaction_not_found", {});
   }

   produce_blocks(10);

   //repeated deferred transactions
   {
      vector<transaction_trace_ptr> traces;
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<0>(x);
         if (t && t->scheduled) {
            traces.push_back( t );
         }
      } );

      CALL_TEST_FUNCTION(*this, "test_transaction", "repeat_deferred_transaction", fc::raw::pack( (uint32_t)5 ) );

      produce_block();

      c.disconnect();

      BOOST_CHECK_EQUAL( traces.size(), 5 );
   }

   produce_blocks(10);

   {
      // Trigger a tx which in turn sends a deferred tx with payer != receiver
      // Payer is alice in this case, this tx should fail since we don't have the authorization of alice
      dtt_action dtt_act1;
      dtt_act1.payer = "alice"_n.to_uint64_t();
      BOOST_CHECK_THROW(CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act1)), action_validate_exception);

      // Send a tx which in turn sends a deferred tx with the deferred tx's receiver != this tx receiver
      // This will include the authorization of the receiver, and impose any related delay associated with the authority
      // We set the authorization delay to be 10 sec here, and since the deferred tx delay is set to be 5 sec, so this tx should fail
      dtt_action dtt_act2;
      dtt_act2.deferred_account = "testapi2"_n.to_uint64_t();
      dtt_act2.permission_name = "additional"_n.to_uint64_t();
      dtt_act2.delay_sec = 5;

      auto auth = authority(get_public_key(name("testapi"), name(dtt_act2.permission_name).to_string()), 10);
      auth.accounts.push_back( permission_level_weight{{"testapi"_n, config::eosio_code_name}, 1} );

      push_action(config::system_account_name, updateauth::get_name(), name("testapi"), fc::mutable_variant_object()
              ("account", "testapi")
              ("permission", name(dtt_act2.permission_name))
              ("parent", "active")
              ("auth", auth)
      );
      push_action(config::system_account_name, linkauth::get_name(), name("testapi"), fc::mutable_variant_object()
              ("account", "testapi")
              ("code", name(dtt_act2.deferred_account))
              ("type", name(dtt_act2.deferred_action))
              ("requirement", name(dtt_act2.permission_name)));
      BOOST_CHECK_THROW(CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act2)), unsatisfied_authorization);

      // But if the deferred transaction has a sufficient delay, then it should work.
      dtt_act2.delay_sec = 10;
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act2));

      // If the deferred tx receiver == this tx receiver, the authorization checking would originally be bypassed.
      // But not anymore. With the RESTRICT_ACTION_TO_SELF protocol feature activated, it should now objectively
      // fail because testapi@additional permission is not unilaterally satisfied by testapi@eosio.code.
      dtt_action dtt_act3;
      dtt_act3.deferred_account = "testapi"_n.to_uint64_t();
      dtt_act3.permission_name = "additional"_n.to_uint64_t();
      push_action(config::system_account_name, linkauth::get_name(), name("testapi"), fc::mutable_variant_object()
            ("account", "testapi")
            ("code", name(dtt_act3.deferred_account))
            ("type", name(dtt_act3.deferred_action))
            ("requirement", name(dtt_act3.permission_name)));
      BOOST_CHECK_THROW(CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act3)), unsatisfied_authorization);

      // But it should again work if the deferred transaction has a sufficient delay.
      dtt_act3.delay_sec = 10;
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act3));

      // If we make testapi account to be priviledged account:
      // - the deferred transaction will work no matter who is the payer
      // - the deferred transaction will not care about the delay of the authorization
      push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  mutable_variant_object()
                                                          ("account", "testapi")
                                                          ("is_priv", 1));
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act1));
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act2));
   }

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(more_deferred_transaction_tests) { try {
   fc::temp_directory tempdir;
   validating_tester chain( tempdir, true );
   chain.execute_setup_policy( setup_policy::preactivate_feature_and_new_bios );

   const auto& pfm = chain.control->get_protocol_feature_manager();
   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::replace_deferred );
   BOOST_REQUIRE( d );

   chain.preactivate_protocol_features( {*d} );
   chain.produce_block();

   const auto& index = chain.control->db().get_index<generated_transaction_multi_index,by_id>();

   auto print_deferred = [&index]() {
      for( const auto& gto : index ) {
         wlog("id = ${id}, trx_id = ${trx_id}", ("id", gto.id)("trx_id", gto.trx_id));
      }
   };

   const auto& contract_account = account_name("tester");
   const auto& test_account = account_name("alice");

   chain.create_accounts( {contract_account, test_account} );
   chain.set_code( contract_account, contracts::deferred_test_wasm() );
   chain.set_abi( contract_account, contracts::deferred_test_abi().data() );
   chain.produce_block();

   BOOST_REQUIRE_EQUAL(0, index.size());

   chain.push_action( contract_account, "delayedcall"_n, test_account, fc::mutable_variant_object()
      ("payer",     test_account)
      ("sender_id", 0)
      ("contract",  contract_account)
      ("payload",   42)
      ("delay_sec", 1000)
      ("replace_existing", false)
   );

   BOOST_REQUIRE_EQUAL(1, index.size());
   print_deferred();

   signed_transaction trx;
   trx.actions.emplace_back(
      chain.get_action( contract_account, "delayedcall"_n,
                  vector<permission_level>{{test_account, config::active_name}},
                  fc::mutable_variant_object()
                     ("payer",     test_account)
                     ("sender_id", 0)
                     ("contract",  contract_account)
                     ("payload",   13)
                     ("delay_sec", 1000)
                     ("replace_existing", true)
      )
   );
   trx.actions.emplace_back(
      chain.get_action( contract_account, "delayedcall"_n,
                  vector<permission_level>{{test_account, config::active_name}},
                  fc::mutable_variant_object()
                     ("payer",     test_account)
                     ("sender_id", 1)
                     ("contract",  contract_account)
                     ("payload",   42)
                     ("delay_sec", 1000)
                     ("replace_existing", false)
      )
   );
   trx.actions.emplace_back(
      chain.get_action( contract_account, "fail"_n,
                  vector<permission_level>{},
                  fc::mutable_variant_object()
      )
   );
   chain.set_transaction_headers(trx);
   trx.sign( chain.get_private_key( test_account, "active" ), chain.control->get_chain_id() );
   BOOST_REQUIRE_EXCEPTION(
      chain.push_transaction( trx ),
      eosio_assert_message_exception,
      eosio_assert_message_is("fail")
   );

   BOOST_REQUIRE_EQUAL(1, index.size());
   print_deferred();

   chain.produce_blocks(2);

   chain.push_action( contract_account, "delayedcall"_n, test_account, fc::mutable_variant_object()
      ("payer",     test_account)
      ("sender_id", 1)
      ("contract",  contract_account)
      ("payload",   101)
      ("delay_sec", 1000)
      ("replace_existing", false)
   );

   chain.push_action( contract_account, "delayedcall"_n, test_account, fc::mutable_variant_object()
      ("payer",     test_account)
      ("sender_id", 2)
      ("contract",  contract_account)
      ("payload",   102)
      ("delay_sec", 1000)
      ("replace_existing", false)
   );

   BOOST_REQUIRE_EQUAL(3, index.size());
   print_deferred();

   BOOST_REQUIRE_THROW(
      chain.push_action( contract_account, "delayedcall"_n, test_account, fc::mutable_variant_object()
         ("payer",     test_account)
         ("sender_id", 2)
         ("contract",  contract_account)
         ("payload",   101)
         ("delay_sec", 1000)
         ("replace_existing", true)
      ),
      fc::exception
   );

   BOOST_REQUIRE_EQUAL(3, index.size());
   print_deferred();

   signed_transaction trx2;
   trx2.actions.emplace_back(
      chain.get_action( contract_account, "delayedcall"_n,
                  vector<permission_level>{{test_account, config::active_name}},
                  fc::mutable_variant_object()
                     ("payer",     test_account)
                     ("sender_id", 1)
                     ("contract",  contract_account)
                     ("payload",   100)
                     ("delay_sec", 1000)
                     ("replace_existing", true)
      )
   );
   trx2.actions.emplace_back(
      chain.get_action( contract_account, "delayedcall"_n,
                  vector<permission_level>{{test_account, config::active_name}},
                  fc::mutable_variant_object()
                     ("payer",     test_account)
                     ("sender_id", 2)
                     ("contract",  contract_account)
                     ("payload",   101)
                     ("delay_sec", 1000)
                     ("replace_existing", true)
      )
   );
   trx2.actions.emplace_back(
      chain.get_action( contract_account, "delayedcall"_n,
                  vector<permission_level>{{test_account, config::active_name}},
                  fc::mutable_variant_object()
                     ("payer",     test_account)
                     ("sender_id", 1)
                     ("contract",  contract_account)
                     ("payload",   102)
                     ("delay_sec", 1000)
                     ("replace_existing", true)
      )
   );
   trx2.actions.emplace_back(
      chain.get_action( contract_account, "fail"_n,
                  vector<permission_level>{},
                  fc::mutable_variant_object()
      )
   );
   chain.set_transaction_headers(trx2);
   trx2.sign( chain.get_private_key( test_account, "active" ), chain.control->get_chain_id() );
   BOOST_REQUIRE_EXCEPTION(
      chain.push_transaction( trx2 ),
      eosio_assert_message_exception,
      eosio_assert_message_is("fail")
   );

   BOOST_REQUIRE_EQUAL(3, index.size());
   print_deferred();

   BOOST_REQUIRE_EQUAL( chain.validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * chain_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(chain_tests, TESTER) { try {
   produce_blocks(2);

   create_account( "testapi"_n );

   vector<account_name> producers = { "inita"_n,
                                      "initb"_n,
                                      "initc"_n,
                                      "initd"_n,
                                      "inite"_n,
                                      "initf"_n,
                                      "initg"_n,
                                      "inith"_n,
                                      "initi"_n,
                                      "initj"_n,
                                      "initk"_n,
                                      "initl"_n,
                                      "initm"_n,
                                      "initn"_n,
                                      "inito"_n,
                                      "initp"_n,
                                      "initq"_n,
                                      "initr"_n,
                                      "inits"_n,
                                      "initt"_n,
                                      "initu"_n
   };

   create_accounts( producers );
   set_producers (producers );

   set_code( "testapi"_n, contracts::test_api_wasm() );
   produce_blocks(100);

   vector<account_name> prods( control->active_producers().producers.size() );
   for ( uint32_t i = 0; i < prods.size(); i++ ) {
      prods[i] = control->active_producers().producers[i].producer_name;
   }

   CALL_TEST_FUNCTION( *this, "test_chain", "test_activeprods", fc::raw::pack(prods) );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

// Must not leak memory when passing an out-of-bounds unaligned address to get_active_producers
static const char get_active_producers1_wast[] = R"=====(
(module
 (import "env" "get_active_producers" (func $get_active_producers (param i32 i32) (result i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $get_active_producers
   (i32.const 1)
   (i32.const 0xFFFFFFFF)
  ))
 )
)
)=====";

BOOST_FIXTURE_TEST_CASE(get_producers1_tests, TESTER) { try {
   produce_blocks(2);
   create_account( "getprods"_n );
   set_code( "getprods"_n, get_active_producers1_wast );
   produce_block();

   for(int i = 0; i < 100; ++i) {
      signed_transaction trx;
      trx.actions.push_back({ { { "getprods"_n, config::active_name } }, "getprods"_n, ""_n, bytes() });
      set_transaction_headers(trx);
      trx.sign(get_private_key("getprods"_n, "active"), control->get_chain_id());
      BOOST_CHECK_THROW(push_transaction(trx), wasm_exception);
      produce_block();
   }

} FC_LOG_AND_RETHROW() }

// get_active_producers interprets its second argument as the size in bytes (but see below).
// This WASM expects that the current producers are the default {"eosio"}.
static const char get_active_producers2_wast[] = R"=====(
(module
 (import "env" "get_active_producers" (func $get_active_producers (param i32 i32) (result i32)))
 (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (i64.store (i32.const 8) (i64.const 0xCCCCCCCCCCCCCCCC))
  (call $eosio_assert
   (i32.eq
    (call $get_active_producers
     (i32.const 8)
     (i32.const 1))
    (i32.const 1))
   (i32.const 256))
  (call $eosio_assert
   (i64.eq (i64.load (i32.const 8)) (i64.const 0xCCCCCCCCCCCCCC00))
   (i32.const 512)))
 (data (i32.const 256) "get_active_producers should return 1")
 (data (i32.const 512) "get_active_producers should only write one byte"))
)=====";

BOOST_FIXTURE_TEST_CASE(get_active_producers2, TESTER) {
   produce_block();
   create_account( "getprods"_n );
   set_code( "getprods"_n, get_active_producers2_wast );
   produce_block();

   signed_transaction trx;
   trx.actions.push_back({ { { "getprods"_n, config::active_name } }, "getprods"_n, ""_n, bytes() });
   set_transaction_headers(trx);
   trx.sign(get_private_key("getprods"_n, "active"), control->get_chain_id());
   push_transaction(trx);
   produce_block();
}

// When validating memory bounds, however, get_active_producers
// treats the size as counting 8-byte elements.
static const char get_active_producers3_wast[] = R"=====(
(module
 (import "env" "get_active_producers" (func $get_active_producers (param i32 i32) (result i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $get_active_producers
   (i32.wrap/i64 (get_local 2))
   (i32.const 1)))))
)=====";

BOOST_FIXTURE_TEST_CASE(get_active_producers3, TESTER) {
   produce_block();
   create_account( "getprods"_n );
   set_code( "getprods"_n, get_active_producers3_wast );
   produce_block();

   auto pushit = [&](int offset) {
      signed_transaction trx;
      trx.actions.push_back({ { { "getprods"_n, config::active_name } }, "getprods"_n, name(offset), bytes() });
      set_transaction_headers(trx);
      trx.sign(get_private_key("getprods"_n, "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
   };
   pushit(65528);
   BOOST_CHECK_THROW(pushit(65529), wasm_execution_error);
}

// Some db_idx256 intrinsics take both an aligned array and another reference argument.
// Make sure that the copy of the array does not leak when the other argument is out of bounds.
static const char memalign_noleak_wast[] = R"=====(
(module
 (import "env" "db_idx256_find_secondary" (func $db_idx256_find_secondary (param i64 i64 i64 i32 i32 i32) (result i32)))
 (memory 528)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $db_idx256_find_secondary
   (i64.const 0)
   (i64.const 0)
   (i64.const 0)
   (i32.const 1)
   (i32.const 1081343) ;; Just under 33 MiB (32 bytes per element)
   (i32.const 0xFFFFFFFF)
  ))
 )
)
)=====";

BOOST_FIXTURE_TEST_CASE(memalign_noleak_tests, TESTER) { try {
   produce_blocks(2);
   create_account( "noleak"_n );
   set_code( "noleak"_n, memalign_noleak_wast );
   produce_block();

   // This will leak ~33 GiB if there's a memory leak, which should be
   // enough to be noticeable.
   for(int i = 0; i < 1000; ++i) {
      signed_transaction trx;
      trx.actions.push_back({ { { "noleak"_n, config::active_name } }, "noleak"_n, ""_n, bytes() });
      set_transaction_headers(trx);
      trx.sign(get_private_key("noleak"_n, "active"), control->get_chain_id());
      BOOST_CHECK_THROW(push_transaction(trx), wasm_exception);
      produce_block();
   }

} FC_LOG_AND_RETHROW() }

using tester_suites = boost::mpl::list<TESTER, ROCKSDB_TESTER>;

/*************************************************************************************
 * db_tests test case
 *************************************************************************************/
BOOST_AUTO_TEST_CASE_TEMPLATE(db_tests, TESTER_T, tester_suites) { try {
   TESTER_T t;
   t.produce_blocks(2);
   t.create_account( "testapi"_n );
   t.create_account( "testapi2"_n );
   t.produce_blocks(10);
   t.set_code( "testapi"_n, contracts::test_api_db_wasm() );
   t.set_abi(  "testapi"_n, contracts::test_api_db_abi().data() );
   t.set_code( "testapi2"_n, contracts::test_api_db_wasm() );
   t.set_abi(  "testapi2"_n, contracts::test_api_db_abi().data() );
   t.produce_blocks(1);

   t.push_action( "testapi"_n, "pg"_n,  "testapi"_n, mutable_variant_object() ); // primary_i64_general
   t.push_action( "testapi"_n, "pl"_n,  "testapi"_n, mutable_variant_object() ); // primary_i64_lowerbound
   t.push_action( "testapi"_n, "pu"_n,  "testapi"_n, mutable_variant_object() ); // primary_i64_upperbound
   t.push_action( "testapi"_n, "s1g"_n, "testapi"_n, mutable_variant_object() ); // idx64_general
   t.push_action( "testapi"_n, "s1l"_n, "testapi"_n, mutable_variant_object() ); // idx64_lowerbound
   t.push_action( "testapi"_n, "s1u"_n, "testapi"_n, mutable_variant_object() ); // idx64_upperbound

   // Store value in primary table
   t.push_action( "testapi"_n, "tia"_n, "testapi"_n, mutable_variant_object() // test_invalid_access
      ("code", "testapi")
      ("val", 10)
      ("index", 0)
      ("store", true)
   );

   // Attempt to change the value stored in the primary table under the code of "testapi"_n
   BOOST_CHECK_EXCEPTION( t.push_action( "testapi2"_n, "tia"_n, "testapi2"_n, mutable_variant_object()
                              ("code", "testapi")
                              ("val", "20")
                              ("index", 0)
                              ("store", true)
                           ), table_access_violation,
                           fc_exception_message_is("db access violation")
   );

   t.push_action( "testapi"_n, "tia"_n, "testapi"_n, mutable_variant_object()
      ("code", "testapi")
      ("val", 10)
      ("index", 0)
      ("store", false)
   );

   // Store value in secondary table
   t.push_action( "testapi"_n, "tia"_n, "testapi"_n, mutable_variant_object() // test_invalid_access
      ("code", "testapi")
      ("val", 10)
      ("index", 1)
      ("store", true)
   );

   // Attempt to change the value stored in the secondary table under the code of "testapi"_n
   BOOST_CHECK_EXCEPTION( t.push_action( "testapi2"_n, "tia"_n, "testapi2"_n, mutable_variant_object()
                              ("code", "testapi")
                              ("val", "20")
                              ("index", 1)
                              ("store", true)
                           ), table_access_violation,
                           fc_exception_message_is("db access violation")
   );

   // Verify that the value has not changed.
   t.push_action( "testapi"_n, "tia"_n, "testapi"_n, mutable_variant_object()
      ("code", "testapi")
      ("val", 10)
      ("index", 1)
      ("store", false)
   );

   // idx_double_nan_create_fail
   BOOST_CHECK_EXCEPTION(  t.push_action( "testapi"_n, "sdnancreate"_n, "testapi"_n, mutable_variant_object() ),
                           transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   // idx_double_nan_modify_fail
   BOOST_CHECK_EXCEPTION(  t.push_action( "testapi"_n, "sdnanmodify"_n, "testapi"_n, mutable_variant_object() ),
                           transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   // idx_double_nan_lookup_fail
   BOOST_CHECK_EXCEPTION(  t.push_action( "testapi"_n, "sdnanlookup"_n, "testapi"_n, mutable_variant_object()
                                        ("lookup_type", 0) // 0 for find
                           ), transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   BOOST_CHECK_EXCEPTION(  t.push_action( "testapi"_n, "sdnanlookup"_n, "testapi"_n, mutable_variant_object()
                              ("lookup_type", 1) // 1 for lower bound
                           ), transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   BOOST_CHECK_EXCEPTION(  t.push_action( "testapi"_n, "sdnanlookup"_n, "testapi"_n, mutable_variant_object()
                              ("lookup_type", 2) // 2 for upper bound
                           ), transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   t.push_action( "testapi"_n, "sk32align"_n, "testapi"_n, mutable_variant_object() ); // misaligned_secondary_key256_tests

   BOOST_REQUIRE_EQUAL( t.validate(), true );
} FC_LOG_AND_RETHROW() }

// The multi_index iterator cache is preserved across notifications for the same action.
BOOST_FIXTURE_TEST_CASE(db_notify_tests, TESTER) {
   create_accounts( { "notifier"_n, "notified"_n } );
   const char notifier[] = R"=====(
(module
 (func $db_store_i64 (import "env" "db_store_i64") (param i64 i64 i64 i64 i32 i32) (result i32))
 (func $db_find_i64 (import "env" "db_find_i64") (param i64 i64 i64 i64) (result i32))
 (func $db_idx64_store (import "env" "db_idx64_store") (param i64 i64 i64 i64 i32) (result i32))
 (func $db_idx64_find_primary (import "env" "db_idx64_find_primary") (param i64 i64 i64 i32 i64) (result i32))
 (func $db_idx128_store (import "env" "db_idx128_store") (param i64 i64 i64 i64 i32) (result i32))
 (func $db_idx128_find_primary (import "env" "db_idx128_find_primary") (param i64 i64 i64 i32 i64) (result i32))
 (func $db_idx256_store (import "env" "db_idx256_store") (param i64 i64 i64 i64 i32 i32) (result i32))
 (func $db_idx256_find_primary (import "env" "db_idx256_find_primary") (param i64 i64 i64 i32 i32 i64) (result i32))
 (func $db_idx_double_store (import "env" "db_idx_double_store") (param i64 i64 i64 i64 i32) (result i32))
 (func $db_idx_double_find_primary (import "env" "db_idx_double_find_primary") (param i64 i64 i64 i32 i64) (result i32))
 (func $db_idx_long_double_store (import "env" "db_idx_long_double_store") (param i64 i64 i64 i64 i32) (result i32))
 (func $db_idx_long_double_find_primary (import "env" "db_idx_long_double_find_primary") (param i64 i64 i64 i32 i64) (result i32))
 (func $eosio_assert (import "env" "eosio_assert") (param i32 i32))
 (func $require_recipient (import "env" "require_recipient") (param i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (local i32)
  (set_local 3 (i64.ne (get_local 0) (get_local 1)))
  (if (get_local 3) (then (i32.store8 (i32.const 7) (i32.const 100))))
  (drop (call $db_store_i64 (i64.const 0) (i64.const 0) (get_local 0) (i64.const 0) (i32.const 0) (i32.const 0)))
  (drop (call $db_idx64_store (i64.const 0) (i64.const 0) (get_local 0) (i64.const 0) (i32.const 256)))
  (drop (call $db_idx128_store (i64.const 0) (i64.const 0) (get_local 0) (i64.const 0) (i32.const 256)))
  (drop (call $db_idx256_store (i64.const 0) (i64.const 0) (get_local 0) (i64.const 0) (i32.const 256) (i32.const 2)))
  (drop (call $db_idx_double_store (i64.const 0) (i64.const 0) (get_local 0) (i64.const 0) (i32.const 256)))
  (drop (call $db_idx_long_double_store (i64.const 0) (i64.const 0) (get_local 0) (i64.const 0) (i32.const 256)))
  (call $eosio_assert (i32.eq (call $db_find_i64 (get_local 0) (i64.const 0) (i64.const 0) (i64.const 0) ) (get_local 3)) (i32.const 0))
  (call $eosio_assert (i32.eq (call $db_idx64_find_primary (get_local 0) (i64.const 0) (i64.const 0) (i32.const 256) (i64.const 0)) (get_local 3)) (i32.const 32))
  (call $eosio_assert (i32.eq (call $db_idx128_find_primary (get_local 0) (i64.const 0) (i64.const 0) (i32.const 256) (i64.const 0)) (get_local 3)) (i32.const 64))
  (call $eosio_assert (i32.eq (call $db_idx256_find_primary (get_local 0) (i64.const 0) (i64.const 0) (i32.const 256) (i32.const 2) (i64.const 0)) (get_local 3)) (i32.const 96))
  (call $eosio_assert (i32.eq (call $db_idx_double_find_primary (get_local 0) (i64.const 0) (i64.const 0) (i32.const 256) (i64.const 0)) (get_local 3)) (i32.const 128))
  (call $eosio_assert (i32.eq (call $db_idx_long_double_find_primary (get_local 0) (i64.const 0) (i64.const 0) (i32.const 256) (i64.const 0)) (get_local 3)) (i32.const 160))
  (call $require_recipient (i64.const 11327368596746665984))
 )
 (data (i32.const 0) "notifier: primary")
 (data (i32.const 32) "notifier: idx64")
 (data (i32.const 64) "notifier: idx128")
 (data (i32.const 96) "notifier: idx256")
 (data (i32.const 128) "notifier: idx_double")
 (data (i32.const 160) "notifier: idx_long_double")
)
)=====";
   set_code( "notifier"_n, notifier );
   set_code( "notified"_n, notifier );

   BOOST_TEST_REQUIRE(push_action( action({}, "notifier"_n, name(), {}), "notifier"_n.to_uint64_t() ) == "");
}

/*************************************************************************************
 * multi_index_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(multi_index_tests, TESTER) { try {
   produce_blocks(1);
   create_account( "testapi"_n );
   produce_blocks(1);
   set_code( "testapi"_n, contracts::test_api_multi_index_wasm() );
   set_abi( "testapi"_n, contracts::test_api_multi_index_abi().data() );
   produce_blocks(1);

   auto check_failure = [this]( action_name a, const char* expected_error_msg ) {
      BOOST_CHECK_EXCEPTION(  push_action( "testapi"_n, a, "testapi"_n, {} ),
                              eosio_assert_message_exception,
                              eosio_assert_message_is( expected_error_msg )
      );
   };

   push_action( "testapi"_n, "s1g"_n,  "testapi"_n, {} );        // idx64_general
   push_action( "testapi"_n, "s1store"_n,  "testapi"_n, {} );    // idx64_store_only
   push_action( "testapi"_n, "s1check"_n,  "testapi"_n, {} );    // idx64_check_without_storing
   push_action( "testapi"_n, "s2g"_n,  "testapi"_n, {} );        // idx128_general
   push_action( "testapi"_n, "s2store"_n,  "testapi"_n, {} );    // idx128_store_only
   push_action( "testapi"_n, "s2check"_n,  "testapi"_n, {} );    // idx128_check_without_storing
   push_action( "testapi"_n, "s2autoinc"_n,  "testapi"_n, {} );  // idx128_autoincrement_test
   push_action( "testapi"_n, "s2autoinc1"_n,  "testapi"_n, {} ); // idx128_autoincrement_test_part1
   push_action( "testapi"_n, "s2autoinc2"_n,  "testapi"_n, {} ); // idx128_autoincrement_test_part2
   push_action( "testapi"_n, "s3g"_n,  "testapi"_n, {} );        // idx256_general
   push_action( "testapi"_n, "sdg"_n,  "testapi"_n, {} );        // idx_double_general
   push_action( "testapi"_n, "sldg"_n,  "testapi"_n, {} );       // idx_long_double_general

   check_failure( "s1pkend"_n, "cannot increment end iterator" ); // idx64_pk_iterator_exceed_end
   check_failure( "s1skend"_n, "cannot increment end iterator" ); // idx64_sk_iterator_exceed_end
   check_failure( "s1pkbegin"_n, "cannot decrement iterator at beginning of table" ); // idx64_pk_iterator_exceed_begin
   check_failure( "s1skbegin"_n, "cannot decrement iterator at beginning of index" ); // idx64_sk_iterator_exceed_begin
   check_failure( "s1pkref"_n, "object passed to iterator_to is not in multi_index" ); // idx64_pass_pk_ref_to_other_table
   check_failure( "s1skref"_n, "object passed to iterator_to is not in multi_index" ); // idx64_pass_sk_ref_to_other_table
   check_failure( "s1pkitrto"_n, "object passed to iterator_to is not in multi_index" ); // idx64_pass_pk_end_itr_to_iterator_to
   check_failure( "s1pkmodify"_n, "cannot pass end iterator to modify" ); // idx64_pass_pk_end_itr_to_modify
   check_failure( "s1pkerase"_n, "cannot pass end iterator to erase" ); // idx64_pass_pk_end_itr_to_erase
   check_failure( "s1skitrto"_n, "object passed to iterator_to is not in multi_index" ); // idx64_pass_sk_end_itr_to_iterator_to
   check_failure( "s1skmodify"_n, "cannot pass end iterator to modify" ); // idx64_pass_sk_end_itr_to_modify
   check_failure( "s1skerase"_n, "cannot pass end iterator to erase" ); // idx64_pass_sk_end_itr_to_erase
   check_failure( "s1modpk"_n, "updater cannot change primary key when modifying an object" ); // idx64_modify_primary_key
   check_failure( "s1exhaustpk"_n, "next primary key in table is at autoincrement limit" ); // idx64_run_out_of_avl_pk
   check_failure( "s1findfail1"_n, "unable to find key" ); // idx64_require_find_fail
   check_failure( "s1findfail2"_n, "unable to find primary key in require_find" );// idx64_require_find_fail_with_msg
   check_failure( "s1findfail3"_n, "unable to find secondary key" ); // idx64_require_find_sk_fail
   check_failure( "s1findfail4"_n, "unable to find sec key" ); // idx64_require_find_sk_fail_with_msg

   push_action( "testapi"_n, "s1skcache"_n,  "testapi"_n, {} ); // idx64_sk_cache_pk_lookup
   push_action( "testapi"_n, "s1pkcache"_n,  "testapi"_n, {} ); // idx64_pk_cache_sk_lookup

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * crypto_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(crypto_tests, TESTER) { try {
   produce_block();
   create_account("testapi"_n );
   produce_block();
   set_code("testapi"_n, contracts::test_api_wasm() );
   produce_block();
   {
      signed_transaction trx;

      auto pl = vector<permission_level>{{"testapi"_n, config::active_name}};

      action act(pl, test_api_action<TEST_METHOD("test_crypto", "test_recover_key")>{});
      const auto priv_key = get_private_key("testapi"_n, "active" );
      const auto pub_key = priv_key.get_public_key();
      auto hash = trx.sig_digest( control->get_chain_id() );
      auto sig = priv_key.sign(hash);

      auto pk     = fc::raw::pack( pub_key );
      auto sigs   = fc::raw::pack( sig );
      vector<char> payload(8192);
      datastream<char*> payload_ds(payload.data(), payload.size());
      fc::raw::pack(payload_ds,  hash, (uint32_t)pk.size(), (uint32_t)sigs.size() );
      payload_ds.write(pk.data(), pk.size() );
      payload_ds.write(sigs.data(), sigs.size());
      payload.resize(payload_ds.tellp());

      //No Error Here
      CALL_TEST_FUNCTION( *this, "test_crypto", "test_recover_key", payload );
      // Error Here
      CALL_TEST_FUNCTION( *this, "test_crypto", "test_recover_key_assert_true", payload );
      payload[payload.size()-1] = 0;
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_crypto", "test_recover_key_assert_false", payload ),
                             crypto_api_exception, fc_exception_message_is("Error expected key different than recovered key") );
   }

   {
      signed_transaction trx;

      auto pl = vector<permission_level>{{"testapi"_n, config::active_name}};

      action act(pl, test_api_action<TEST_METHOD("test_crypto", "test_recover_key_partial")>{});

      // construct a mock WebAuthN pubkey and signature, as it is the only type that would be variable-sized
      const auto priv_key = get_private_key<mock::webauthn_private_key>("testapi"_n, "active" );
      const auto pub_key = priv_key.get_public_key();
      auto hash  = trx.sig_digest( control->get_chain_id() );
      auto sig = priv_key.sign(hash);

      auto pk     = fc::raw::pack( pub_key );
      auto sigs   = fc::raw::pack( sig );
      vector<char> payload(8192);
      datastream<char*> payload_ds(payload.data(), payload.size());
      fc::raw::pack(payload_ds,  hash, (uint32_t)pk.size(), (uint32_t)sigs.size() );
      payload_ds.write(pk.data(), pk.size() );
      payload_ds.write(sigs.data(), sigs.size());
      payload.resize(payload_ds.tellp());

      //No Error Here
      CALL_TEST_FUNCTION( *this, "test_crypto", "test_recover_key_partial", payload );
   }


   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha1", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha256", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha512", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_ripemd160", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha1_no_data", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha256_no_data", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha512_no_data", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "ripemd160_no_data", {} );

   CALL_TEST_FUNCTION_AND_CHECK_EXCEPTION( *this, "test_crypto", "assert_sha256_false", {},
                                           crypto_api_exception, "hash mismatch" );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha256_true", {} );

   CALL_TEST_FUNCTION_AND_CHECK_EXCEPTION( *this, "test_crypto", "assert_sha1_false", {},
                                           crypto_api_exception, "hash mismatch" );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha1_true", {} );

   CALL_TEST_FUNCTION_AND_CHECK_EXCEPTION( *this, "test_crypto", "assert_sha512_false", {},
                                           crypto_api_exception, "hash mismatch" );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha512_true", {} );

   CALL_TEST_FUNCTION_AND_CHECK_EXCEPTION( *this, "test_crypto", "assert_ripemd160_false", {},
                                           crypto_api_exception, "hash mismatch" );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_ripemd160_true", {} );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * memory_tests test case
 *************************************************************************************/
static const char memcpy_pass_wast[] = R"======(
(module
 (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
 (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (i64.store (i32.const 0) (i64.const 0x8877665544332211))
  (call $eosio_assert (i32.eq (call $memcpy (i32.const 65535) (i32.const 0) (i32.const 1)) (i32.const 65535)) (i32.const 128))
  (call $eosio_assert (i64.eq (i64.load (i32.const 65528)) (i64.const 0x1100000000000000)) (i32.const 256))
  (drop (call $memcpy (i32.const 8) (i32.const 7) (i32.const 1)))
  (drop (call $memcpy (i32.const 7) (i32.const 8) (i32.const 1)))
 )
 (data (i32.const 128) "expected memcpy to return 65535")
 (data (i32.const 256) "expected memcpy to write one byte")
)
)======";

static const char memcpy_overlap_wast[] = R"======(
(module
 (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $memcpy (i32.const 16) (i32.wrap/i64 (get_local 2)) (i32.const 8)))
 )
)
)======";

static const char memcpy_past_end_wast[] = R"======(
(module
 (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $memcpy (i32.const 65535) (i32.const 0) (i32.const 2)))
 )
)
)======";

static const char memmove_pass_wast[] = R"======(
(module
 (import "env" "memmove" (func $memmove (param i32 i32 i32) (result i32)))
 (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
 (memory 1)
 (func $fillmem (param i32 i32)
  (loop
   (i32.store8 (get_local 0) (get_local 1))
   (set_local 1 (i32.sub (get_local 1) (i32.const 1)))
   (set_local 0 (i32.add (get_local 0) (i32.const 1)))
   (br_if 0 (get_local 1))
  )
 )
 (func $checkmem (param i32 i32 i32)
   (loop
    (call $eosio_assert (i32.eq (i32.load8_u (get_local 0)) (get_local 1)) (get_local 2))
    (set_local 1 (i32.sub (get_local 1) (i32.const 1)))
    (set_local 0 (i32.add (get_local 0) (i32.const 1)))
    (br_if 0 (get_local 1))
   )
 )
 (func (export "apply") (param i64 i64 i64)
  (i64.store (i32.const 0) (i64.const 0x8877665544332211))
  (call $eosio_assert (i32.eq (call $memmove (i32.const 65535) (i32.const 0) (i32.const 1)) (i32.const 65535)) (i32.const 128))
  (call $eosio_assert (i64.eq (i64.load (i32.const 65528)) (i64.const 0x1100000000000000)) (i32.const 256))

  (call $fillmem (i32.const 8) (i32.const 128))
  (drop (call $memmove (i32.const 64) (i32.const 8) (i32.const 128)))
  (call $checkmem (i32.const 64) (i32.const 128) (i32.const 384))

  (call $fillmem (i32.const 8) (i32.const 128))
  (drop (call $memmove (i32.const 8) (i32.const 8) (i32.const 128)))
  (call $checkmem (i32.const 8) (i32.const 128) (i32.const 512))

  (call $fillmem (i32.const 64) (i32.const 128))
  (drop (call $memmove (i32.const 8) (i32.const 64) (i32.const 128)))
  (call $checkmem (i32.const 8) (i32.const 128) (i32.const 640))
 )
 (data (i32.const 128) "expected memmove to return 65535")
 (data (i32.const 256) "expected memmove to write one byte")
 (data (i32.const 384) "memmove overlap dest above src")
 (data (i32.const 512) "memmove overlap exact")
 (data (i32.const 640) "memmove overlap src above dst")
)
)======";

static const char memcmp_pass_wast[] = R"======(
(module
 (import "env" "memcmp" (func $memcmp (param i32 i32 i32) (result i32)))
 (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $eosio_assert (i32.eq (call $memcmp (i32.const 65535) (i32.const 65535) (i32.const 1)) (i32.const 0)) (i32.const 128))
  (call $eosio_assert (i32.eq (call $memcmp (i32.const 0) (i32.const 2) (i32.const 3)) (i32.const 0)) (i32.const 256))
  (call $eosio_assert (i32.eq (call $memcmp (i32.const 0) (i32.const 2) (i32.const 6)) (i32.const -1)) (i32.const 384))
  (call $eosio_assert (i32.eq (call $memcmp (i32.const 2) (i32.const 0) (i32.const 6)) (i32.const 1)) (i32.const 512))
 )
 (data (i32.const 0) "abababcdcdcd")
 (data (i32.const 128) "memcmp at end of memory")
 (data (i32.const 256) "memcmp overlap eq1")
 (data (i32.const 384) "memcmp overlap <")
 (data (i32.const 512) "memcmp overlap >")
)
)======";

static const char memset_pass_wast[] = R"======(
(module
 (import "env" "memset" (func $memset (param i32 i32 i32) (result i32)))
 (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $eosio_assert (i32.eq (call $memset (i32.const 65535) (i32.const 0xCC) (i32.const 1)) (i32.const 65535)) (i32.const 128))
  (call $eosio_assert (i64.eq (i64.load (i32.const 65528)) (i64.const 0xCC00000000000000)) (i32.const 256))
 )
 (data (i32.const 128) "expected memset to return 65535")
 (data (i32.const 256) "expected memset to write one byte")
)
)======";

BOOST_FIXTURE_TEST_CASE(memory_tests, TESTER) {
   produce_block();
   create_accounts( { "memcpy"_n, "memcpy2"_n, "memcpy3"_n, "memmove"_n, "memcmp"_n, "memset"_n } );
   set_code( "memcpy"_n, memcpy_pass_wast );
   set_code( "memcpy2"_n, memcpy_overlap_wast );
   set_code( "memcpy3"_n, memcpy_past_end_wast );
   set_code( "memmove"_n, memmove_pass_wast );
   set_code( "memcmp"_n, memcmp_pass_wast );
   set_code( "memset"_n, memset_pass_wast );
   auto pushit = [&](name acct, name act) {
      signed_transaction trx;
      trx.actions.push_back({ { {acct, config::active_name} }, acct, act, bytes()});
      set_transaction_headers(trx);
      trx.sign(get_private_key(acct, "active"), control->get_chain_id());
      push_transaction(trx);
   };
   pushit("memcpy"_n, name());
   pushit("memcpy2"_n, name(0));
   pushit("memcpy2"_n, name(8));
   BOOST_CHECK_THROW(pushit("memcpy2"_n, name(12)), overlapping_memory_error);
   BOOST_CHECK_THROW(pushit("memcpy2"_n, name(16)), overlapping_memory_error);
   BOOST_CHECK_THROW(pushit("memcpy2"_n, name(20)), overlapping_memory_error);
   BOOST_CHECK_THROW(pushit("memcpy3"_n, name()), wasm_execution_error);
   pushit("memcpy2"_n, name(24));

   pushit("memmove"_n, name());
   pushit("memcmp"_n, name());
   pushit("memset"_n, name());
}

/*************************************************************************************
 * print_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(print_tests, TESTER) { try {
	produce_blocks(2);
	create_account("testapi"_n );
	produce_blocks(1000);

	set_code("testapi"_n, contracts::test_api_wasm() );
	produce_blocks(1000);
	string captured = "";

	// test prints
   auto tx1_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_prints", {} );
   auto tx1_act_cnsl = tx1_trace->action_traces.front().console;
   BOOST_CHECK_EQUAL(tx1_act_cnsl == "abcefg", true);

   // test prints_l
   auto tx2_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_prints_l", {} );
   auto tx2_act_cnsl = tx2_trace->action_traces.front().console;
   BOOST_CHECK_EQUAL(tx2_act_cnsl == "abatest", true);


   // test printi
   auto tx3_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printi", {} );
   auto tx3_act_cnsl = tx3_trace->action_traces.front().console;
   BOOST_CHECK_EQUAL( tx3_act_cnsl.substr(0,1), I64Str(0) );
   BOOST_CHECK_EQUAL( tx3_act_cnsl.substr(1,6), I64Str(556644) );
   BOOST_CHECK_EQUAL( tx3_act_cnsl.substr(7, std::string::npos), I64Str(-1) );

   // test printui
   auto tx4_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printui", {} );
   auto tx4_act_cnsl = tx4_trace->action_traces.front().console;
   BOOST_CHECK_EQUAL( tx4_act_cnsl.substr(0,1), U64Str(0) );
   BOOST_CHECK_EQUAL( tx4_act_cnsl.substr(1,6), U64Str(556644) );
   BOOST_CHECK_EQUAL( tx4_act_cnsl.substr(7, std::string::npos), U64Str(-1) ); // "18446744073709551615"

   // test printn
   auto tx5_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printn", {} );
   auto tx5_act_cnsl = tx5_trace->action_traces.front().console;

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(0,1), "1" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(1,1), "5" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(2,1), "a" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(3,1), "z" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(4,3), "abc" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(7,3), "123" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(10,7), "abc.123" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(17,7), "123.abc" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(24,13), "12345abcdefgj" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(37,13), "ijklmnopqrstj" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(50,13), "vwxyz.12345aj" );

   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(63, 13), "111111111111j" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(76, 13), "555555555555j" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(89, 13), "aaaaaaaaaaaaj" );
   BOOST_CHECK_EQUAL( tx5_act_cnsl.substr(102,13), "zzzzzzzzzzzzj" );

   // test printi128
   auto tx6_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printi128", {} );
   auto tx6_act_cnsl = tx6_trace->action_traces.front().console;
   size_t start = 0;
   size_t end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), U128Str(1) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), U128Str(0) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), "-" + U128Str(static_cast<unsigned __int128>(std::numeric_limits<__int128>::lowest())) );
   start = end + 1; end = tx6_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx6_act_cnsl.substr(start, end-start), "-" + U128Str(87654323456) );

   // test printui128
   auto tx7_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printui128", {} );
   auto tx7_act_cnsl = tx7_trace->action_traces.front().console;
   start = 0; end = tx7_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx7_act_cnsl.substr(start, end-start), U128Str(std::numeric_limits<unsigned __int128>::max()) );
   start = end + 1; end = tx7_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx7_act_cnsl.substr(start, end-start), U128Str(0) );
   start = end + 1; end = tx7_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx7_act_cnsl.substr(start, end-start), U128Str(87654323456) );

   // test printsf
   auto tx8_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printsf", {} );
   auto tx8_act_cnsl = tx8_trace->action_traces.front().console;
   start = 0; end = tx8_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx8_act_cnsl.substr(start, end-start), "5.000000e-01" );
   start = end + 1; end = tx8_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx8_act_cnsl.substr(start, end-start), "-3.750000e+00" );
   start = end + 1; end = tx8_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx8_act_cnsl.substr(start, end-start), "6.666667e-07" );

   // test printdf
   auto tx9_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printdf", {} );
   auto tx9_act_cnsl = tx9_trace->action_traces.front().console;
   start = 0; end = tx9_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx9_act_cnsl.substr(start, end-start), "5.000000000000000e-01" );
   start = end + 1; end = tx9_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx9_act_cnsl.substr(start, end-start), "-3.750000000000000e+00" );
   start = end + 1; end = tx9_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx9_act_cnsl.substr(start, end-start), "6.666666666666666e-07" );

   // test printqf
#ifdef __x86_64__
   std::string expect1 = "5.000000000000000000e-01";
   std::string expect2 = "-3.750000000000000000e+00";
   std::string expect3 = "6.666666666666666667e-07";
#else
   std::string expect1 = "5.000000000000000e-01";
   std::string expect2 = "-3.750000000000000e+00";
   std::string expect3 = "6.666666666666667e-07";
#endif
   auto tx10_trace = CALL_TEST_FUNCTION( *this, "test_print", "test_printqf", {} );
   auto tx10_act_cnsl = tx10_trace->action_traces.front().console;
   start = 0; end = tx10_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx10_act_cnsl.substr(start, end-start), expect1 );
   start = end + 1; end = tx10_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx10_act_cnsl.substr(start, end-start), expect2 );
   start = end + 1; end = tx10_act_cnsl.find('\n', start);
   BOOST_CHECK_EQUAL( tx10_act_cnsl.substr(start, end-start), expect3 );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * types_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(types_tests, TESTER) { try {
	produce_blocks(1000);
	create_account( "testapi"_n );

	produce_blocks(1000);
	set_code( "testapi"_n, contracts::test_api_wasm() );
	produce_blocks(1000);

	CALL_TEST_FUNCTION( *this, "test_types", "types_size", {});
	CALL_TEST_FUNCTION( *this, "test_types", "char_to_symbol", {});
	CALL_TEST_FUNCTION( *this, "test_types", "string_to_name", {});

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * permission_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(permission_tests, TESTER) { try {
   produce_blocks(1);
   create_account( "testapi"_n );

   produce_blocks(1);
   set_code( "testapi"_n, contracts::test_api_wasm() );
   produce_blocks(1);

   auto get_result_int64 = [&]() -> int64_t {
      const auto& db = control->db();
      const auto* t_id = db.find<table_id_object, by_code_scope_table>(boost::make_tuple("testapi"_n, "testapi"_n, "testapi"_n));

      FC_ASSERT(t_id != 0, "Table id not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      FC_ASSERT( 0 != itr->value.size(), "unexpected result size");
      return *reinterpret_cast<const int64_t *>(itr->value.data());
   };

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = "testapi"_n,
         .permission = "active"_n,
         .pubkeys    = {
            get_public_key("testapi"_n, "active")
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(1), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = "testapi"_n,
         .permission = "active"_n,
         .pubkeys    = {
            public_key_type(string("EOS7GfRtyDWWgxV88a5TRaYY59XmHptyfjsFmHHfioGNJtPjpSmGX"))
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = "testapi"_n,
         .permission = "active"_n,
         .pubkeys    = {
            get_public_key("testapi"_n, "active"),
            public_key_type(string("EOS7GfRtyDWWgxV88a5TRaYY59XmHptyfjsFmHHfioGNJtPjpSmGX"))
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() ); // Failure due to irrelevant signatures

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = "noname"_n,
         .permission = "active"_n,
         .pubkeys    = {
            get_public_key("testapi"_n, "active")
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = "testapi"_n,
         .permission = "active"_n,
         .pubkeys    = {}
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = "testapi"_n,
         .permission = "noname"_n,
         .pubkeys    = {
            get_public_key("testapi"_n, "active")
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

} FC_LOG_AND_RETHROW() }

static const char resource_limits_wast[] = R"=====(
(module
 (func $set_resource_limits (import "env" "set_resource_limits") (param i64 i64 i64 i64))
 (func $get_resource_limits (import "env" "get_resource_limits") (param i64 i32 i32 i32))
 (func $eosio_assert (import "env" "eosio_assert") (param i32 i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $set_resource_limits (get_local 2) (i64.const 2788) (i64.const 11) (i64.const 12))
  (call $get_resource_limits (get_local 2) (i32.const 0x100) (i32.const 0x108) (i32.const 0x110))
  (call $eosio_assert (i64.eq (i64.const 2788) (i64.load (i32.const 0x100))) (i32.const 8))
  (call $eosio_assert (i64.eq (i64.const 11) (i64.load (i32.const 0x108))) (i32.const 32))
  (call $eosio_assert (i64.eq (i64.const 12) (i64.load (i32.const 0x110))) (i32.const 64))
  ;; Aligned overlap
  (call $get_resource_limits (get_local 2) (i32.const 0x100) (i32.const 0x100) (i32.const 0x110))
  (call $eosio_assert (i64.eq (i64.const 11) (i64.load (i32.const 0x100))) (i32.const 96))
  (call $get_resource_limits (get_local 2) (i32.const 0x100) (i32.const 0x110) (i32.const 0x110))
  (call $eosio_assert (i64.eq (i64.const 12) (i64.load (i32.const 0x110))) (i32.const 128))
  ;; Unaligned beats aligned
  (call $get_resource_limits (get_local 2) (i32.const 0x101) (i32.const 0x108) (i32.const 0x100))
  (call $eosio_assert (i64.eq (i64.const 2788) (i64.load (i32.const 0x101))) (i32.const 160))
  ;; Unaligned overlap
  (call $get_resource_limits (get_local 2) (i32.const 0x101) (i32.const 0x101) (i32.const 0x110))
  (call $eosio_assert (i64.eq (i64.const 11) (i64.load (i32.const 0x101))) (i32.const 192))
  (call $get_resource_limits (get_local 2) (i32.const 0x100) (i32.const 0x111) (i32.const 0x111))
  (call $eosio_assert (i64.eq (i64.const 12) (i64.load (i32.const 0x111))) (i32.const 224))
 )
 (data (i32.const 8) "expected ram 2788")
 (data (i32.const 32) "expected net 11")
 (data (i32.const 64) "expected cpu 12")
 (data (i32.const 96) "expected net to overwrite ram")
 (data (i32.const 128) "expected cpu to overwrite net")
 (data (i32.const 160) "expected unaligned")
 (data (i32.const 192) "expected unet to overwrite uram")
 (data (i32.const 224) "expected ucpu to overwrite unet")
)
)=====";

static const char get_resource_limits_null_ram_wast[] = R"=====(
(module
 (func $get_resource_limits (import "env" "get_resource_limits") (param i64 i32 i32 i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $get_resource_limits (get_local 2) (i32.const 0) (i32.const 0x10) (i32.const 0x10))
 )
)
)=====";

static const char get_resource_limits_null_net_wast[] = R"=====(
(module
 (func $get_resource_limits (import "env" "get_resource_limits") (param i64 i32 i32 i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $get_resource_limits (get_local 2) (i32.const 0x10) (i32.const 0) (i32.const 0x10))
 )
)
)=====";

static const char get_resource_limits_null_cpu_wast[] = R"=====(
(module
 (func $get_resource_limits (import "env" "get_resource_limits") (param i64 i32 i32 i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $get_resource_limits (get_local 2) (i32.const 0x10) (i32.const 0x10) (i32.const 0))
 )
)
)=====";

BOOST_FIXTURE_TEST_CASE(resource_limits_tests, TESTER) {
   create_accounts( { "rlimits"_n, "testacnt"_n } );
   set_code("rlimits"_n, resource_limits_wast);
   push_action( "eosio"_n, "setpriv"_n, "eosio"_n, mutable_variant_object()("account", "rlimits"_n)("is_priv", 1));
   produce_block();

   auto pushit = [&]{
      signed_transaction trx;
      trx.actions.push_back({ { { "rlimits"_n, config::active_name } }, "rlimits"_n, "testacnt"_n, bytes{}});
      set_transaction_headers(trx);
      trx.sign(get_private_key( "rlimits"_n, "active" ), control->get_chain_id());
      push_transaction(trx);
   };
   pushit();
   produce_block();

   set_code("rlimits"_n, get_resource_limits_null_ram_wast);
   BOOST_CHECK_THROW(pushit(), wasm_exception);

   set_code("rlimits"_n, get_resource_limits_null_net_wast);
   BOOST_CHECK_THROW(pushit(), wasm_exception);

   set_code("rlimits"_n, get_resource_limits_null_cpu_wast);
   BOOST_CHECK_THROW(pushit(), wasm_exception);
}

static const char is_privileged_wast[] = R"======(
(module
  (import "env" "is_privileged" (func $is_privileged (param i64) (result i32)))
  (func (export "apply") (param i64 i64 i64)
    (drop (call $is_privileged (get_local 2)))
  )
)
)======";

BOOST_FIXTURE_TEST_CASE(is_privileged, TESTER) {
   create_accounts( {"priv"_n, "unpriv"_n, "a"_n} );
   push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  mutable_variant_object()
               ("account", "priv")
               ("is_priv", 1));
   set_code( "priv"_n, is_privileged_wast );
   set_code( "unpriv"_n, is_privileged_wast );
   auto pushit = [&](name account, name action) {
      signed_transaction trx;
      trx.actions.push_back({ { { account, config::active_name } }, account, action, bytes() });
      set_transaction_headers(trx);
      trx.sign(get_private_key( account, "active" ), control->get_chain_id());
      push_transaction(trx);
   };
   pushit("priv"_n, "a"_n);
   BOOST_CHECK_EXCEPTION(pushit("unpriv"_n, "a"_n), chain::unaccessible_api,
                         fc_exception_message_is("unpriv does not have permission to call this API"));
   BOOST_CHECK_THROW(pushit("priv"_n, "bcd"_n), fc::exception);
}

static const char set_privileged1_wast[] = R"======(
(module
  (import "env" "set_privileged" (func $set_privileged (param i64 i32)))
  (func (export "apply") (param i64 i64 i64)
    (call $set_privileged (get_local 2) (i32.const 1))
  )
)
)======";

BOOST_FIXTURE_TEST_CASE(set_privileged1, TESTER) {
   create_accounts( {"priv"_n, "unpriv"_n, "a"_n} );
   push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  mutable_variant_object()
               ("account", "priv")
               ("is_priv", 1));
   set_code( "priv"_n, set_privileged1_wast );
   set_code( "unpriv"_n, set_privileged1_wast );
   auto pushit = [&](name account, name action) {
      signed_transaction trx;
      trx.actions.push_back({ { { account, config::active_name } }, account, action, bytes() });
      set_transaction_headers(trx);
      trx.sign(get_private_key( account, "active" ), control->get_chain_id());
      push_transaction(trx);
   };
   pushit("priv"_n, "a"_n);
   BOOST_CHECK_EXCEPTION(pushit("unpriv"_n, "a"_n), chain::unaccessible_api,
                         fc_exception_message_is("unpriv does not have permission to call this API"));
   BOOST_CHECK_THROW(pushit("priv"_n, "bcd"_n), fc::exception);
}

// If an account marks itself as unprivileged, it takes effect at the end of the action.
// However, is_privileged reports the change in status immediately.
static const char set_privileged2_wast[] = R"======(
(module
  (import "env" "set_privileged" (func $set_privileged (param i64 i32)))
  (import "env" "is_privileged" (func $is_privileged (param i64) (result i32)))
  (import "env" "eosio_assert" (func $eosio_assert (param i32 i32)))
  (memory 1)
  (func (export "apply") (param i64 i64 i64)
    (call $set_privileged (get_local 0) (i32.const 0))
    (call $eosio_assert (i32.eqz (call $is_privileged (get_local 0))) (i32.const 0))
  )
  (data (i32.const 0) "is_privileged should return false")
)
)======";

BOOST_FIXTURE_TEST_CASE(set_privileged2, TESTER) {
   create_accounts( {"priv"_n} );
   push_action(config::system_account_name, "setpriv"_n, config::system_account_name,  mutable_variant_object()
               ("account", "priv")
               ("is_priv", 1));
   set_code( "priv"_n, set_privileged2_wast );
   auto pushit = [&](name account, name action) {
      signed_transaction trx;
      trx.actions.push_back({ { { account, config::active_name } }, account, action, bytes() });
      set_transaction_headers(trx);
      trx.sign(get_private_key( account, "active" ), control->get_chain_id());
      push_transaction(trx);
   };
   pushit("priv"_n, ""_n);
}

#if 0
/*************************************************************************************
 * privileged_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(privileged_tests, tester) { try {
	produce_blocks(2);
	create_account( "testapi"_n );
	create_account( "acc1"_n );
	produce_blocks(100);
	set_code( "testapi"_n, contracts::test_api_wasm() );
	produce_blocks(1);

   {
		signed_transaction trx;

      auto pl = vector<permission_level>{{config::system_account_name, config::active_name}};
      action act(pl, test_chain_action<"setprods"_n>());
      vector<producer_key> prod_keys = {
                                          { "inita"_n, get_public_key( "inita"_n, "active" ) },
                                          { "initb"_n, get_public_key( "initb"_n, "active" ) },
                                          { "initc"_n, get_public_key( "initc"_n, "active" ) },
                                          { "initd"_n, get_public_key( "initd"_n, "active" ) },
                                          { "inite"_n, get_public_key( "inite"_n, "active" ) },
                                          { "initf"_n, get_public_key( "initf"_n, "active" ) },
                                          { "initg"_n, get_public_key( "initg"_n, "active" ) },
                                          { "inith"_n, get_public_key( "inith"_n, "active" ) },
                                          { "initi"_n, get_public_key( "initi"_n, "active" ) },
                                          { "initj"_n, get_public_key( "initj"_n, "active" ) },
                                          { "initk"_n, get_public_key( "initk"_n, "active" ) },
                                          { "initl"_n, get_public_key( "initl"_n, "active" ) },
                                          { "initm"_n, get_public_key( "initm"_n, "active" ) },
                                          { "initn"_n, get_public_key( "initn"_n, "active" ) },
                                          { "inito"_n, get_public_key( "inito"_n, "active" ) },
                                          { "initp"_n, get_public_key( "initp"_n, "active" ) },
                                          { "initq"_n, get_public_key( "initq"_n, "active" ) },
                                          { "initr"_n, get_public_key( "initr"_n, "active" ) },
                                          { "inits"_n, get_public_key( "inits"_n, "active" ) },
                                          { "initt"_n, get_public_key( "initt"_n, "active" ) },
                                          { "initu"_n, get_public_key( "initu"_n, "active" ) }
                                       };
      vector<char> data = fc::raw::pack(uint32_t(0));
      vector<char> keys = fc::raw::pack(prod_keys);
      data.insert( data.end(), keys.begin(), keys.end() );
      act.data = data;
      trx.actions.push_back(act);

		set_tapos(trx);

		auto sigs = trx.sign(get_private_key(config::system_account_name, "active"), control->get_chain_id());
      trx.get_signature_keys(control->get_chain_id() );
		auto res = push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
	}

   CALL_TEST_FUNCTION( *this, "test_privileged", "test_is_privileged", {} );
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_privileged", "test_is_privileged", {} ), transaction_exception,
         [](const fc::exception& e) {
            return expect_assert_message(e, "context.privileged: testapi does not have permission to call this API");
         }
       );

} FC_LOG_AND_RETHROW() }
#endif

/*************************************************************************************
 * real_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(datastream_tests, TESTER) { try {
   produce_blocks(1000);
   create_account("testapi"_n );
   produce_blocks(1000);
   set_code("testapi"_n, contracts::test_api_wasm() );
   produce_blocks(1000);

   CALL_TEST_FUNCTION( *this, "test_datastream", "test_basic", {} );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * permission_usage_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(permission_usage_tests, TESTER) { try {
   produce_block();
   create_accounts( {"testapi"_n, "alice"_n, "bob"_n} );
   produce_block();
   set_code("testapi"_n, contracts::test_api_wasm() );
   produce_block();

   push_reqauth( "alice"_n, {{"alice"_n, config::active_name}}, {get_private_key("alice"_n, "active")} );

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                           "alice"_n, config::active_name,
                           control->pending_block_time()
                       })
   );

   // Fails because the last used time is updated after the transaction executes.
   BOOST_CHECK_THROW( CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       "testapi"_n, config::active_name,
                                       control->head_block_time() + fc::milliseconds(config::block_interval_ms)
                                     })
   ), eosio_assert_message_exception );

   produce_blocks(5);

   set_authority( "bob"_n, "perm1"_n, authority( get_private_key("bob"_n, "perm1").get_public_key() ) );

   push_action(config::system_account_name, linkauth::get_name(), "bob"_n, fc::mutable_variant_object()
           ("account", "bob")
           ("code", "eosio")
           ("type", "reqauth")
           ("requirement", "perm1")
   );

   auto permission_creation_time = control->pending_block_time();

   produce_blocks(5);

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       "bob"_n, "perm1"_n,
                                       permission_creation_time
                                     })
   );

   produce_blocks(5);

   push_reqauth( "bob"_n, {{"bob"_n, "perm1"_n}}, {get_private_key("bob"_n, "perm1")} );

   auto perm1_last_used_time = control->pending_block_time();

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       "bob"_n, config::active_name,
                                       permission_creation_time
                                     })
   );

   BOOST_CHECK_THROW( CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                                          fc::raw::pack(test_permission_last_used_action{
                                                            "bob"_n, "perm1"_n,
                                                            permission_creation_time
                                          })
   ), eosio_assert_message_exception );

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       "bob"_n, "perm1"_n,
                                       perm1_last_used_time
                                     })
   );

   produce_block();

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * account_creation_time_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(account_creation_time_tests, TESTER) { try {
   produce_block();
   create_account( "testapi"_n );
   produce_block();
   set_code("testapi"_n, contracts::test_api_wasm() );
   produce_block();

   create_account( "alice"_n );
   auto alice_creation_time = control->pending_block_time();

   produce_blocks(10);

   CALL_TEST_FUNCTION( *this, "test_permission", "test_account_creation_time",
                       fc::raw::pack(test_permission_last_used_action{
                           "alice"_n, config::active_name,
                           alice_creation_time
                       })
   );

   produce_block();

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * extended_symbol_api_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(extended_symbol_api_tests, TESTER) { try {
   name n0{"1"};
   name n1{"5"};
   name n2{"a"};
   name n3{"z"};
   name n4{"111111111111j"};
   name n5{"555555555555j"};
   name n6{"zzzzzzzzzzzzj"};

   symbol s0{4, ""};
   symbol s1{5, "Z"};
   symbol s2{10, "AAAAA"};
   symbol s3{10, "ZZZZZ"};

   // Test comparison operators

   BOOST_REQUIRE( (extended_symbol{s0, n0} == extended_symbol{s0, n0}) );
   BOOST_REQUIRE( (extended_symbol{s1, n3} == extended_symbol{s1, n3}) );
   BOOST_REQUIRE( (extended_symbol{s2, n4} == extended_symbol{s2, n4}) );
   BOOST_REQUIRE( (extended_symbol{s3, n6} == extended_symbol{s3, n6}) );

   BOOST_REQUIRE( (extended_symbol{s0, n0} != extended_symbol{s1, n0}) );
   BOOST_REQUIRE( (extended_symbol{s0, n0} != extended_symbol{s0, n1}) );
   BOOST_REQUIRE( (extended_symbol{s1, n1} != extended_symbol{s2, n2}) );

   BOOST_REQUIRE( (extended_symbol{s0, n0} < extended_symbol{s1, n0}) );
   BOOST_REQUIRE( (extended_symbol{s0, n0} < extended_symbol{s0, n1}) );
   BOOST_REQUIRE( (extended_symbol{s0, n5} < extended_symbol{s0, n3}) );
   BOOST_REQUIRE( (extended_symbol{s2, n0} < extended_symbol{s3, n0}) );

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * eosio_assert_code_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(eosio_assert_code_tests, TESTER) { try {
   produce_block();
   create_account( "testapi"_n );
   produce_block();
   set_code("testapi"_n, contracts::test_api_wasm() );

   const char* abi_string = R"=====(
{
   "version": "eosio::abi/1.0",
   "types": [],
   "structs": [],
   "actions": [],
   "tables": [],
   "ricardian_clauses": [],
   "error_messages": [
      {"error_code": 1, "error_msg": "standard error message" },
      {"error_code": 42, "error_msg": "The answer to life, the universe, and everything."}
   ]
   "abi_extensions": []
}
)=====";

   set_abi( "testapi"_n, abi_string );

   auto var = fc::json::from_string(abi_string);
   abi_serializer abis(var.as<abi_def>(), abi_serializer::create_yield_function( abi_serializer_max_time ));

   produce_blocks(10);

   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_action", "test_assert_code", fc::raw::pack((uint64_t)42) ),
                          eosio_assert_code_exception, eosio_assert_code_is(42)                                        );


   auto trace = CALL_TEST_FUNCTION_NO_THROW( *this, "test_action", "test_assert_code", fc::raw::pack((uint64_t)42) );
   BOOST_REQUIRE( trace );
   BOOST_REQUIRE( trace->except );
   BOOST_REQUIRE( trace->error_code );
   BOOST_REQUIRE_EQUAL( *trace->error_code, 42 );
   BOOST_REQUIRE_EQUAL( trace->action_traces.size(), 1 );
   BOOST_REQUIRE( trace->action_traces[0].except );
   BOOST_REQUIRE( trace->action_traces[0].error_code );
   BOOST_REQUIRE_EQUAL( *trace->action_traces[0].error_code, 42 );

   produce_block();

   auto omsg1 = abis.get_error_message(1);
   BOOST_REQUIRE_EQUAL( omsg1.has_value(), true );
   BOOST_CHECK_EQUAL( *omsg1, "standard error message" );

   auto omsg2 = abis.get_error_message(2);
   BOOST_CHECK_EQUAL( omsg2.has_value(), false );

   auto omsg3 = abis.get_error_message(42);
   BOOST_REQUIRE_EQUAL( omsg3.has_value(), true );
   BOOST_CHECK_EQUAL( *omsg3, "The answer to life, the universe, and everything." );

   produce_block();

   auto trace2 = CALL_TEST_FUNCTION_NO_THROW(
                  *this, "test_action", "test_assert_code",
                  fc::raw::pack( static_cast<uint64_t>(system_error_code::generic_system_error) )
   );
   BOOST_REQUIRE( trace2 );
   BOOST_REQUIRE( trace2->except );
   BOOST_REQUIRE( trace2->error_code );
   BOOST_REQUIRE_EQUAL( *trace2->error_code, static_cast<uint64_t>(system_error_code::contract_restricted_error_code) );
   BOOST_REQUIRE_EQUAL( trace2->action_traces.size(), 1 );
   BOOST_REQUIRE( trace2->action_traces[0].except );
   BOOST_REQUIRE( trace2->action_traces[0].error_code );
   BOOST_REQUIRE_EQUAL( *trace2->action_traces[0].error_code, static_cast<uint64_t>(system_error_code::contract_restricted_error_code) );

   produce_block();

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
+ * action_ordinal_test test cases
+ *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_ordinal_test, TESTER) { try {

   produce_blocks(1);
   create_account("testapi"_n );
   set_code( "testapi"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("bob"_n );
   set_code( "bob"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("charlie"_n );
   set_code( "charlie"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("david"_n );
   set_code( "david"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("erin"_n );
   set_code( "erin"_n, contracts::test_api_wasm() );
   produce_blocks(1);

   // prove act digest
   auto pad = [](const digest_type& expected_act_digest, const action& act, const vector<char>& act_output)
   {
      std::vector<char> buf1;
      buf1.resize(64);
      datastream<char*> ds(buf1.data(), buf1.size());

      {
         std::vector<char> buf2;
         const action_base* act_base = &act;
         buf2.resize(fc::raw::pack_size(*act_base));
         datastream<char*> ds2(buf2.data(), buf2.size());
         fc::raw::pack(ds2, *act_base);
         fc::raw::pack(ds, sha256::hash(buf2.data(), buf2.size()));
      }

      {
         std::vector<char> buf2;
         buf2.resize(fc::raw::pack_size(act.data) + fc::raw::pack_size(act_output));
         datastream<char*> ds2(buf2.data(), buf2.size());
         fc::raw::pack(ds2, act.data);
         fc::raw::pack(ds2, act_output);
         fc::raw::pack(ds, sha256::hash(buf2.data(), buf2.size()));
      }

      digest_type computed_act_digest = sha256::hash(buf1.data(), ds.tellp());

      return expected_act_digest == computed_act_digest;
   };

   transaction_trace_ptr txn_trace = CALL_TEST_FUNCTION_SCOPE( *this, "test_action", "test_action_ordinal1",
      {}, vector<account_name>{ "testapi"_n});

   BOOST_REQUIRE_EQUAL( validate(), true );

   BOOST_REQUIRE_EQUAL( txn_trace != nullptr, true);
   BOOST_REQUIRE_EQUAL( txn_trace->action_traces.size(), 11);

   auto &atrace = txn_trace->action_traces;
   BOOST_REQUIRE_EQUAL((int)atrace[0].action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[0].creator_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL((int)atrace[0].closest_unnotified_ancestor_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(atrace[0].return_value), unsigned_int(1) );
   BOOST_REQUIRE(pad(atrace[0].receipt->act_digest, atrace[0].act, atrace[0].return_value));
   int start_gseq = atrace[0].receipt->global_sequence;

   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(atrace[1].return_value), "bob" );
   BOOST_REQUIRE(pad(atrace[1].receipt->act_digest, atrace[1].act, atrace[1].return_value));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->global_sequence, start_gseq + 1);

   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<name>(atrace[2].return_value), name("five") );
   BOOST_REQUIRE(pad(atrace[2].receipt->act_digest, atrace[2].act, atrace[2].return_value));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt->global_sequence, start_gseq + 4);

   BOOST_REQUIRE_EQUAL((int)atrace[3].action_ordinal, 4);
   BOOST_REQUIRE_EQUAL((int)atrace[3].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[3].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[3].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[3].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[3].act.name, TEST_METHOD("test_action", "test_action_ordinal3"));
   BOOST_REQUIRE_EQUAL(atrace[3].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(atrace[3].return_value), unsigned_int(9) );
   BOOST_REQUIRE(pad(atrace[3].receipt->act_digest, atrace[3].act, atrace[3].return_value));
   BOOST_REQUIRE_EQUAL(atrace[3].receipt->global_sequence, start_gseq + 8);

   BOOST_REQUIRE_EQUAL((int)atrace[4].action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[4].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[4].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[4].receiver, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[4].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[4].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[4].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(atrace[4].return_value), "charlie" );
   BOOST_REQUIRE(pad(atrace[4].receipt->act_digest, atrace[4].act, atrace[4].return_value));
   BOOST_REQUIRE_EQUAL(atrace[4].receipt->global_sequence, start_gseq + 2);

   BOOST_REQUIRE_EQUAL((int)atrace[5].action_ordinal, 6);
   BOOST_REQUIRE_EQUAL((int)atrace[5].creator_action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[5].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[5].receiver, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[5].act.account, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[5].act.name, TEST_METHOD("test_action", "test_action_ordinal_foo"));
   BOOST_REQUIRE_EQUAL(atrace[5].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<double>(atrace[5].return_value), 13.23 );
   BOOST_REQUIRE(pad(atrace[5].receipt->act_digest, atrace[5].act, atrace[5].return_value));
   BOOST_REQUIRE_EQUAL(atrace[5].receipt->global_sequence, start_gseq + 9);

   BOOST_REQUIRE_EQUAL((int)atrace[6].action_ordinal, 7);
   BOOST_REQUIRE_EQUAL((int)atrace[6].creator_action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[6].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[6].receiver, "david"_n);
   BOOST_REQUIRE_EQUAL(atrace[6].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[6].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[6].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(atrace[6].return_value), "david" );
   BOOST_REQUIRE(pad(atrace[6].receipt->act_digest, atrace[6].act, atrace[6].return_value));
   BOOST_REQUIRE_EQUAL(atrace[6].receipt->global_sequence, start_gseq + 3);

   BOOST_REQUIRE_EQUAL((int)atrace[7].action_ordinal, 8);
   BOOST_REQUIRE_EQUAL((int)atrace[7].creator_action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[7].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[7].receiver, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[7].act.account, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[7].act.name, TEST_METHOD("test_action", "test_action_ordinal_bar"));
   BOOST_REQUIRE_EQUAL(atrace[7].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<float>(atrace[7].return_value), 11.42f );
   BOOST_REQUIRE(pad(atrace[7].receipt->act_digest, atrace[7].act, atrace[7].return_value));
   BOOST_REQUIRE_EQUAL(atrace[7].receipt->global_sequence, start_gseq + 10);

   BOOST_REQUIRE_EQUAL((int)atrace[8].action_ordinal, 9);
   BOOST_REQUIRE_EQUAL((int)atrace[8].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[8].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[8].receiver, "david"_n);
   BOOST_REQUIRE_EQUAL(atrace[8].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[8].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[8].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<bool>(atrace[8].return_value), true );
   BOOST_REQUIRE(pad(atrace[8].receipt->act_digest, atrace[8].act, atrace[8].return_value));
   BOOST_REQUIRE_EQUAL(atrace[8].receipt->global_sequence, start_gseq + 5);

   BOOST_REQUIRE_EQUAL((int)atrace[9].action_ordinal, 10);
   BOOST_REQUIRE_EQUAL((int)atrace[9].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[9].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[9].receiver, "erin"_n);
   BOOST_REQUIRE_EQUAL(atrace[9].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[9].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[9].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<signed_int>(atrace[9].return_value), signed_int(7) );
   BOOST_REQUIRE(pad(atrace[9].receipt->act_digest, atrace[9].act, atrace[9].return_value));
   BOOST_REQUIRE_EQUAL(atrace[9].receipt->global_sequence, start_gseq + 6);

   BOOST_REQUIRE_EQUAL((int)atrace[10].action_ordinal, 11);
   BOOST_REQUIRE_EQUAL((int)atrace[10].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[10].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[10].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[10].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[10].act.name, TEST_METHOD("test_action", "test_action_ordinal4"));
   BOOST_REQUIRE_EQUAL(atrace[10].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(atrace[10].return_value.size(), 0 );
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->global_sequence, start_gseq + 7);
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
+ * action_ordinal_failtest1 test cases
+ *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_ordinal_failtest1, TESTER) { try {

   produce_blocks(1);
   create_account("testapi"_n );
   set_code( "testapi"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("bob"_n );
   set_code( "bob"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("charlie"_n );
   set_code( "charlie"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("david"_n );
   set_code( "david"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("erin"_n );
   set_code( "erin"_n, contracts::test_api_wasm() );
   produce_blocks(1);

   create_account("fail1"_n ); // <- make first action fails in the middle
   produce_blocks(1);

   transaction_trace_ptr txn_trace =
      CALL_TEST_FUNCTION_NO_THROW( *this, "test_action", "test_action_ordinal1", {});

   BOOST_REQUIRE_EQUAL( validate(), true );

   BOOST_REQUIRE_EQUAL( txn_trace != nullptr, true);
   BOOST_REQUIRE_EQUAL( txn_trace->action_traces.size(), 3);

   auto &atrace = txn_trace->action_traces;

   // fails here after creating one notify action and one inline action
   BOOST_REQUIRE_EQUAL((int)atrace[0].action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[0].creator_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL((int)atrace[0].closest_unnotified_ancestor_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[0].except.has_value(), true);
   BOOST_REQUIRE_EQUAL(atrace[0].except->code(), 3050003);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[1].except.has_value(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[2].except.has_value(), false);

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
+ * action_ordinal_failtest2 test cases
+ *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_ordinal_failtest2, TESTER) { try {

   produce_blocks(1);
   create_account("testapi"_n );
   set_code( "testapi"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("bob"_n );
   set_code( "bob"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("charlie"_n );
   set_code( "charlie"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("david"_n );
   set_code( "david"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("erin"_n );
   set_code( "erin"_n, contracts::test_api_wasm() );
   produce_blocks(1);

   create_account("fail3"_n ); // <- make action 3 fails in the middle
   produce_blocks(1);

   transaction_trace_ptr txn_trace =
      CALL_TEST_FUNCTION_NO_THROW( *this, "test_action", "test_action_ordinal1", {});

   BOOST_REQUIRE_EQUAL( validate(), true );

   BOOST_REQUIRE_EQUAL( txn_trace != nullptr, true);
   BOOST_REQUIRE_EQUAL( txn_trace->action_traces.size(), 8);

   auto &atrace = txn_trace->action_traces;

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[0].action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[0].creator_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL((int)atrace[0].closest_unnotified_ancestor_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(atrace[0].return_value), unsigned_int(1) );
   BOOST_REQUIRE_EQUAL(atrace[0].except.has_value(), false);
   int start_gseq = atrace[0].receipt->global_sequence;

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(atrace[1].return_value), "bob" );
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->global_sequence, start_gseq + 1);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[2].except.has_value(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[3].action_ordinal, 4);
   BOOST_REQUIRE_EQUAL((int)atrace[3].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[3].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[3].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[3].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[3].act.name, TEST_METHOD("test_action", "test_action_ordinal3"));
   BOOST_REQUIRE_EQUAL(atrace[3].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[3].except.has_value(), false);

   // hey exception is here
   BOOST_REQUIRE_EQUAL((int)atrace[4].action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[4].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[4].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[4].receiver, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[4].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[4].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[4].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[4].except.has_value(), true);
   BOOST_REQUIRE_EQUAL(atrace[4].except->code(), 3050003);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[5].action_ordinal, 6);
   BOOST_REQUIRE_EQUAL((int)atrace[5].creator_action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[5].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[5].receiver, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[5].act.account, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[5].act.name, TEST_METHOD("test_action", "test_action_ordinal_foo"));
   BOOST_REQUIRE_EQUAL(atrace[5].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[5].except.has_value(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[6].action_ordinal, 7);
   BOOST_REQUIRE_EQUAL((int)atrace[6].creator_action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[6].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[6].receiver, "david"_n);
   BOOST_REQUIRE_EQUAL(atrace[6].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[6].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[6].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[6].except.has_value(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[7].action_ordinal, 8);
   BOOST_REQUIRE_EQUAL((int)atrace[7].creator_action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[7].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[7].receiver, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[7].act.account, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[7].act.name, TEST_METHOD("test_action", "test_action_ordinal_bar"));
   BOOST_REQUIRE_EQUAL(atrace[7].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[7].except.has_value(), false);

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
+ * action_ordinal_failtest3 test cases
+ *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_ordinal_failtest3, TESTER) { try {

   produce_blocks(1);
   create_account("testapi"_n );
   set_code( "testapi"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("bob"_n );
   set_code( "bob"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("charlie"_n );
   set_code( "charlie"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("david"_n );
   set_code( "david"_n, contracts::test_api_wasm() );
   produce_blocks(1);
   create_account("erin"_n );
   set_code( "erin"_n, contracts::test_api_wasm() );
   produce_blocks(1);

   create_account("failnine"_n ); // <- make action 9 fails in the middle
   produce_blocks(1);

   transaction_trace_ptr txn_trace =
      CALL_TEST_FUNCTION_NO_THROW( *this, "test_action", "test_action_ordinal1", {});

   BOOST_REQUIRE_EQUAL( validate(), true );

   BOOST_REQUIRE_EQUAL( txn_trace != nullptr, true);
   BOOST_REQUIRE_EQUAL( txn_trace->action_traces.size(), 11);

   auto &atrace = txn_trace->action_traces;

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[0].action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[0].creator_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL((int)atrace[0].closest_unnotified_ancestor_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(atrace[0].return_value), unsigned_int(1) );
   BOOST_REQUIRE_EQUAL(atrace[0].except.has_value(), false);
   int start_gseq = atrace[0].receipt->global_sequence;

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(atrace[1].return_value), "bob" );
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->global_sequence, start_gseq + 1);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<name>(atrace[2].return_value), name("five") );
   BOOST_REQUIRE_EQUAL(atrace[2].receipt->global_sequence, start_gseq + 4);

   // fails here
   BOOST_REQUIRE_EQUAL((int)atrace[3].action_ordinal, 4);
   BOOST_REQUIRE_EQUAL((int)atrace[3].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[3].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[3].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[3].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[3].act.name, TEST_METHOD("test_action", "test_action_ordinal3"));
   BOOST_REQUIRE_EQUAL(atrace[3].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[3].except.has_value(), true);
   BOOST_REQUIRE_EQUAL(atrace[3].except->code(), 3050003);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[4].action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[4].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[4].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[4].receiver, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[4].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[4].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[4].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(atrace[4].return_value), "charlie" );
   BOOST_REQUIRE_EQUAL(atrace[4].receipt->global_sequence, start_gseq + 2);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[5].action_ordinal, 6);
   BOOST_REQUIRE_EQUAL((int)atrace[5].creator_action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[5].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[5].receiver, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[5].act.account, "bob"_n);
   BOOST_REQUIRE_EQUAL(atrace[5].act.name, TEST_METHOD("test_action", "test_action_ordinal_foo"));
   BOOST_REQUIRE_EQUAL(atrace[5].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[5].except.has_value(), false);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[6].action_ordinal, 7);
   BOOST_REQUIRE_EQUAL((int)atrace[6].creator_action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[6].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[6].receiver, "david"_n);
   BOOST_REQUIRE_EQUAL(atrace[6].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[6].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[6].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(atrace[6].return_value), "david" );
   BOOST_REQUIRE_EQUAL(atrace[6].receipt->global_sequence, start_gseq + 3);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[7].action_ordinal, 8);
   BOOST_REQUIRE_EQUAL((int)atrace[7].creator_action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[7].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[7].receiver, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[7].act.account, "charlie"_n);
   BOOST_REQUIRE_EQUAL(atrace[7].act.name, TEST_METHOD("test_action", "test_action_ordinal_bar"));
   BOOST_REQUIRE_EQUAL(atrace[7].receipt.has_value(), false);
   BOOST_REQUIRE_EQUAL(atrace[7].except.has_value(), false);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[8].action_ordinal, 9);
   BOOST_REQUIRE_EQUAL((int)atrace[8].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[8].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[8].receiver, "david"_n);
   BOOST_REQUIRE_EQUAL(atrace[8].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[8].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[8].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<bool>(atrace[8].return_value), true );
   BOOST_REQUIRE_EQUAL(atrace[8].receipt->global_sequence, start_gseq + 5);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[9].action_ordinal, 10);
   BOOST_REQUIRE_EQUAL((int)atrace[9].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[9].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[9].receiver, "erin"_n);
   BOOST_REQUIRE_EQUAL(atrace[9].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[9].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[9].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<signed_int>(atrace[9].return_value), signed_int(7) );
   BOOST_REQUIRE_EQUAL(atrace[9].receipt->global_sequence, start_gseq + 6);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[10].action_ordinal, 11);
   BOOST_REQUIRE_EQUAL((int)atrace[10].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[10].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[10].receiver, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[10].act.account, "testapi"_n);
   BOOST_REQUIRE_EQUAL(atrace[10].act.name, TEST_METHOD("test_action", "test_action_ordinal4"));
   BOOST_REQUIRE_EQUAL(atrace[10].receipt.has_value(), true);
   BOOST_REQUIRE_EQUAL(atrace[10].return_value.size(), 0 );
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->global_sequence, start_gseq + 7);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(action_results_tests, TESTER) { try {
   produce_blocks(2);
   create_account( "test"_n );
   set_code( "test"_n, contracts::action_results_wasm() );
   produce_blocks(1);

   auto call_autoresret_and_check = [&]( account_name contract, account_name signer, action_name action, auto&& checker ) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{signer, config::active_name}}, contract, action, bytes{} );
      this->set_transaction_headers( trx, this->DEFAULT_EXPIRATION_DELTA );
      trx.sign( this->get_private_key(signer, "active"), control->get_chain_id() );
      auto res = this->push_transaction(trx);
      checker( res );
   };

   call_autoresret_and_check( "test"_n, "test"_n, "actionresret"_n, [&]( const transaction_trace_ptr& res ) {
      BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed );

      auto &atrace = res->action_traces;
      BOOST_REQUIRE_EQUAL( atrace[0].receipt.has_value(), true );
      BOOST_REQUIRE_EQUAL( atrace[0].return_value.size(), 4 );
      BOOST_REQUIRE_EQUAL( fc::raw::unpack<int>(atrace[0].return_value), 10 );
   } );

   produce_blocks(1);

   call_autoresret_and_check( "test"_n, "test"_n, "retlim"_n, [&]( const transaction_trace_ptr& res ) {
      BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed );

      auto &atrace = res->action_traces;
      BOOST_REQUIRE_EQUAL( atrace[0].receipt.has_value(), true );
      BOOST_REQUIRE_EQUAL( atrace[0].return_value.size(), 256 );
      vector<char> expected_vec(256 - 2, '0');//2 bytes for size of type unsigned_int
      vector<char> ret_vec = fc::raw::unpack<vector<char>>(atrace[0].return_value);

      BOOST_REQUIRE_EQUAL_COLLECTIONS( ret_vec.begin(),
                                       ret_vec.end(),
                                       expected_vec.begin(),
                                       expected_vec.end() );
   } );

   produce_blocks(1);
   BOOST_REQUIRE_THROW(call_autoresret_and_check( "test"_n, "test"_n, "retoverlim"_n, [&]( auto res ) {}),
                       action_return_value_exception);
   produce_blocks(1);
   BOOST_REQUIRE_THROW(call_autoresret_and_check( "test"_n, "test"_n, "ret1overlim"_n, [&]( auto res ) {}),
                       action_return_value_exception);
   produce_blocks(1);
   set_code( config::system_account_name, contracts::action_results_wasm() );
   produce_blocks(1);
   call_autoresret_and_check( config::system_account_name,
                              config::system_account_name, 
                              "retmaxlim"_n, 
                              [&]( const transaction_trace_ptr& res ) {
                                 BOOST_CHECK_EQUAL( res->receipt->status, transaction_receipt::executed );

                                 auto &atrace = res->action_traces;
                                 BOOST_REQUIRE_EQUAL( atrace[0].receipt.has_value(), true );
                                 BOOST_REQUIRE_EQUAL( atrace[0].return_value.size(), 20*1024*1024 );
                                 vector<char> expected_vec(20*1024*1024, '1');

                                 BOOST_REQUIRE_EQUAL_COLLECTIONS( atrace[0].return_value.begin(),
                                                                  atrace[0].return_value.end(),
                                                                  expected_vec.begin(),
                                                                  expected_vec.end() );
                              } );
   produce_blocks(1);
   BOOST_REQUIRE_THROW(call_autoresret_and_check( config::system_account_name, 
                                                  config::system_account_name, 
                                                  "setliminv"_n, 
                                                  [&]( auto res ) {}),
                       action_validate_exception);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
