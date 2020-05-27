#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/process.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <contracts.hpp>

#include <fc/io/fstream.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>
#include <cstdlib>
#include <array>
#include <utility>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

namespace {
   std::string get_command_result_str(const std::string& system_cmd)
   {
      std::cout << "\n About to run cmd: " << system_cmd << std::endl;
      boost::process::ipstream out;
      boost::process::system(system_cmd, boost::process::std_out > out);
      std::string out_str;
      while(!out.eof())
      {
         std::string line;
         out >> line;
         out_str += line;
      }
      std::cout << "cmd out: " << out_str << std::endl;
      return out_str;
   }
   void bounce_nodeos()
   {
      const std::string kill_nodeos_str = "pkill -9 -f \"nodeos\"";
      system(kill_nodeos_str.c_str());
   }
}

BOOST_AUTO_TEST_SUITE(chain_plugin_tests)

BOOST_FIXTURE_TEST_CASE( get_block_with_invalid_abi, TESTER ) try {
   produce_blocks(2);

   create_accounts( {N(asserter)} );
   produce_block();

   // setup contract and abi
   set_code( N(asserter), contracts::asserter_wasm() );
   set_abi( N(asserter), contracts::asserter_abi().data() );
   produce_blocks(1);

   auto resolver = [&,this]( const account_name& name ) -> optional<abi_serializer> {
      try {
         const auto& accnt  = this->control->db().get<account_object,by_name>( name );
         abi_def abi;
         if (abi_serializer::to_abi(accnt.abi, abi)) {
            return abi_serializer(abi, abi_serializer::create_yield_function( abi_serializer_max_time ));
         }
         return optional<abi_serializer>();
      } FC_RETHROW_EXCEPTIONS(error, "resolver failed at chain_plugin_tests::abi_invalid_type");
   };

   // abi should be resolved
   BOOST_REQUIRE_EQUAL(true, resolver(N(asserter)).valid());

   // make an action using the valid contract & abi
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
   abi_serializer::from_variant(pretty_trx, trx, resolver, abi_serializer::create_yield_function( abi_serializer_max_time ));
   set_transaction_headers(trx);
   trx.sign( get_private_key( N(asserter), "active" ), control->get_chain_id() );
   push_transaction( trx );
   produce_blocks(1);

   // retrieve block num
   uint32_t headnum = this->control->head_block_num();

   char headnumstr[20];
   sprintf(headnumstr, "%d", headnum);
   chain_apis::read_only::get_block_params param{headnumstr};
   chain_apis::read_only plugin(*(this->control), fc::microseconds::maximum());

   // block should be decoded successfully
   std::string block_str = json::to_pretty_string(plugin.get_block(param));
   BOOST_TEST(block_str.find("procassert") != std::string::npos);
   BOOST_TEST(block_str.find("condition") != std::string::npos);
   BOOST_TEST(block_str.find("Should Not Assert!") != std::string::npos);
   BOOST_TEST(block_str.find("011253686f756c64204e6f742041737365727421") != std::string::npos); //action data

   // set an invalid abi (int8->xxxx)
   std::string abi2 = contracts::asserter_abi().data();
   auto pos = abi2.find("int8");
   BOOST_TEST(pos != std::string::npos);
   abi2.replace(pos, 4, "xxxx");
   set_abi(N(asserter), abi2.c_str());
   produce_blocks(1);

   // resolving the invalid abi result in exception
   BOOST_CHECK_THROW(resolver(N(asserter)), invalid_type_inside_abi);

   // get the same block as string, results in decode failed(invalid abi) but not exception
   std::string block_str2 = json::to_pretty_string(plugin.get_block(param));
   BOOST_TEST(block_str2.find("procassert") != std::string::npos);
   BOOST_TEST(block_str2.find("condition") == std::string::npos); // decode failed
   BOOST_TEST(block_str2.find("Should Not Assert!") == std::string::npos); // decode failed
   BOOST_TEST(block_str2.find("011253686f756c64204e6f742041737365727421") != std::string::npos); //action data

} FC_LOG_AND_RETHROW() /// get_block_with_invalid_abi

