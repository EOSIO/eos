/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <contracts.hpp>

#include <fc/io/fstream.hpp>
#include <fc/io/fstream.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

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
            return abi_serializer(abi, abi_serializer_max_time);
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
   abi_serializer::from_variant(pretty_trx, trx, resolver, abi_serializer_max_time);
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

BOOST_FIXTURE_TEST_CASE( get_table_test, TESTER ) try {
   create_account(N(test));

   // setup contract and abi
   set_code( N(test), contracts::get_table_test_wasm() );
   set_abi( N(test), contracts::get_table_test_abi().data() );
   produce_block();

   // Init some data
   push_action(N(test), N(addnumobj), N(test), mutable_variant_object()("input", 2));
   push_action(N(test), N(addnumobj), N(test), mutable_variant_object()("input", 5));
   push_action(N(test), N(addnumobj), N(test), mutable_variant_object()("input", 7));
   push_action(N(test), N(addhashobj), N(test), mutable_variant_object()("hashinput", "firstinput"));
   push_action(N(test), N(addhashobj), N(test), mutable_variant_object()("hashinput", "secondinput"));
   push_action(N(test), N(addhashobj), N(test), mutable_variant_object()("hashinput", "thirdinput"));
   produce_block();

   // The result of the init will populate
   // For numobjs table (secondary index is on sec64, sec128, secdouble, secldouble)
   // {
   //   "rows": [{
   //       "key": 0,
   //       "sec64": 2,
   //       "sec128": "0x02000000000000000000000000000000",
   //       "secdouble": "2.00000000000000000",
   //       "secldouble": "0x00000000000000000000000000000040"
   //     },{
   //       "key": 1,
   //       "sec64": 5,
   //       "sec128": "0x05000000000000000000000000000000",
   //       "secdouble": "5.00000000000000000",
   //       "secldouble": "0x00000000000000000000000000400140"
   //     },{
   //       "key": 2,
   //       "sec64": 7,
   //       "sec128": "0x07000000000000000000000000000000",
   //       "secdouble": "7.00000000000000000",
   //       "secldouble": "0x00000000000000000000000000c00140"
   //     }
   //   "more": false,
   //   "more2": ""
   // }
   // For hashobjs table (secondary index is on sec256 and sec160):
   // {
   //   "rows": [{
   //       "key": 0,
   //       "hash_input": "firstinput",
   //       "sec256": "05f5aa6b6c5568c53e886591daa9d9f636fa8e77873581ba67ca46a0f96c226e",
   //       "sec160": "2a9baa59f1e376eda2e963c140d13c7e77c2f1fb"
   //     },{
   //       "key": 1,
   //       "hash_input": "secondinput",
   //       "sec256": "3cb93a80b47b9d70c5296be3817d34b48568893b31468e3a76337bb7d3d0c264",
   //       "sec160": "fb9d03d3012dc2a6c7b319f914542e3423550c2a"
   //     },{
   //       "key": 2,
   //       "hash_input": "thirdinput",
   //       "sec256": "2652d68fbbf6000c703b35fdc607b09cd8218cbeea1d108b5c9e84842cdd5ea5",
   //       "sec160": "ab4314638b573fdc39e5a7b107938ad1b5a16414"
   //     }
   //   ],
   //   "more": false,
   //   "more2": ""
   // }


   chain_apis::read_only plugin(*(this->control), fc::microseconds::maximum());
   chain_apis::read_only::get_table_rows_params params{
      .code=N(test),
      .scope="test",
      .limit=1,
      .json=true
   };

      params.table = N(numobjs);

      // i64 primary key type
      params.key_type = "i64";
      params.index_position = "1";
      params.lower_bound = "0";

      auto res_1 = plugin.get_table_rows(params);
      BOOST_TEST(res_1.rows[0].get_object()["key"].as<uint64_t>() == 0);
      BOOST_TEST(res_1.more2 == "1");

      // i64 secondary key type
      params.key_type = "i64";
      params.index_position = "2";
      params.lower_bound = "5";

      auto res_2 = plugin.get_table_rows(params);
      BOOST_TEST(res_2.rows[0].get_object()["sec64"].as<uint64_t>() == 5);
      BOOST_TEST(res_2.more2 == "7");

      // i128 secondary key type
      params.key_type = "i128";
      params.index_position = "3";
      params.lower_bound = "0x05000000000000000000000000000000";

      auto res_3 = plugin.get_table_rows(params);
      chain::uint128_t sec128_expected_value = 5;
      BOOST_CHECK(res_3.rows[0].get_object()["sec128"].as<chain::uint128_t>() == sec128_expected_value);
      BOOST_TEST(res_3.more2 == "0x07000000000000000000000000000000");

      // float64 secondary key type
      params.key_type = "float64";
      params.index_position = "4";
      params.lower_bound = "5.0";

      auto res_4 = plugin.get_table_rows(params);
      float64_t secdouble_expected_value = ui64_to_f64(5);
      double secdouble_res_value = res_4.rows[0].get_object()["secdouble"].as<double>();
      BOOST_CHECK(*reinterpret_cast<float64_t*>(&secdouble_res_value) == secdouble_expected_value);
      BOOST_TEST(res_4.more2 == "7.00000000000000000");

      // float128 secondary key type
      params.key_type = "float128";
      params.index_position = "5";
      params.lower_bound = "0x00000000000000000000000000400140";

      auto res_5 = plugin.get_table_rows(params);
      float128_t secldouble_expected_value = ui64_to_f128(5);
      chain::uint128_t secldouble_res_value =  res_5.rows[0].get_object()["secldouble"].as<chain::uint128_t>();
      BOOST_TEST(*reinterpret_cast<float128_t*>(&secldouble_res_value) == secldouble_expected_value);
      BOOST_TEST(res_5.more2 == "0x00000000000000000000000000c00140");

      params.table = N(hashobjs);

       // sha256 secondary key type
      params.key_type = "sha256";
      params.index_position = "2";
      params.lower_bound = "2652d68fbbf6000c703b35fdc607b09cd8218cbeea1d108b5c9e84842cdd5ea5";

      auto res_6 = plugin.get_table_rows(params);
     
      checksum256_type sec256_expected_value = checksum256_type::hash(std::string("thirdinput"));
      
      checksum256_type sec256_res_value = res_6.rows[0].get_object()["sec256"].as<checksum256_type>();
      BOOST_TEST(std::string(sec256_res_value) == std::string(sec256_expected_value));
      BOOST_TEST(res_6.rows[0].get_object()["hash_input"].as<string>() == std::string("thirdinput"));
      BOOST_TEST(res_6.more2 == "3cb93a80b47b9d70c5296be3817d34b48568893b31468e3a76337bb7d3d0c264");

      // i256 secondary key type
      params.key_type = "i256";
      params.index_position = "2";
      params.lower_bound = "0x2652d68fbbf6000c703b35fdc607b09cd8218cbeea1d108b5c9e84842cdd5ea5";

      auto res_7 = plugin.get_table_rows(params);
      checksum256_type i256_expected_value = checksum256_type::hash(std::string("thirdinput"));
      checksum256_type i256_res_value = res_7.rows[0].get_object()["sec256"].as<checksum256_type>();
      BOOST_CHECK(i256_res_value == i256_expected_value);
      BOOST_TEST(res_7.rows[0].get_object()["hash_input"].as<string>() == "thirdinput");
      BOOST_TEST(res_7.more2 == "0x3cb93a80b47b9d70c5296be3817d34b48568893b31468e3a76337bb7d3d0c264");

      // ripemd160 secondary key type
      params.key_type = "ripemd160";
      params.index_position = "3";
      params.lower_bound = "ab4314638b573fdc39e5a7b107938ad1b5a16414";

      auto res_8 = plugin.get_table_rows(params);
      ripemd160 sec160_expected_value =  ripemd160::hash(std::string("thirdinput"));
      ripemd160 sec160_res_value =  res_8.rows[0].get_object()["sec160"].as<ripemd160>();
      BOOST_CHECK(sec160_res_value == sec160_expected_value);
      BOOST_TEST(res_8.rows[0].get_object()["hash_input"].as<string>() == "thirdinput");
      BOOST_TEST(res_8.more2 == "fb9d03d3012dc2a6c7b319f914542e3423550c2a");

  

} FC_LOG_AND_RETHROW() /// get_table_test
BOOST_AUTO_TEST_SUITE_END()
