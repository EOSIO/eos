#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <identity/identity.wast.hpp>
#include <identity/identity.abi.hpp>
#include <identity/test/identity_test.wast.hpp>
#include <identity/test/identity_test.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include "test_wasts.hpp"

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::chain_apis;
using namespace eosio::testing;
using namespace fc;

class identity_tester : public tester {
public:
   uint64_t get_result_uint64(abi_serializer& abi_ser) {
      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(0, N(identitytest), N(result)));
      FC_ASSERT(t_id != 0, "Table id not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      FC_ASSERT( N(result) == itr->primary_key, "row with result not found");
      FC_ASSERT( sizeof(uint64_t) == itr->value.size(), "unexpected result size");
      return *reinterpret_cast<const uint64_t*>(itr->value.data());
   }

   uint64_t get_owner_for_identity(abi_serializer& abi_ser, uint64_t identity)
   {
      signed_transaction trx;
      action get_owner_act;
      get_owner_act.account = N(identitytest);
      get_owner_act.name = N(getowner);
      get_owner_act.data = abi_ser.variant_to_binary("getowner", mutable_variant_object()
                                                     ("identity", identity)
      );
      trx.actions.emplace_back(std::move(get_owner_act));
      set_tapos(trx);
      control->push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));

      return get_result_uint64(abi_ser);
   }
};

BOOST_AUTO_TEST_SUITE(identity_tests)

