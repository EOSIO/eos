#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/fixed_key.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <chainbase/chainbase.hpp>

#include <identity/identity.wast.hpp>
#include <identity/identity.abi.hpp>
#include <identity/test/identity_test.wast.hpp>
#include <identity/test/identity_test.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <algorithm>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

class identity_tester : public TESTER {
public:

   identity_tester() {
      produce_blocks(2);

      create_accounts( {N(identity), N(identitytest), N(alice), N(bob), N(carol)} );
      produce_blocks(1000);

      set_code(N(identity), identity_wast);
      set_abi(N(identity), identity_abi);
      set_code(N(identitytest), identity_test_wast);
      set_abi(N(identitytest), identity_test_abi);
      produce_blocks(1);

      const auto& accnt = control->db().get<account_object,by_name>( N(identity) );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi);

      const auto& acnt_test = control->db().get<account_object,by_name>( N(identitytest) );
      abi_def abi_test;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(acnt_test.abi, abi_test), true);
      abi_ser_test.set_abi(abi_test);

      const auto& ap = control->active_producers();
      FC_ASSERT(0 < ap.producers.size(), "No producers");
      producer_name = (string)ap.producers.front().producer_name;
   }

   uint64_t get_result_uint64() {
      const auto& db = control->db();
      const auto* t_id = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(identitytest), 0, N(result)));
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
      action get_owner_act;
      get_owner_act.account = N(identitytest);
      get_owner_act.name = N(getowner);
      get_owner_act.data = abi_ser_test.variant_to_binary("getowner", mutable_variant_object()
                                                          ("identity", identity)
      );
      BOOST_REQUIRE_EQUAL(success(), push_action(std::move(get_owner_act), N(alice)));
      return get_result_uint64();
   }

   uint64_t get_identity_for_account(const string& account)
   {
      action get_identity_act;
      get_identity_act.account = N(identitytest);
      get_identity_act.name = N(getidentity);
      get_identity_act.data = abi_ser_test.variant_to_binary("getidentity", mutable_variant_object()
                                                          ("account", account)
      );
      BOOST_REQUIRE_EQUAL(success(), push_action(std::move(get_identity_act), N(alice)));
      return get_result_uint64();
   }

   action_result create_identity(const string& account_name, uint64_t identity, bool auth = true) {
      action create_act;
      create_act.account = N(identity);
      create_act.name = N(create);
      create_act.data = abi_ser.variant_to_binary("create", mutable_variant_object()
                                                  ("creator", account_name)
                                                  ("identity", identity)
      );
      return push_action( std::move(create_act), (auth ? string_to_name(account_name.c_str()) : (string_to_name(account_name.c_str()) == N(bob)) ? N(alice) : N(bob)));
   }

   fc::variant get_identity(uint64_t idnt) {
      const auto& db = control->db();
      const auto* t_id = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(identity), N(identity), N(ident)));
      FC_ASSERT(t_id != 0, "object not found");

      const auto& idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, idnt));
      FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id, "lower_bound failed");
      BOOST_REQUIRE_EQUAL(idnt, itr->primary_key);

      vector<char> data;
      copy_row(*itr, data);
      return abi_ser.binary_to_variant("identrow", data);
   }

   action_result certify(const string& certifier, uint64_t identity, const vector<fc::variant>& fields, bool auth = true) {
      action cert_act;
      cert_act.account = N(identity);
      cert_act.name = N(certprop);
      cert_act.data = abi_ser.variant_to_binary("certprop", mutable_variant_object()
                                                ("bill_storage_to", certifier)
                                                ("certifier", certifier)
                                                ("identity", identity)
                                                ("value", fields)
      );
      return push_action( std::move(cert_act), (auth ? string_to_name(certifier.c_str()) : (string_to_name(certifier.c_str()) == N(bob)) ? N(alice) : N(bob)));
   }

   fc::variant get_certrow(uint64_t identity, const string& property, uint64_t trusted, const string& certifier) {
      const auto& db = control->db();
      const auto* t_id = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(identity), identity, N( certs )));
      if ( !t_id ) {
         return fc::variant(nullptr);
      }

      const auto& idx = db.get_index<index256_index, by_secondary>();
      auto key = key256::make_from_word_sequence<uint64_t>(string_to_name(property.c_str()), trusted, string_to_name(certifier.c_str()));

      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, key.get_array()));
      if (itr != idx.end() && itr->t_id == t_id->id && itr->secondary_key == key.get_array()) {
         auto primary_key = itr->primary_key;
         const auto& idx = db.get_index<key_value_index, by_scope_primary>();

         auto itr = idx.lower_bound(boost::make_tuple(t_id->id, primary_key));
         FC_ASSERT( itr != idx.end() && itr->t_id == t_id->id && primary_key == itr->primary_key,
                    "Record found in secondary index, but not found in primary index."
         );
         vector<char> data;
         copy_row(*itr, data);
         return abi_ser.binary_to_variant("certrow", data);
      } else {
         return fc::variant(nullptr);
      }
   }

   fc::variant get_accountrow(const string& account) {
      const auto& db = control->db();
      uint64_t acnt = string_to_name(account.c_str());
      const auto* t_id = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(identity), acnt, N(account)));
      if (!t_id) {
         return fc::variant(nullptr);
      }
      const auto& idx = db.get_index<key_value_index, by_scope_primary>();
      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, N(account)));
      if( itr != idx.end() && itr->t_id == t_id->id && N(account) == itr->primary_key) {
         vector<char> data;
         copy_row(*itr, data);
         return abi_ser.binary_to_variant("accountrow", data);
      } else {
         return fc::variant(nullptr);
      }
   }

   action_result settrust(const string& trustor, const string& trusting, uint64_t trust, bool auth = true)
   {
      signed_transaction trx;
      action settrust_act;
      settrust_act.account = N(identity);
      settrust_act.name = N(settrust);
      settrust_act.data = abi_ser.variant_to_binary("settrust", mutable_variant_object()
                                                    ("trustor", trustor)
                                                    ("trusting", trusting)
                                                    ("trust", trust)
      );
      auto tr = string_to_name(trustor.c_str());
      return push_action( std::move(settrust_act), (auth ? tr : 0) );
   }

   bool get_trust(const string& trustor, const string& trusting) {
      const auto& db = control->db();
      const auto* t_id = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(identity), string_to_name(trustor.c_str()), N(trust)));
      if (!t_id) {
         return false;
      }

      uint64_t tng = string_to_name(trusting.c_str());
      const auto& idx = db.get_index<key_value_index, by_scope_primary>();
      auto itr = idx.lower_bound(boost::make_tuple(t_id->id, tng));
      return ( itr != idx.end() && itr->t_id == t_id->id && tng == itr->primary_key ); //true if found
   }

