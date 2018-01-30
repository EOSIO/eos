#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <identity/identity.wast.hpp>
#include <identity/identity.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include "test_wasts.hpp"

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::chain_apis;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(identity_tests)

/**
 * Prove that action reading and assertions are working
 */
BOOST_FIXTURE_TEST_CASE( identity_test, tester ) try {
   produce_blocks(2);

   constexpr uint64_t identity_val = 1;
   create_accounts( {N(identity), N(alice), N(bob)}, asset::from_string("100000.0000 EOS") );
   produce_blocks(1000);

   set_code(N(identity), identity_wast);
   set_abi(N(identity), identity_abi);
   produce_blocks(1);

   const auto& accnt = control->get_database().get<account_object,by_name>( N(identity) );
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);

   {
      signed_transaction trx;
      action create_act;
      create_act.account = N(identity);
      create_act.name = N(create);
      create_act.authorization = vector<permission_level>{{N(alice), config::active_name}};
      create_act.data = abi_ser.variant_to_binary("create", mutable_variant_object()
                                                  ("creator", "alice")
                                                  ("identity", identity_val)
      );
      trx.actions.emplace_back(std::move(create_act));
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      control->push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));

      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(N(identity), N(identity), N(ident)));
      FC_ASSERT(t_id != 0, "object not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      vector<char> data;
      read_only::copy_row(*itr, data);
      fc::variant obj = abi_ser.binary_to_variant("identrow", data);
      BOOST_REQUIRE_EQUAL( identity_val, obj["identity"].as_uint64());
      BOOST_REQUIRE_EQUAL( "alice", obj["creator"].as_string());
   }

   auto to_vector = [](const string& s) {
      vector<uint8_t> v(s.size());
      copy(s.begin(), s.end(), v.begin());
      return v;
   };

   auto to_string = [](const vector<uint8_t>& v) {
      string s(v.size(), 0);
      copy(v.begin(), v.end(), s.begin());
      return s;
   };

   {
      signed_transaction trx;
      action cert_act;
      cert_act.account = N(identity);
      cert_act.name = N(certprop);
      cert_act.authorization = vector<permission_level>{{N(alice), config::active_name}};

      auto alice_email = mutable_variant_object()
         ("property", "email")
         ("type", "string")
         ("data", to_vector("alice@alice.name"))
         ("memo", "official email")
         ("confidence", 95);
      vector<fc::variant> v;
      v.push_back(alice_email);

      cert_act.data = abi_ser.variant_to_binary("certprop", mutable_variant_object()
                                                ("bill_storage_to", "alice")
                                                ("certifier","alice")
                                                ("identity", identity_val)
                                                ("value", v)
      );
      trx.actions.emplace_back(std::move(cert_act));
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      control->push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));

      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(identity_val, N(identity), N(certs)));
      FC_ASSERT(t_id != 0, "object not found");

      const auto& idx = db.get_index<key64x64x64_value_index, by_scope_primary>();

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      vector<char> data;
      read_only::copy_row(*itr, data);
      fc::variant obj = abi_ser.binary_to_variant("certrow", data);
      BOOST_REQUIRE_EQUAL( "email", obj["property"].as_string() );
      BOOST_REQUIRE_EQUAL( 0, obj["trusted"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "alice", obj["certifier"].as_string() );
      BOOST_REQUIRE_EQUAL( 95, obj["confidence"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "string", obj["type"].as_string() );
      vector<uint8_t> blob;
      from_variant<uint8_t>(obj["data"], blob);
      BOOST_REQUIRE_EQUAL( "alice@alice.name", to_string(blob) );
   }
} FC_LOG_AND_RETHROW() /// identity_test

BOOST_AUTO_TEST_SUITE_END()
