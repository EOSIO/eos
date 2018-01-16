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

#include <boost/test/unit_test.hpp>
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

//TODO this should be in eosio not eos
#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

//#include "../common/database_fixture.hpp"

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <test_api/test_api.wast.hpp>
#include <test_api/test_api.hpp>


#include <eosio/chain/staked_balance_objects.hpp>
//#include <eosio/chain/balance_object.hpp>

FC_REFLECT( dummy_action, (a)(b)(c) );
FC_REFLECT( u128_msg, (values) );

using namespace eosio;
using namespace eosio::testing;
using namespace chain;
using namespace chain::contracts;
using namespace fc;

namespace bio = boost::iostreams;

template<uint64_t NAME>
struct test_api_action {
	static scope_name get_scope() {
		return N(testapi);
	}

	static action_name get_name() {
		return action_name(NAME);
	}
};
FC_REFLECT_TEMPLATE((uint64_t T), test_api_action<T>, BOOST_PP_SEQ_NIL);

struct assert_message_is {
	assert_message_is(string expected) 
		: expected(expected) {}
	
	bool operator()( const fc::assert_exception& ex) {
		auto msg = ex.get_log().at(0).get_message();
		return boost::algorithm::ends_with(msg, expected);
	}

	string expected;
};

constexpr uint64_t TEST_METHOD(const char* CLASS, const char *METHOD) {
  //std::cerr << CLASS << "::" << METHOD << std::endl;
  return ( (uint64_t(DJBH(CLASS))<<32) | uint32_t(DJBH(METHOD)) );
}

string U64Str(uint64_t i)
{
	std::stringstream ss;
	ss << i;
	return ss.str();
}


// run the `METHOD` and capture it's output then check some common parts that all the test_print methods have
#define CAPTURE_AND_PRE_TEST_PRINT(METHOD) \
	{ \
		auto U64Str = [](uint64_t v) -> std::string { std::stringstream ss; ss << v; return ss.str(); }; \
		BOOST_TEST_MESSAGE( "Running test_print::" << METHOD ); \
		CAPTURE( cerr, CALL_TEST_FUNCTION( *this, "test_print", METHOD, {} ) ); \
		BOOST_CHECK_EQUAL( capture.size(), 7 ); \
		captured = capture[3]; \
	}

BOOST_AUTO_TEST_SUITE(api_tests)


/*
vector<uint8_t> assemble_wast( const std::string& wast ) {
   //   std::cout << "\n" << wast << "\n";
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << "Error parsing WebAssembly text file:" << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_ASSERT( !"error parsing wast" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << "Error serializing WebAssembly binary file:" << std::endl;
      std::cerr << exception.message << std::endl;
      throw;
   }
}
*/
}

template <typename T>
uint32_t CallFunction(tester& test, T tm, const vector<char>& data, const vector<account_name>& scope = {N(testapi)}) {
	static int expiration = 1;
	{
		signed_transaction trx;
		trx.write_scope = scope;
      
      //vector<char>& dest = *(vector<char> *)(&
      action act(vector<permission_level>{{scope[0], config::active_name}}, tm);
      vector<char>& dest = *(vector<char> *)(&act.data);
      std::copy(data.begin(), data.end(), std::back_inserter(dest));
      trx.actions.push_back(act);
		//trx.actions.emplace_back(vector<permission_level>{{scope[0], config::active_name}},
	//									 tm);
		test.set_tapos(trx);
		trx.sign(test.get_private_key(scope[0], "active"), chain_id_type());
		auto res = test.control->push_transaction(trx);
		BOOST_CHECK_EQUAL(res.status, transaction_receipt::executed);
		//BOOST_TEST_MESSAGE("ACTION TRACE SIZE : " << res.action_traces.size());
//		BOOST_TEST_MESSAGE("ACTION TRACE RECEIVER : " << res.action_traces.at(0).receiver.to_string());
//		BOOST_TEST_MESSAGE("ACTION TRACE SCOPE : " << res.action_traces.at(0).act.scope.to_string());
//		BOOST_TEST_MESSAGE("ACTION TRACE NAME : " << res.action_traces.at(0).act.name.to_string());
//		BOOST_TEST_MESSAGE("ACTION TRACE AUTH SIZE : " << res.action_traces.at(0).act.authorization.size());
//		BOOST_TEST_MESSAGE("ACTION TRACE ACTOR : " << res.action_traces.at(0).act.authorization.at(0).actor.to_string());
//		BOOST_TEST_MESSAGE("ACTION TRACE PERMISSION : " << res.action_traces.at(0).act.authorization.at(0).permission.to_string());
		test.produce_block();
	}
	return 3;
}