public:
   abi_serializer abi_ser;
   abi_serializer abi_ser_test;
   std::string producer_name;
};

constexpr uint64_t identity_val = 0xffffffffffffffff; //64-bit value

BOOST_AUTO_TEST_SUITE(identity_tests)

BOOST_FIXTURE_TEST_CASE( identity_create, identity_tester ) try {

   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", identity_val));
   fc::variant idnt = get_identity(identity_val);
   BOOST_REQUIRE_EQUAL( identity_val, idnt["identity"].as_uint64());
   BOOST_REQUIRE_EQUAL( "alice", idnt["creator"].as_string());

   //attempts to create already existing identity should fail
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("identity already exists"), create_identity("alice", identity_val));
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("identity already exists"), create_identity("bob", identity_val));

   //alice can create more identities
   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", 2));
   fc::variant idnt2 = get_identity(2);
   BOOST_REQUIRE_EQUAL( 2, idnt2["identity"].as_uint64());
   BOOST_REQUIRE_EQUAL( "alice", idnt2["creator"].as_string());

   //bob can create an identity as well
   BOOST_REQUIRE_EQUAL(success(), create_identity("bob", 1));

   //identity == 0 has special meaning, should be impossible to create
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("identity=0 is not allowed"), create_identity("alice", 0));

   //creating adentity without authentication is not allowed
   BOOST_REQUIRE_EQUAL(error("missing authority of alice"), create_identity("alice", 3, false));

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
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
                                                                                     ("property", "name")
                                                                                     ("type", "string")
                                                                                     ("data", to_uint8_vector("Alice Smith"))
                                                                                     ("memo", "")
                                                                                     ("confidence", 100)
               }));

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
   vector<fc::variant> fields = { mutable_variant_object()
                                  ("property", "email")
                                  ("type", "string")
                                  ("data", to_uint8_vector("alice@alice.name"))
                                  ("memo", "official email")
                                  ("confidence", 95),
                                  mutable_variant_object()
                                  ("property", "address")
                                  ("type", "string")
                                  ("data", to_uint8_vector("1750 Kraft Drive SW, Blacksburg, VA 24060"))
                                  ("memo", "official address")
                                  ("confidence", 80)
   };

   //shouldn't be able to certify without authorization
   BOOST_REQUIRE_EQUAL(error("missing authority of bob"), certify("bob", identity_val, fields, false));

   //certifying non-existent identity is not allowed
   uint64_t non_existent = 11;
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("identity does not exist"),
                       certify("alice", non_existent, vector<fc::variant>{ mutable_variant_object()
                                ("property", "name")
                                ("type", "string")
                                ("data", to_uint8_vector("Alice Smith"))
                                ("memo", "")
                                ("confidence", 100)
                                })
   );

   //parameter "type" should be not longer than 32 bytes
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("certrow::type should be not longer than 32 bytes"),
                       certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
                                ("property", "height")
                                ("type", "super_long_type_name_wich_is_not_allowed")
                                ("data", to_uint8_vector("Alice Smith"))
                                ("memo", "")
                                ("confidence", 100)
                                })
   );

   //bob also should be able to certify
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
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
                                                                                     ("property", "email")
                                                                                     ("type", "string")
                                                                                     ("data", to_uint8_vector("alice.smith@gmail.com"))
                                                                                     ("memo", "")
                                                                                     ("confidence", 100)
               }));
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
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
                                                                                     ("property", "email")
                                                                                     ("type", "string")
                                                                                     ("data", to_uint8_vector(""))
                                                                                     ("memo", "")
                                                                                     ("confidence", 0)
               }));
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
   BOOST_REQUIRE_EQUAL(true, get_trust("bob", "alice"));

   //relation of trust in opposite direction should not exist
   BOOST_REQUIRE_EQUAL(false, get_trust("alice", "bob"));

   //remove trust
   BOOST_REQUIRE_EQUAL(success(), settrust("bob", "alice", 0));
   BOOST_REQUIRE_EQUAL(false, get_trust("bob", "alice"));

} FC_LOG_AND_RETHROW() //trust_untrust

