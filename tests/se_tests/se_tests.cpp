#define BOOST_TEST_MODULE se_tests
#include <boost/test/included/unit_test.hpp>

#include <eosio/se-helpers/se-helpers.hpp>

#include <fc/crypto/public_key.hpp>
#include <fc/crypto/signature.hpp>

using namespace std::literals::string_literals;

struct se_cleanup_fixture {
   se_cleanup_fixture() {
      erase_all();
   }

   ~se_cleanup_fixture() {
      erase_all();
   }

   void erase_all() {
      std::set<eosio::secure_enclave::secure_enclave_key> all_keys = eosio::secure_enclave::get_all_keys();

      while(!all_keys.empty())
         try {
            eosio::secure_enclave::delete_key(std::move(all_keys.extract(all_keys.begin()).value()));
         } catch(fc::assert_exception& ea) {}
   }
};

static bool pubkey_in_se_keyset(const fc::crypto::public_key& pub, const std::set<eosio::secure_enclave::secure_enclave_key>& keys) {
   for(const eosio::secure_enclave::secure_enclave_key& sekey : keys)
      if(sekey.public_key() == pub)
         return true;

   return false;
}

BOOST_TEST_GLOBAL_FIXTURE(se_cleanup_fixture);

BOOST_AUTO_TEST_SUITE(se_tests)

BOOST_AUTO_TEST_CASE(se_test) try {
   //Should start empty
   BOOST_CHECK_EQUAL(eosio::secure_enclave::get_all_keys().size(), 0);

   fc::crypto::public_key pub1, pub2, publast;

   //Create two keys
   pub1 = eosio::secure_enclave::create_key().public_key();
   pub2 = eosio::secure_enclave::create_key().public_key();

   //query for all SE keys, should be able to find both of those
   {
      auto se_keys = eosio::secure_enclave::get_all_keys();
      BOOST_CHECK_EQUAL(se_keys.size(), 2);
      BOOST_CHECK(pubkey_in_se_keyset(pub1, se_keys));
      BOOST_CHECK(pubkey_in_se_keyset(pub2, se_keys));
   }

   //remove one of the SE keys
   {
      auto se_keys = eosio::secure_enclave::get_all_keys();
      BOOST_CHECK(se_keys.begin()->public_key() == pub1 || se_keys.begin()->public_key() == pub2);
      //and make sure the one remaining is not the one removed
      if(se_keys.begin()->public_key() == pub1) {
         eosio::secure_enclave::delete_key(std::move(se_keys.extract(se_keys.begin()).value()));
         BOOST_CHECK(eosio::secure_enclave::get_all_keys().begin()->public_key() == pub2);
         publast = pub2;
      }
      else {
         eosio::secure_enclave::delete_key(std::move(se_keys.extract(se_keys.begin()).value()));
         BOOST_CHECK(eosio::secure_enclave::get_all_keys().begin()->public_key() == pub1);
         publast = pub1;
      }
   }

   BOOST_CHECK_EQUAL(eosio::secure_enclave::get_all_keys().size(), 1);

   //check that signing and key recovery matches up
   fc::sha256 banana = fc::sha256::hash("banana"s);
   fc::crypto::signature bananasig = eosio::secure_enclave::get_all_keys().begin()->sign(banana);
   BOOST_CHECK_EQUAL(fc::crypto::public_key(bananasig, banana), eosio::secure_enclave::get_all_keys().begin()->public_key());
   BOOST_CHECK_EQUAL(fc::crypto::public_key(bananasig, banana), publast);

   //check that duplicate key handles still work even after disposing of others
   {
      auto se_keys1 = eosio::secure_enclave::get_all_keys();
      auto se_keys2 = eosio::secure_enclave::get_all_keys();

      eosio::secure_enclave::delete_key(std::move(se_keys1.extract(se_keys1.begin()).value()));
      BOOST_CHECK_EQUAL(eosio::secure_enclave::get_all_keys().size(), 0);

      //make sure we can still use the second active keyhandle
      fc::crypto::signature bananasig2 = se_keys2.begin()->sign(banana);
      BOOST_CHECK_EQUAL(fc::crypto::public_key(bananasig2, banana), publast);
   }

   BOOST_CHECK_EQUAL(eosio::secure_enclave::get_all_keys().size(), 0);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_SUITE_END()