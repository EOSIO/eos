/**
 *  @file api_tests.cpp
 *  @copyright defined in eos/LICENSE.txt
 */
#include <algorithm>
#include <random>
#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>
#include <numeric>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/wasm_interface.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <test_api/test_api.wast.hpp>
#include <test_api_mem/test_api_mem.wast.hpp>
#include <test_api_db/test_api_db.wast.hpp>
#include <test_api_multi_index/test_api_multi_index.wast.hpp>
#include <test_api/test_api.hpp>

FC_REFLECT( dummy_action, (a)(b)(c) );
FC_REFLECT( u128_action, (values) );

using namespace eosio;
using namespace eosio::testing;
using namespace chain;
using namespace chain::contracts;
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

FC_REFLECT_TEMPLATE((uint64_t T), test_api_action<T>, BOOST_PP_SEQ_NIL);

template<uint64_t NAME>
struct test_chain_action {
	static account_name get_account() {
		return account_name(config::system_account_name);
	}

	static action_name get_name() {
		return action_name(NAME);
	}
};

FC_REFLECT_TEMPLATE((uint64_t T), test_chain_action<T>, BOOST_PP_SEQ_NIL);



bool expect_assert_message(const fc::exception& ex, string expected) {
   BOOST_TEST_MESSAGE("LOG : " << "expected: " << expected << ", actual: " << ex.get_log().at(0).get_message());
   return (ex.get_log().at(0).get_message().find(expected) != std::string::npos);
}

struct assert_message_is {
	bool operator()( const fc::exception& ex, string expected) {
		auto act = ex.get_log().at(0).get_message();
		return boost::algorithm::ends_with(act, expected);
	}

	string expected;
};

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
void CallAction(tester& test, T ac, const vector<account_name>& scope = {N(testapi)}) {
   signed_transaction trx;

   auto pl = vector<permission_level>{{scope[0], config::active_name}};
   if (scope.size() > 1)
      for (int i = 1; i < scope.size(); i++)
         pl.push_back({scope[i], config::active_name});

   action act(pl, ac);
   trx.actions.push_back(act);

   test.set_tapos(trx);
   auto sigs = trx.sign(test.get_private_key(scope[0], "active"), chain_id_type());
   trx.get_signature_keys(chain_id_type());
   auto res = test.push_transaction(trx);
   BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
   test.produce_block();
}

template <typename T>
void CallFunction(tester& test, T ac, const vector<char>& data, const vector<account_name>& scope = {N(testapi)}) {
	{
		signed_transaction trx;

      auto pl = vector<permission_level>{{scope[0], config::active_name}};
      if (scope.size() > 1)
         for (unsigned int i=1; i < scope.size(); i++)
            pl.push_back({scope[i], config::active_name});

      action act(pl, ac);
      act.data = data;
      trx.actions.push_back(act);

		test.set_tapos(trx);
		auto sigs = trx.sign(test.get_private_key(scope[0], "active"), chain_id_type());
      trx.get_signature_keys(chain_id_type() );
		auto res = test.push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
		test.produce_block();
	}
}

#define CALL_TEST_FUNCTION(TESTER, CLS, MTH, DATA) CallFunction(TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)
#define CALL_TEST_FUNCTION_SYSTEM(TESTER, CLS, MTH, DATA) CallFunction(TESTER, test_chain_action<TEST_METHOD(CLS, MTH)>{}, DATA, {N(eosio)} )
#define CALL_TEST_FUNCTION_SCOPE(TESTER, CLS, MTH, DATA, ACCOUNT) CallFunction(TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA, ACCOUNT)

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
bool is_access_violation(const Runtime::Exception& e) {
   return true;
}
bool is_assert_exception(fc::assert_exception const & e) { return true; }
bool is_page_memory_error(page_memory_error const &e) { return true; }
bool is_tx_missing_auth(tx_missing_auth const & e) { return true; }
bool is_tx_missing_recipient(tx_missing_recipient const & e) { return true;}
bool is_tx_missing_sigs(tx_missing_sigs const & e) { return true;}
bool is_wasm_execution_error(eosio::chain::wasm_execution_error const& e) {return true;}
bool is_tx_resource_exhausted(const tx_resource_exhausted& e) { return true; }
bool is_checktime_exceeded(const checktime_exceeded& e) { return true; }


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

