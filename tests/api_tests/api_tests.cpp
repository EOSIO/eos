/**
 *  @file
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

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/block_summary_object.hpp>
#include <eos/chain/wasm_interface.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/exception/exception.hpp>

#include "../common/database_fixture.hpp"

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <test_api/test_api.wast.hpp>
#include <test_api/test_api.hpp>

#include "memory_test/memory_test.wast.hpp"
#include "extended_memory_test/extended_memory_test.wast.hpp"
#include "table_abi_test/table_abi_test.wast.hpp"
#include "table_abi_test/table_abi_test.abi.hpp"

#include <eos/chain/staked_balance_objects.hpp>
#include <eos/chain/balance_object.hpp>

FC_REFLECT( dummy_message, (a)(b)(c) );
FC_REFLECT( u128_msg, (values) );

using namespace eosio;
using namespace chain;

namespace bio = boost::iostreams;

BOOST_AUTO_TEST_SUITE(api_tests)

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

uint32_t CallFunction( testing_blockchain& chain, const types::message& msg, const vector<char>& data, const vector<account_name>& scope = {N(testapi)}) {
   static int expiration = 1;
   eosio::chain::signed_transaction trx;
   trx.scope = scope;
   
   //msg.data.clear();
   vector<char>& dest = *(vector<char> *)(&msg.data);
   std::copy(data.begin(), data.end(), std::back_inserter(dest));

   //std::cout << "MANDO: " << msg.code << " " << msg.type << std::endl;
   transaction_emplace_message(trx, msg);
   
   trx.expiration = chain.head_block_time() + expiration++;
   transaction_set_reference_block(trx, chain.head_block_id());
   //idump((trx));
   chain.push_transaction(trx);

   //
   auto& wasm = wasm_interface::get();
   uint32_t *res = Runtime::memoryArrayPtr<uint32_t>(wasm.current_memory, (1<<16) - 2*sizeof(uint32_t), 2);

   if(res[0] == WASM_TEST_FAIL) {
      char *str_err = &Runtime::memoryRef<char>(wasm.current_memory, res[1]);
      wlog( "${err}", ("err",str_err));
   }

   return res[0];
}


using namespace std;
string readFile2(const string &fileName)
{
    ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

    ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, ios::beg);

    vector<char> bytes(fileSize);
    ifs.read(bytes.data(), fileSize);

    return string(bytes.data(), fileSize);
}

uint64_t TEST_METHOD(const char* CLASS, const char *METHOD) {
  //std::cerr << CLASS << "::" << METHOD << std::endl;
  return ( (uint64_t(DJBH(CLASS))<<32) | uint32_t(DJBH(METHOD)) );
} 

#define CALL_TEST_FUNCTION(TYPE, AUTH, DATA) CallFunction(chain, message{"testapi", AUTH, TYPE}, DATA)
#define CALL_TEST_FUNCTION_SCOPE(TYPE, AUTH, DATA, SCOPE) CallFunction(chain, message{"testapi", AUTH, TYPE}, DATA, SCOPE)

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

bool is_tx_missing_recipient(tx_missing_recipient const & e) { return true;}
bool is_tx_missing_auth(tx_missing_auth const & e) { return true; }
bool is_tx_missing_scope(tx_missing_scope const& e) { return true; }
bool is_tx_resource_exhausted(const tx_resource_exhausted& e) { return true; }
bool is_tx_unknown_argument(const tx_unknown_argument& e) { return true; }
bool is_assert_exception(fc::assert_exception const & e) { return true; }
bool is_tx_resource_exhausted_or_checktime(const transaction_exception& e) {
   return (e.code() == tx_resource_exhausted::code_value) || (e.code() == checktime_exceeded::code_value);
}

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

void send_set_code_message(testing_blockchain& chain, types::setcode& handler, account_name account)
{
   eosio::chain::signed_transaction trx;
   handler.account = account;
   trx.scope = { account };
   trx.messages.resize(1);
   trx.messages[0].authorization = {{account,"active"}};
   trx.messages[0].code = config::eos_contract_name;
   transaction_set_message(trx, 0, "setcode", handler);
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   chain.push_transaction(trx);
   chain.produce_blocks(1);
}

BOOST_FIXTURE_TEST_CASE(test_all, testing_fixture)
{ try {
      //auto wasm = assemble_wast( readFile2("/home/matu/Documents/Dev/eos/build/contracts/test_api/test_api.wast").c_str() );
      auto wasm = assemble_wast( test_api_wast );

      Make_Blockchain(chain);
      chain.produce_blocks(2);
      Make_Account(chain, testapi);
      Make_Account(chain, another);
      Make_Account(chain, acc1);
      Make_Account(chain, acc2);
      Make_Account(chain, acc3);
      Make_Account(chain, acc4);
      chain.produce_blocks(1);

      //Set test code
      types::setcode handler;
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      send_set_code_message(chain, handler, "testapi");
      send_set_code_message(chain, handler, "acc1");
      send_set_code_message(chain, handler, "acc2");

      //Test types
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "types_size"),     {}, {} ) == WASM_TEST_PASS, "test_types::types_size()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "char_to_symbol"), {}, {} ) == WASM_TEST_PASS, "test_types::char_to_symbol()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "string_to_name"), {}, {} ) == WASM_TEST_PASS, "test_types::string_to_name()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "name_class"),     {}, {} ) == WASM_TEST_PASS, "test_types::name_class()" );

      //Test message
      dummy_message dummy13{DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "read_message_normal"), {}, fc::raw::pack(dummy13) ) == WASM_TEST_PASS, "test_message::read_message_normal()" );

      std::vector<char> raw_bytes((1<<16));
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "read_message_to_0"), {}, raw_bytes) == WASM_TEST_PASS, "test_message::read_message_to_0()" );

      raw_bytes.resize((1<<16)+1);
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "read_message_to_0"), {}, raw_bytes), 
         fc::unhandled_exception, is_access_violation );

      raw_bytes.resize(1);
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "read_message_to_64k"), {}, raw_bytes) == WASM_TEST_PASS, "test_message::read_message_to_64k()" );

      raw_bytes.resize(2);
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "read_message_to_64k"), {}, raw_bytes), 
         fc::unhandled_exception, is_access_violation );
      
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "require_notice"), {}, raw_bytes) == WASM_TEST_PASS, "test_message::require_notice()" );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "require_auth"), {}, {}), 
         tx_missing_auth, is_tx_missing_auth );
      
      auto a3only = vector<types::account_permission>{{"acc3","active"}};
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "require_auth"), a3only, {}), 
         tx_missing_auth, is_tx_missing_auth );

      auto a4only = vector<types::account_permission>{{"acc4","active"}};
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "require_auth"), a4only, {}),
         tx_missing_auth, is_tx_missing_auth );

      auto a3a4 = vector<types::account_permission>{{"acc3","active"}, {"acc4","active"}};
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "require_auth"), a3a4, {}) == WASM_TEST_PASS, "test_message::require_auth()");

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "assert_false"), {}, {}),
         fc::assert_exception, is_assert_exception );

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "assert_true"), {}, {}) == WASM_TEST_PASS, "test_message::assert_true()");

      chain.produce_blocks(1);
      
      uint32_t now = chain.head_block_time().sec_since_epoch();
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "now"), {}, fc::raw::pack(now)) == WASM_TEST_PASS, "test_message::now()");

      chain.produce_blocks(1);
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "now"), {}, fc::raw::pack(now)) == WASM_TEST_FAIL, "test_message::now()");

      //Test print
      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_print", "test_prints"), {}, {}) );
      
      BOOST_CHECK_EQUAL( last_fnc_err , WASM_TEST_PASS);
      BOOST_CHECK_EQUAL( capture.size() , 3);
      BOOST_CHECK_EQUAL( (capture[0] == "ab" && capture[1] == "c" && capture[2] == "efg") , true);

      auto U64Str = [](uint64_t v) -> std::string { 
         std::stringstream s;
         s << v;
         return s.str();
      };

      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_print", "test_printi"), {}, {}) );
      BOOST_CHECK_EQUAL( capture.size() , 3);
      BOOST_CHECK_EQUAL( capture[0], U64Str(0) );
      BOOST_CHECK_EQUAL( capture[1], U64Str(556644) );
      BOOST_CHECK_EQUAL( capture[2], U64Str(-1) );

      auto U128Str = [](uint128_t value) -> std::string { 
        fc::uint128_t v(value>>64, uint64_t(value) );
        return fc::variant(v).get_string();
      };

      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_print", "test_printi128"), {}, {}) );
      BOOST_CHECK_EQUAL( capture.size() , 3);
      BOOST_CHECK_EQUAL( capture[0], U128Str(-1));
      BOOST_CHECK_EQUAL( capture[1], U128Str(0));
      BOOST_CHECK_EQUAL( capture[2], U128Str(87654323456));

      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_print", "test_printn"), {}, {}) );
      BOOST_CHECK_EQUAL( capture.size() , 8);

      std::cout << capture[0] << std::endl
                << capture[1] << std::endl
                << capture[2] << std::endl
                << capture[3] << std::endl
                << capture[4] << std::endl
                << capture[5] << std::endl
                << capture[6] << std::endl
                << capture[7] << std::endl;

      BOOST_CHECK_EQUAL( (
         capture[0] == "abcde" && 
         capture[1] == "ab.de"  && 
         capture[2] == "1q1q1q" &&
         capture[3] == "abcdefghijk" &&
         capture[4] == "abcdefghijkl" &&
         capture[5] == "abcdefghijkl1" &&
         capture[6] == "abcdefghijkl1" &&
         capture[7] == "abcdefghijkl1" 
      ), true);

      //Test math
      std::random_device rd;
      std::mt19937_64 gen(rd());
      std::uniform_int_distribution<unsigned long long> dis;

      for(int i=0; i<10; i++) {
         u128_msg msg;
         msg.values[0] = dis(gen); msg.values[0] <<= 64; msg.values[0] |= dis(gen);
         msg.values[1] = dis(gen); msg.values[1] <<= 64; msg.values[1] |= dis(gen);
         msg.values[2] = msg.values[0] * msg.values[1];
         BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_math", "test_multeq_i128"), {}, fc::raw::pack(msg) ) == WASM_TEST_PASS, "test_math::test_multeq_i128()" );
      }

      for(int i=0; i<10; i++) {
         u128_msg msg;
         msg.values[0] = dis(gen); msg.values[0] <<= 64; msg.values[0] |= dis(gen);
         msg.values[1] = dis(gen); msg.values[1] <<= 64; msg.values[1] |= dis(gen);
         msg.values[2] = msg.values[0] / msg.values[1];
         BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_math", "test_diveq_i128"), {}, fc::raw::pack(msg) ) == WASM_TEST_PASS, "test_math::test_diveq_i128()" );
      }

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_math", "test_diveq_i128_by_0"), {}, {} ),
         fc::assert_exception, is_assert_exception );

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_math", "test_double_api"), {}, {} ) == WASM_TEST_PASS, "test_math::test_double_api()" );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_math", "test_double_api_div_0"), {}, {} ),
      fc::assert_exception, is_assert_exception );
      
      //Test db (i64)
      const auto& idx = chain_db.get_index<key_value_index, by_scope_primary>();

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64_general"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i64_general()" );
      BOOST_CHECK_EQUAL( std::distance(idx.begin(), idx.end()) , 4);
      
      auto itr = idx.lower_bound( boost::make_tuple( N(testapi), N(testapi), N(test_table)) );

      BOOST_CHECK_EQUAL((uint64_t)itr->primary_key, N(alice)); ++itr;
      BOOST_CHECK_EQUAL((uint64_t)itr->primary_key, N(bob)); ++itr;
      BOOST_CHECK_EQUAL((uint64_t)itr->primary_key, N(carol)); ++itr;
      BOOST_CHECK_EQUAL((uint64_t)itr->primary_key, N(dave));

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64_remove_all"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i64_remove_all()" );
      BOOST_CHECK_EQUAL( std::distance(idx.begin(), idx.end()) , 0);

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64_small_load"), {}, {} ),
         fc::assert_exception, is_assert_exception );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64_small_store"), {}, {} ),
         fc::assert_exception, is_assert_exception );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_i64_store_scope"), {}, {}, {N(another)} ),
         tx_missing_scope, is_tx_missing_scope );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_i64_remove_scope"), {}, {}, {N(another)} ),
         tx_missing_scope, is_tx_missing_scope );

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64_not_found"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i64_not_found()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64_front_back"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i64_front_back()" );

      //Test db (i128i128)
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i128i128_general"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i128i128_general()" );

      //Test db (i64i64i64)
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64i64i64_general"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i64i64i64_general()" );

      //Test db (str)
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_str_general"), {}, {} ) == WASM_TEST_PASS, "test_db::key_str_general()" );

      //Test crypto
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_crypto", "test_sha256"), {}, {} ) == WASM_TEST_PASS, "test_crypto::test_sha256()" );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_crypto", "sha256_no_data"), {}, {} ),
         fc::assert_exception, is_assert_exception );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_crypto", "asert_sha256_false"), {}, {} ),
         fc::assert_exception, is_assert_exception );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_crypto", "asert_sha256_true"), {}, {} ) == WASM_TEST_PASS, "test_crypto::asert_sha256_true()" );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_crypto", "asert_no_data"), {}, {} ),
         fc::assert_exception, is_assert_exception );

      //Test transaction
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_message"), {}, {}) == WASM_TEST_PASS, "test_transaction::send_message()");
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_message_empty"), {}, {}) == WASM_TEST_PASS, "test_transaction::send_message_empty()");
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_message_large"), {}, {} ),
         tx_resource_exhausted, is_tx_resource_exhausted );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_message_max"), {}, {} ),
         tx_resource_exhausted, is_tx_resource_exhausted );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_message_recurse"), {}, fc::raw::pack(dummy13) ),
         transaction_exception, is_tx_resource_exhausted_or_checktime );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_message_inline_fail"), {}, {} ),
         fc::assert_exception, is_assert_exception );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_transaction"), {}, {}) == WASM_TEST_PASS, "test_transaction::send_message()");
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_transaction_empty"), {}, {} ),
         tx_unknown_argument, is_tx_unknown_argument );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_transaction_large"), {}, {} ),
         tx_resource_exhausted, is_tx_resource_exhausted );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_transaction", "send_transaction_max"), {}, {} ),
         tx_resource_exhausted, is_tx_resource_exhausted );

      auto& gpo = chain_db.get<global_property_object>();
      std::vector<account_name> prods(gpo.active_producers.size());
      std::copy(gpo.active_producers.begin(), gpo.active_producers.end(), prods.begin());
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_chain", "test_activeprods"), {}, fc::raw::pack(prods) ) == WASM_TEST_PASS, "test_chain::test_activeprods()" );

      // Test string
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "construct_with_size"), {}, {} ) == WASM_TEST_PASS, "test_string::construct_with_size()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "construct_with_data"), {}, {} ) == WASM_TEST_PASS, "test_string::construct_with_data()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "construct_with_data_copied"), {}, {} ) == WASM_TEST_PASS, "test_string::construct_with_data_copied()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "construct_with_data_partially"), {}, {} ) == WASM_TEST_PASS, "test_string::construct_with_data_partially()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "copy_constructor"), {}, {} ) == WASM_TEST_PASS, "test_string::copy_constructor()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "assignment_operator"), {}, {} ) == WASM_TEST_PASS, "test_string::assignment_operator()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "index_operator"), {}, {} ) == WASM_TEST_PASS, "test_string::index_operator()" );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "index_out_of_bound"), {}, {} ), fc::assert_exception, is_assert_exception );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "substring"), {}, {} ) == WASM_TEST_PASS, "test_string::substring()" );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "substring_out_of_bound"), {}, {} ), fc::assert_exception, is_assert_exception );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "concatenation_null_terminated"), {}, {} ) == WASM_TEST_PASS, "test_string::concatenation_null_terminated()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "concatenation_non_null_terminated"), {}, {} ) == WASM_TEST_PASS, "test_string::concatenation_non_null_terminated()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "assign"), {}, {} ) == WASM_TEST_PASS, "test_string::assign()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "comparison_operator"), {}, {} ) == WASM_TEST_PASS, "test_string::comparison_operator()" );

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_fixedpoint", "create_instances"), {}, {} ) == WASM_TEST_PASS, "test_fixedpoint::create_instances()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_fixedpoint", "test_addition"), {}, {} ) == WASM_TEST_PASS, "test_fixedpoint::test_addition()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_fixedpoint", "test_subtraction"), {}, {} ) == WASM_TEST_PASS, "test_fixedpoint::test_subtraction()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_fixedpoint", "test_multiplication"), {}, {} ) == WASM_TEST_PASS, "test_fixedpoint::test_multiplication()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_fixedpoint", "test_division"), {}, {} ) == WASM_TEST_PASS, "test_fixedpoint::test_division()" );

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_real", "create_instances"), {}, {} ) == WASM_TEST_PASS, "test_real::create_instances()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_real", "test_addition"), {}, {} ) == WASM_TEST_PASS, "test_real::test_addition()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_real", "test_multiplication"), {}, {} ) == WASM_TEST_PASS, "test_real::test_multiplication()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_real", "test_division"), {}, {} ) == WASM_TEST_PASS, "test_real::test_division()" );
      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_real", "create_instances"), {}, {}) );
      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_string", "print_null_terminated"), {}, {}) );

      BOOST_CHECK_EQUAL( capture.size() , 1);
      BOOST_CHECK_EQUAL( capture[0], "Hello World!");
      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_string", "print_non_null_terminated"), {}, {}) );
      BOOST_CHECK_EQUAL( capture.size() , 1);
      BOOST_CHECK_EQUAL( capture[0], "Hello World!");
      CAPTURE(cerr, CALL_TEST_FUNCTION( TEST_METHOD("test_string", "print_unicode"), {}, {}) );
      BOOST_CHECK_EQUAL( capture.size() , 1);
      BOOST_CHECK_EQUAL( capture[0], "你好，世界！");
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "valid_utf8"), {}, {} ) == WASM_TEST_PASS, "test_string::valid_utf8()" );
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "invalid_utf8"), {}, {} ), fc::assert_exception, is_assert_exception );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_string", "string_literal"), {}, {} ) == WASM_TEST_PASS, "test_string::string_literal()" );

} FC_LOG_AND_RETHROW() }

#define RUN_CODE_HANDLER_WITH_TRANSFER(account_name, test_wast)                                            \
      Make_Blockchain(chain);                                                                              \
      chain.produce_blocks(1);                                                                             \
      Make_Account(chain, account_name);                                                                   \
      chain.produce_blocks(1);                                                                             \
                                                                                                           \
                                                                                                           \
      handler.account = #account_name;                                                                     \
                                                                                                           \
      auto wasm = assemble_wast( test_wast );                                                              \
      handler.code.resize(wasm.size());                                                                    \
      memcpy( handler.code.data(), wasm.data(), wasm.size() );                                             \
                                                                                                           \
      {                                                                                                    \
         eosio::chain::signed_transaction trx;                                                             \
         trx.scope = {#account_name};                                                                      \
         trx.messages.resize(1);                                                                           \
         trx.messages[0].code = config::eos_contract_name;                                                 \
         trx.messages[0].authorization.emplace_back(types::account_permission{#account_name,"active"});    \
         transaction_set_message(trx, 0, "setcode", handler);                                              \
         trx.expiration = chain.head_block_time() + 100;                                                   \
         transaction_set_reference_block(trx, chain.head_block_id());                                      \
         chain.push_transaction(trx);                                                                      \
         chain.produce_blocks(1);                                                                          \
      }                                                                                                    \
                                                                                                           \
                                                                                                           \
      {                                                                                                    \
         eosio::chain::signed_transaction trx;                                                             \
         trx.scope = sort_names({#account_name,"inita"});                                                  \
         transaction_emplace_message(trx, #account_name,                                                   \
                            vector<types::account_permission>{},                                           \
                            "transfer", types::transfer{#account_name, "inita", 1,""});                    \
         trx.expiration = chain.head_block_time() + 100;                                                   \
         transaction_set_reference_block(trx, chain.head_block_id());                                      \
         chain.push_transaction(trx);                                                                      \
         chain.produce_blocks(1);                                                                          \
      }

#define RUN_CODE_WITH_TRANSFER(account_name, test_wast)                                                    \
      types::setcode handler;                                                                              \
      RUN_CODE_HANDLER_WITH_TRANSFER(account_name, test_wast)

#define TEST_CASE_RUN_CODE_W_XFER(test_case_name, account_name, test_wast)                                 \
BOOST_FIXTURE_TEST_CASE(test_case_name, testing_fixture)                                                   \
{ try{                                                                                                     \
   RUN_CODE_WITH_TRANSFER(account_name, test_wast);                                                        \
} FC_LOG_AND_RETHROW() }

#define RUN_CODE_ABI_WITH_TRANSFER(account_name, test_wast, test_abi)                                      \
      types::setcode handler;                                                                              \
      handler.code_abi = fc::json::from_string(test_abi).as<types::abi>();                                 \
      RUN_CODE_HANDLER_WITH_TRANSFER(account_name, test_wast)

#define TEST_CASE_RUN_CODE_ABI_W_XFER(test_case_name, account_name, test_wast, test_abi)                   \
BOOST_FIXTURE_TEST_CASE(test_case_name, testing_fixture)                                                   \
{ try{                                                                                                     \
   RUN_CODE_ABI_WITH_TRANSFER(account_name, test_wast, test_abi);                                          \
} FC_LOG_AND_RETHROW() }

//Test wasm memory allocation
TEST_CASE_RUN_CODE_W_XFER(test_memory, testmemory, memory_test_wast)

//Test memcmp
TEST_CASE_RUN_CODE_W_XFER(test_memcmp, testmemcmp, memory_test_wast)

//Test wasm memory allocation of one large hunk
TEST_CASE_RUN_CODE_W_XFER(test_memory_hunk, testmemhunk, memory_test_wast)

//Test wasm memory allocation of many medium hunks in contiguous 2nd heap
TEST_CASE_RUN_CODE_W_XFER(test_memory_hunks, testmemhunks, memory_test_wast)

//Test wasm memory allocation of many medium hunks with disjoint heaps
TEST_CASE_RUN_CODE_W_XFER(test_memory_hunks_disjoint, testdisjoint, memory_test_wast)

//Test intrinsic provided memset and memcpy
TEST_CASE_RUN_CODE_W_XFER(test_memset_memcpy, testmemset, memory_test_wast)

//Test memcpy overlap at start of destination
BOOST_FIXTURE_TEST_CASE(test_memcpy_overlap_start, testing_fixture)
{
   try {
      RUN_CODE_WITH_TRANSFER(testolstart, memory_test_wast);
      BOOST_FAIL("memcpy should have thrown assert exception");
   }
   catch(fc::assert_exception& ex)
   {
      BOOST_REQUIRE(ex.to_detail_string().find("overlap of memory range is undefined") != std::string::npos);
   }
}

//Test memcpy overlap at end of destination
BOOST_FIXTURE_TEST_CASE(test_memcpy_overlap_end, testing_fixture)
{
   try {
      RUN_CODE_WITH_TRANSFER(testolend, memory_test_wast);
      BOOST_FAIL("memcpy should have thrown assert exception");
   }
   catch(fc::assert_exception& ex)
   {
      BOOST_REQUIRE(ex.to_detail_string().find("overlap of memory range is undefined") != std::string::npos);
   }
}

//Test logic for memory.hpp adding extra pages of memory
TEST_CASE_RUN_CODE_W_XFER(test_extended_memory, testextmem, extended_memory_test_wast)

//Test logic for extra pages of memory
TEST_CASE_RUN_CODE_W_XFER(test_page_memory, testpagemem, extended_memory_test_wast)

//Test logic for exceeding extra pages of memory
BOOST_FIXTURE_TEST_CASE(test_page_memory_exceeded, testing_fixture)
{
   try {
      RUN_CODE_WITH_TRANSFER(testmemexc, extended_memory_test_wast);
      BOOST_FAIL("sbrk should have thrown exception");
   }
   catch (eosio::chain::page_memory_error& )
   {
      // expected behavior
   }
}

//Test logic for preventing reducing page memory
BOOST_FIXTURE_TEST_CASE(test_page_memory_negative_bytes, testing_fixture)
{
   try {
      RUN_CODE_WITH_TRANSFER(testnegbytes, extended_memory_test_wast);
      BOOST_FAIL("sbrk should have thrown exception");
   }
   catch (fc::assert_exception& ex)
   {
      BOOST_REQUIRE(ex.to_detail_string().find("not reduce") != std::string::npos);
   }
}

TEST_CASE_RUN_CODE_ABI_W_XFER(test_table_store_i64, storei, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_RUN_CODE_ABI_W_XFER(test_table_store_i128i128, storeii, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_RUN_CODE_ABI_W_XFER(test_table_store_i64i64i64, storeiii, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_RUN_CODE_ABI_W_XFER(test_table_store_str, storestr, table_abi_test_wast, table_abi_test_abi)

#define TEST_CASE_TABLE_TYPE_FAILURE(test_case_name, account_name, test_wast, test_abi)                   \
BOOST_FIXTURE_TEST_CASE(test_case_name, testing_fixture)                                                  \
{                                                                                                         \
   try {                                                                                                  \
      RUN_CODE_ABI_WITH_TRANSFER(account_name, test_wast, test_abi);                                      \
      BOOST_FAIL("should have thrown assert exception");                                                  \
   }                                                                                                      \
   catch(fc::assert_exception& ex)                                                                        \
   {                                                                                                      \
      const auto first = ex.to_detail_string().find("abi definition for");                                \
      BOOST_REQUIRE(first != std::string::npos);                                                          \
      BOOST_REQUIRE(ex.to_detail_string().substr(first).find(" expects ") != std::string::npos);          \
   }                                                                                                      \
}

TEST_CASE_TABLE_TYPE_FAILURE(test_table_store_fail_str_with_i64, strnoti, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_TABLE_TYPE_FAILURE(test_table_store_fail_i64_with_i128i128, inotii, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_TABLE_TYPE_FAILURE(test_table_store_fail_i128i128_with_i64i64i64, iinotiii, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_TABLE_TYPE_FAILURE(test_table_store_fail_i64i64i64_with_str, iiinotstr, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_TABLE_TYPE_FAILURE(test_table_load_fail_str_with_i64, ldstrnoti, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_TABLE_TYPE_FAILURE(test_table_load_fail_i64_with_i128i128, ldinotii, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_TABLE_TYPE_FAILURE(test_table_load_fail_i128i128_with_i64i64i64, ldiinotiii, table_abi_test_wast, table_abi_test_abi)

TEST_CASE_TABLE_TYPE_FAILURE(test_table_load_fail_i64i64i64_with_str, ldiiinotstr, table_abi_test_wast, table_abi_test_abi)

#define VERIFY_DB_KEY_TYPES(KEY_TYPE) \
{ try { \
   auto wasm = assemble_wast( test_api_wast ); \
   \
   Make_Blockchain(chain, 5000, \
         ::eosio::chain_plugin::default_received_block_transaction_execution_time, \
         ::eosio::chain_plugin::default_create_block_transaction_execution_time, chain_controller::txn_msg_limits {}); \
   chain.produce_blocks(2); \
   Make_Account(chain, testapi); \
   Make_Account(chain, dblimits); \
   Make_Account(chain, another); \
   chain.produce_blocks(1); \
   \
   types::setcode handler; \
   handler.code.resize(wasm.size()); \
   memcpy( handler.code.data(), wasm.data(), wasm.size() ); \
   \
   send_set_code_message(chain, handler, "testapi"); \
   send_set_code_message(chain, handler, "another"); \
   \
   /* setup the limit */ \
   BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_" KEY_TYPE "_setup_limit"), {}, {}, {N(dblimits)} ) == WASM_TEST_PASS, "test_db::key_" KEY_TYPE "_setup_limit()" ); \
   /* verify exception for exceeding limit */ \
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_" KEY_TYPE "_min_exceed_limit"), {}, {}, {N(dblimits)} ), \
         tx_code_db_limit_exceeded, [](const tx_code_db_limit_exceeded& e) -> bool { return e.to_detail_string().find("Database limit exceeded") != std::string::npos; } ); \
   /* verify we can update and be under limit */ \
   BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_" KEY_TYPE "_under_limit"), {}, {}, {N(dblimits)} ) == WASM_TEST_PASS, "test_db::key_" KEY_TYPE "_under_limit()" ); \
   /* verify exception for allocating more than remainder */ \
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_" KEY_TYPE "_available_space_exceed_limit"), {}, {}, {N(dblimits)} ), \
         tx_code_db_limit_exceeded, [](const tx_code_db_limit_exceeded& e) -> bool { return e.to_detail_string().find("Database limit exceeded") != std::string::npos; } ); \
   /* verify removing and updating under limit is allowed */ \
   BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_" KEY_TYPE "_another_under_limit"), {}, {}, {N(dblimits)} ) == WASM_TEST_PASS, "test_db::key_" KEY_TYPE "_another_under_limit()" ); \
   /* verify exception for exceeding limit */ \
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_db", "key_" KEY_TYPE "_min_exceed_limit"), {}, {}, {N(dblimits)} ), \
         tx_code_db_limit_exceeded, [](const tx_code_db_limit_exceeded& e) -> bool { return e.to_detail_string().find("Database limit exceeded") != std::string::npos; } ); \
} FC_LOG_AND_RETHROW() }

