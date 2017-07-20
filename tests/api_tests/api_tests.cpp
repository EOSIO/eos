#include <boost/test/unit_test.hpp>

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

#include <test_api/test_api.hpp>
#include <test_api/test_types.hpp>
#include <test_api/test_message.hpp>

FC_REFLECT( dummy_message, (a)(b)(c) );

using namespace eos;
using namespace chain;

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
         trx.messages[0].code = config::SystemContractName;
         trx.messages[0].recipients = {account};
         trx.setMessage(0, "setcode", handler);
         trx.expiration = chain.head_block_time() + 100;
         trx.set_reference_block(chain.head_block_id());
         chain.push_transaction(trx);
         chain.produce_blocks(1);
      }
} FC_LOG_AND_RETHROW( ) }

uint32_t CallFunction( testing_blockchain& chain, const types::Message& msg ) {
   eos::chain::SignedTransaction trx;
   trx.scope = {msg.code};
   
   trx.emplaceMessage(msg);
   
   trx.expiration = chain.head_block_time() + 100;
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

#define TEST_METHOD(C, N) ((uint64_t(C::test_id)<<32) | N)

#define CALL_TEST_FUNCTION(TYPE, REC, AUTH) CallFunction(chain, Message{"test_api", REC, AUTH, TYPE})
#define CALL_TEST_FUNCTION2(TYPE, REC, AUTH, DATA) CallFunction(chain, Message{"test_api", REC, AUTH, TYPE, DATA})

bool is_access_violation(Runtime::Exception const & e) { return e.cause == Runtime::Exception::Cause::accessViolation; }
bool is_tx_missing_recipient(tx_missing_recipient const & e) { return true;}
bool is_tx_missing_auth(tx_missing_auth const & e) { return true; }
bool is_assert_exception(fc::assert_exception const & e) { return true; }

BOOST_FIXTURE_TEST_CASE(test_all, testing_fixture)
{ try {

      auto test_api_wast = readFile2("/home/matu/Documents/Dev/eos/contracts/test_api/test_api.wast");
      std::cout << test_api_wast << std::endl;

      Make_Blockchain(chain);
      chain.produce_blocks(2);
      Make_Account(chain, test_api);
      Make_Account(chain, acc1);
      Make_Account(chain, acc2);
      Make_Account(chain, acc3);
      Make_Account(chain, acc4);
      chain.produce_blocks(1);

      //Set test code
      SetCode(chain, "test_api", test_api_wast.c_str());

      //Test types
      for(int i=1; i<=test_types::total_tests(); i++)
         BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD(test_types, i), {}, {} ) == WASM_TEST_PASS, "test_types::test"<<i<<"()" );

      //Test message
      dummy_message dummy13{DUMMY_MESSAGE_DEFAULT_A, DUMMY_MESSAGE_DEFAULT_B, DUMMY_MESSAGE_DEFAULT_C};
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION2( TEST_METHOD(test_message, 1), {}, {}, dummy13) == WASM_TEST_PASS, "test_message::test1()" );

      // std::vector<char> bytes64k((1<<16));
      // BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION2( TEST_METHOD(test_message, 2), {}, {}, bytes64k) == WASM_TEST_PASS, "test_message::test2()" );

      // BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION2( TEST_METHOD(test_message, 3), {}, {}, bytes64k), 
      //    Runtime::Exception, is_access_violation );

      // BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION2( TEST_METHOD(test_message, 4), {}, {}, bytes64k), 
      //    Runtime::Exception, is_access_violation );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 5), {}, {}), 
         tx_missing_recipient, is_tx_missing_recipient );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 5), sort_names({"acc1"}), {}), 
         tx_missing_recipient, is_tx_missing_recipient );

      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 5), sort_names({"acc2"}), {}), 
         tx_missing_recipient, is_tx_missing_recipient );

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 5), sort_names({"acc1","acc2"}), {}) == WASM_TEST_PASS, "test_message::test5()");



      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 6), {}, {}), 
         tx_missing_auth, is_tx_missing_auth );

      auto a3only = vector<types::AccountPermission>{{"acc3","active"}};
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 6), {}, a3only), 
         tx_missing_auth, is_tx_missing_auth );

      auto a4only = vector<types::AccountPermission>{{"acc4","active"}};
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 6), {}, a4only),
         tx_missing_auth, is_tx_missing_auth );

      auto a3a4 = vector<types::AccountPermission>{{"acc3","active"}, {"acc4","active"}};
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 6), {}, a3a4 ) == WASM_TEST_PASS, "test_message::test6()");

      
      BOOST_CHECK_EXCEPTION( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 7), {}, {}),
         fc::assert_exception, is_assert_exception );

      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION( TEST_METHOD(test_message, 8), {}, {} ) == WASM_TEST_PASS, "test_message::test8()");


      chain.produce_blocks(1);
      
      uint32_t now = chain.head_block_time().sec_since_epoch();
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION2( TEST_METHOD(test_message, 9), {}, {}, now) == WASM_TEST_PASS, "test_message::test9()");

      chain.produce_blocks(1);
      BOOST_CHECK_MESSAGE( CALL_TEST_FUNCTION2( TEST_METHOD(test_message, 9), {}, {}, now) == WASM_TEST_FAIL, "test_message::test9()");


} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