BOOST_AUTO_TEST_CASE( chain_api_test ) try {

   bounce_nodeos();
   {  // get_info without parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_info";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["server_version_string"].get_type() == variant::type_id::string_type);
   }
   {  // get_info with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_info";
      test_cmd += " -X POST -d '{dsadsa}'";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_activated_protocol_features with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_activated_protocol_features -X POST -d ";
      test_cmd += "\"{ \"lower_bound\":1, \"upper_bound\":1000, \"limit\":10, \"search_by_block_num\":true, \"reverse\":true}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // could be empty but only check whethere the field is there.
      BOOST_REQUIRE(test_cmd_ret_variant["activated_protocol_features"].get_type() == variant::type_id::array_type);
   }
   {  // get_activated_protocol_features without parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_activated_protocol_features";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_activated_protocol_features with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_activated_protocol_features ";
      test_cmd += "-X POST -d '{dsadsad}'";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_block with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block -X POST -d ";
      test_cmd += "\"{\"block_num_or_id\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["block_num"].as<int>() == 1);
   }
   {  // get_block with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_block with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block";
      test_cmd += "\"{\"block_id\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
   {  // get_block_info with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block_info -X POST -d ";
      test_cmd += "\"{\"block_num\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["block_num"].as<int>() == 1);
   }
   {  // get_block_info with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block_info";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_block_info with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block_info";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
   {  // get_block_header_state with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block_header_state -X POST -d ";
      test_cmd += "\"{\"block_num_or_id\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // the irreversible is not available, unknown block number
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3100002) );
   }
   {  // get_block_header_state with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block_header_state";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // the irreversible is not available, invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_block_header_state with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_block_header_state";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
   {  // get_account with valid parameter, but unknown account name
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_account -X POST -d ";
      test_cmd += "\"{\"account_name\":\"default\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown account name
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_account with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_account";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_account with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_account";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
   {  // get_code with valid parameter, but unknown account name
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_code -X POST -d ";
      test_cmd += "\"{\"account_name\":\"default\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown account name
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_code with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_code";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      //  invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_code with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_code";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }



   {  // get_code_hash with valid parameter, but unknown account name
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_code_hash -X POST -d ";
      test_cmd += "\"{\"account_name\":\"default\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown account name
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_code_hash with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_code_hash";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_code_hash with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_code_hash";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }



   {  // get_abi with valid parameter, but unknown account name
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_abi -X POST -d ";
      test_cmd += "\"{\"account_name\":\"default\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown account name
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_abi with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_abi";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_abi with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_abi";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_raw_code_and_abi with valid parameter, but unknown account name
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_raw_code_and_abi -X POST -d ";
      test_cmd += "\"{\"account_name\":\"default\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown account name
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_raw_code_and_abi with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_raw_code_and_abi";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_raw_code_and_abi with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_raw_code_and_abi";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_raw_abi with valid parameter, but unknown account name
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_raw_abi -X POST -d ";
      test_cmd += "\"{\"account_name\":\"default\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown account name
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_raw_abi with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_raw_abi";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_raw_abi with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_raw_abi";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_table_rows with valid parameter, but unknown table
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_table_rows -X POST -d ";
      test_cmd += "\"{\"json\":true,\"code\":\"cancancan345\",\"scope\":\"cancancan345\",\"table\":\"vote\", \"index_position\":2,\"key_type\":\"i128\",\"lower_bound\":\"0x0000000000000000D0F2A472A8EB6A57\",\"upper_bound\":\"0xFFFFFFFFFFFFFFFFD0F2A472A8EB6A57\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown table
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_table_rows with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_table_rows";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_table_rows with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_table_rows";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_table_by_scope with valid parameter, but unknown table
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_table_by_scope -X POST -d ";
      test_cmd += "\"{\"code\":\"cancancan345\",\"table\":\"vote\", \"index_position\":2,\"lower_bound\":\"0x0000000000000000D0F2A472A8EB6A57\",\"upper_bound\":\"0xFFFFFFFFFFFFFFFFD0F2A472A8EB6A57\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown table
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_table_by_scope with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_table_by_scope";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_table_by_scope with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_table_by_scope";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }




   {  // get_currency_balance with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_currency_balance -X POST -d ";
      test_cmd += "\"{\"code\":\"eosio.token\",\"account\":\"unknown\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknown account
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_currency_balance with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_currency_balance";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_currency_balance with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_currency_balance";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }





   {  // get_currency_stats with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_currency_stats -X POST -d ";
      test_cmd += "\"{\"code\":\"eosio.token\",\"symbol\":\"SYS\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_currency_stats with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_currency_stats";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_currency_stats with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_currency_stats";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }



   {  // get_producers with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_producers -X POST -d ";
      test_cmd += "\"{\"json\":true,\"lower_bound\":\"\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["rows"].get_type() == variant::type_id::array_type );
   }
   {  // get_producers with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_producers";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_producers with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_producers";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }



   {  // get_producer_schedule without parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_producer_schedule";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["active"].get_type() == variant::type_id::object_type );
   }
   {  // get_producer_schedule with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_producer_schedule";
      test_cmd += " -X POST -d '{dsadsa}'";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }


   {  // get_scheduled_transactions with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_scheduled_transactions -X POST -d ";
      test_cmd += "\"{\"json\":true,\"lower_bound\":\"\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["transactions"].get_type() == variant::type_id::array_type );
   }
   {  // get_scheduled_transactions with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_scheduled_transactions";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_scheduled_transactions with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_scheduled_transactions";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }




   {  // abi_json_to_bin with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/abi_json_to_bin -X POST -d ";
      test_cmd += "\"{\"code\":\"eosio.token\",\"action\":\"issue\",\"args\":{\"to\":\"eosio.token\", \"quantity\":\"1.0000\%20EOS\",\"memo\":\"m\"}}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // eosio.token account not existed
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // abi_json_to_bin with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/abi_json_to_bin";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // abi_json_to_bin with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/abi_json_to_bin";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // abi_bin_to_json with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/abi_bin_to_json -X POST -d ";
      test_cmd += "\"{\"code\":\"eosio.token\",\"action\":\"issue\",\"args\":\"ee6fff5a5c02c55b6304000000000100a6823403ea3055000000572d3ccdcd0100000000007015d600000000a8ed32322a00000000007015d6000000005c95b1ca102700000000000004454f53000000000968656c6c6f206d616e00\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknownkey(eosio::chain::name):eosio.token
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // abi_bin_to_json with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/abi_bin_to_json";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // abi_bin_to_json with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/abi_bin_to_json";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_required_keys with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_required_keys -X POST -d ";
      test_cmd += "\"{\"transaction\": {\"ref_block_num\":\"100\", \"ref_block_prefix\": \"137469861\",\"expiration\": \"2020-09-25T06:28:49\",\"scope\":[\"initb\", \"initc\"],";
      test_cmd += "\"actions\": [{\"code\": \"currency\",\"type\":\"transfer\",\"recipients\": [\"initb\", \"initc\"],";
      test_cmd += "\"authorization\": [{\"account\": \"initb\",\"permission\": \"active\"}],\"data\":\"000000000041934b000000008041934be803000000000000\"}],";
      test_cmd += "\"signatures\": [],\"authorizations\": []},";
      test_cmd += "\"available_keys\": [\"EOS4toFS3YXEQCkuuw1aqDLrtHim86Gz9u3hBdcBw5KNPZcursVHq\",";
      test_cmd += "\"EOS7d9A3uLe6As66jzN8j44TXJUqJSK3bFjjEEqR4oTvNAB3iM9SA\",\"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\"]}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // unknownkey(eosio::chain::name):eosio.token
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_required_keys with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_required_keys";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_required_keys with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_required_keys";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_transaction_id with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_transaction_id -X POST -d ";
      test_cmd += "\"{\"expiration\":\"2020-08-01T07:15:49\",\"ref_block_num\": 34881,\"ref_block_prefix\": 2972818865,\"max_net_usage_words\": 0,\"max_cpu_usage_ms\": 0,\"delay_sec\": 0,";
      test_cmd += "\"context_free_actions\": [],\"actions\": [{\"account\": \"eosio.token\",\"name\": \"transfer\",\"authorization\": [{\"actor\": \"han\",\"permission\": \"active\"}],";
      test_cmd += "\"data\": \"000000000000a6690000000000ea305501000000000000000453595300000000016d\"}],\"transaction_extensions\": [],";
      test_cmd += "\"signatures\": [\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"],\"context_free_data\": []}\"";
      BOOST_REQUIRE( get_command_result_str(test_cmd) == "\"0be762a6406bab15530e87f21e02d1c58e77944ee55779a76f4112e3b65cac48\"");
   }
   {  // get_transaction_id with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_transaction_id";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_transaction_id with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/get_transaction_id";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   // the result seems not consistent, returns 500 error or just {}
   {  // push_block with valid parameter, but invalid transaction
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_block -X POST -d ";
      test_cmd += "\"{\"block\":\"signed_block\"}\"";
      const std::string ret_str = get_command_result_str(test_cmd);
      if (ret_str != "{}")
      {
         auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
         // invalid transaction
         BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
      }
   }
   {  // push_block with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_block";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // push_block with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_block";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // push_transaction with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_transaction -X POST -d ";
      test_cmd += "\"{\"signatures\":[\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"],";
      test_cmd += "\"compression\": true, \"packed_context_free_data\": \"context_free_data\", \"packed_trx\": \"packed_trx\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // invalid transaction
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // push_transaction with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_transaction";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // push_transaction with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_transaction";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   // does not return 500 when push_transactions failed
   {  // push_transactions with valid parameter, but unknown account
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_transactions -X POST -d ";
      test_cmd += "\"[{\"signatures\":[\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"],";
      test_cmd += "\"compression\": true, \"packed_context_free_data\": \"context_free_data\", \"packed_trx\": \"packed_trx\"}]\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // invalid transaction
      const size_t ret_pos = 0;
      BOOST_REQUIRE(test_cmd_ret_variant[ret_pos]["transaction_id"].as<string>() == "0000000000000000000000000000000000000000000000000000000000000000");
   }
   {  // push_transaction with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_transactions";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // push_transaction with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/push_transactions";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // send_transaction with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/send_transaction -X POST -d ";
      test_cmd += "\"{\"signatures\":[\"SIG_K1_KeqfqiZu1GwUxQb7jzK9Fdks6HFaVBQ9AJtCZZj56eG9qGgvVMVtx8EerBdnzrhFoX437sgwtojf2gfz6S516Ty7c22oEp\"],";
      test_cmd += "\"compression\": true, \"packed_context_free_data\": \"context_free_data\", \"packed_trx\": \"packed_trx\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // invalid transaction
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // send_transaction with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/send_transaction";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // send_transaction with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/chain/send_transaction";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( history_api_test ) try {
   {  // get_actions with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_actions -X POST -d ";
      test_cmd += "\"{\"account_name\":\"test\", \"pos\":-1, \"offset\":2}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // invalid transaction
      BOOST_REQUIRE(test_cmd_ret_variant["last_irreversible_block"].as<int>() > 0);
   }
   {  // get_actions with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_actions";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_actions with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_actions";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_transaction with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_transaction -X POST -d ";
      test_cmd += "\"{\"id\":\"test\", \"block_num_hint\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // invalid transaction
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_transaction with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_transaction";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_transaction with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_transaction";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_key_accounts with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_key_accounts -X POST -d ";
      test_cmd += "\"{\"public_key\":\"EOS6FxXbikY5ZUN9qEdeLbEYLKZzJwRYRr2PuC3rqfSu67LvhPARi\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // account can not be found
      BOOST_REQUIRE(test_cmd_ret_variant["account_names"].get_type() == variant::type_id::array_type);
   }
   {  // get_key_accounts with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_key_accounts";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_key_accounts with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_key_accounts";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_controlled_accounts with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_controlled_accounts -X POST -d ";
      test_cmd += "\"{\"controlling_account\":\"test\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // account can not be found
      BOOST_REQUIRE(test_cmd_ret_variant["controlled_accounts"].get_type() == variant::type_id::array_type);
   }
   {  // get_controlled_accounts with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_controlled_accounts";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      int err_code = test_cmd_ret_variant["error"]["code"].as<int>();
      // invalid_http_request
      BOOST_REQUIRE( (test_cmd_ret_variant["code"].as<int>() == 400) && (err_code == 3200006) );
   }
   {  // get_controlled_accounts with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/history/get_controlled_accounts";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( net_api_test ) try {
   {  // connect with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/connect -X POST -d ";
      test_cmd += "\"{\"endpoint\": \"localhost\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // connect with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/connect";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // connect with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/connect";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // disconnect with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/disconnect -X POST -d ";
      test_cmd += "\"{\"endpoint\": \"localhost\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // disconnect with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/disconnect";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // disconnect with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/disconnect -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }

   {  // status with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/status -X POST -d ";
      test_cmd += "\"{\"endpoint\": \"localhost\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // status with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/status";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // status with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/status -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }

   {  // connections with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/connections";
      BOOST_REQUIRE(get_command_result_str(test_cmd) == "[]");
   }
   {  // connections with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/net/connections -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto ret_str = get_command_result_str(test_cmd);
      if (ret_str != "[]") {
         auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
         BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
      }
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( producer_api_test ) try {
   {  // pause with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/pause";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["result"].as<string>() == "ok" );
   }
   {  // pause with invalid parameter, should it return 404?
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/pause -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["result"].as<string>() == "ok" );
//      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
   {  // resume with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/resume";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["result"].as<string>() == "ok" );
   }
   {  // resume with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/resume -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["result"].as<string>() == "ok" );
//      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
   // should it with return code and result?
   {  // paused with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/paused";
      auto test_cmd_ret_str = get_command_result_str(test_cmd);
      BOOST_REQUIRE( test_cmd_ret_str == "false" || test_cmd_ret_str == "true" );
   }
   {  // paused with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/paused -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_str = get_command_result_str(test_cmd);
      BOOST_REQUIRE( test_cmd_ret_str == "false" || test_cmd_ret_str == "true" );
//      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }


   {  // get_runtime_options with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_runtime_options";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["max_transaction_time"].get_type() == variant::type_id::uint64_type );
   }
   {  // get_runtime_options with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_runtime_options -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["max_transaction_time"].get_type() == variant::type_id::uint64_type );
   }
   {  // update_runtime_options with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/update_runtime_options";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // update_runtime_options with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/update_runtime_options  -X POST -d ";
      test_cmd += "\"{\"max_transaction_time\":30, \"max_irreversible_block_age\":1, \"produce_time_offset_us\":10000,";
      test_cmd += "\"last_block_time_offset_us\":0, \"max_scheduled_transaction_time_per_block_ms\":10000,";
      test_cmd += "\"subjective_cpu_leeway_us\":0, \"incoming_defer_ratio\":1.0, \"greylist_limit\":100}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }
   {  // update_runtime_options with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/update_runtime_options ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // update_runtime_options with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/update_runtime_options -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }

   {  // add_greylist_accounts with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/add_greylist_accounts  -X POST -d ";
      test_cmd += "\"{\"accounts\":[\"test1\", \"test2\"]}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }
   {  // add_greylist_accounts with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/add_greylist_accounts ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // add_greylist_accounts with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/add_greylist_accounts -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }

   {  // remove_greylist_accounts with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/remove_greylist_accounts  -X POST -d ";
      test_cmd += "\"{\"accounts\":[\"test1\", \"test2\"]}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }
   {  // remove_greylist_accounts with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/remove_greylist_accounts ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // remove_greylist_accounts with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/remove_greylist_accounts -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }

   {  // get_greylist with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_greylist ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["accounts"].get_type() == variant::type_id::array_type);
   }
   {  // get_greylist with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_greylist -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["accounts"].get_type() == variant::type_id::array_type);
   }
   {  // get_whitelist_blacklist with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_whitelist_blacklist ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["actor_whitelist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["actor_blacklist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["contract_whitelist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["contract_blacklist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["action_blacklist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["key_blacklist"].get_type() == variant::type_id::array_type);
   }
   {  // get_whitelist_blacklist with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_whitelist_blacklist -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["actor_whitelist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["actor_blacklist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["contract_whitelist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["contract_blacklist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["action_blacklist"].get_type() == variant::type_id::array_type &&
                     test_cmd_ret_variant["key_blacklist"].get_type() == variant::type_id::array_type);
   }
   {  // set_whitelist_blacklist with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/set_whitelist_blacklist  -X POST -d ";
      test_cmd += "\"{\"actor_whitelist\":[\"test2\"], \"actor_blacklist\":[\"test3\"], \"contract_whitelist\":[\"test4\"], ";
      test_cmd += "\"contract_blacklist\":[\"test5\"], \"action_blacklist\":[], \"key_blacklist\":[]}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }
   {  // set_whitelist_blacklist with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/set_whitelist_blacklist ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // set_whitelist_blacklist with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/set_whitelist_blacklist -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }

   {  // get_integrity_hash with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_integrity_hash ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["head_block_id"].get_type() == variant::type_id::string_type &&
                     test_cmd_ret_variant["integrity_hash"].get_type() == variant::type_id::string_type);
   }
   {  // get_integrity_hash with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_integrity_hash -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["head_block_id"].get_type() == variant::type_id::string_type &&
            test_cmd_ret_variant["integrity_hash"].get_type() == variant::type_id::string_type);
   }

   {  // create_snapshot with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/create_snapshot  -X POST -d ";
      test_cmd += "\"content-type: application/x-www-form-urlencoded; charset=UTF-8\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // create_snapshot is disabled
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // create_snapshot with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/create_snapshot ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // create_snapshot is disabled
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // create_snapshot with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/create_snapshot -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // create_snapshot is disabled
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }

   {  // get_scheduled_protocol_feature_activations with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_scheduled_protocol_feature_activations ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["protocol_features_to_activate"].get_type() == variant::type_id::array_type);
   }
   {  // get_scheduled_protocol_feature_activations with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_scheduled_protocol_feature_activations -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE( test_cmd_ret_variant["protocol_features_to_activate"].get_type() == variant::type_id::array_type);
   }

   {  // schedule_protocol_feature_activations with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/schedule_protocol_feature_activations  -X POST -d ";
      test_cmd += "\"{\"protocol_features_to_activate\":[]}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }
   {  // schedule_protocol_feature_activations with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/schedule_protocol_feature_activations ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // schedule_protocol_feature_activations with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/schedule_protocol_feature_activations -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["result"].as<string>() == "ok");
   }


   {  // get_supported_protocol_features with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_supported_protocol_features  -X POST -d ";
      test_cmd += "\"{\"exclude_disabled\":true, \"exclude_unactivatable\":true}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant[(size_t)0]["feature_digest"].get_type() == variant::type_id::string_type);
   }
   {  // get_supported_protocol_features with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_supported_protocol_features ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_supported_protocol_features with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_supported_protocol_features -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant[(size_t)0]["feature_digest"].get_type() == variant::type_id::string_type);
   }

   {  // get_account_ram_corrections with valid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_account_ram_corrections  -X POST -d ";
      test_cmd += "\"{\"lower_bound\":\"\", \"upper_bound\":\"\", \"limit\":1, \"reverse\":false}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["rows"].get_type() == variant::type_id::array_type);
   }
   {  // get_account_ram_corrections with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_account_ram_corrections ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_account_ram_corrections with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/producer/get_account_ram_corrections -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["rows"].get_type() == variant::type_id::array_type);
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( wallet_api_test ) try {
   {  // set_timeout with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/set_timeout  -X POST -d ";
      test_cmd += "\"1234567\"";
      BOOST_REQUIRE(get_command_result_str(test_cmd) == "{}");
   }
   {  // set_timeout with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/set_timeout ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // set_timeout with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/set_timeout -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }/*
   {  // sign_transaction with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/sign_transaction  -X POST -d ";
      test_cmd += "\"{\"expiration\": \"2018-09-08T07:36:24\",\"ref_block_num\": 8873,\"ref_block_prefix\": 3785634453,";
      test_cmd += "\"max_net_usage_words\": 0,\"max_cpu_usage_ms\": 0,\"delay_sec\": 0,\"context_free_actions\": [],";
      test_cmd += "\"actions\": [],\"transaction_extensions\": [],\"signatures\": [],";
      test_cmd += "\"context_free_data\": []},[\"EOS772JxdvxG8dykycZg9CUwuMMeVxuPHcQfpqqjGL8EdsYoibxVv\"],";
      test_cmd += "\"cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }*/
   {  // sign_transaction with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/sign_transaction ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 422);
   }
   {  // sign_transaction with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/sign_transaction -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }

   {  // create with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/create  -X POST -d ";
      test_cmd += "\"test1\"";
      BOOST_REQUIRE(get_command_result_str(test_cmd).empty() == false);
   }
   {  // create with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/create ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // create with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/create -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }

   {  // open with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/open  -X POST -d ";
      test_cmd += "\"test1\"";
      BOOST_REQUIRE(get_command_result_str(test_cmd).empty() == false);
   }
   {  // open with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/open ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // open with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/open -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }

   {  // lock_all with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/lock_all ";
      BOOST_REQUIRE(get_command_result_str(test_cmd) == "{}");
   }
   {  // lock_all with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/lock_all -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      BOOST_REQUIRE(get_command_result_str(test_cmd) == "{}");
   }
   {  // lock with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/lock -X POST -d ";
      test_cmd += "\"name\":\"auser\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // lock with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/lock ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // lock with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/lock -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }

   {  // unlock with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/unlock -X POST -d ";
      test_cmd += "\"{\"name\":\"auser\", \"password\":\"nopassword\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // unlock with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/unlock ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // unlock with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/unlock -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      /// TODO: need to throw invalid http exception with error return code 400
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }



   {  // import_key with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/import_key -X POST -d ";
      test_cmd += "\"{\"name\":\"auser\", \"wif_key\":\"nokey\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // import_key with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/import_key ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // import_key with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/import_key -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      /// TODO: need to throw invalid http exception with error return code 400
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }

   {  // remove_key with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/remove_key -X POST -d ";
      test_cmd += "\"{\"name\":\"auser\", \"password\":\"nopassword\", \"key\":\"unknownkey\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // remove_key with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/remove_key ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // remove_key with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/remove_key -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      /// TODO: need to throw invalid http exception with error return code 400
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }

   {  // create_key with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/create_key -X POST -d ";
      test_cmd += "\"{\"name\":\"auser\", \"key_type\":\"unknownkey\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // create_key with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/create_key ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // create_key with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/create_key -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      /// TODO: need to throw invalid http exception with error return code 400
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }

   {  // list_wallets with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/list_wallets ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant.get_type() == variant::type_id::array_type);
   }
   {  // list_wallets with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/list_wallets -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      /// TODO: need to throw invalid http exception with error return code 400
      BOOST_REQUIRE(test_cmd_ret_variant.get_type() == variant::type_id::array_type);
   }

   {  // list_keys with valid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/list_keys -X POST -d ";
      test_cmd += "\"{\"name\":\"auser\", \"pw\":\"unknownkey\"}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // list_keys with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/list_keys ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // list_keys with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/list_keys -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      /// TODO: need to throw invalid http exception with error return code 400
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_public_keys with empty parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/get_public_keys ";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
   {  // get_public_keys with invalid parameter
      string test_cmd = "curl http://127.0.0.1:7777/v1/wallet/get_public_keys -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      /// TODO: need to throw invalid http exception with error return code 400
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 500);
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( test_control_api_test ) try {
   {  // kill_node_on_producer with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/test_control/kill_node_on_producer -X POST -d ";
      test_cmd += "\"{\"name\":\"auser\", \"where_in_sequence\":12, \"based_on_lib\":true}\"";
      BOOST_REQUIRE(get_command_result_str(test_cmd) == "{}");
   }
   {  // kill_node_on_producer with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/test_control/kill_node_on_producer";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // kill_node_on_producer with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/test_control/kill_node_on_producer -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      // should be 400 instead
      BOOST_REQUIRE(get_command_result_str(test_cmd) == "{}");
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( trace_api_test ) try {
   {  // get_block with valid parameter,
      string test_cmd = "curl http://127.0.0.1:8888/v1/trace_api/get_block -X POST -d ";
      test_cmd += "\"{\"block_num\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 404);
   }
   {  // get_block with empty parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/trace_api/get_block";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
   {  // get_block with invalid parameter
      string test_cmd = "curl http://127.0.0.1:8888/v1/trace_api/get_block -X POST -d ";
      test_cmd += "\"{\"block\":1}\"";
      auto test_cmd_ret_variant = fc::json::from_string(get_command_result_str(test_cmd));
      BOOST_REQUIRE(test_cmd_ret_variant["code"].as<int>() == 400);
   }
} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