#define CALL_TEST_FUNCTION(TESTER, CLS, MTH, DATA) CallFunction(TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA)
#define CALL_TEST_FUNCTION_SCOPE(TESTER, CLS, MTH, DATA, ACCOUNT) CallFunction(TESTER, test_api_action<TEST_METHOD(CLS, MTH)>{}, DATA, ACCOUNT)

bool is_access_violation(fc::unhandled_exception const & e) {
   try {
      std::rethrow_exception(e.get_inner_exception());
    }
    catch (const Runtime::Exception& e) {
      return e.cause == Runtime::Exception::Cause::accessViolation;
    } catch (...) {

    }
   return false;
}
bool is_assert_exception(fc::assert_exception const & e) { return true; }
bool is_page_memory_error(page_memory_error const &e) { return true; }
bool is_tx_missing_auth(tx_missing_auth const & e) { return true; }
/*
bool is_tx_missing_recipient(tx_missing_recipient const & e) { return true;}
bool is_tx_missing_auth(tx_missing_auth const & e) { return true; }
bool is_tx_missing_scope(tx_missing_scope const& e) { return true; }
bool is_tx_resource_exhausted(const tx_resource_exhausted& e) { return true; }
bool is_tx_unknown_argument(const tx_unknown_argument& e) { return true; }
bool is_assert_exception(fc::assert_exception const & e) { return true; }
bool is_tx_resource_exhausted_or_checktime(const transaction_exception& e) {
   return (e.code() == tx_resource_exhausted::code_value) || (e.code() == checktime_exceeded::code_value);
}
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
      last_fnc_err = EXEC; \
      std::STREAM.rdbuf(oldbuf); \
   }

#define TEST_PRINT_METHOD( METHOD , OUT) \
	{ \
		auto U64Str = [](uint64_t v) -> std::string { std::stringstream s; s << v; return s.str(); }; \
		CAPTURE( cerr, CALL_TEST_FUNCTION(*this, "test_print", METHOD, {}) ); \
		BOOST_TEST_MESSAGE( "Running test_print::" << METHOD ); \
		BOOST_CHECK_EQUAL( capture.size(), 7 ); \
		BOOST_TEST_MESSAGE( "Captured Output : " << capture[3] ); \
		BOOST_CHECK_EQUAL( capture[3].substr(0,1), U64Str(0) ); \
		BOOST_CHECK_EQUAL( capture[3].substr(1,6), U64Str(556644) ); \
		BOOST_CHECK_EQUAL( capture[3].substr(7, capture[3].size()), U64Str(-1) ); \
	}


// TODO missing intrinsic account_balance_get
#if 0
/*************************************************************************************
 * account_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(account_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	create_account( N(acc1), asset::from_string("0.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	//transfer( N(inita), N(acc1), "1000.0000 EOS", "test");
	eosio::chain::signed_transaction trx;
	trx.write_scope = { N(acc1), N(inita) };

	trx.actions.emplace_back(vector<permission_level>{{N(inita), config::active_name}}, 
									 contracts::transfer{N(inita), N(acc1), 1000, "memo"});


	trx.expiration = control->head_block_time() + fc::seconds(100);
	trx.set_reference_block(control->head_block_id());

	trx.sign(get_private_key(N(inita), "active"), chain_id_type());
	push_transaction(trx);
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);
	
	//const auto& balance = get_balance(N(acc1));
	//CALL_TEST_FUNCTION( *this, "test_account", "test_balance_acc1", {});
} FC_LOG_AND_RETHROW() }
#endif

/*************************************************************************************
 * action_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(action_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("100000.0000 EOS") );
   create_account( N(acc1), asset::from_string("1.0000 EOS") );
   create_account( N(acc2), asset::from_string("1.0000 EOS") );
   create_account( N(acc3), asset::from_string("1.0000 EOS") );
   create_account( N(acc4), asset::from_string("1.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

	CALL_TEST_FUNCTION( *this, "test_action", "assert_true", {});
	BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_action", "assert_false", {}),
                         fc::assert_exception, is_assert_exception);

   dummy_action dummy13{DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
   CALL_TEST_FUNCTION( *this, "test_action", "read_action_normal", fc::raw::pack(dummy13));

   //std::vector<char> raw_bytes((1<<16)-1);
   std::vector<char> raw_bytes(64);
   //std::vector<char> raw_bytes((1<<16)-1);
   CALL_TEST_FUNCTION(*this, "test_action", "read_action_to_0", raw_bytes);

   //raw_bytes.resize((1<<16)+1);
   //CALL_TEST_FUNCTION(*this, "test_action", "read_action_to_0", raw_bytes);
   //std::vector<char> raw_bytes2((1<<30));
   //raw_bytes.resize((1<<32));
   //BOOST_TEST_MESSAGE("SIZE " << raw_bytes2.size());
   //CALL_TEST_FUNCTION(*this, "test_action", "read_action_to_0", raw_bytes2);
   //BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_action", "read_action_to_0", raw_bytes), fc::unhandled_exception, is_access_violation);
   //BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION(*this, "test_action", "require_auth", {}), tx_missing_auth, is_tx_missing_auth);
} FC_LOG_AND_RETHROW() }

// TODO missing intrinsic get_active_producers
#if 0
/*************************************************************************************
 * chain_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(chain_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	create_account( N(acc1), asset::from_string("0.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );

	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);
   
   auto& gpo = control->get_global_properties();   
   BOOST_TEST_MESSAGE("ACTIVE PROD SIZE " << &gpo << "\n");
	//CALL_TEST_FUNCTION( *this, "test_chain", "test_activeprods", {});
} FC_LOG_AND_RETHROW() }
#endif

// (Bucky) TODO got to fix macros in test_db.cpp
#if 0
/*************************************************************************************
 * db_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(db_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("100000.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

	CALL_TEST_FUNCTION( *this, "test_db", "key_i64_general", {});
} FC_LOG_AND_RETHROW() }
#endif

// TODO missing intrinsic __ashlti3
#if 0
/*************************************************************************************
 * fixedpoint_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(fixedpoint_tests, tester) { try {
	produce_blocks(2);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	produce_blocks(1000);
	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1);

	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "create_instances", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_addition", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_subtraction", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_multiplication", {});
	CALL_TEST_FUNCTION( *this, "test_fixedpoint", "test_division", {});
} FC_LOG_AND_RETHROW() }
#endif

// TODO missing intrinsics for doubles
#if 0
/*************************************************************************************
 * real_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(real_test, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_real", "create_instances", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_real", "test_addition", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_real", "test_multiplication", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_real", "test_division", {} );
} FC_LOG_AND_RETHROW() }
#endif

// TODO missing intrinsics assert_sha256
#if 0
/*************************************************************************************
 * crypto_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(real_test, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_crypto", "test_sha256", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_crypto", "sha256_no_data", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_crypto", "asert_sha256_false", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_crypto", "asert_sha256_true", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_crypto", "asert_no_data", {} );
} FC_LOG_AND_RETHROW() }
#endif 

#if 1
/*************************************************************************************
 * memory_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(memory_test, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_allocs", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunk", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunks", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memory_hunks_disjoint", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memset_memcpy", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcpy_overlap_start", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcpy_overlap_end", {} );
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_memory", "test_memcmp", {} );
} FC_LOG_AND_RETHROW() }
#endif

#if 1
/*************************************************************************************
 * extended_memory_tests test cases
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(extended_memory_test_initial_memory, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_initial_buffer", {} );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory", {} );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory_exceeded, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory_exceeded", {} ),
                         page_memory_error, is_page_memory_error);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(extended_memory_test_page_memory_negative_bytes, tester) { try {
   produce_blocks(1000);
   create_account(N(testapi), asset::from_string("1000.0000 EOS"));
   produce_blocks(1000);
   transfer(N(inita), N(testapi), "100.0000 EOS", "memo");
   produce_blocks(1000);
   set_code(N(testapi), test_api_wast);
   produce_blocks(1000);
   BOOST_CHECK_EXCEPTION(CALL_TEST_FUNCTION( *this, "test_extended_memory", "test_page_memory_negative_bytes", {} ),
                         page_memory_error, is_page_memory_error);
} FC_LOG_AND_RETHROW() }
#endif


#if 0
/*************************************************************************************
 * string_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(string_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	create_account( N(testextmem), asset::from_string("100.0000 EOS") );
	produce_blocks(1000);

	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

	//transfer( N(testapi), N(testextmem), "1.0000 EOS", "memo" );
	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_size", {});
	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_data", {});
	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_data_partially", {});
	CALL_TEST_FUNCTION( *this, "test_string", "construct_with_data_copied", {});
	CALL_TEST_FUNCTION( *this, "test_string", "copy_constructor", {});
	CALL_TEST_FUNCTION( *this, "test_string", "assignment_operator", {});
	CALL_TEST_FUNCTION( *this, "test_string", "index_operator", {});
	BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_string", "index_out_of_bound", {}), fc::assert_exception, is_assert_exception );
	CALL_TEST_FUNCTION( *this, "test_string", "substring", {});
	BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( *this, "test_string", "substring_out_of_bound", {}), fc::assert_exception, is_assert_exception );
	CALL_TEST_FUNCTION( *this, "test_string", "concatenation_null_terminated", {});
	CALL_TEST_FUNCTION( *this, "test_string", "concatenation_non_null_terminated", {});
	CALL_TEST_FUNCTION( *this, "test_string", "assign", {});
	CALL_TEST_FUNCTION( *this, "test_string", "comparison_operator", {});
	CALL_TEST_FUNCTION( *this, "test_string", "print_null_terminated", {});
	CALL_TEST_FUNCTION( *this, "test_string", "print_non_null_terminated", {});
	CALL_TEST_FUNCTION( *this, "test_string", "print_unicode", {});
	CALL_TEST_FUNCTION( *this, "test_string", "string_literal", {});
//TODO missing assert_is_utf8
#if 0
	CALL_TEST_FUNCTION( *this, "test_string", "valid_utf8", {});
	CALL_TEST_FUNCTION( *this, "test_string", "invalid_utf8", {});
#endif
} FC_LOG_AND_RETHROW() }
#endif

/*************************************************************************************
 * print_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(print_tests, tester) { try {
	produce_blocks(2);
	create_account(N(testapi), asset::from_string("1000.0000 EOS"));
	create_account(N(another), asset::from_string("1.0000 EOS"));
	create_account(N(acc1), asset::from_string("1.0000 EOS"));
	create_account(N(acc2), asset::from_string("1.0000 EOS"));
	create_account(N(acc3), asset::from_string("1.0000 EOS"));
	create_account(N(acc4), asset::from_string("1.0000 EOS"));
	produce_blocks(1);

	//Set test code
	transfer(N(inita), N(testapi), "10.0000 EOS", "memo");
	set_code(N(testapi), test_api_wast); 
	produce_blocks(1);
	string captured = "";


#if 1
	// test prints
	CAPTURE_AND_PRE_TEST_PRINT("test_prints");
	BOOST_CHECK_EQUAL(captured == "abcefg", true);

	// test printi
	CAPTURE_AND_PRE_TEST_PRINT("test_printi");
	BOOST_CHECK_EQUAL( captured.substr(0,1), U64Str(0) );  						// "0"
	BOOST_CHECK_EQUAL( captured.substr(1,6), U64Str(556644) );					// "556644" 
	BOOST_CHECK_EQUAL( captured.substr(7, capture[3].size()), U64Str(-1) ); // "18446744073709551615"

	//TODO come back to this, no native uint128_t
	// test printi128
//	CAPTURE_AND_PRE_TEST_PRINT("test_printi128");
//	BOOST_CHECK_EQUAL( captured.substr(0, 38), U128Str(-1) );

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
#endif	

} FC_LOG_AND_RETHROW() }


// missing intriniscs
#if 1
/*************************************************************************************
 * math_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(math_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	produce_blocks(1000);

	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

	CALL_TEST_FUNCTION( *this, "test_math", "test_multeq", {});
	CALL_TEST_FUNCTION( *this, "test_math", "test_diveq", {});
	CALL_TEST_FUNCTION( *this, "test_math", "test_diveq_by_0", {});
	CALL_TEST_FUNCTION( *this, "test_math", "test_double_api", {});
	CALL_TEST_FUNCTION( *this, "test_math", "test_double_api_div_0", {});
} FC_LOG_AND_RETHROW() }
#endif

/*************************************************************************************
 * types_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(types_tests, tester) { try {
	produce_blocks(1000);
	create_account( N(testapi), asset::from_string("1000.0000 EOS") );
	produce_blocks(1000);

	transfer( N(inita), N(testapi), "100.0000 EOS", "memo" );
	produce_blocks(1000);
	set_code( N(testapi), test_api_wast );
	produce_blocks(1000);

	CALL_TEST_FUNCTION( *this, "test_types", "types_size", {});
	CALL_TEST_FUNCTION( *this, "test_types", "char_to_symbol", {});
	CALL_TEST_FUNCTION( *this, "test_types", "string_to_name", {});
	CALL_TEST_FUNCTION( *this, "test_types", "name_class", {});
} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