BOOST_FIXTURE_TEST_CASE( certify_decertify_owner, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", identity_val));

   //bob certifies ownership for alice
   BOOST_REQUIRE_EQUAL(success(), certify("bob", identity_val, vector<fc::variant>{ mutable_variant_object()
                                                                              ("property", "owner")
                                                                              ("type", "account")
                                                                              ("data", to_uint8_vector(N(alice)))
                                                                              ("memo", "claiming onwership")
                                                                              ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "bob").is_object() );
   //it should not affect "account" singleton in alice's scope since it's not self-certification
   BOOST_REQUIRE_EQUAL( true, get_accountrow("alice").is_null() );
   //it also shouldn't affect "account" singleton in bob's scope since he certified not himself
   BOOST_REQUIRE_EQUAL( true, get_accountrow("bob").is_null() );

   // alice certifies her ownership, should populate "account" singleton
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
                                                                              ("property", "owner")
                                                                              ("type", "account")
                                                                              ("data", to_uint8_vector(N(alice)))
                                                                              ("memo", "claiming onwership")
                                                                              ("confidence", 100)
               }));
   fc::variant certrow = get_certrow(identity_val, "owner", 0, "alice");
   BOOST_REQUIRE_EQUAL( true, certrow.is_object() );
   BOOST_REQUIRE_EQUAL( "owner", certrow["property"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, certrow["trusted"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "alice", certrow["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( 100, certrow["confidence"].as_uint64() );
   BOOST_REQUIRE_EQUAL( "account", certrow["type"].as_string() );
   BOOST_REQUIRE_EQUAL( N(alice), to_uint64(certrow["data"]) );

   //check that singleton "account" in the alice's scope contains the identity
   fc::variant acntrow = get_accountrow("alice");
   BOOST_REQUIRE_EQUAL( true, certrow.is_object() );
   BOOST_REQUIRE_EQUAL( identity_val, acntrow["identity"].as_uint64() );

   // ownership was certified by alice, but not by a block producer or someone trusted by a block producer
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(0, get_identity_for_account("alice"));

   //remove bob's certification
   BOOST_REQUIRE_EQUAL(success(), certify("bob", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "claiming onwership")
               ("confidence", 0)
               }));
   //singleton "account" in the alice's scope still should contain the identity
   acntrow = get_accountrow("alice");
   BOOST_REQUIRE_EQUAL( true, certrow.is_object() );
   BOOST_REQUIRE_EQUAL( identity_val, acntrow["identity"].as_uint64() );

   //remove owner certification
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
                                                                              ("property", "owner")
                                                                              ("type", "account")
                                                                              ("data", to_uint8_vector(N(alice)))
                                                                              ("memo", "claiming onwership")
                                                                              ("confidence", 0)
               }));
   certrow = get_certrow(identity_val, "owner", 0, "alice");
   BOOST_REQUIRE_EQUAL(true, certrow.is_null());

   //check that singleton "account" in the alice's scope doesn't contain the identity
   acntrow = get_accountrow("alice");
   BOOST_REQUIRE_EQUAL(true, certrow.is_null());

} FC_LOG_AND_RETHROW() //certify_decertify_owner

