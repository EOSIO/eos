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

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <array>
#include <utility>

#include <eosio/testing/backing_store_tester_macros.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(get_table_tests)

using backing_store_ts = boost::mpl::list<TESTER, ROCKSDB_TESTER>;

transaction_trace_ptr
issue_tokens( TESTER& t, account_name issuer, account_name to, const asset& amount,
              std::string memo = "", account_name token_contract = "eosio.token"_n )
{
   signed_transaction trx;

   trx.actions.emplace_back( t.get_action( token_contract, "issue"_n,
                                           vector<permission_level>{{issuer, config::active_name}},
                                           mutable_variant_object()
                                             ("to", issuer.to_string())
                                             ("quantity", amount)
                                             ("memo", memo)
   ) );

   trx.actions.emplace_back( t.get_action( token_contract, "transfer"_n,
                                           vector<permission_level>{{issuer, config::active_name}},
                                           mutable_variant_object()
                                             ("from", issuer.to_string())
                                             ("to", to.to_string())
                                             ("quantity", amount)
                                             ("memo", memo)
   ) );

   t.set_transaction_headers(trx);
   trx.sign( t.get_private_key( issuer, "active" ), t.control->get_chain_id()  );
   return t.push_transaction( trx );
}

BOOST_AUTO_TEST_CASE_TEMPLATE( get_scope_test, TESTER_T, backing_store_ts) { try {
   TESTER_T t;
   t.produce_blocks(2);

   t.create_accounts({ "eosio.token"_n, "eosio.ram"_n, "eosio.ramfee"_n, "eosio.stake"_n,
      "eosio.bpay"_n, "eosio.vpay"_n, "eosio.saving"_n, "eosio.names"_n });

   std::vector<account_name> accs{"inita"_n, "initb"_n, "initc"_n, "initd"_n};
   t.create_accounts(accs);
   t.produce_block();

   t.set_code( "eosio.token"_n, contracts::eosio_token_wasm() );
   t.set_abi( "eosio.token"_n, contracts::eosio_token_abi().data() );
   t.produce_blocks(1);

   // create currency
   auto act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", eosio::chain::asset::from_string("1000000000.0000 SYS"));
   t.push_action("eosio.token"_n, "create"_n, "eosio.token"_n, act );

   // issue
   for (account_name a: accs) {
      issue_tokens( t, config::system_account_name, a, eosio::chain::asset::from_string("999.0000 SYS") );
   }
   t.produce_blocks(1);

   // iterate over scope
   eosio::chain_apis::read_only plugin(*(t.control), {}, fc::microseconds::maximum());
   eosio::chain_apis::read_only::get_table_by_scope_params param{"eosio.token"_n, "accounts"_n, "inita", "", 10};
   eosio::chain_apis::read_only::get_table_by_scope_result result = plugin.read_only::get_table_by_scope(param);

   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more);
   if (result.rows.size() >= 4) {
      BOOST_REQUIRE_EQUAL(name("eosio.token"_n), result.rows[0].code);
      BOOST_REQUIRE_EQUAL(name("inita"_n), result.rows[0].scope);
      BOOST_REQUIRE_EQUAL(name("accounts"_n), result.rows[0].table);
      BOOST_REQUIRE_EQUAL(name("eosio"_n), result.rows[0].payer);
      BOOST_REQUIRE_EQUAL(1u, result.rows[0].count);

      BOOST_REQUIRE_EQUAL(name("initb"_n), result.rows[1].scope);
      BOOST_REQUIRE_EQUAL(name("initc"_n), result.rows[2].scope);
      BOOST_REQUIRE_EQUAL(name("initd"_n), result.rows[3].scope);
   }

   param.lower_bound = "initb";
   param.upper_bound = "initc";
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more);
   if (result.rows.size() >= 2) {
      BOOST_REQUIRE_EQUAL(name("initb"_n), result.rows[0].scope);
      BOOST_REQUIRE_EQUAL(name("initc"_n), result.rows[1].scope);
   }

   param.lower_bound = "initc";
   param.upper_bound = "initb";
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());

   param.lower_bound = "initb";
   param.upper_bound = "inite";
   param.reverse = true;
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(3u, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more);

   BOOST_REQUIRE_EQUAL(name("initd"_n), result.rows[0].scope);
   BOOST_REQUIRE_EQUAL(name("initc"_n), result.rows[1].scope);
   BOOST_REQUIRE_EQUAL(name("initb"_n), result.rows[2].scope);

   param.limit = 1;
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL("initc", result.more);

   param.table = name(0);
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL("initc", result.more);

   param.table = "invalid"_n;
   result = plugin.read_only::get_table_by_scope(param);
   BOOST_REQUIRE_EQUAL(0u, result.rows.size());
   BOOST_REQUIRE_EQUAL("", result.more);

} FC_LOG_AND_RETHROW() } /// get_scope_test

