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

#if 0
class identity_tester : public tester {
public:

   identity_tester() {
      produce_blocks(2);

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
      abi_ser.set_abi(abi);

      const auto& acnt_test = control->get_database().get<account_object,by_name>( N(identitytest) );
      abi_def abi_test;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(acnt_test.abi, abi_test), true);
      abi_ser_test.set_abi(abi_test);
   }

   uint64_t get_result_uint64() {
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

   uint64_t get_owner_for_identity(uint64_t identity)
   {
      signed_transaction trx;
      action get_owner_act;
      get_owner_act.account = N(identitytest);
      get_owner_act.name = N(getowner);
      get_owner_act.data = abi_ser_test.variant_to_binary("getowner", mutable_variant_object()
                                                          ("identity", identity)
      );
      trx.actions.emplace_back(std::move(get_owner_act));
      set_tapos(trx);
      control->push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));

      return get_result_uint64();
   }

   string create_identity(const string& account_name, uint64_t identity, bool auth = true) {
      signed_transaction trx;
      action create_act;
      create_act.account = N(identity);
      create_act.name = N(create);
      if (auth) {
         create_act.authorization = vector<permission_level>{{string_to_name(account_name.c_str()), config::active_name}};
      }
      create_act.data = abi_ser.variant_to_binary("create", mutable_variant_object()
                                                  ("creator", account_name)
                                                  ("identity", identity)
      );
      trx.actions.emplace_back(std::move(create_act));
      set_tapos(trx);
      if (auth) {
         trx.sign(get_private_key(string_to_name(account_name.c_str()), "active"), chain_id_type());
      }
      try {
         control->push_transaction(trx);
      } catch (const fc::exception& ex) {
         return error(ex.top_message());
      }
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      return success();
   }

   fc::variant get_identity(uint64_t idnt) {
      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(N(identity), N(identity), N(ident)));
      FC_ASSERT(t_id != 0, "object not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, idnt));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");
      BOOST_REQUIRE_EQUAL(idnt, itr->primary_key);

      vector<char> data;
      read_only::copy_row(*itr, data);
      return abi_ser.binary_to_variant("identrow", data);
   }

   string certify(const string& certifier, uint64_t identity, const vector<fc::variant>& fields, bool auth = true) {
      signed_transaction trx;
      action cert_act;
      cert_act.account = N(identity);
      cert_act.name = N(certprop);
      if (auth) {
         cert_act.authorization = vector<permission_level>{{string_to_name(certifier.c_str()), config::active_name}};
      }
      cert_act.data = abi_ser.variant_to_binary("certprop", mutable_variant_object()
                                                ("bill_storage_to", certifier)
                                                ("certifier", certifier)
                                                ("identity", identity)
                                                ("value", fields)
      );
      trx.actions.emplace_back(std::move(cert_act));
      set_tapos(trx);
      if (auth) {
         trx.sign(get_private_key(string_to_name(certifier.c_str()), "active"), chain_id_type());
      }
      try {
         control->push_transaction(trx);
      } catch (const fc::exception& ex) {
         return error(ex.top_message());
      }
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      return success();
   }

   fc::variant get_certrow(uint64_t identity, const string& property, uint64_t trusted, const string& certifier) {
      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(identity, N(identity), N(certs)));
      FC_ASSERT(t_id != 0, "certrow not found");

      uint64_t prop = string_to_name(property.c_str());
      uint64_t cert = string_to_name(certifier.c_str());
      const auto& idx = db.get_index<key64x64x64_value_index, by_scope_primary>();
      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, prop, trusted, cert));

      if (itr != idx.end() && itr->t_id == t_id->id && prop == itr->primary_key && trusted == itr->secondary_key && cert == itr->tertiary_key) {
         vector<char> data;
         read_only::copy_row(*itr, data);
         return abi_ser.binary_to_variant("certrow", data);
      } else {
         return fc::variant(nullptr);
      }
   }

   fc::variant get_accountrow(uint64_t identity, const string& account) {
      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(identity, N(identity), N(account)));
      if (!t_id) {
         return fc::variant(nullptr);
      }
      const auto& idx = db.get_index<key_value_index, by_scope_primary>();
      uint64_t acnt = string_to_name(account.c_str());
      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, acnt));
      if( itr != idx.end() && itr->t_id == t_id->id && acnt == itr->primary_key) {
         vector<char> data;
         read_only::copy_row(*itr, data);
         return abi_ser.binary_to_variant("accountrow", data);
      } else {
         return fc::variant(nullptr);
      }
   }


   string settrust(const string& trustor, const string& trusting, uint64_t trust, bool auth = true)
   {
      signed_transaction trx;
      action settrust_act;
      settrust_act.account = N(identity);
      settrust_act.name = N(settrust);
      auto tr = string_to_name(trustor.c_str());
      if (auth) {
         settrust_act.authorization = vector<permission_level>{{tr, config::active_name}};
      }
      settrust_act.data = abi_ser.variant_to_binary("settrust", mutable_variant_object()
                                                    ("trustor", trustor)
                                                    ("trusting", trusting)
                                                    ("trust", trust)
      );
      trx.actions.emplace_back(std::move(settrust_act));
      set_tapos(trx);
      if (auth) {
         trx.sign(get_private_key(tr, "active"), chain_id_type());
      }
      try {
         control->push_transaction(trx);
      } catch (const fc::exception& ex) {
         return error(ex.top_message());
      }
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      return success();
   }

   fc::variant get_trustrow(const string& trustor, const string& trusting) {
      const auto& db = control->get_database();
      const auto* t_id = db.find<table_id_object, by_scope_code_table>(boost::make_tuple(string_to_name(trustor.c_str()), N(identity), N(trust)));
      if (!t_id) {
         return fc::variant(nullptr);
      }

      uint64_t tng = string_to_name(trusting.c_str());
      const auto& idx = db.get_index<key_value_index, by_scope_primary>();
      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, tng));
      if ( itr != idx.end() && itr->t_id == t_id->id && tng == itr->primary_key ) {
         vector<char> data;
         read_only::copy_row(*itr, data);
         return abi_ser.binary_to_variant("trustrow", data);
      } else {
         return fc::variant(nullptr);
      }
   }