BOOST_FIXTURE_TEST_CASE( owner_certified_by_producer, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", identity_val));

   // certify owner by a block producer, should result in trusted certification
   BOOST_REQUIRE_EQUAL(success(), certify( producer_name, identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "")
               ("confidence", 100)
               }));
   fc::variant certrow = get_certrow(identity_val, "owner", 1, producer_name);
   BOOST_REQUIRE_EQUAL( true, certrow.is_object() );
   BOOST_REQUIRE_EQUAL( "owner", certrow["property"].as_string() );
   BOOST_REQUIRE_EQUAL( 1, certrow["trusted"].as_uint64() );
   BOOST_REQUIRE_EQUAL( producer_name, certrow["certifier"].as_string() );
   BOOST_REQUIRE_EQUAL( N(alice), to_uint64(certrow["data"]) );

   //uncertified copy of that row shouldn't exist
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, producer_name).is_null());

   //alice still has not claimed the identity - she is not the official owner yet
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(0, get_identity_for_account("alice"));


      //alice claims it
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "claiming onwership")
               ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "alice").is_object());

   //now alice should be the official owner
   BOOST_REQUIRE_EQUAL(N(alice), get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(identity_val, get_identity_for_account("alice"));

   //block producer decertifies ownership
   BOOST_REQUIRE_EQUAL(success(), certify(producer_name, identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "")
               ("confidence", 0)
               }));
   //self-certification made by alice still exists
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "alice").is_object());
   //but now she is not official owner
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(0, get_identity_for_account("alice"));

} FC_LOG_AND_RETHROW() //owner_certified_by_producer