#define CAPTURE(STREAM, EXEC) \
   {\
      capture.clear(); \
      bio::stream_buffer<MySink> sb; sb.open(MySink()); \
      std::streambuf *oldbuf = std::STREAM.rdbuf(&sb); \
      EXEC; \
      std::STREAM.rdbuf(oldbuf); \
   }

#define CAPTURE_AND_PRE_TEST_PRINT(METHOD) \
	{ \
		BOOST_TEST_MESSAGE( "Running test_print::" << METHOD ); \
		CAPTURE( cerr, CALL_TEST_FUNCTION( *this, "test_print", METHOD, {} ) ); \
		BOOST_CHECK_EQUAL( capture.size(), 7 ); \
		captured = capture[3]; \
	}

/*************************************************************************************
 * action_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi) );
	create_account( N(acc1) );
	create_account( N(acc2) );
	create_account( N(acc3) );
	create_account( N(acc4) );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

   // test assert_true
	CALL_TEST_FUNCTION( *this, "test_action", "assert_true", {});

   //test assert_false
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "assert_false", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "test_action::assert_false");
         }
      );

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
   auto test_require_notice = [](auto& test, std::vector<char>& data, std::vector<account_name>& scope){
      signed_transaction trx;
      auto tm = test_api_action<TEST_METHOD("test_action", "require_notice")>{};

      action act(std::vector<permission_level>{{N(testapi), config::active_name}}, tm);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(data.begin(), data.end(), std::back_inserter(dest));
      trx.actions.push_back(act);

		test.set_tapos(trx);
		trx.sign(test.get_private_key(N(inita), "active"), chain_id_type());
		auto res = test.push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
   };
   BOOST_CHECK_EXCEPTION(test_require_notice(*this, raw_bytes, scope), tx_missing_sigs,
         [](const tx_missing_sigs& e) {
            return expect_assert_message(e, "transaction declares authority");
         }
      );

   // test require_auth
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", {}), tx_missing_auth,
         [](const tx_missing_auth& e) {
            return expect_assert_message(e, "missing authority of");
         }
      );

   // test require_auth
   auto a3only = std::vector<permission_level>{{N(acc3), config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a3only)), tx_missing_auth,
         [](const tx_missing_auth& e) {
            return expect_assert_message(e, "missing authority of");
         }
      );

   // test require_auth
   auto a4only = std::vector<permission_level>{{N(acc4), config::active_name}};
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "require_auth", fc::raw::pack(a4only)), tx_missing_auth,
         [](const tx_missing_auth& e) {
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
      trx.actions.push_back(act);

		set_tapos(trx);
		trx.sign(get_private_key(N(acc3), "active"), chain_id_type());
		trx.sign(get_private_key(N(acc4), "active"), chain_id_type());
		auto res = push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
   }

   uint32_t now = control->head_block_time().sec_since_epoch();
   CALL_TEST_FUNCTION( *this, "test_action", "now", fc::raw::pack(now));

   // test now
   produce_block();
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "now", fc::raw::pack(now)), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "assertion failed: tmp == now");
         }
      );

   // test test_current_receiver
   CALL_TEST_FUNCTION( *this, "test_action", "test_current_receiver", fc::raw::pack(N(testapi)));

   // test send_action_sender
   CALL_TEST_FUNCTION( *this, "test_transaction", "send_action_sender", fc::raw::pack(N(testapi)));
   control->push_deferred_transactions( true );

   // test_publication_time
   uint32_t pub_time = control->head_block_time().sec_since_epoch();
   CALL_TEST_FUNCTION( *this, "test_action", "test_publication_time", fc::raw::pack(pub_time) );

   // test test_abort
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "test_abort", {} ), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "abort() called");
         }
      );

   dummy_action da = { DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C };
   CallAction(*this, da);
  
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * context free action tests
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(cf_action_tests, tester) { try {
      produce_blocks(2);
      create_account( N(testapi) );
      produce_blocks(1000);
      set_code( N(testapi), test_api_wast );
      produce_blocks(1);

      cf_action cfa;
      signed_transaction trx;
      action act({}, cfa);
      trx.context_free_actions.push_back(act);
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100)); // verify payload matches context free data
      trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
      set_tapos(trx);

      // signing a transaction with only context_free_actions should not be allowed
      auto sigs = trx.sign(get_private_key(N(testapi), "active"), chain_id_type());
      BOOST_CHECK_EXCEPTION(push_transaction(trx), tx_irrelevant_sig,
                            [](const fc::assert_exception& e) {
                               return expect_assert_message(e, "signatures");
                            }
      );

      // clear signatures, so should now pass
      trx.signatures.clear();
      auto res = push_transaction(trx);
      BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);

      // add a normal action along with cfa
      dummy_action da = { DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C };
      auto pl = vector<permission_level>{{N(testapi), config::active_name}};
      action act1(pl, da);
      trx.actions.push_back(act1);
      // run normal passing case
      sigs = trx.sign(get_private_key(N(testapi), "active"), chain_id_type());
      res = push_transaction(trx);
      BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);

      // attempt to access context free api in non context free action
      da = { DUMMY_ACTION_DEFAULT_A, 200, DUMMY_ACTION_DEFAULT_C };
      action act2(pl, da);
      trx.signatures.clear();
      trx.actions.clear();
      trx.actions.push_back(act2);
      // run normal passing case
      sigs = trx.sign(get_private_key(N(testapi), "active"), chain_id_type());
      BOOST_CHECK_EXCEPTION(push_transaction(trx), fc::assert_exception,
                            [](const fc::assert_exception& e) {
                               return expect_assert_message(e, "may only be called from context_free");
                            }
      );

      // back to normal action
      trx.signatures.clear();
      trx.actions.clear();
      trx.actions.push_back(act1);

      // attempt to access non context free api
      for (uint32_t i = 200; i <= 204; ++i) {
         trx.context_free_actions.clear();
         trx.context_free_data.clear();
         cfa.payload = i;
         cfa.cfd_idx = 1;
         action cfa_act({}, cfa);
         trx.context_free_actions.emplace_back(cfa_act);
         trx.signatures.clear();
         sigs = trx.sign(get_private_key(N(testapi), "active"), chain_id_type());
         BOOST_CHECK_EXCEPTION(push_transaction(trx), fc::assert_exception,
              [](const fc::assert_exception& e) {
                 return expect_assert_message(e, "context_free: only context free api's can be used in this context");
              }
         );
      }

      produce_block();

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * checktime_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(checktime_pass_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi) );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

   // test checktime_pass
   CALL_TEST_FUNCTION( *this, "test_checktime", "checktime_pass", {});

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(checktime_fail_tests) {
   try {
      tester t( {fc::milliseconds(1), fc::milliseconds(1)} );
      t.produce_blocks(2);

      t.create_account( N(testapi) );
      t.set_code( N(testapi), test_api_wast );

   auto call_test = [](tester& test, auto ac) {
		signed_transaction trx;

      auto pl = vector<permission_level>{{N(testapi), config::active_name}};
      action act(pl, ac);

      trx.actions.push_back(act);
		test.set_tapos(trx);
		auto sigs = trx.sign(test.get_private_key(N(testapi), "active"), chain_id_type());
      trx.get_signature_keys(chain_id_type() );
		auto res = test.push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
		test.produce_block();
	};

   BOOST_CHECK_EXCEPTION(call_test( t, test_api_action<TEST_METHOD("test_checktime", "checktime_failure")>{}), checktime_exceeded, is_checktime_exceeded);

   } FC_LOG_AND_RETHROW();
}
/*************************************************************************************
 * compiler_builtins_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(compiler_builtins_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi) );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

   // test test_multi3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_multi3", {});

   // test test_divti3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_divti3", {});

   // test test_divti3_by_0
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_divti3_by_0", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "divide by zero");
         }
      );

   // test test_udivti3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_udivti3", {});

   // test test_udivti3_by_0
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_udivti3_by_0", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "divide by zero");
         }
      );

   // test test_modti3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_modti3", {});

   // test test_modti3_by_0
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_modti3_by_0", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "divide by zero");
         }
      );

   // test test_lshlti3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_lshlti3", {});

   // test test_lshrti3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_lshrti3", {});

   // test test_ashlti3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_ashlti3", {});

   // test test_ashrti3
   CALL_TEST_FUNCTION( *this, "test_compiler_builtins", "test_ashrti3", {});
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * transaction_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(transaction_tests, tester) { try {
   produce_blocks(2);
   create_account( N(testapi) );
   produce_blocks(100);
   set_code( N(testapi), test_api_wast );
   produce_blocks(1);

   // test send_action
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action", {});

   // test send_action_empty
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_empty", {});

   // test send_action_large
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_large", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "abort()");
            //return expect_assert_message(e, "data_len < config::default_max_inline_action_size: inline action too big");
         }
      );

   // test send_action_inline_fail
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_inline_fail", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "test_action::assert_false");
         }
      );
   control->push_deferred_transactions( true );

   // test send_transaction
   CALL_TEST_FUNCTION(*this, "test_transaction", "send_transaction", {});
   control->push_deferred_transactions( true );

   // test send_transaction_empty
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_transaction_empty", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "transaction must have at least one action");
         }
      );
   control->push_deferred_transactions( true );

   // test test_transaction_size
   CALL_TEST_FUNCTION(*this, "test_transaction", "test_transaction_size", fc::raw::pack(56) );
   control->push_deferred_transactions( true );

   // test test_read_transaction
   // this is a bit rough, but I couldn't figure out a better way to compare the hashes
   CAPTURE( cerr, CALL_TEST_FUNCTION( *this, "test_transaction", "test_read_transaction", {} ) );
   BOOST_CHECK_EQUAL( capture.size(), 7 );
   string sha_expect = "bdeb5b58dda272e4b23ee7d2a5f0ff034820c156364893b758892e06fa39e7fe";
   BOOST_CHECK_EQUAL(capture[3] == sha_expect, true);
   // test test_tapos_block_num
   CALL_TEST_FUNCTION(*this, "test_transaction", "test_tapos_block_num", fc::raw::pack(control->head_block_num()) );

   // test test_tapos_block_prefix
   CALL_TEST_FUNCTION(*this, "test_transaction", "test_tapos_block_prefix", fc::raw::pack(control->head_block_id()._hash[1]) );

   // test send_action_recurse
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_transaction", "send_action_recurse", {}), eosio::chain::transaction_exception,
         [](const eosio::chain::transaction_exception& e) {
            return expect_assert_message(e, "inline action recursion depth reached");
         }
      );

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
BOOST_FIXTURE_TEST_CASE(chain_tests, tester) { try {
	produce_blocks(2);
   create_account( N(inita) );
   create_account( N(initb) );
   create_account( N(initc) );
   create_account( N(initd) );
   create_account( N(inite) );
   create_account( N(initf) );
   create_account( N(initg) );
   create_account( N(inith) );
   create_account( N(initi) );
   create_account( N(initj) );
   create_account( N(initk) );
   create_account( N(initl) );
   create_account( N(initm) );
   create_account( N(initn) );
   create_account( N(inito) );
   create_account( N(initp) );
   create_account( N(initq) );
   create_account( N(initr) );
   create_account( N(inits) );
   create_account( N(initt) );
   create_account( N(initu) );
   create_account( N(initv) );

	create_account( N(testapi) );

   // set active producers
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

		auto sigs = trx.sign(get_private_key(config::system_account_name, "active"), chain_id_type());
      trx.get_signature_keys(chain_id_type() );
		auto res = push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
	}

	set_code( N(testapi), test_api_wast );
	produce_blocks(100);
   auto& gpo = control->get_global_properties();
   std::vector<account_name> prods(gpo.active_producers.producers.size());
   for ( unsigned int i=0; i < gpo.active_producers.producers.size(); i++ ) {
      prods[i] = gpo.active_producers.producers[i].producer_name;
   }

	CALL_TEST_FUNCTION( *this, "test_chain", "test_activeprods", fc::raw::pack(prods));
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * db_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(db_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi) );
	produce_blocks(1000);
	set_code( N(testapi), test_api_db_wast );
	produce_blocks(1);

	CALL_TEST_FUNCTION( *this, "test_db", "primary_i64_general", {});
	CALL_TEST_FUNCTION( *this, "test_db", "primary_i64_lowerbound", {});
	CALL_TEST_FUNCTION( *this, "test_db", "primary_i64_upperbound", {});
	CALL_TEST_FUNCTION( *this, "test_db", "idx64_general", {});
	CALL_TEST_FUNCTION( *this, "test_db", "idx64_lowerbound", {});
	CALL_TEST_FUNCTION( *this, "test_db", "idx64_upperbound", {});
   /*
	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_general", {});
	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_remove_all", {});
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_db", "key_i64_small_load", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "Data is not long enough to contain keys");
         }
      );

	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_db", "key_i64_small_store", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "Data is not long enough to contain keys");
         }
      );

	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_store_scope", {});
	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_remove_scope", {});
	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_not_found", {});
	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_front_back", {});
	//CALL_TEST_FUNCTION( *this, "test_db", "key_i64i64i64_general", {});
	CALL_TEST_FUNCTION( *this, "test_db", "key_i128i128_general", {});
   */
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * multi_index_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(multi_index_tests, tester) { try {
   produce_blocks(1);
   create_account( N(testapi) );
   produce_blocks(1);
   set_code( N(testapi), test_api_multi_index_wast );
   produce_blocks(1);

   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx64_general", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx64_store_only", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx64_check_without_storing", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx128_general", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx128_store_only", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx128_check_without_storing", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx128_autoincrement_test", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx128_autoincrement_test_part1", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx128_autoincrement_test_part2", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx256_general", {});
   CALL_TEST_FUNCTION( *this, "test_multi_index", "idx_double_general", {});
} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * fixedpoint_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(fixedpoint_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi) );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "create_instances", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_addition", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_subtraction", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_multiplication", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_division", {});
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_division_by_0", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "divide by zero");
         }
      );

} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * real_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(real_tests, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);

   CALL_TEST_FUNCTION( *this, "test_real", "create_instances", {} );
   CALL_TEST_FUNCTION( *this, "test_real", "test_addition", {} );
   CALL_TEST_FUNCTION( *this, "test_real", "test_multiplication", {} );
   CALL_TEST_FUNCTION( *this, "test_real", "test_division", {} );
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_real", "test_division_by_0", {}), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "divide by zero");
         }
      );


} FC_LOG_AND_RETHROW() }