BOOST_FIXTURE_TEST_CASE( identity_test, identity_tester ) try {
   produce_blocks(2);

   constexpr uint64_t identity_val = 1;
   create_accounts( {N(identity), N(identitytest), N(alice), N(bob)}, asset::from_string("100000.0000 EOS") );
   produce_blocks(1000);

   set_code(N(identity), identity_wast);
   set_abi(N(identity), identity_abi);
   set_code(N(identitytest), identity_test_wast);
   set_abi(N(identitytest), identity_test_abi);
   produce_blocks(1);

   const auto& accnt = control->get_database().get<account_object,by_name>( N(identity) );
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);

   const auto& acnt_test = control->get_database().get<account_object,by_name>( N(identitytest) );
   abi_def abi_test;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(acnt_test.abi, abi_test), true);
   abi_serializer abi_ser_test(abi_test);

   //identity doesn't exist
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(abi_ser_test, identity_val));

   //create identity
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

   //identity exists, but nobody claimed it so far
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(abi_ser_test, identity_val));

   //certify 2 fields
   {
      signed_transaction trx;
      action cert_act;
      cert_act.account = N(identity);
      cert_act.name = N(certprop);
      cert_act.authorization = vector<permission_level>{{N(alice), config::active_name}};


      vector<fc::variant> fields;
      fields.push_back(mutable_variant_object()
         ("property", "email")
         ("type", "string")
         ("data", to_uint8_vector("alice@alice.name"))
         ("memo", "official email")
         ("confidence", 95)
      );
      fields.push_back(mutable_variant_object()
         ("property", "address")
         ("type", "string")
         ("data", to_uint8_vector("1750 Kraft Drive SW, Blacksburg, VA 24060"))
         ("memo", "official address")
         ("confidence", 80)
      );

      cert_act.data = abi_ser.variant_to_binary("certprop", mutable_variant_object()
                                                ("bill_storage_to", "alice")
                                                ("certifier","alice")
                                                ("identity", identity_val)
                                                ("value", std::move(fields))
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

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, N(email), 0 /*trusted*/, N(alice)));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      vector<char> data;
      read_only::copy_row(*itr, data);
      fc::variant obj = abi_ser.binary_to_variant("certrow", data);
      BOOST_REQUIRE_EQUAL( "email", obj["property"].as_string() );
      BOOST_REQUIRE_EQUAL( 0, obj["trusted"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "alice", obj["certifier"].as_string() );
      BOOST_REQUIRE_EQUAL( 95, obj["confidence"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "string", obj["type"].as_string() );
      BOOST_REQUIRE_EQUAL( "alice@alice.name", to_string(obj["data"]) );

      itr = idx.lower_bound(boost::make_tuple(t_id->id, N(address), 0 /*trusted*/, N(alice)));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");
      data.clear();
      read_only::copy_row(*itr, data);
      obj = abi_ser.binary_to_variant("certrow", data);
      BOOST_REQUIRE_EQUAL( "address", obj["property"].as_string() );
      BOOST_REQUIRE_EQUAL( 0, obj["trusted"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "alice", obj["certifier"].as_string() );
      BOOST_REQUIRE_EQUAL( 80, obj["confidence"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "string", obj["type"].as_string() );
      BOOST_REQUIRE_EQUAL( "1750 Kraft Drive SW, Blacksburg, VA 24060", to_string(obj["data"]) );
   }

   // certify owner, should populate "account" singleton
   {
      signed_transaction trx;
      action cert_act;
      cert_act.account = N(identity);
      cert_act.name = N(certprop);
      cert_act.authorization = vector<permission_level>{{N(alice), config::active_name}};
      cert_act.data = abi_ser.variant_to_binary("certprop", mutable_variant_object()
                                                ("bill_storage_to", "alice")
                                                ("certifier","alice")
                                                ("identity", identity_val)
                                                ("value", vector<fc::variant>(1, mutable_variant_object()
                                                                              ("property", "owner")
                                                                              ("type", "account")
                                                                              ("data", to_uint8_vector(N(alice)))
                                                                              ("memo", "claiming onwership")
                                                                              ("confidence", 100)
                                                ))
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

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, N(owner), 0 /*trusted*/, N(alice)));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      vector<char> data;
      read_only::copy_row(*itr, data);
      fc::variant certrow = abi_ser.binary_to_variant("certrow", data);
      BOOST_REQUIRE_EQUAL( "owner", certrow["property"].as_string() );
      BOOST_REQUIRE_EQUAL( 0, certrow["trusted"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "alice", certrow["certifier"].as_string() );
      BOOST_REQUIRE_EQUAL( 100, certrow["confidence"].as_uint64() );
      BOOST_REQUIRE_EQUAL( "account", certrow["type"].as_string() );
      BOOST_REQUIRE_EQUAL( N(alice), to_uint64(certrow["data"]) );

      t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(N(alice), N(identity), N(account)));
      FC_ASSERT(t_id != 0, "object not found");
      const auto& acnt_idx = db.get_index<key_value_index, by_scope_primary>();
      auto acnt_itr = acnt_idx.lower_bound(boost::make_tuple(t_id->id));
      FC_ASSERT( acnt_itr != acnt_idx.end() && acnt_itr->t_id == t_id->id, "lower_bound failed");
      data.clear();
      read_only::copy_row(*acnt_itr, data);

      fc::variant acntrow = abi_ser.binary_to_variant("accountrow", data);
      BOOST_REQUIRE_EQUAL( N(account), acntrow["singleton_name"].as_uint64() );
      BOOST_REQUIRE_EQUAL( identity_val, acntrow["identity"].as_uint64() );
   }

   // ownership was certified by alice, but not by a block producer or someone trusted by a block producer
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(abi_ser_test, identity_val));

   // settrust, check that certified property became trusted
   {
      signed_transaction trx;
      action settrust_act;
      settrust_act.account = N(identity);
      settrust_act.name = N(settrust);
      settrust_act.authorization = vector<permission_level>{{N(bob), config::active_name}};
      settrust_act.data = abi_ser.variant_to_binary("settrust", mutable_variant_object()
                                                    ("trustor", "bob")
                                                    ("trusting","alice")
                                                    ("trust", 1)
      );
      trx.actions.emplace_back(std::move(settrust_act));
      set_tapos(trx);
      trx.sign(get_private_key(N(bob), "active"), chain_id_type());
      control->push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));

      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(N(bob), N(identity), N(trust)));
      FC_ASSERT(t_id != 0, "object not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();
      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, N(alice)));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");

      vector<char> data;
      read_only::copy_row(*itr, data);
      fc::variant row = abi_ser.binary_to_variant("trustrow", data);
      BOOST_REQUIRE_EQUAL( "alice", row["account"].as_string() );
      BOOST_REQUIRE_EQUAL( 1, row["trusted"].as_uint64() );
   }

   // settrust to 0, check that certified property became untrusted
   {
   }

} FC_LOG_AND_RETHROW() /// identity_test

BOOST_AUTO_TEST_SUITE_END()