BOOST_FIXTURE_TEST_CASE( owner_certified_by_trusted_account, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), create_identity("bob", identity_val));

   //alice claims the identity created by bob
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "claiming onwership")
               ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "alice").is_object());
   //alice claimed the identity, but it hasn't been certified yet - she is not the official owner
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(0, get_identity_for_account("alice"));

   //block producer trusts bob
   BOOST_REQUIRE_EQUAL(success(), settrust(producer_name, "bob", 1));
   BOOST_REQUIRE_EQUAL(true, get_trust(producer_name, "bob"));

   // bob (trusted account) certifies alice's ownership, it should result in trusted certification
   BOOST_REQUIRE_EQUAL(success(), certify("bob", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "")
               ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 1, "bob").is_object() );
   //no untrusted row should exist
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "bob").is_null() );

   //now alice should be the official owner
   BOOST_REQUIRE_EQUAL(N(alice), get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(identity_val, get_identity_for_account("alice"));

   //block producer stops trusting bob
   BOOST_REQUIRE_EQUAL(success(), settrust(producer_name, "bob", 0));
   BOOST_REQUIRE_EQUAL(false, get_trust(producer_name, "bob"));

   //certification made by bob is still flaged as trusted
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 1, "bob").is_object() );

   //but now alice shouldn't be the official owner
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(0, get_identity_for_account("alice"));

} FC_LOG_AND_RETHROW() //owner_certified_by_trusted_account

BOOST_FIXTURE_TEST_CASE( owner_certification_becomes_trusted, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), create_identity("bob", identity_val));

   // bob (not trusted so far) certifies alice's ownership, it should result in untrusted certification
   BOOST_REQUIRE_EQUAL(success(), certify("bob", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "")
               ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "bob").is_object() );
   //no trusted row should exist
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 1, "bob").is_null() );

   //alice claims the identity created by bob
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "claiming onwership")
               ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "alice").is_object());
   //alice claimed the identity, but it is certified by untrusted accounts only - she is not the official owner
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(0, get_identity_for_account("alice"));

   //block producer trusts bob
   BOOST_REQUIRE_EQUAL(success(), settrust(producer_name, "bob", 1));
   BOOST_REQUIRE_EQUAL(true, get_trust(producer_name, "bob"));

   //old certification made by bob still shouldn't be flaged as trusted
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 0, "bob").is_object() );

   //but effectively bob's certification should became trusted
   BOOST_REQUIRE_EQUAL(N(alice), get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(identity_val, get_identity_for_account("alice"));

} FC_LOG_AND_RETHROW() //owner_certification_becomes_trusted

BOOST_FIXTURE_TEST_CASE( ownership_contradiction, identity_tester ) try {
   BOOST_REQUIRE_EQUAL(success(), create_identity("alice", identity_val));

   //alice claims identity
   BOOST_REQUIRE_EQUAL(success(), certify("alice", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "claiming onwership")
               ("confidence", 100)
               }));

   // block producer certifies alice's ownership
   BOOST_REQUIRE_EQUAL(success(), certify(producer_name, identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(alice)))
               ("memo", "")
               ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 1, producer_name).is_object() );

   //now alice is the official owner of the identity
   BOOST_REQUIRE_EQUAL(N(alice), get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(identity_val, get_identity_for_account("alice"));

   //bob claims identity
   BOOST_REQUIRE_EQUAL(success(), certify("bob", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(bob)))
               ("memo", "claiming onwership")
               ("confidence", 100)
               }));


   //block producer trusts carol
   BOOST_REQUIRE_EQUAL(success(), settrust(producer_name, "carol", 1));
   BOOST_REQUIRE_EQUAL(true, get_trust(producer_name, "carol"));

   //another trusted delegate certifies bob's identity (to the identity already certified to alice)
   BOOST_REQUIRE_EQUAL(success(), certify("carol", identity_val, vector<fc::variant>{ mutable_variant_object()
               ("property", "owner")
               ("type", "account")
               ("data", to_uint8_vector(N(bob)))
               ("memo", "")
               ("confidence", 100)
               }));
   BOOST_REQUIRE_EQUAL( true, get_certrow(identity_val, "owner", 1, producer_name).is_object() );

   //now neither alice or bob are official owners, because we have 2 trusted certifications in contradiction to each other
   BOOST_REQUIRE_EQUAL(0, get_owner_for_identity(identity_val));
   BOOST_REQUIRE_EQUAL(0, get_identity_for_account("alice"));

} FC_LOG_AND_RETHROW() //ownership_contradiction

BOOST_AUTO_TEST_SUITE_END()