BOOST_AUTO_TEST_CASE_TEMPLATE( get_table_test, TESTER_T, backing_store_ts) { try {
   TESTER_T t;
   t.produce_blocks(2);

   t.create_accounts({ "eosio.token"_n, "eosio.ram"_n, "eosio.ramfee"_n, "eosio.stake"_n,
      "eosio.bpay"_n, "eosio.vpay"_n, "eosio.saving"_n, "eosio.names"_n });

   std::vector<account_name> accs{"inita"_n, "initb"_n};
   t.create_accounts(accs);
   t.produce_block();

   t.set_code( "eosio.token"_n, contracts::eosio_token_wasm() );
   t.set_abi( "eosio.token"_n, contracts::eosio_token_abi().data() );
   t.produce_blocks(1);

   // create currency
   auto act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", eosio::chain::asset::from_string("1000000000.0000 SYS"));
   t.push_action("eosio.token"_n, "create"_n, "eosio.token"_n, act );

   // issue
   for (account_name a: accs) {
      issue_tokens( t, config::system_account_name, a, eosio::chain::asset::from_string("10000.0000 SYS") );
   }
   t.produce_blocks(1);

   // create currency 2
   act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", eosio::chain::asset::from_string("1000000000.0000 AAA"));
   t.push_action("eosio.token"_n, "create"_n, "eosio.token"_n, act );
   // issue
   for (account_name a: accs) {
      issue_tokens( t, config::system_account_name, a, eosio::chain::asset::from_string("9999.0000 AAA") );
   }
   t.produce_blocks(1);

   // create currency 3
   act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", eosio::chain::asset::from_string("1000000000.0000 CCC"));
   t.push_action("eosio.token"_n, "create"_n, "eosio.token"_n, act );
   // issue
   for (account_name a: accs) {
      issue_tokens( t, config::system_account_name, a, eosio::chain::asset::from_string("7777.0000 CCC") );
   }
   t.produce_blocks(1);

   // create currency 3
   act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", eosio::chain::asset::from_string("1000000000.0000 BBB"));
   t.push_action("eosio.token"_n, "create"_n, "eosio.token"_n, act );
   // issue
   for (account_name a: accs) {
      issue_tokens( t, config::system_account_name, a, eosio::chain::asset::from_string("8888.0000 BBB") );
   }
   t.produce_blocks(1);

   // get table: normal case
   eosio::chain_apis::read_only plugin(*(t.control), {}, fc::microseconds::maximum());
   eosio::chain_apis::read_only::get_table_rows_params p;
   p.code = "eosio.token"_n;
   p.scope = "inita";
   p.table = "accounts"_n;
   p.json = true;
   p.index_position = "primary";
   eosio::chain_apis::read_only::get_table_rows_result result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   BOOST_REQUIRE_EQUAL(false, result.more);
   if (result.rows.size() >= 4) {
      BOOST_REQUIRE_EQUAL("9999.0000 AAA", result.rows[0]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("8888.0000 BBB", result.rows[1]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("7777.0000 CCC", result.rows[2]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("10000.0000 SYS", result.rows[3]["balance"].as_string());
   }

   // get table: reverse ordered
   p.reverse = true;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   BOOST_REQUIRE_EQUAL(false, result.more);
   if (result.rows.size() >= 4) {
      BOOST_REQUIRE_EQUAL("9999.0000 AAA", result.rows[3]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("8888.0000 BBB", result.rows[2]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("7777.0000 CCC", result.rows[1]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("10000.0000 SYS", result.rows[0]["balance"].as_string());
   }

   // get table: reverse ordered, with ram payer
   p.reverse = true;
   p.show_payer = true;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   BOOST_REQUIRE_EQUAL(false, result.more);
   if (result.rows.size() >= 4) {
      BOOST_REQUIRE_EQUAL("9999.0000 AAA", result.rows[3]["data"]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("8888.0000 BBB", result.rows[2]["data"]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("7777.0000 CCC", result.rows[1]["data"]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("10000.0000 SYS", result.rows[0]["data"]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("eosio", result.rows[0]["payer"].as_string());
      BOOST_REQUIRE_EQUAL("eosio", result.rows[1]["payer"].as_string());
      BOOST_REQUIRE_EQUAL("eosio", result.rows[2]["payer"].as_string());
      BOOST_REQUIRE_EQUAL("eosio", result.rows[3]["payer"].as_string());
   }
   p.show_payer = false;

   // get table: normal case, with bound
   p.lower_bound = "BBB";
   p.upper_bound = "CCC";
   p.reverse = false;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   BOOST_REQUIRE_EQUAL(false, result.more);
   if (result.rows.size() >= 2) {
      BOOST_REQUIRE_EQUAL("8888.0000 BBB", result.rows[0]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("7777.0000 CCC", result.rows[1]["balance"].as_string());
   }

   // get table: reverse case, with bound
   p.lower_bound = "BBB";
   p.upper_bound = "CCC";
   p.reverse = true;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(2u, result.rows.size());
   BOOST_REQUIRE_EQUAL(false, result.more);
   if (result.rows.size() >= 2) {
      BOOST_REQUIRE_EQUAL("8888.0000 BBB", result.rows[1]["balance"].as_string());
      BOOST_REQUIRE_EQUAL("7777.0000 CCC", result.rows[0]["balance"].as_string());
   }

   // get table: normal case, with limit
   p.lower_bound = p.upper_bound = "";
   p.limit = 1;
   p.reverse = false;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL(true, result.more);
   if (result.rows.size() >= 1) {
      BOOST_REQUIRE_EQUAL("9999.0000 AAA", result.rows[0]["balance"].as_string());
   }

   // get table: reverse case, with limit
   p.lower_bound = p.upper_bound = "";
   p.limit = 1;
   p.reverse = true;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL(true, result.more);
   if (result.rows.size() >= 1) {
      BOOST_REQUIRE_EQUAL("10000.0000 SYS", result.rows[0]["balance"].as_string());
   }

   // get table: normal case, with bound & limit
   p.lower_bound = "BBB";
   p.upper_bound = "CCC";
   p.limit = 1;
   p.reverse = false;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL(true, result.more);
   if (result.rows.size() >= 1) {
      BOOST_REQUIRE_EQUAL("8888.0000 BBB", result.rows[0]["balance"].as_string());
   }

   // get table: reverse case, with bound & limit
   p.lower_bound = "BBB";
   p.upper_bound = "CCC";
   p.limit = 1;
   p.reverse = true;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL(true, result.more);
   if (result.rows.size() >= 1) {
      BOOST_REQUIRE_EQUAL("7777.0000 CCC", result.rows[0]["balance"].as_string());
   }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE_TEMPLATE( get_table_by_seckey_test, TESTER_T, backing_store_ts) { try {
   TESTER_T t;
   t.produce_blocks(2);

   t.create_accounts({ "eosio.token"_n, "eosio.ram"_n, "eosio.ramfee"_n, "eosio.stake"_n,
      "eosio.bpay"_n, "eosio.vpay"_n, "eosio.saving"_n, "eosio.names"_n });

   std::vector<account_name> accs{"inita"_n, "initb"_n, "initc"_n, "initd"_n};
   t.create_accounts(accs);
   t.produce_block();

   t.set_code( "eosio.token"_n, contracts::eosio_token_wasm() );
   t.set_abi( "eosio.token"_n, contracts::eosio_token_abi().data() );
   t.produce_blocks(1);

   // create currency
   auto act = mutable_variant_object()
         ("issuer",       "eosio")
         ("maximum_supply", eosio::chain::asset::from_string("1000000000.0000 SYS"));
   t.push_action("eosio.token"_n, "create"_n, "eosio.token"_n, act );

   // issue
   for (account_name a: accs) {
      issue_tokens( t, config::system_account_name, a, eosio::chain::asset::from_string("10000.0000 SYS") );
   }
   t.produce_blocks(1);

   t.set_code( config::system_account_name, contracts::eosio_system_wasm() );
   t.set_abi( config::system_account_name, contracts::eosio_system_abi().data() );

   t.push_action(config::system_account_name, "init"_n,
                 config::system_account_name,  mutable_variant_object()
                 ("version", 0)
                 ("core", "4,SYS"));

   // bidname
   auto bidname = [&t]( const account_name& bidder, const account_name& newname, const asset& bid ) {
      return t.push_action( "eosio"_n, "bidname"_n, bidder, fc::mutable_variant_object()
                           ("bidder",  bidder)
                           ("newname", newname)
                           ("bid", bid)
                            );
   };

   bidname("inita"_n, "com"_n, eosio::chain::asset::from_string("10.0000 SYS"));
   bidname("initb"_n, "org"_n, eosio::chain::asset::from_string("11.0000 SYS"));
   bidname("initc"_n, "io"_n, eosio::chain::asset::from_string("12.0000 SYS"));
   bidname("initd"_n, "html"_n, eosio::chain::asset::from_string("14.0000 SYS"));
   t.produce_blocks(1);

   // get table: normal case
   eosio::chain_apis::read_only plugin(*(t.control), {}, fc::microseconds::maximum());
   eosio::chain_apis::read_only::get_table_rows_params p;
   p.code = "eosio"_n;
   p.scope = "eosio";
   p.table = "namebids"_n;
   p.json = true;
   p.index_position = "secondary"; // ordered by high_bid
   p.key_type = "i64";
   eosio::chain_apis::read_only::get_table_rows_result result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   BOOST_REQUIRE_EQUAL(false, result.more);
   if (result.rows.size() >= 4) {
      BOOST_REQUIRE_EQUAL("html", result.rows[0]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("initd", result.rows[0]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("140000", result.rows[0]["high_bid"].as_string());

      BOOST_REQUIRE_EQUAL("io", result.rows[1]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("initc", result.rows[1]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("120000", result.rows[1]["high_bid"].as_string());

      BOOST_REQUIRE_EQUAL("org", result.rows[2]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("initb", result.rows[2]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("110000", result.rows[2]["high_bid"].as_string());

      BOOST_REQUIRE_EQUAL("com", result.rows[3]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("inita", result.rows[3]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("100000", result.rows[3]["high_bid"].as_string());
   }

   // reverse search, with show ram payer
   p.reverse = true;
   p.show_payer = true;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(4u, result.rows.size());
   BOOST_REQUIRE_EQUAL(false, result.more);
   if (result.rows.size() >= 4) {
      BOOST_REQUIRE_EQUAL("html", result.rows[3]["data"]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("initd", result.rows[3]["data"]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("140000", result.rows[3]["data"]["high_bid"].as_string());
      BOOST_REQUIRE_EQUAL("initd", result.rows[3]["payer"].as_string());

      BOOST_REQUIRE_EQUAL("io", result.rows[2]["data"]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("initc", result.rows[2]["data"]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("120000", result.rows[2]["data"]["high_bid"].as_string());
      BOOST_REQUIRE_EQUAL("initc", result.rows[2]["payer"].as_string());

      BOOST_REQUIRE_EQUAL("org", result.rows[1]["data"]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("initb", result.rows[1]["data"]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("110000", result.rows[1]["data"]["high_bid"].as_string());
      BOOST_REQUIRE_EQUAL("initb", result.rows[1]["payer"].as_string());

      BOOST_REQUIRE_EQUAL("com", result.rows[0]["data"]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("inita", result.rows[0]["data"]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("100000", result.rows[0]["data"]["high_bid"].as_string());
      BOOST_REQUIRE_EQUAL("inita", result.rows[0]["payer"].as_string());
   }

   // limit to 1 (get the highest bidname)
   p.reverse = false;
   p.show_payer = false;
   p.limit = 1;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL(true, result.more);
   if (result.rows.size() >= 1) {
      BOOST_REQUIRE_EQUAL("html", result.rows[0]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("initd", result.rows[0]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("140000", result.rows[0]["high_bid"].as_string());
   }

   // limit to 1 reverse, (get the lowest bidname)
   p.reverse = true;
   p.show_payer = false;
   p.limit = 1;
   result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, result.rows.size());
   BOOST_REQUIRE_EQUAL(true, result.more);
   if (result.rows.size() >= 1) {
      BOOST_REQUIRE_EQUAL("com", result.rows[0]["newname"].as_string());
      BOOST_REQUIRE_EQUAL("inita", result.rows[0]["high_bidder"].as_string());
      BOOST_REQUIRE_EQUAL("100000", result.rows[0]["high_bid"].as_string());
   }

   // all digits name search
   const std::string all_digits_name_1 = "1234";
   const std::string all_digits_name_2 = "2345";
   bidname("inita"_n, name(all_digits_name_1), eosio::chain::asset::from_string("1.0000 SYS"));
   bidname("inita"_n, name(all_digits_name_2), eosio::chain::asset::from_string("1.0000 SYS"));
   t.produce_blocks(1);
   p.lower_bound = all_digits_name_1;
   p.upper_bound = all_digits_name_1;
   p.index_position = "primary";
   p.key_type = "name";
   p.limit = 10;
   eosio::chain_apis::read_only::get_table_rows_result all_digits_result = plugin.read_only::get_table_rows(p);
   BOOST_REQUIRE_EQUAL(1u, all_digits_result.rows.size());
   BOOST_REQUIRE_EQUAL(false, all_digits_result.more);
   BOOST_REQUIRE_EQUAL(all_digits_name_1, all_digits_result.rows[0]["newname"].as_string());

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE_TEMPLATE( get_table_next_key_test, TESTER_T, backing_store_ts) { try {
   TESTER_T t;
   t.create_account("test"_n);

   // setup contract and abi
   t.set_code( "test"_n, contracts::get_table_test_wasm() );
   t.set_abi( "test"_n, contracts::get_table_test_abi().data() );
   t.produce_block();

   // Init some data
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 2));
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 5));
   t.push_action("test"_n, "addnumobj"_n, "test"_n, mutable_variant_object()("input", 7));
   t.push_action("test"_n, "addhashobj"_n, "test"_n, mutable_variant_object()("hashinput", "firstinput"));
   t.push_action("test"_n, "addhashobj"_n, "test"_n, mutable_variant_object()("hashinput", "secondinput"));
   t.push_action("test"_n, "addhashobj"_n, "test"_n, mutable_variant_object()("hashinput", "thirdinput"));
   t.produce_block();

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
   //   "next_key": ""
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
   //   "next_key": ""
   // }


   chain_apis::read_only plugin(*(t.control), {}, fc::microseconds::maximum());
   chain_apis::read_only::get_table_rows_params params = []{
      chain_apis::read_only::get_table_rows_params params{};
      params.json=true;
      params.code="test"_n;
      params.scope="test";
      params.limit=1;
      return params;
   }();

   params.table = "numobjs"_n;

   // i64 primary key type
   params.key_type = "i64";
   params.index_position = "1";
   params.lower_bound = "0";

   auto res_1 = plugin.get_table_rows(params);
   BOOST_REQUIRE(res_1.rows.size() > 0);
   BOOST_TEST(res_1.rows[0].get_object()["key"].as<uint64_t>() == 0);
   BOOST_TEST(res_1.next_key == "1");
   params.lower_bound = res_1.next_key;
   auto more2_res_1 = plugin.get_table_rows(params);
   BOOST_REQUIRE(more2_res_1.rows.size() > 0);
   BOOST_TEST(more2_res_1.rows[0].get_object()["key"].as<uint64_t>() == 1);


   // i64 secondary key type
   params.key_type = "i64";
   params.index_position = "2";
   params.lower_bound = "5";

   auto res_2 = plugin.get_table_rows(params);
   BOOST_REQUIRE(res_2.rows.size() > 0);
   BOOST_TEST(res_2.rows[0].get_object()["sec64"].as<uint64_t>() == 5);
   BOOST_TEST(res_2.next_key == "7");
   params.lower_bound = res_2.next_key;
   auto more2_res_2 = plugin.get_table_rows(params);
   BOOST_REQUIRE(more2_res_2.rows.size() > 0);
   BOOST_TEST(more2_res_2.rows[0].get_object()["sec64"].as<uint64_t>() == 7);

   // i128 secondary key type
   params.key_type = "i128";
   params.index_position = "3";
   params.lower_bound = "5";

   auto res_3 = plugin.get_table_rows(params);
   chain::uint128_t sec128_expected_value = 5;
   BOOST_REQUIRE(res_3.rows.size() > 0);
   BOOST_CHECK(res_3.rows[0].get_object()["sec128"].as<chain::uint128_t>() == sec128_expected_value);
   BOOST_TEST(res_3.next_key == "7");
   params.lower_bound = res_3.next_key;
   auto more2_res_3 = plugin.get_table_rows(params);
   chain::uint128_t more2_sec128_expected_value = 7;
   BOOST_REQUIRE(more2_res_3.rows.size() > 0);
   BOOST_CHECK(more2_res_3.rows[0].get_object()["sec128"].as<chain::uint128_t>() == more2_sec128_expected_value);

   // float64 secondary key type
   params.key_type = "float64";
   params.index_position = "4";
   params.lower_bound = "5.0";

   auto res_4 = plugin.get_table_rows(params);
   float64_t secdouble_expected_value = ui64_to_f64(5);
   BOOST_REQUIRE(res_4.rows.size() > 0);
   double secdouble_res_value = res_4.rows[0].get_object()["secdouble"].as<double>();
   BOOST_CHECK(*reinterpret_cast<float64_t*>(&secdouble_res_value) == secdouble_expected_value);
   BOOST_TEST(res_4.next_key == "7.00000000000000000");
   params.lower_bound = res_4.next_key;
   auto more2_res_4 = plugin.get_table_rows(params);
   float64_t more2_secdouble_expected_value = ui64_to_f64(7);
   BOOST_REQUIRE(more2_res_4.rows.size() > 0);
   double more2_secdouble_res_value = more2_res_4.rows[0].get_object()["secdouble"].as<double>();
   BOOST_CHECK(*reinterpret_cast<float64_t*>(&more2_secdouble_res_value) == more2_secdouble_expected_value);

   // float128 secondary key type
   params.key_type = "float128";
   params.index_position = "5";
   params.lower_bound = "5.0";

   auto res_5 = plugin.get_table_rows(params);
   float128_t secldouble_expected_value = ui64_to_f128(5);
   BOOST_REQUIRE(res_5.rows.size() > 0);
   float128_t secldouble_res_value =  res_5.rows[0].get_object()["secldouble"].as<float128_t>();
   BOOST_TEST(secldouble_res_value == secldouble_expected_value);
   BOOST_TEST(res_5.next_key == "7.00000000000000000");
   params.lower_bound = res_5.next_key;
   auto more2_res_5 = plugin.get_table_rows(params);
   float128_t more2_secldouble_expected_value = ui64_to_f128(7);
   BOOST_REQUIRE(more2_res_5.rows.size() > 0);
   float128_t more2_secldouble_res_value =  more2_res_5.rows[0].get_object()["secldouble"].as<float128_t>();
   BOOST_TEST(more2_secldouble_res_value == more2_secldouble_expected_value);

   params.table = "hashobjs"_n;

   // sha256 secondary key type
   params.key_type = "sha256";
   params.index_position = "2";
   params.lower_bound = "2652d68fbbf6000c703b35fdc607b09cd8218cbeea1d108b5c9e84842cdd5ea5"; // This is hash of "thirdinput"

   auto res_6 = plugin.get_table_rows(params);
   checksum256_type sec256_expected_value = checksum256_type::hash(std::string("thirdinput"));
   BOOST_REQUIRE(res_6.rows.size() > 0);
   checksum256_type sec256_res_value = res_6.rows[0].get_object()["sec256"].as<checksum256_type>();
   BOOST_TEST(sec256_res_value == sec256_expected_value);
   BOOST_TEST(res_6.rows[0].get_object()["hash_input"].as<string>() == std::string("thirdinput"));
   BOOST_TEST(res_6.next_key == "3cb93a80b47b9d70c5296be3817d34b48568893b31468e3a76337bb7d3d0c264");
   params.lower_bound = res_6.next_key;
   auto more2_res_6 = plugin.get_table_rows(params);
   checksum256_type more2_sec256_expected_value = checksum256_type::hash(std::string("secondinput"));
   BOOST_REQUIRE(more2_res_6.rows.size() > 0);
   checksum256_type more2_sec256_res_value = more2_res_6.rows[0].get_object()["sec256"].as<checksum256_type>();
   BOOST_TEST(more2_sec256_res_value == more2_sec256_expected_value);
   BOOST_TEST(more2_res_6.rows[0].get_object()["hash_input"].as<string>() == std::string("secondinput"));

   // i256 secondary key type
   params.key_type = "i256";
   params.index_position = "2";
   params.lower_bound = "0x2652d68fbbf6000c703b35fdc607b09cd8218cbeea1d108b5c9e84842cdd5ea5"; // This is sha256 hash of "thirdinput" as number

   auto res_7 = plugin.get_table_rows(params);
   checksum256_type i256_expected_value = checksum256_type::hash(std::string("thirdinput"));
   BOOST_REQUIRE(res_7.rows.size() > 0);
   checksum256_type i256_res_value = res_7.rows[0].get_object()["sec256"].as<checksum256_type>();
   BOOST_TEST(i256_res_value == i256_expected_value);
   BOOST_TEST(res_7.rows[0].get_object()["hash_input"].as<string>() == "thirdinput");
   BOOST_TEST(res_7.next_key == "0x3cb93a80b47b9d70c5296be3817d34b48568893b31468e3a76337bb7d3d0c264");
   params.lower_bound = res_7.next_key;
   auto more2_res_7 = plugin.get_table_rows(params);
   checksum256_type more2_i256_expected_value = checksum256_type::hash(std::string("secondinput"));
   BOOST_REQUIRE(more2_res_7.rows.size() > 0);
   checksum256_type more2_i256_res_value = more2_res_7.rows[0].get_object()["sec256"].as<checksum256_type>();
   BOOST_TEST(more2_i256_res_value == more2_i256_expected_value);
   BOOST_TEST(more2_res_7.rows[0].get_object()["hash_input"].as<string>() == "secondinput");

   // ripemd160 secondary key type
   params.key_type = "ripemd160";
   params.index_position = "3";
   params.lower_bound = "ab4314638b573fdc39e5a7b107938ad1b5a16414"; // This is ripemd160 hash of "thirdinput"

   auto res_8 = plugin.get_table_rows(params);
   ripemd160 sec160_expected_value = ripemd160::hash(std::string("thirdinput"));
   BOOST_REQUIRE(res_8.rows.size() > 0);
   ripemd160 sec160_res_value = res_8.rows[0].get_object()["sec160"].as<ripemd160>();
   BOOST_TEST(sec160_res_value == sec160_expected_value);
   BOOST_TEST(res_8.rows[0].get_object()["hash_input"].as<string>() == "thirdinput");
   BOOST_TEST(res_8.next_key == "fb9d03d3012dc2a6c7b319f914542e3423550c2a");
   params.lower_bound = res_8.next_key;
   auto more2_res_8 = plugin.get_table_rows(params);
   ripemd160 more2_sec160_expected_value = ripemd160::hash(std::string("secondinput"));
   BOOST_REQUIRE(more2_res_8.rows.size() > 0);
   ripemd160 more2_sec160_res_value = more2_res_8.rows[0].get_object()["sec160"].as<ripemd160>();
   BOOST_TEST(more2_sec160_res_value == more2_sec160_expected_value);
   BOOST_TEST(more2_res_8.rows[0].get_object()["hash_input"].as<string>() == "secondinput");

} FC_LOG_AND_RETHROW() } /// get_table_next_key_test

BOOST_AUTO_TEST_SUITE_END()
