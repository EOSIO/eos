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

struct dummy_action {
   static eosio::chain::name get_name() {
      return N(dummyaction);
   }
   static eosio::chain::name get_account() {
      return N(testapi);
   }

  char a; //1
  uint64_t b; //8
  int32_t  c; //4
};

struct u128_action {
  unsigned __int128  values[3]; //16*3
};

struct cf_action {
   static eosio::chain::name get_name() {
      return N(cfaction);
   }
   static eosio::chain::name get_account() {
      return N(testapi);
   }

   uint32_t       payload = 100;
   uint32_t       cfd_idx = 0; // context free data index
};

// Deferred Transaction Trigger Action
struct dtt_action {
   static uint64_t get_name() {
      return WASM_TEST_ACTION("test_transaction", "send_deferred_tx_with_dtt_action");
   }
   static uint64_t get_account() {
      return N(testapi).to_uint64_t();
   }

   uint64_t       payer = N(testapi).to_uint64_t();
   uint64_t       deferred_account = N(testapi).to_uint64_t();
   uint64_t       deferred_action = WASM_TEST_ACTION("test_transaction", "deferred_print");
   uint64_t       permission_name = N(active).to_uint64_t();
   uint32_t       delay_sec = 2;
};

struct invalid_access_action {
   uint64_t code;
   uint64_t val;
   uint32_t index;
   bool store;
};

FC_REFLECT( dummy_action, (a)(b)(c) )
FC_REFLECT( u128_action, (values) )
FC_REFLECT( cf_action, (payload)(cfd_idx) )
FC_REFLECT( dtt_action, (payer)(deferred_account)(deferred_action)(permission_name)(delay_sec) )
FC_REFLECT( invalid_access_action, (code)(val)(index)(store) )

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::testing;
using namespace chain;
using namespace fc;

namespace bio = boost::iostreams;