//Test logic for database storage limiting
BOOST_FIXTURE_TEST_CASE(test_database_limiting_for_str, testing_fixture)
   VERIFY_DB_KEY_TYPES("str")

//Test logic for database storage limiting
BOOST_FIXTURE_TEST_CASE(test_database_limiting_for_i64, testing_fixture)
   VERIFY_DB_KEY_TYPES("i64")

//Test logic for database storage limiting
BOOST_FIXTURE_TEST_CASE(test_database_limiting_for_i128i128, testing_fixture)
   VERIFY_DB_KEY_TYPES("i128i128")

//Test logic for database storage limiting
BOOST_FIXTURE_TEST_CASE(test_database_limiting_for_i64i64i64, testing_fixture)
   VERIFY_DB_KEY_TYPES("i64i64i64")

//Test logic for preventing reducing page memory
BOOST_FIXTURE_TEST_CASE(test_account_api_balance, testing_fixture)
{ try {
   //auto wasm = assemble_wast( readFile2("/home/matu/Documents/Dev/eos/build/contracts/test_api/test_api.wast").c_str() );
   auto wasm = assemble_wast( test_api_wast );

   Make_Blockchain(chain);
   chain.produce_blocks(2);
   Make_Account(chain, testapi);
   Make_Account(chain, acc1);
   chain.produce_blocks(1);

   //Set test code
   types::setcode handler;
   handler.code.resize(wasm.size());
   memcpy( handler.code.data(), wasm.data(), wasm.size() );

   send_set_code_message(chain, handler, "testapi");

   eosio::chain::signed_transaction trx;
   trx.scope = sort_names({"acc1","inita"});
   transaction_emplace_message(trx, "eos",
                      vector<types::account_permission>{ {"inita","active"} },
                      "transfer", types::transfer{"inita", "acc1", 1000,""});
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   chain.push_transaction(trx);
   chain.produce_blocks(1);

   const auto& balance        = chain_db.get<eosio::chain::balance_object,eosio::chain::by_owner_name>( "acc1" );
   const auto& staked_balance = chain_db.get<eosio::chain::staked_balance_object,eosio::chain::by_owner_name>( "acc1" );

   // manually set balance so it can be verified in test_account
   chain_db.modify(balance, [](eosio::chain::balance_object& bo) {
      bo.balance = 24;
   });
   chain_db.modify(staked_balance, [](eosio::chain::staked_balance_object& sbo) {
      sbo.staked_balance = 23;
      sbo.unstaking_balance = 14;
      sbo.last_unstaking_time = types::time(55);
   });

   // Test account
   BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_account", "test_balance_acc1"), {}, {} ),tx_missing_scope, is_tx_missing_scope );
   vector<account_name> scope = sort_names({"testapi", "acc1"});
   BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION_SCOPE( TEST_METHOD("test_account", "test_balance_acc1"), {}, {}, scope ) == WASM_TEST_PASS, "test_account::test_balance_acc1()" );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