public:
   abi_serializer abi_ser;
   abi_serializer abi_ser_test;
};

constexpr uint64_t identity_val = 0xffffffffffffffff; //64-bit value

BOOST_AUTO_TEST_SUITE(identity_tests)

BOOST_FIXTURE_TEST_CASE( identity_create, identity_tester ) try {

   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", identity_val));
   fc::variant idnt = get_identity(identity_val);
   BOOST_REQUIRE_EQUAL( identity_val, idnt["identity"].as_uint64());
   BOOST_REQUIRE_EQUAL( "alice", idnt["creator"].as_string());

   //attempts to create already existing identity should fail
   BOOST_REQUIRE_EQUAL(error("condition: assertion failed: identity already exists"), create_identity("alice", identity_val));
   BOOST_REQUIRE_EQUAL(error("condition: assertion failed: identity already exists"), create_identity("bob", identity_val));

   //alice can create more identities
   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", 2));
   fc::variant idnt2 = get_identity(2);
   BOOST_REQUIRE_EQUAL( 2, idnt2["identity"].as_uint64());
   BOOST_REQUIRE_EQUAL( "alice", idnt2["creator"].as_string());

   //identity == 0 has special meaning, should be impossible to create
   BOOST_REQUIRE_EQUAL(error("condition: assertion failed: identity=0 is not allowed"), create_identity("alice", 0));

   //creating adentity without authentication is not allowed
   BOOST_REQUIRE_EQUAL(error("missing authority of alice"), create_identity("alice", 3, false));

   //bob can create an identity as well
   BOOST_REQUIRE_EQUAL(success(), create_identity("bob", 1));
   fc::variant idnt_bob = get_identity(1);
   BOOST_REQUIRE_EQUAL( 1, idnt_bob["identity"].as_uint64());
   BOOST_REQUIRE_EQUAL( "bob", idnt_bob["creator"].as_string());

   //previously created identity should still exist
   idnt = get_identity(identity_val);
   BOOST_REQUIRE_EQUAL( identity_val, idnt["identity"].as_uint64());

 } FC_LOG_AND_RETHROW() //identity_create