template<uint64_t NAME>
struct test_api_action {
	static account_name get_account() {
		return N(testapi);
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
   return fc::variant(fc::uint128_t(i)).get_string();
}

template <typename T>
transaction_trace_ptr CallAction(TESTER& test, T ac, const vector<account_name>& scope = {N(testapi)}) {
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

template <typename T>
transaction_trace_ptr CallFunction(TESTER& test, T ac, const vector<char>& data, const vector<account_name>& scope = {N(testapi)}, bool no_throw = false) {
   {
      signed_transaction trx;

      auto pl = vector<permission_level>{{scope[0], config::active_name}};
      if (scope.size() > 1)
         for (unsigned int i=1; i < scope.size(); i++)
            pl.push_back({scope[i], config::active_name});

      action act(pl, ac);
      act.data = data;
      act.authorization = {{N(testapi), config::active_name}};
      trx.actions.push_back(act);

      test.set_transaction_headers(trx, test.DEFAULT_EXPIRATION_DELTA);
      auto sigs = trx.sign(test.get_private_key(scope[0], "active"), test.control->get_chain_id());

      flat_set<public_key_type> keys;
      trx.get_signature_keys(test.control->get_chain_id(), fc::time_point::maximum(), keys);

      auto res = test.push_transaction(trx, fc::time_point::maximum(), TESTER::DEFAULT_BILLED_CPU_TIME_US, no_throw);
      if (!no_throw) {
         BOOST_CHECK_EQUAL(res->receipt->status, transaction_receipt::executed);
      }
      test.produce_block();
      return res;
   }
}

#define CALL_TEST_FUNCTION(_TESTER, CLS, MTH, DATA) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)
#define CALL_TEST_FUNCTION_SYSTEM(_TESTER, CLS, MTH, DATA) CallFunction(_TESTER, test_chain_action<TEST_METHOD(CLS, MTH)>{}, DATA, {config::system_account_name} )
#define CALL_TEST_FUNCTION_SCOPE(_TESTER, CLS, MTH, DATA, ACCOUNT) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA, ACCOUNT)
#define CALL_TEST_FUNCTION_NO_THROW(_TESTER, CLS, MTH, DATA) CallFunction(_TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA, {N(testapi)}, true)
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
   create_account( N(test) );
   set_code( N(test), contracts::payloadless_wasm() );
   produce_blocks(1);

   auto call_doit_and_check = [&]( account_name contract, account_name signer, auto&& checker ) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{signer, config::active_name}}, contract, N(doit), bytes{} );
      this->set_transaction_headers( trx, this->DEFAULT_EXPIRATION_DELTA );
      trx.sign( this->get_private_key(signer, "active"), control->get_chain_id() );
      auto res = this->push_transaction(trx);
      checker( res );
   };

   auto call_provereset_and_check = [&]( account_name contract, account_name signer, auto&& checker ) {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{signer, config::active_name}}, contract, N(provereset), bytes{} );
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
   call_doit_and_check( N(test), N(test), [&]( const transaction_trace_ptr& res ) {
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

   set_code( N(test), contracts::asserter_wasm() );
   set_code( config::system_account_name, contracts::payloadless_wasm() );

   call_provereset_and_check( N(test), N(test), [&]( const transaction_trace_ptr& res ) {
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

   call_doit_and_check( config::system_account_name, N(test), [&]( const transaction_trace_ptr& res ) {
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

   set_code( N(test), contracts::eosio_bios_wasm() );
   set_abi( N(test), contracts::eosio_bios_abi().data() );
	set_code( N(test), contracts::payloadless_wasm() );

   call_doit_and_check( N(test), N(test), [&]( const transaction_trace_ptr& res ) {
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
	create_account( N(testapi) );
	create_account( N(acc1) );
	create_account( N(acc2) );
	create_account( N(acc3) );
	create_account( N(acc4) );
	produce_blocks(10);
	set_code( N(testapi), contracts::test_api_wasm() );
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
   auto scope = std::vector<account_name>{N(testapi)};
   auto test_require_notice = [this](auto& test, std::vector<char>& data, std::vector<account_name>& scope){
      signed_transaction trx;
      auto tm = test_api_action<TEST_METHOD("test_action", "require_notice")>{};

      action act(std::vector<permission_level>{{N(testapi), config::active_name}}, tm);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(data.begin(), data.end(), std::back_inserter(dest));
      trx.actions.push_back(act);

      test.set_transaction_headers(trx);
      trx.sign(test.get_private_key(N(inita), "active"), control->get_chain_id());
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
   auto a3only = std::vector<permission_level>{{N(acc3), config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a3only)), missing_auth_exception,
         [](const missing_auth_exception& e) {
            return expect_assert_message(e, "missing authority of");
         }
      );

   // test require_auth
   auto a4only = std::vector<permission_level>{{N(acc4), config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a4only)), missing_auth_exception,
         [](const missing_auth_exception& e) {
            return expect_assert_message(e, "missing authority of");
         }
      );

   // test require_auth
   auto a3a4 = std::vector<permission_level>{{N(acc3), config::active_name}, {N(acc4), config::active_name}};
   auto a3a4_scope = std::vector<account_name>{N(acc3), N(acc4)};
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
      act.authorization = {{N(testapi), config::active_name}, {N(acc3), config::active_name}, {N(acc4), config::active_name}};
      trx.actions.push_back(act);

      set_transaction_headers(trx);
      trx.sign(get_private_key(N(testapi), "active"), control->get_chain_id());
      trx.sign(get_private_key(N(acc3), "active"), control->get_chain_id());
      trx.sign(get_private_key(N(acc4), "active"), control->get_chain_id());
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
   CALL_TEST_FUNCTION( *this, "test_action", "test_current_receiver", fc::raw::pack(N(testapi)));

   // test send_action_sender
   CALL_TEST_FUNCTION( *this, "test_transaction", "send_action_sender", fc::raw::pack(N(testapi)));

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
      create_account( N(testapi) );
      create_account( N(acc5) );
      produce_blocks(1);
      set_code( N(testapi), contracts::test_api_wasm() );
      set_code( N(acc5), contracts::test_api_wasm() );
      produce_blocks(1);

      // test require_notice
      signed_transaction trx;
      auto tm = test_api_action<TEST_METHOD( "test_action", "require_notice_tests" )>{};

      action act( std::vector<permission_level>{{N( testapi ), config::active_name}}, tm );
      trx.actions.push_back( act );

      set_transaction_headers( trx );
      trx.sign( get_private_key( N( testapi ), "active" ), control->get_chain_id() );
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
   chain.create_account( N(testapi) );
   chain.create_account( N(testapi2) );
   chain.produce_blocks(10);
   chain.set_code( N(testapi), contracts::test_api_wasm() );
   chain.produce_blocks(1);
   chain.set_code( N(testapi2), contracts::test_api_wasm() );
   chain.produce_blocks(1);

   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( chain, "test_action", "test_ram_billing_in_notify",
                                              fc::raw::pack( ((unsigned __int128)N(testapi2).to_uint64_t() << 64) | N(testapi).to_uint64_t() ) ),
                          subjective_block_production_exception,
                          fc_exception_message_is("Cannot charge RAM to other accounts during notify.")
   );


   CALL_TEST_FUNCTION( chain, "test_action", "test_ram_billing_in_notify", fc::raw::pack( ((unsigned __int128)N(testapi2).to_uint64_t() << 64) | 0 ) );

   CALL_TEST_FUNCTION( chain, "test_action", "test_ram_billing_in_notify", fc::raw::pack( ((unsigned __int128)N(testapi2).to_uint64_t() << 64) | N(testapi2).to_uint64_t() ) );

   BOOST_REQUIRE_EQUAL( chain.validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * context free action tests
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(cf_action_tests, TESTER) { try {
      produce_blocks(2);
      create_account( N(testapi) );
      create_account( N(dummy) );
      produce_blocks(10);
      set_code( N(testapi), contracts::test_api_wasm() );
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
      auto pl = vector<permission_level>{{N(testapi), config::active_name}};
      action act1(pl, da);
      trx.actions.push_back(act1);
      set_transaction_headers(trx);
      // run normal passing case
      auto sigs = trx.sign(get_private_key(N(testapi), "active"), control->get_chain_id());
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
      sigs = trx.sign(get_private_key(N(testapi), "active"), control->get_chain_id());
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
         for (uint32_t i = 200; i <= 211; ++i) {
            trx.context_free_actions.clear();
            trx.context_free_data.clear();
            cfa.payload = i;
            cfa.cfd_idx = 1;
            action cfa_act({}, cfa);
            trx.context_free_actions.emplace_back(cfa_act);
            trx.signatures.clear();
            set_transaction_headers(trx);
            sigs = trx.sign(get_private_key(N(testapi), "active"), control->get_chain_id());
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
   BOOST_TEST((std::string)tx1.sign(priv_key, control->get_chain_id()) != (std::string)tx2.sign(priv_key, control->get_chain_id()));

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(cfa_stateful_api, TESTER)  try {

   create_account( N(testapi) );
	produce_blocks(1);
	set_code( N(testapi), contracts::test_api_wasm() );

   account_name a = N(testapi2);
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

   create_account( N(testapi) );
	produce_blocks(1);
	set_code( N(testapi), contracts::test_api_wasm() );

   account_name a = N(testapi2);
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
   create_account( N(testapi2) );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(deferred_cfa_success, TESTER)  try {

   create_account( N(testapi) );
	produce_blocks(1);
	set_code( N(testapi), contracts::test_api_wasm() );

   account_name a = N(testapi2);
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
   BOOST_CHECK_EXCEPTION(create_account( N(testapi2) ), fc::exception,
      [&](const fc::exception &e) {
         return expect_assert_message(e, "Cannot create account named testapi2, as that name is already taken");
      });
   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

/*************************************************************************************
 * checktime_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(checktime_pass_tests, TESTER) { try {
	produce_blocks(2);
	create_account( N(testapi) );
	produce_blocks(10);
	set_code( N(testapi), contracts::test_api_wasm() );
	produce_blocks(1);

   // test checktime_pass
   CALL_TEST_FUNCTION( *this, "test_checktime", "checktime_pass", {});

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

template<class T>
void call_test(TESTER& test, T ac, uint32_t billed_cpu_time_us , uint32_t max_cpu_usage_ms = 200, std::vector<char> payload = {} ) {
   signed_transaction trx;

   auto pl = vector<permission_level>{{N(testapi), config::active_name}};
   action act(pl, ac);
   act.data = payload;

   trx.actions.push_back(act);
   test.set_transaction_headers(trx);
   auto sigs = trx.sign(test.get_private_key(N(testapi), "active"), test.control->get_chain_id());
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
   t.create_account( N(testapi) );
   ilog( "set code" );
   t.set_code( N(testapi), contracts::test_api_wasm() );
   ilog( "produce block" );
   t.produce_blocks(1);

   int64_t x; int64_t net; int64_t cpu;
   t.control->get_resource_limits_manager().get_account_limits( N(testapi), x, net, cpu );
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
      t.push_dummy( N(testapi), dummy_string + std::to_string(i), increment );
      time_left_in_block_us -= increment;
   }
   BOOST_CHECK_EXCEPTION( call_test( t, test_api_action<TEST_METHOD("test_checktime", "checktime_failure")>{},
                                    0, 200, fc::raw::pack(10000000000000000000ULL) ),
                          block_cpu_usage_exceeded, is_block_cpu_usage_exceeded );

   BOOST_REQUIRE_EQUAL( t.validate(), true );
} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE(checktime_intrinsic, TESTER) { try {
	produce_blocks(2);
	create_account( N(testapi) );
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
	set_code( N(testapi), ss.str().c_str() );
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
	create_account( N(testapi) );
	produce_blocks(10);
	set_code( N(testapi), contracts::test_api_wasm() );
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
   create_account( N(testapi) );
   produce_blocks(100);
   set_code( N(testapi), contracts::test_api_wasm() );
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
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_large", {}), inline_action_too_big,
         [](const fc::exception& e) {
            return expect_assert_message(e, "inline action too big");
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
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
         auto& t = std::get<0>(x);
         if (t && t->receipt && t->receipt->status != transaction_receipt::executed) { trace = t; }
      } );

      // test error handling on deferred transaction failure
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_transaction_trigger_error_handler", {});

      BOOST_REQUIRE(trace);
      BOOST_CHECK_EQUAL(trace->receipt->status, transaction_receipt::soft_fail);
      c.disconnect();
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
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(deferred_transaction_tests, TESTER) { try {
   produce_blocks(2);
   create_accounts( {N(testapi), N(testapi2), N(alice)} );
   set_code( N(testapi), contracts::test_api_wasm() );
   set_code( N(testapi2), contracts::test_api_wasm() );
   produce_blocks(1);

   //schedule
   {
      transaction_trace_ptr trace;
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
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
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
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
         control->push_scheduled_transaction(trx, fc::time_point::maximum());
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
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
         auto& t = std::get<0>(x);
         if (t && t->scheduled) { trace = t; ++count; }
      } );
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction_replace", {});
      CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_transaction_replace", {});
      produce_blocks( 3 );

      //check that only one deferred transaction executed
      auto dtrxs = get_scheduled_transactions();
      BOOST_CHECK_EQUAL(dtrxs.size(), 1);
      for (const auto& trx: dtrxs) {
         control->push_scheduled_transaction(trx, fc::time_point::maximum());
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
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
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
      auto c = control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
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
      dtt_act1.payer = N(alice).to_uint64_t();
      BOOST_CHECK_THROW(CALL_TEST_FUNCTION(*this, "test_transaction", "send_deferred_tx_with_dtt_action", fc::raw::pack(dtt_act1)), action_validate_exception);

      // Send a tx which in turn sends a deferred tx with the deferred tx's receiver != this tx receiver
      // This will include the authorization of the receiver, and impose any related delay associated with the authority
      // We set the authorization delay to be 10 sec here, and since the deferred tx delay is set to be 5 sec, so this tx should fail
      dtt_action dtt_act2;
      dtt_act2.deferred_account = N(testapi2).to_uint64_t();
      dtt_act2.permission_name = N(additional).to_uint64_t();
      dtt_act2.delay_sec = 5;

      auto auth = authority(get_public_key(name("testapi"), name(dtt_act2.permission_name).to_string()), 10);
      auth.accounts.push_back( permission_level_weight{{N(testapi), config::eosio_code_name}, 1} );

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
      dtt_act3.deferred_account = N(testapi).to_uint64_t();
      dtt_act3.permission_name = N(additional).to_uint64_t();
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
      push_action(config::system_account_name, N(setpriv), config::system_account_name,  mutable_variant_object()
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

   chain.push_action( contract_account, N(delayedcall), test_account, fc::mutable_variant_object()
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
      chain.get_action( contract_account, N(delayedcall),
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
      chain.get_action( contract_account, N(delayedcall),
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
      chain.get_action( contract_account, N(fail),
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

   chain.push_action( contract_account, N(delayedcall), test_account, fc::mutable_variant_object()
      ("payer",     test_account)
      ("sender_id", 1)
      ("contract",  contract_account)
      ("payload",   101)
      ("delay_sec", 1000)
      ("replace_existing", false)
   );

   chain.push_action( contract_account, N(delayedcall), test_account, fc::mutable_variant_object()
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
      chain.push_action( contract_account, N(delayedcall), test_account, fc::mutable_variant_object()
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
      chain.get_action( contract_account, N(delayedcall),
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
      chain.get_action( contract_account, N(delayedcall),
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
      chain.get_action( contract_account, N(delayedcall),
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
      chain.get_action( contract_account, N(fail),
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

template <uint64_t NAME>
struct setprod_act {
   static account_name get_account() {
      return N(config::system_account_name);
   }

   static action_name get_name() {
      return action_name(NAME);
   }
};

/*************************************************************************************
 * chain_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(chain_tests, TESTER) { try {
   produce_blocks(2);

   create_account( N(testapi) );

   vector<account_name> producers = { N(inita),
                                      N(initb),
                                      N(initc),
                                      N(initd),
                                      N(inite),
                                      N(initf),
                                      N(initg),
                                      N(inith),
                                      N(initi),
                                      N(initj),
                                      N(initk),
                                      N(initl),
                                      N(initm),
                                      N(initn),
                                      N(inito),
                                      N(initp),
                                      N(initq),
                                      N(initr),
                                      N(inits),
                                      N(initt),
                                      N(initu)
   };

   create_accounts( producers );
   set_producers (producers );

   set_code( N(testapi), contracts::test_api_wasm() );
   produce_blocks(100);

   vector<account_name> prods( control->active_producers().producers.size() );
   for ( uint32_t i = 0; i < prods.size(); i++ ) {
      prods[i] = control->active_producers().producers[i].producer_name;
   }

   CALL_TEST_FUNCTION( *this, "test_chain", "test_activeprods", fc::raw::pack(prods) );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * db_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(db_tests, TESTER) { try {
   produce_blocks(2);
   create_account( N(testapi) );
   create_account( N(testapi2) );
   produce_blocks(10);
   set_code( N(testapi), contracts::test_api_db_wasm() );
   set_abi(  N(testapi), contracts::test_api_db_abi().data() );
   set_code( N(testapi2), contracts::test_api_db_wasm() );
   set_abi(  N(testapi2), contracts::test_api_db_abi().data() );
   produce_blocks(1);

   push_action( N(testapi), N(pg),  N(testapi), mutable_variant_object() ); // primary_i64_general
   push_action( N(testapi), N(pl),  N(testapi), mutable_variant_object() ); // primary_i64_lowerbound
   push_action( N(testapi), N(pu),  N(testapi), mutable_variant_object() ); // primary_i64_upperbound
   push_action( N(testapi), N(s1g), N(testapi), mutable_variant_object() ); // idx64_general
   push_action( N(testapi), N(s1l), N(testapi), mutable_variant_object() ); // idx64_lowerbound
   push_action( N(testapi), N(s1u), N(testapi), mutable_variant_object() ); // idx64_upperbound

   // Store value in primary table
   push_action( N(testapi), N(tia), N(testapi), mutable_variant_object() // test_invalid_access
      ("code", "testapi")
      ("val", 10)
      ("index", 0)
      ("store", true)
   );

   // Attempt to change the value stored in the primary table under the code of N(testapi)
   BOOST_CHECK_EXCEPTION( push_action( N(testapi2), N(tia), N(testapi2), mutable_variant_object()
                              ("code", "testapi")
                              ("val", "20")
                              ("index", 0)
                              ("store", true)
                           ), table_access_violation,
                           fc_exception_message_is("db access violation")
   );

   // Verify that the value has not changed.
   push_action( N(testapi), N(tia), N(testapi), mutable_variant_object()
      ("code", "testapi")
      ("val", 10)
      ("index", 0)
      ("store", false)
   );

   // Store value in secondary table
   push_action( N(testapi), N(tia), N(testapi), mutable_variant_object() // test_invalid_access
      ("code", "testapi")
      ("val", 10)
      ("index", 1)
      ("store", true)
   );

   // Attempt to change the value stored in the secondary table under the code of N(testapi)
   BOOST_CHECK_EXCEPTION( push_action( N(testapi2), N(tia), N(testapi2), mutable_variant_object()
                              ("code", "testapi")
                              ("val", "20")
                              ("index", 1)
                              ("store", true)
                           ), table_access_violation,
                           fc_exception_message_is("db access violation")
   );

   // Verify that the value has not changed.
   push_action( N(testapi), N(tia), N(testapi), mutable_variant_object()
      ("code", "testapi")
      ("val", 10)
      ("index", 1)
      ("store", false)
   );

   // idx_double_nan_create_fail
   BOOST_CHECK_EXCEPTION(  push_action( N(testapi), N(sdnancreate), N(testapi), mutable_variant_object() ),
                           transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   // idx_double_nan_modify_fail
   BOOST_CHECK_EXCEPTION(  push_action( N(testapi), N(sdnanmodify), N(testapi), mutable_variant_object() ),
                           transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   // idx_double_nan_lookup_fail
   BOOST_CHECK_EXCEPTION(  push_action( N(testapi), N(sdnanlookup), N(testapi), mutable_variant_object()
                              ("lookup_type", 0) // 0 for find
                           ), transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   BOOST_CHECK_EXCEPTION(  push_action( N(testapi), N(sdnanlookup), N(testapi), mutable_variant_object()
                              ("lookup_type", 1) // 1 for lower bound
                           ), transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   BOOST_CHECK_EXCEPTION(  push_action( N(testapi), N(sdnanlookup), N(testapi), mutable_variant_object()
                              ("lookup_type", 2) // 2 for upper bound
                           ), transaction_exception,
                           fc_exception_message_is("NaN is not an allowed value for a secondary key")
   );

   push_action( N(testapi), N(sk32align), N(testapi), mutable_variant_object() ); // misaligned_secondary_key256_tests

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * multi_index_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(multi_index_tests, TESTER) { try {
   produce_blocks(1);
   create_account( N(testapi) );
   produce_blocks(1);
   set_code( N(testapi), contracts::test_api_multi_index_wasm() );
   set_abi( N(testapi), contracts::test_api_multi_index_abi().data() );
   produce_blocks(1);

   auto check_failure = [this]( action_name a, const char* expected_error_msg ) {
      BOOST_CHECK_EXCEPTION(  push_action( N(testapi), a, N(testapi), {} ),
                              eosio_assert_message_exception,
                              eosio_assert_message_is( expected_error_msg )
      );
   };

   push_action( N(testapi), N(s1g),  N(testapi), {} );        // idx64_general
   push_action( N(testapi), N(s1store),  N(testapi), {} );    // idx64_store_only
   push_action( N(testapi), N(s1check),  N(testapi), {} );    // idx64_check_without_storing
   push_action( N(testapi), N(s2g),  N(testapi), {} );        // idx128_general
   push_action( N(testapi), N(s2store),  N(testapi), {} );    // idx128_store_only
   push_action( N(testapi), N(s2check),  N(testapi), {} );    // idx128_check_without_storing
   push_action( N(testapi), N(s2autoinc),  N(testapi), {} );  // idx128_autoincrement_test
   push_action( N(testapi), N(s2autoinc1),  N(testapi), {} ); // idx128_autoincrement_test_part1
   push_action( N(testapi), N(s2autoinc2),  N(testapi), {} ); // idx128_autoincrement_test_part2
   push_action( N(testapi), N(s3g),  N(testapi), {} );        // idx256_general
   push_action( N(testapi), N(sdg),  N(testapi), {} );        // idx_double_general
   push_action( N(testapi), N(sldg),  N(testapi), {} );       // idx_long_double_general

   check_failure( N(s1pkend), "cannot increment end iterator" ); // idx64_pk_iterator_exceed_end
   check_failure( N(s1skend), "cannot increment end iterator" ); // idx64_sk_iterator_exceed_end
   check_failure( N(s1pkbegin), "cannot decrement iterator at beginning of table" ); // idx64_pk_iterator_exceed_begin
   check_failure( N(s1skbegin), "cannot decrement iterator at beginning of index" ); // idx64_sk_iterator_exceed_begin
   check_failure( N(s1pkref), "object passed to iterator_to is not in multi_index" ); // idx64_pass_pk_ref_to_other_table
   check_failure( N(s1skref), "object passed to iterator_to is not in multi_index" ); // idx64_pass_sk_ref_to_other_table
   check_failure( N(s1pkitrto), "object passed to iterator_to is not in multi_index" ); // idx64_pass_pk_end_itr_to_iterator_to
   check_failure( N(s1pkmodify), "cannot pass end iterator to modify" ); // idx64_pass_pk_end_itr_to_modify
   check_failure( N(s1pkerase), "cannot pass end iterator to erase" ); // idx64_pass_pk_end_itr_to_erase
   check_failure( N(s1skitrto), "object passed to iterator_to is not in multi_index" ); // idx64_pass_sk_end_itr_to_iterator_to
   check_failure( N(s1skmodify), "cannot pass end iterator to modify" ); // idx64_pass_sk_end_itr_to_modify
   check_failure( N(s1skerase), "cannot pass end iterator to erase" ); // idx64_pass_sk_end_itr_to_erase
   check_failure( N(s1modpk), "updater cannot change primary key when modifying an object" ); // idx64_modify_primary_key
   check_failure( N(s1exhaustpk), "next primary key in table is at autoincrement limit" ); // idx64_run_out_of_avl_pk
   check_failure( N(s1findfail1), "unable to find key" ); // idx64_require_find_fail
   check_failure( N(s1findfail2), "unable to find primary key in require_find" );// idx64_require_find_fail_with_msg
   check_failure( N(s1findfail3), "unable to find secondary key" ); // idx64_require_find_sk_fail
   check_failure( N(s1findfail4), "unable to find sec key" ); // idx64_require_find_sk_fail_with_msg

   push_action( N(testapi), N(s1skcache),  N(testapi), {} ); // idx64_sk_cache_pk_lookup
   push_action( N(testapi), N(s1pkcache),  N(testapi), {} ); // idx64_pk_cache_sk_lookup

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * crypto_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(crypto_tests, TESTER) { try {
   produce_block();
   create_account(N(testapi) );
   produce_block();
   set_code(N(testapi), contracts::test_api_wasm() );
   produce_block();
   {
      signed_transaction trx;

      auto pl = vector<permission_level>{{N(testapi), config::active_name}};

      action act(pl, test_api_action<TEST_METHOD("test_crypto", "test_recover_key")>{});
      const auto priv_key = get_private_key(N(testapi), "active" );
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

      auto pl = vector<permission_level>{{N(testapi), config::active_name}};

      action act(pl, test_api_action<TEST_METHOD("test_crypto", "test_recover_key_partial")>{});

      // construct a mock WebAuthN pubkey and signature, as it is the only type that would be variable-sized
      const auto priv_key = get_private_key<mock::webauthn_private_key>(N(testapi), "active" );
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
 * print_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(print_tests, TESTER) { try {
	produce_blocks(2);
	create_account(N(testapi) );
	produce_blocks(1000);

	set_code(N(testapi), contracts::test_api_wasm() );
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
	create_account( N(testapi) );

	produce_blocks(1000);
	set_code( N(testapi), contracts::test_api_wasm() );
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
   create_account( N(testapi) );

   produce_blocks(1);
   set_code( N(testapi), contracts::test_api_wasm() );
   produce_blocks(1);

   auto get_result_int64 = [&]() -> int64_t {
      const auto& db = control->db();
      const auto* t_id = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(testapi), N(testapi), N(testapi)));

      FC_ASSERT(t_id != 0, "Table id not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      FC_ASSERT( 0 != itr->value.size(), "unexpected result size");
      return *reinterpret_cast<const int64_t *>(itr->value.data());
   };

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = N(testapi),
         .permission = N(active),
         .pubkeys    = {
            get_public_key(N(testapi), "active")
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(1), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = N(testapi),
         .permission = N(active),
         .pubkeys    = {
            public_key_type(string("EOS7GfRtyDWWgxV88a5TRaYY59XmHptyfjsFmHHfioGNJtPjpSmGX"))
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = N(testapi),
         .permission = N(active),
         .pubkeys    = {
            get_public_key(N(testapi), "active"),
            public_key_type(string("EOS7GfRtyDWWgxV88a5TRaYY59XmHptyfjsFmHHfioGNJtPjpSmGX"))
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() ); // Failure due to irrelevant signatures

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = N(noname),
         .permission = N(active),
         .pubkeys    = {
            get_public_key(N(testapi), "active")
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = N(testapi),
         .permission = N(active),
         .pubkeys    = {}
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

   CALL_TEST_FUNCTION( *this, "test_permission", "check_authorization",
      fc::raw::pack( check_auth {
         .account    = N(testapi),
         .permission = N(noname),
         .pubkeys    = {
            get_public_key(N(testapi), "active")
         }
      })
   );
   BOOST_CHECK_EQUAL( int64_t(0), get_result_int64() );

} FC_LOG_AND_RETHROW() }

#if 0
/*************************************************************************************
 * privileged_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(privileged_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi) );
	create_account( N(acc1) );
	produce_blocks(100);
	set_code( N(testapi), contracts::test_api_wasm() );
	produce_blocks(1);

   {
		signed_transaction trx;

      auto pl = vector<permission_level>{{config::system_account_name, config::active_name}};
      action act(pl, test_chain_action<N(setprods)>());
      vector<producer_key> prod_keys = {
                                          { N(inita), get_public_key( N(inita), "active" ) },
                                          { N(initb), get_public_key( N(initb), "active" ) },
                                          { N(initc), get_public_key( N(initc), "active" ) },
                                          { N(initd), get_public_key( N(initd), "active" ) },
                                          { N(inite), get_public_key( N(inite), "active" ) },
                                          { N(initf), get_public_key( N(initf), "active" ) },
                                          { N(initg), get_public_key( N(initg), "active" ) },
                                          { N(inith), get_public_key( N(inith), "active" ) },
                                          { N(initi), get_public_key( N(initi), "active" ) },
                                          { N(initj), get_public_key( N(initj), "active" ) },
                                          { N(initk), get_public_key( N(initk), "active" ) },
                                          { N(initl), get_public_key( N(initl), "active" ) },
                                          { N(initm), get_public_key( N(initm), "active" ) },
                                          { N(initn), get_public_key( N(initn), "active" ) },
                                          { N(inito), get_public_key( N(inito), "active" ) },
                                          { N(initp), get_public_key( N(initp), "active" ) },
                                          { N(initq), get_public_key( N(initq), "active" ) },
                                          { N(initr), get_public_key( N(initr), "active" ) },
                                          { N(inits), get_public_key( N(inits), "active" ) },
                                          { N(initt), get_public_key( N(initt), "active" ) },
                                          { N(initu), get_public_key( N(initu), "active" ) }
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
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), contracts::test_api_wasm() );
   produce_blocks(1000);

   CALL_TEST_FUNCTION( *this, "test_datastream", "test_basic", {} );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * permission_usage_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(permission_usage_tests, TESTER) { try {
   produce_block();
   create_accounts( {N(testapi), N(alice), N(bob)} );
   produce_block();
   set_code(N(testapi), contracts::test_api_wasm() );
   produce_block();

   push_reqauth( N(alice), {{N(alice), config::active_name}}, {get_private_key(N(alice), "active")} );

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                           N(alice), config::active_name,
                           control->pending_block_time()
                       })
   );

   // Fails because the last used time is updated after the transaction executes.
   BOOST_CHECK_THROW( CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       N(testapi), config::active_name,
                                       control->head_block_time() + fc::milliseconds(config::block_interval_ms)
                                     })
   ), eosio_assert_message_exception );

   produce_blocks(5);

   set_authority( N(bob), N(perm1), authority( get_private_key(N(bob), "perm1").get_public_key() ) );

   push_action(config::system_account_name, linkauth::get_name(), N(bob), fc::mutable_variant_object()
           ("account", "bob")
           ("code", "eosio")
           ("type", "reqauth")
           ("requirement", "perm1")
   );

   auto permission_creation_time = control->pending_block_time();

   produce_blocks(5);

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       N(bob), N(perm1),
                                       permission_creation_time
                                     })
   );

   produce_blocks(5);

   push_reqauth( N(bob), {{N(bob), N(perm1)}}, {get_private_key(N(bob), "perm1")} );

   auto perm1_last_used_time = control->pending_block_time();

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       N(bob), config::active_name,
                                       permission_creation_time
                                     })
   );

   BOOST_CHECK_THROW( CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                                          fc::raw::pack(test_permission_last_used_action{
                                                            N(bob), N(perm1),
                                                            permission_creation_time
                                          })
   ), eosio_assert_message_exception );

   CALL_TEST_FUNCTION( *this, "test_permission", "test_permission_last_used",
                       fc::raw::pack(test_permission_last_used_action{
                                       N(bob), N(perm1),
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
   create_account( N(testapi) );
   produce_block();
   set_code(N(testapi), contracts::test_api_wasm() );
   produce_block();

   create_account( N(alice) );
   auto alice_creation_time = control->pending_block_time();

   produce_blocks(10);

   CALL_TEST_FUNCTION( *this, "test_permission", "test_account_creation_time",
                       fc::raw::pack(test_permission_last_used_action{
                           N(alice), config::active_name,
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
   create_account( N(testapi) );
   produce_block();
   set_code(N(testapi), contracts::test_api_wasm() );

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

   set_abi( N(testapi), abi_string );

   auto var = fc::json::from_string(abi_string);
   abi_serializer abis(var.as<abi_def>(), abi_serializer_max_time);

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
   BOOST_REQUIRE_EQUAL( omsg1.valid(), true );
   BOOST_CHECK_EQUAL( *omsg1, "standard error message" );

   auto omsg2 = abis.get_error_message(2);
   BOOST_CHECK_EQUAL( omsg2.valid(), false );

   auto omsg3 = abis.get_error_message(42);
   BOOST_REQUIRE_EQUAL( omsg3.valid(), true );
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
   create_account(N(testapi) );
   set_code( N(testapi), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(bob) );
   set_code( N(bob), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(charlie) );
   set_code( N(charlie), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(david) );
   set_code( N(david), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(erin) );
   set_code( N(erin), contracts::test_api_wasm() );
   produce_blocks(1);

   transaction_trace_ptr txn_trace = CALL_TEST_FUNCTION_SCOPE( *this, "test_action", "test_action_ordinal1",
      {}, vector<account_name>{ N(testapi)});

   BOOST_REQUIRE_EQUAL( validate(), true );

   BOOST_REQUIRE_EQUAL( txn_trace != nullptr, true);
   BOOST_REQUIRE_EQUAL( txn_trace->action_traces.size(), 11);

   auto &atrace = txn_trace->action_traces;
   BOOST_REQUIRE_EQUAL((int)atrace[0].action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[0].creator_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL((int)atrace[0].closest_unnotified_ancestor_action_ordinal, 0);
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[0].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(*atrace[0].receipt->return_value), unsigned_int(1) );
   int start_gseq = atrace[0].receipt->global_sequence;

   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(*atrace[1].receipt->return_value), "bob" );
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->global_sequence, start_gseq + 1);

   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[2].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<name>(*atrace[2].receipt->return_value), name("five") );
   BOOST_REQUIRE_EQUAL(atrace[2].receipt->global_sequence, start_gseq + 4);

   BOOST_REQUIRE_EQUAL((int)atrace[3].action_ordinal, 4);
   BOOST_REQUIRE_EQUAL((int)atrace[3].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[3].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[3].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[3].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[3].act.name, TEST_METHOD("test_action", "test_action_ordinal3"));
   BOOST_REQUIRE_EQUAL(atrace[3].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[3].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(*atrace[3].receipt->return_value), unsigned_int(9) );
   BOOST_REQUIRE_EQUAL(atrace[3].receipt->global_sequence, start_gseq + 8);

   BOOST_REQUIRE_EQUAL((int)atrace[4].action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[4].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[4].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[4].receiver, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[4].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[4].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[4].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[4].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(*atrace[4].receipt->return_value), "charlie" );
   BOOST_REQUIRE_EQUAL(atrace[4].receipt->global_sequence, start_gseq + 2);

   BOOST_REQUIRE_EQUAL((int)atrace[5].action_ordinal, 6);
   BOOST_REQUIRE_EQUAL((int)atrace[5].creator_action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[5].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[5].receiver, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[5].act.account, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[5].act.name, TEST_METHOD("test_action", "test_action_ordinal_foo"));
   BOOST_REQUIRE_EQUAL(atrace[5].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[5].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<double>(*atrace[5].receipt->return_value), 13.23 );
   BOOST_REQUIRE_EQUAL(atrace[5].receipt->global_sequence, start_gseq + 9);

   BOOST_REQUIRE_EQUAL((int)atrace[6].action_ordinal, 7);
   BOOST_REQUIRE_EQUAL((int)atrace[6].creator_action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[6].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[6].receiver, N(david));
   BOOST_REQUIRE_EQUAL(atrace[6].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[6].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[6].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[6].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(*atrace[6].receipt->return_value), "david" );
   BOOST_REQUIRE_EQUAL(atrace[6].receipt->global_sequence, start_gseq + 3);

   BOOST_REQUIRE_EQUAL((int)atrace[7].action_ordinal, 8);
   BOOST_REQUIRE_EQUAL((int)atrace[7].creator_action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[7].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[7].receiver, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[7].act.account, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[7].act.name, TEST_METHOD("test_action", "test_action_ordinal_bar"));
   BOOST_REQUIRE_EQUAL(atrace[7].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[7].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<float>(*atrace[7].receipt->return_value), 11.42f );
   BOOST_REQUIRE_EQUAL(atrace[7].receipt->global_sequence, start_gseq + 10);

   BOOST_REQUIRE_EQUAL((int)atrace[8].action_ordinal, 9);
   BOOST_REQUIRE_EQUAL((int)atrace[8].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[8].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[8].receiver, N(david));
   BOOST_REQUIRE_EQUAL(atrace[8].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[8].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[8].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[8].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<bool>(*atrace[8].receipt->return_value), true );
   BOOST_REQUIRE_EQUAL(atrace[8].receipt->global_sequence, start_gseq + 5);

   BOOST_REQUIRE_EQUAL((int)atrace[9].action_ordinal, 10);
   BOOST_REQUIRE_EQUAL((int)atrace[9].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[9].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[9].receiver, N(erin));
   BOOST_REQUIRE_EQUAL(atrace[9].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[9].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[9].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[9].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<signed_int>(*atrace[9].receipt->return_value), signed_int(7) );
   BOOST_REQUIRE_EQUAL(atrace[9].receipt->global_sequence, start_gseq + 6);

   BOOST_REQUIRE_EQUAL((int)atrace[10].action_ordinal, 11);
   BOOST_REQUIRE_EQUAL((int)atrace[10].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[10].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[10].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[10].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[10].act.name, TEST_METHOD("test_action", "test_action_ordinal4"));
   BOOST_REQUIRE_EQUAL(atrace[10].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->return_value.valid(), true); // return value not set is still a return value, it is just empty
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->return_value->size(), 0 );   // state_history_plugin keys off presence of return_value for version of receipt
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->global_sequence, start_gseq + 7);
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
+ * action_ordinal_failtest1 test cases
+ *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_ordinal_failtest1, TESTER) { try {

   produce_blocks(1);
   create_account(N(testapi) );
   set_code( N(testapi), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(bob) );
   set_code( N(bob), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(charlie) );
   set_code( N(charlie), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(david) );
   set_code( N(david), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(erin) );
   set_code( N(erin), contracts::test_api_wasm() );
   produce_blocks(1);

   create_account(N(fail1) ); // <- make first action fails in the middle
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
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[0].except.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[0].except->code(), 3050003);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[1].except.valid(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[2].except.valid(), false);

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
+ * action_ordinal_failtest2 test cases
+ *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_ordinal_failtest2, TESTER) { try {

   produce_blocks(1);
   create_account(N(testapi) );
   set_code( N(testapi), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(bob) );
   set_code( N(bob), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(charlie) );
   set_code( N(charlie), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(david) );
   set_code( N(david), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(erin) );
   set_code( N(erin), contracts::test_api_wasm() );
   produce_blocks(1);

   create_account(N(fail3) ); // <- make action 3 fails in the middle
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
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[0].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(*atrace[0].receipt->return_value), unsigned_int(1) );
   BOOST_REQUIRE_EQUAL(atrace[0].except.valid(), false);
   int start_gseq = atrace[0].receipt->global_sequence;

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(*atrace[1].receipt->return_value), "bob" );
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->global_sequence, start_gseq + 1);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[2].except.valid(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[3].action_ordinal, 4);
   BOOST_REQUIRE_EQUAL((int)atrace[3].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[3].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[3].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[3].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[3].act.name, TEST_METHOD("test_action", "test_action_ordinal3"));
   BOOST_REQUIRE_EQUAL(atrace[3].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[3].except.valid(), false);

   // hey exception is here
   BOOST_REQUIRE_EQUAL((int)atrace[4].action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[4].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[4].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[4].receiver, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[4].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[4].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[4].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[4].except.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[4].except->code(), 3050003);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[5].action_ordinal, 6);
   BOOST_REQUIRE_EQUAL((int)atrace[5].creator_action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[5].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[5].receiver, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[5].act.account, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[5].act.name, TEST_METHOD("test_action", "test_action_ordinal_foo"));
   BOOST_REQUIRE_EQUAL(atrace[5].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[5].except.valid(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[6].action_ordinal, 7);
   BOOST_REQUIRE_EQUAL((int)atrace[6].creator_action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[6].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[6].receiver, N(david));
   BOOST_REQUIRE_EQUAL(atrace[6].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[6].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[6].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[6].except.valid(), false);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[7].action_ordinal, 8);
   BOOST_REQUIRE_EQUAL((int)atrace[7].creator_action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[7].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[7].receiver, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[7].act.account, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[7].act.name, TEST_METHOD("test_action", "test_action_ordinal_bar"));
   BOOST_REQUIRE_EQUAL(atrace[7].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[7].except.valid(), false);

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
+ * action_ordinal_failtest3 test cases
+ *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_ordinal_failtest3, TESTER) { try {

   produce_blocks(1);
   create_account(N(testapi) );
   set_code( N(testapi), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(bob) );
   set_code( N(bob), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(charlie) );
   set_code( N(charlie), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(david) );
   set_code( N(david), contracts::test_api_wasm() );
   produce_blocks(1);
   create_account(N(erin) );
   set_code( N(erin), contracts::test_api_wasm() );
   produce_blocks(1);

   create_account(N(failnine) ); // <- make action 9 fails in the middle
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
   BOOST_REQUIRE_EQUAL(atrace[0].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[0].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[0].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[0].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<unsigned_int>(*atrace[0].receipt->return_value), unsigned_int(1) );
   BOOST_REQUIRE_EQUAL(atrace[0].except.valid(), false);
   int start_gseq = atrace[0].receipt->global_sequence;

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[1].action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[1].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[1].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[1].receiver, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[1].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[1].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[1].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(*atrace[1].receipt->return_value), "bob" );
   BOOST_REQUIRE_EQUAL(atrace[1].receipt->global_sequence, start_gseq + 1);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[2].action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[2].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[2].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[2].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[2].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[2].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[2].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<name>(*atrace[2].receipt->return_value), name("five") );
   BOOST_REQUIRE_EQUAL(atrace[2].receipt->global_sequence, start_gseq + 4);

   // fails here
   BOOST_REQUIRE_EQUAL((int)atrace[3].action_ordinal, 4);
   BOOST_REQUIRE_EQUAL((int)atrace[3].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[3].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[3].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[3].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[3].act.name, TEST_METHOD("test_action", "test_action_ordinal3"));
   BOOST_REQUIRE_EQUAL(atrace[3].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[3].except.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[3].except->code(), 3050003);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[4].action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[4].creator_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL((int)atrace[4].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[4].receiver, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[4].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[4].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[4].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[4].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(*atrace[4].receipt->return_value), "charlie" );
   BOOST_REQUIRE_EQUAL(atrace[4].receipt->global_sequence, start_gseq + 2);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[5].action_ordinal, 6);
   BOOST_REQUIRE_EQUAL((int)atrace[5].creator_action_ordinal, 2);
   BOOST_REQUIRE_EQUAL((int)atrace[5].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[5].receiver, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[5].act.account, N(bob));
   BOOST_REQUIRE_EQUAL(atrace[5].act.name, TEST_METHOD("test_action", "test_action_ordinal_foo"));
   BOOST_REQUIRE_EQUAL(atrace[5].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[5].except.valid(), false);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[6].action_ordinal, 7);
   BOOST_REQUIRE_EQUAL((int)atrace[6].creator_action_ordinal,2);
   BOOST_REQUIRE_EQUAL((int)atrace[6].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[6].receiver, N(david));
   BOOST_REQUIRE_EQUAL(atrace[6].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[6].act.name, TEST_METHOD("test_action", "test_action_ordinal1"));
   BOOST_REQUIRE_EQUAL(atrace[6].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[6].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<std::string>(*atrace[6].receipt->return_value), "david" );
   BOOST_REQUIRE_EQUAL(atrace[6].receipt->global_sequence, start_gseq + 3);

   // not executed
   BOOST_REQUIRE_EQUAL((int)atrace[7].action_ordinal, 8);
   BOOST_REQUIRE_EQUAL((int)atrace[7].creator_action_ordinal, 5);
   BOOST_REQUIRE_EQUAL((int)atrace[7].closest_unnotified_ancestor_action_ordinal, 1);
   BOOST_REQUIRE_EQUAL(atrace[7].receiver, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[7].act.account, N(charlie));
   BOOST_REQUIRE_EQUAL(atrace[7].act.name, TEST_METHOD("test_action", "test_action_ordinal_bar"));
   BOOST_REQUIRE_EQUAL(atrace[7].receipt.valid(), false);
   BOOST_REQUIRE_EQUAL(atrace[7].except.valid(), false);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[8].action_ordinal, 9);
   BOOST_REQUIRE_EQUAL((int)atrace[8].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[8].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[8].receiver, N(david));
   BOOST_REQUIRE_EQUAL(atrace[8].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[8].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[8].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[8].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<bool>(*atrace[8].receipt->return_value), true );
   BOOST_REQUIRE_EQUAL(atrace[8].receipt->global_sequence, start_gseq + 5);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[9].action_ordinal, 10);
   BOOST_REQUIRE_EQUAL((int)atrace[9].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[9].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[9].receiver, N(erin));
   BOOST_REQUIRE_EQUAL(atrace[9].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[9].act.name, TEST_METHOD("test_action", "test_action_ordinal2"));
   BOOST_REQUIRE_EQUAL(atrace[9].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[9].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(fc::raw::unpack<signed_int>(*atrace[9].receipt->return_value), signed_int(7) );
   BOOST_REQUIRE_EQUAL(atrace[9].receipt->global_sequence, start_gseq + 6);

   // executed
   BOOST_REQUIRE_EQUAL((int)atrace[10].action_ordinal, 11);
   BOOST_REQUIRE_EQUAL((int)atrace[10].creator_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL((int)atrace[10].closest_unnotified_ancestor_action_ordinal, 3);
   BOOST_REQUIRE_EQUAL(atrace[10].receiver, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[10].act.account, N(testapi));
   BOOST_REQUIRE_EQUAL(atrace[10].act.name, TEST_METHOD("test_action", "test_action_ordinal4"));
   BOOST_REQUIRE_EQUAL(atrace[10].receipt.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->return_value.valid(), true);
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->return_value->size(), 0 );
   BOOST_REQUIRE_EQUAL(atrace[10].receipt->global_sequence, start_gseq + 7);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