/*************************************************************************************
 * crypto_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(crypto_tests, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
	{
		signed_transaction trx;

      auto pl = vector<permission_level>{{N(testapi), config::active_name}};

      action act(pl, test_api_action<TEST_METHOD("test_crypto", "test_recover_key")>{});
		auto signatures = trx.sign(get_private_key(N(testapi), "active"), chain_id_type());

		produce_block();

      auto payload   = fc::raw::pack( trx.sig_digest( chain_id_type() ) );
      auto pk     = fc::raw::pack( get_public_key( N(testapi), "active" ) );
      auto sigs   = fc::raw::pack( signatures );
      payload.insert( payload.end(), pk.begin(), pk.end() );
      payload.insert( payload.end(), sigs.begin(), sigs.end() );

      CALL_TEST_FUNCTION( *this, "test_crypto", "test_recover_key", payload );
      CALL_TEST_FUNCTION( *this, "test_crypto", "test_recover_key_assert_true", payload );
      payload[payload.size()-1] = 0;
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_crypto", "test_recover_key_assert_false", payload ), fc::assert_exception,
            [](const fc::assert_exception& e) {
               return expect_assert_message( e, "check == p: Error expected key different than recovered key" );
            }
         );
	}

   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha1", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha256", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha512", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_ripemd160", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha1_no_data", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha256_no_data", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha512_no_data", {} );
   CALL_TEST_FUNCTION( *this, "test_crypto", "ripemd160_no_data", {} );

   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha256_false", {} ), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "hash miss match");
         }
      );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha256_true", {} );

   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha1_false", {} ), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "hash miss match");
         }
      );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha1_true", {} );

   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha1_false", {} ), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "hash miss match");
         }
      );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha1_true", {} );

   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha512_false", {} ), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "hash miss match");
         }
      );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_sha512_true", {} );

   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_crypto", "assert_ripemd160_false", {} ), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "hash miss match");
         }
      );

   CALL_TEST_FUNCTION( *this, "test_crypto", "assert_ripemd160_true", {} );

} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * memory_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(memory_tests, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);

   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_allocs", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunk", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunks", {} );
   produce_blocks(1000);
   //Disabling this for now as it fails due to malloc changes for variable wasm max memory sizes
#if 0
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunks_disjoint", {} );
   produce_blocks(1000);
#endif
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memset_memcpy", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcpy_overlap_start", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcpy_overlap_end", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcmp", {} );
} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * extended_memory_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(extended_memory_test_initial_memory, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_initial_buffer", {} );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory", {} );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory_exceeded, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory_exceeded", {} );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory_negative_bytes, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi) );
   produce_blocks(1000);
   set_code(N(testapi), test_api_mem_wast);
   produce_blocks(1000);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory_negative_bytes", {} ),
      page_memory_error, is_page_memory_error);

} FC_LOG_AND_RETHROW() }

/*************************************************************************************
 * print_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(print_tests, tester) { try {
	produce_blocks(2);
	create_account(N(testapi) );
	produce_blocks(1000);

	set_code(N(testapi), test_api_wast);
	produce_blocks(1000);
	string captured = "";

	// test prints
	CAPTURE_AND_PRE_TEST_PRINT("test_prints");
	BOOST_CHECK_EQUAL(captured == "abcefg", true);

	// test prints_l
	CAPTURE_AND_PRE_TEST_PRINT("test_prints_l");
	BOOST_CHECK_EQUAL(captured == "abatest", true);

	// test printi
	CAPTURE_AND_PRE_TEST_PRINT("test_printi");
	BOOST_CHECK_EQUAL( captured.substr(0,1), I64Str(0) );
	BOOST_CHECK_EQUAL( captured.substr(1,6), I64Str(556644) );
	BOOST_CHECK_EQUAL( captured.substr(7, capture[3].size()), I64Str(-1) );

	// test printui
	CAPTURE_AND_PRE_TEST_PRINT("test_printui");
	BOOST_CHECK_EQUAL( captured.substr(0,1), U64Str(0) );
	BOOST_CHECK_EQUAL( captured.substr(1,6), U64Str(556644) );
	BOOST_CHECK_EQUAL( captured.substr(7, capture[3].size()), U64Str(-1) ); // "18446744073709551615"

	// test printn
	CAPTURE_AND_PRE_TEST_PRINT("test_printn");
	BOOST_CHECK_EQUAL( captured.substr(0,5), "abcde" );
	BOOST_CHECK_EQUAL( captured.substr(5, 5), "ab.de" );
	BOOST_CHECK_EQUAL( captured.substr(10, 6), "1q1q1q");
	BOOST_CHECK_EQUAL( captured.substr(16, 11), "abcdefghijk");
	BOOST_CHECK_EQUAL( captured.substr(27, 12), "abcdefghijkl");
	BOOST_CHECK_EQUAL( captured.substr(39, 13), "abcdefghijkl1");
	BOOST_CHECK_EQUAL( captured.substr(52, 13), "abcdefghijkl1");
	BOOST_CHECK_EQUAL( captured.substr(65, 13), "abcdefghijkl1");

	// test printi128
	CAPTURE_AND_PRE_TEST_PRINT("test_printi128");
	BOOST_CHECK_EQUAL( captured.substr(0, 39), U128Str(-1) );
	BOOST_CHECK_EQUAL( captured.substr(39, 1), U128Str(0) );
	BOOST_CHECK_EQUAL( captured.substr(40, 11), U128Str(87654323456) );

} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * math_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(math_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi) );
	produce_blocks(1000);

	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

   std::random_device rd;
   std::mt19937_64 gen(rd());
   std::uniform_int_distribution<unsigned long long> dis;

   // test mult_eq with 10 random pairs of 128 bit numbers
   for (int i=0; i < 10; i++) {
      u128_action act;
      act.values[0] = dis(gen); act.values[0] <<= 64; act.values[0] |= dis(gen);
      act.values[1] = dis(gen); act.values[1] <<= 64; act.values[1] |= dis(gen);
      act.values[2] = act.values[0] * act.values[1];
      CALL_TEST_FUNCTION( *this, "test_math", "test_multeq", fc::raw::pack(act));
   }
   // test div_eq with 10 random pairs of 128 bit numbers
   for (int i=0; i < 10; i++) {
      u128_action act;
      act.values[0] = dis(gen); act.values[0] <<= 64; act.values[0] |= dis(gen);
      act.values[1] = dis(gen); act.values[1] <<= 64; act.values[1] |= dis(gen);
      act.values[2] = act.values[0] / act.values[1];
      CALL_TEST_FUNCTION( *this, "test_math", "test_diveq", fc::raw::pack(act));
   }
   // test diveq for divide by zero
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_math", "test_diveq_by_0", {}), fc::assert_exception,
          [](const fc::assert_exception& e) {
            return expect_assert_message(e, "divide by zero");
         }
      );

	CALL_TEST_FUNCTION( *this, "test_math", "test_double_api", {});

   union {
      char whole[32];
      double _[4] = {2, -2, 100000, -100000};
   } d_vals;

   std::vector<char> ds(sizeof(d_vals));
   std::copy(d_vals.whole, d_vals.whole+sizeof(d_vals), ds.begin());
   CALL_TEST_FUNCTION( *this, "test_math", "test_i64_to_double", ds);

   std::copy(d_vals.whole, d_vals.whole+sizeof(d_vals), ds.begin());
   CALL_TEST_FUNCTION( *this, "test_math", "test_double_to_i64", ds);

	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_math", "test_double_api_div_0", {}), fc::assert_exception,
          [](const fc::assert_exception& e) {
            return expect_assert_message(e, "divide by zero");
         }
      );

} FC_LOG_AND_RETHROW() }


/*************************************************************************************
 * types_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(types_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi) );

	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

	CALL_TEST_FUNCTION( *this, "test_types", "types_size", {});
	CALL_TEST_FUNCTION( *this, "test_types", "char_to_symbol", {});
	CALL_TEST_FUNCTION( *this, "test_types", "string_to_name", {});
	CALL_TEST_FUNCTION( *this, "test_types", "name_class", {});
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
	set_code( N(testapi), test_api_wast );
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

		auto sigs = trx.sign(get_private_key(config::system_account_name, "active"), chain_id_type());
      trx.get_signature_keys(chain_id_type() );
		auto res = push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
	}

   CALL_TEST_FUNCTION( *this, "test_privileged", "test_is_privileged", {} );
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_privileged", "test_is_privileged", {} ), fc::assert_exception,
         [](const fc::assert_exception& e) {
            return expect_assert_message(e, "context.privileged: testapi does not have permission to call this API");
         }
       );

} FC_LOG_AND_RETHROW() }
#endif

BOOST_AUTO_TEST_SUITE_END()