BOOST_FIXTURE_TEST_CASE( certify_decertify, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", identity_val));

   //alice (creator of the identity) certifies 1 property
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>(1, mutable_variant_object()
                                                                                     ("property", "name")
                                                                                     ("type", "string")
                                                                                     ("data", to_uint8_vector("Alice Smith"))
                                                                                     ("memo", "")
                                                                                     ("confidence", 100)
                                          )));

   auto obj = get_certrow(identity_val, "name", 0, "alice");
   BOOST_REQUIRE_EQUAL(true, obj.is_object());
   BOOST_REQUIRE_EQUAL( "name", obj["property"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, obj["trusted"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "alice", obj["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( 100, obj["confidence"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "string", obj["type"].as_string() );
   BOOST_REQUIRE_EQUAL( "Alice Smith", to_string(obj["data"]) );

   //check that there is no trusted row with the same data
   BOOST_REQUIRE_EQUAL(true, get_certrow(identity_val, "name", 1, "alice").is_null());

   //bob certifies 2 properties
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

   //shouldn't be able to certify without authorization
   BOOST_REQUIRE_EQUAL(error("missing authority of bob"), certify("bob", identity_val, fields, false));

   BOOST_REQUIRE_EQUAL(success(), certify("bob", identity_val, fields));

   obj = get_certrow(identity_val, "email", 0, "bob");
   BOOST_REQUIRE_EQUAL(true, obj.is_object());
   BOOST_REQUIRE_EQUAL( "email", obj["property"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, obj["trusted"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "bob", obj["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( 95, obj["confidence"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "string", obj["type"].as_string() );
   BOOST_REQUIRE_EQUAL( "alice@alice.name", to_string(obj["data"]) );

   obj = get_certrow(identity_val, "address", 0, "bob");
   BOOST_REQUIRE_EQUAL(true, obj.is_object());
   BOOST_REQUIRE_EQUAL( "address", obj["property"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, obj["trusted"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "bob", obj["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( 80, obj["confidence"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "string", obj["type"].as_string() );
   BOOST_REQUIRE_EQUAL( "1750 Kraft Drive SW, Blacksburg, VA 24060", to_string(obj["data"]) );

   //now alice certifies another email
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>(1, mutable_variant_object()
                                                                                     ("property", "email")
                                                                                     ("type", "string")
                                                                                     ("data", to_uint8_vector("alice.smith@gmail.com"))
                                                                                     ("memo", "")
                                                                                     ("confidence", 100)
                                          )));
   obj = get_certrow(identity_val, "email", 0, "alice");
   BOOST_REQUIRE_EQUAL(true, obj.is_object());
   BOOST_REQUIRE_EQUAL( "email", obj["property"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, obj["trusted"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "alice", obj["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( 100, obj["confidence"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "string", obj["type"].as_string() );
   BOOST_REQUIRE_EQUAL( "alice.smith@gmail.com", to_string(obj["data"]) );

   //email certification made by bob should be still in place
   obj = get_certrow(identity_val, "email", 0, "bob");
   BOOST_REQUIRE_EQUAL( "bob", obj["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( "alice@alice.name", to_string(obj["data"]) );

   //remove email certification made by alice
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>(1, mutable_variant_object()
                                                                                     ("property", "email")
                                                                                     ("type", "string")
                                                                                     ("data", to_uint8_vector(""))
                                                                                     ("memo", "")
                                                                                     ("confidence", 0)
                                          )));
   BOOST_REQUIRE_EQUAL(true, get_certrow(identity_val, "email", 0, "alice").is_null());

   //email certification made by bob should still be in place
   obj = get_certrow(identity_val, "email", 0, "bob");
   BOOST_REQUIRE_EQUAL( "bob", obj["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( "alice@alice.name", to_string(obj["data"]) );

   //name certification made by alice should still be in place
   obj = get_certrow(identity_val, "name", 0, "alice");
   BOOST_REQUIRE_EQUAL(true, obj.is_object());
   BOOST_REQUIRE_EQUAL( "Alice Smith", to_string(obj["data"]) );

} FC_LOG_AND_RETHROW() //certify_decertify

BOOST_FIXTURE_TEST_CASE( trust_untrust, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), settrust("bob", "alice", 1));
   auto obj = get_trustrow("bob", "alice");
   BOOST_REQUIRE_EQUAL(true, obj.is_object());
   BOOST_REQUIRE_EQUAL( "alice", obj["account"].as_string() );
   BOOST_REQUIRE_EQUAL( 1, obj["trusted"].as_uint64() );

   obj = get_trustrow("alice", "bob");
   BOOST_REQUIRE_EQUAL(true, obj.is_null());

   //remove trust
   BOOST_REQUIRE_EQUAL(success(), settrust("bob", "alice", 0));
   obj = get_trustrow("bob", "alice");
   BOOST_REQUIRE_EQUAL(true, obj.is_null());

} FC_LOG_AND_RETHROW() //trust_untrust

BOOST_FIXTURE_TEST_CASE( certify_decertify_owner, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", identity_val));

   // certify owner, should populate "account" singleton
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>(1, mutable_variant_object()
                                                                              ("property", "owner")
                                                                              ("type", "account")
                                                                              ("data", to_uint8_vector(N(alice)))
                                                                              ("memo", "claiming onwership")
                                                                              ("confidence", 100)
                                                )));
   fc::variant certrow = get_certrow(identity_val, "owner", 0, "alice");
   BOOST_REQUIRE_EQUAL( true, certrow.is_object() );
   BOOST_REQUIRE_EQUAL( "owner", certrow["property"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, certrow["trusted"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "alice", certrow["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( 100, certrow["confidence"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "account", certrow["type"].as_string() );
   BOOST_REQUIRE_EQUAL( N(alice), to_uint64(certrow["data"]) );

   //check that singleton "account" in the scope of identity contains the owner
   fc::variant acntrow = get_accountrow(identity_val, "alice");
   BOOST_REQUIRE_EQUAL( true, certrow.is_object() );
   BOOST_REQUIRE_EQUAL( "alice", acntrow["account"].as_string() );

   // ownership was certified by alice, but not by a block producer or someone trusted by a block producer
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));

   //remove owner certification
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>(1, mutable_variant_object()
                                                                              ("property", "owner")
                                                                              ("type", "account")
                                                                              ("data", to_uint8_vector(N(alice)))
                                                                              ("memo", "claiming onwership")
                                                                              ("confidence", 0)
                                          )));
   certrow = get_certrow(identity_val, "owner", 0, "alice");
   BOOST_REQUIRE_EQUAL(true, certrow.is_null());

   //check that singleton "account" in the scope of identity contains the owner
   acntrow = get_accountrow(identity_val, "alice");
   BOOST_REQUIRE_EQUAL(true, certrow.is_null());

} FC_LOG_AND_RETHROW() //certify_owner

BOOST_AUTO_TEST_SUITE_END()
#endif
