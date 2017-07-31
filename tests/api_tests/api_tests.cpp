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
FC_REFLECT( dummy_message, (a)(b)(c) );
FC_REFLECT( u128_msg, (values) );

using namespace eos;
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

void SetCode( testing_blockchain& chain, AccountName account, const char* wast ) {
   try {
      types::setcode handler;
      handler.account = account;

      auto wasm = assemble_wast( wast );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      {
         eos::chain::SignedTransaction trx;
         trx.scope = {account};
         trx.messages.resize(1);
         trx.messages[0].code = config::EosContractName;
         //trx.messages[0].recipients = {account};
         trx.setMessage(0, "setcode", handler);
         trx.expiration = chain.head_block_time() + 100;
         trx.set_reference_block(chain.head_block_id());
         chain.push_transaction(trx);
         chain.produce_blocks(1);
      }
} FC_LOG_AND_RETHROW( ) }

uint32_t CallFunction( testing_blockchain& chain, const types::Message& msg, const vector<types::AccountPermission>& auths, const vector<char>& data, const vector<AccountName>& scope = {N(test_api)}) {
   static int expiration = 1;
   eos::chain::SignedTransaction trx;
   trx.authorizations = auths;
   trx.scope = scope;
   
   //msg.data.clear();
   vector<char>& dest = *(vector<char> *)(&msg.data);
   std::copy(data.begin(), data.end(), std::back_inserter(dest));

   std::cout << "MANDO: " << msg.code << " " << msg.type << std::endl;
   trx.emplaceMessage(msg);
   
   trx.expiration = chain.head_block_time() + expiration++;
   trx.set_reference_block(chain.head_block_id());
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

#define CALL_TEST_FUNCTION(TYPE, AUTH, DATA) CallFunction(chain, Message{"test_api", TYPE}, AUTH, DATA)
#define CALL_TEST_FUNCTION_SCOPE(TYPE, AUTH, DATA, SCOPE) CallFunction(chain, Message{"test_api", TYPE}, AUTH, DATA, SCOPE)

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
bool is_assert_exception(fc::assert_exception const & e) { return true; }

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


BOOST_FIXTURE_TEST_CASE(test_all, testing_fixture)
{ try {

      std::string test_api_wast_str(test_api_wast);
      //auto test_api_wast = readFile2("/home/matu/Documents/Dev/eos/contracts/test_api/test_api.wast");
      //std::cout << test_api_wast << std::endl;

      Make_Blockchain(chain);
      chain.produce_blocks(2);
      Make_Account(chain, test_api);
      Make_Account(chain, another);
      Make_Account(chain, acc1);
      Make_Account(chain, acc2);
      Make_Account(chain, acc3);
      Make_Account(chain, acc4);
      chain.produce_blocks(1);

      //Set test code
      SetCode(chain, "test_api", test_api_wast_str.c_str());
      SetCode(chain, "acc1", test_api_wast_str.c_str());
      SetCode(chain, "acc2", test_api_wast_str.c_str());

      //Test types
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "types_size"),     {}, {} ) == WASM_TEST_PASS, "test_types::types_size()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "char_to_symbol"), {}, {} ) == WASM_TEST_PASS, "test_types::char_to_symbol()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "string_to_name"), {}, {} ) == WASM_TEST_PASS, "test_types::string_to_name()" );
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_types", "name_class"),     {}, {} ) == WASM_TEST_PASS, "test_types::name_class()" );

      //Test message
      dummy_message dummy13{DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "read_message"), {}, fc::raw::pack(dummy13) ) == WASM_TEST_PASS, "test_message::read_message()" );

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
      
      auto a3only = vector<types::AccountPermission>{{"acc3","active"}};
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "require_auth"), a3only, {}),
         tx_missing_auth, is_tx_missing_auth );

      auto a4only = vector<types::AccountPermission>{{"acc4","active"}};
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD("test_message", "require_auth"), a4only, {}),
         tx_missing_auth, is_tx_missing_auth );

      auto a3a4 = vector<types::AccountPermission>{{"acc3","active"}, {"acc4","active"}};
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
      BOOST_CHECK_EQUAL( (
         capture[0] == "abcde" && 
         capture[1] == "ab.de"  && 
         capture[2] == "1q1q1q" &&
         capture[3] == "abcdefghijk" &&
         capture[4] == "abcdefghijkl" &&
         capture[5] == "abcdefghijklm" &&
         capture[6] == "abcdefghijklm" &&
         capture[7] == "abcdefghijklm" 
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

      //Test db (i64)
      const auto& idx = chain_db.get_index<key_value_index, by_scope_key>();

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i64_general"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i64_general()" );
      BOOST_CHECK_EQUAL( std::distance(idx.begin(), idx.end()) , 4);
      
      auto itr = idx.lower_bound( boost::make_tuple( N(test_api), N(test_api), N(test_table)) );

      BOOST_CHECK_EQUAL((uint64_t)itr->key, N(alice)); ++itr;
      BOOST_CHECK_EQUAL((uint64_t)itr->key, N(bob)); ++itr;
      BOOST_CHECK_EQUAL((uint64_t)itr->key, N(carol)); ++itr;
      BOOST_CHECK_EQUAL((uint64_t)itr->key, N(dave));

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

      //Test db (i128i128)
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD("test_db", "key_i128i128_general"), {}, {} ) == WASM_TEST_PASS, "test_db::key_i128i128_general()" );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
