/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/utilities/key_conversion.hpp>
#include <eosio/utilities/rand.hpp>
#include <eosio/wallet_plugin/wallet.hpp>
#include <eosio/wallet_plugin/wallet_manager.hpp>

#include <boost/test/unit_test.hpp>
#include <eosio/chain/authority.hpp>

namespace eosio {

BOOST_AUTO_TEST_SUITE(wallet_tests)


/// Test creating the wallet
BOOST_AUTO_TEST_CASE(wallet_test)
{ try {
   using namespace eosio::wallet;
   using namespace eosio::utilities;

   wallet_data d;
   wallet_api wallet(d);
   BOOST_CHECK(wallet.is_locked());

   wallet.set_password("pass");
   BOOST_CHECK(wallet.is_locked());

   wallet.unlock("pass");
   BOOST_CHECK(!wallet.is_locked());

   wallet.set_wallet_filename("test");
   BOOST_CHECK_EQUAL("test", wallet.get_wallet_filename());

   BOOST_CHECK_EQUAL(0, wallet.list_keys().size());

   auto priv = fc::crypto::private_key::generate();
   auto pub = priv.get_public_key();
   auto wif = (std::string)priv;
   wallet.import_key(wif);
   BOOST_CHECK_EQUAL(1, wallet.list_keys().size());

   auto privCopy = wallet.get_private_key(pub);
   BOOST_CHECK_EQUAL(wif, (std::string)privCopy);

   wallet.lock();
   BOOST_CHECK(wallet.is_locked());
   wallet.unlock("pass");
   BOOST_CHECK_EQUAL(1, wallet.list_keys().size());
   wallet.save_wallet_file("wallet_test.json");

   wallet_data d2;
   wallet_api wallet2(d2);

   BOOST_CHECK(wallet2.is_locked());
   wallet2.load_wallet_file("wallet_test.json");
   BOOST_CHECK(wallet2.is_locked());

   wallet2.unlock("pass");
   BOOST_CHECK_EQUAL(1, wallet2.list_keys().size());

   auto privCopy2 = wallet2.get_private_key(pub);
   BOOST_CHECK_EQUAL(wif, (std::string)privCopy2);
} FC_LOG_AND_RETHROW() }

/// Test wallet manager
BOOST_AUTO_TEST_CASE(wallet_manager_test)
{ try {
   using namespace eosio::wallet;

   if (fc::exists("test.wallet")) fc::remove("test.wallet");
   if (fc::exists("test2.wallet")) fc::remove("test2.wallet");

   constexpr auto key1 = "5JktVNHnRX48BUdtewU7N1CyL4Z886c42x7wYW7XhNWkDQRhdcS";
   constexpr auto key2 = "5Ju5RTcVDo35ndtzHioPMgebvBM6LkJ6tvuU6LTNQv8yaz3ggZr";
   constexpr auto key3 = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"; // eosio key

   wallet_manager wm;
   BOOST_CHECK_EQUAL(0, wm.list_wallets().size());
   BOOST_CHECK_EQUAL(0, wm.list_keys().size());
   BOOST_CHECK_NO_THROW(wm.lock_all());

   BOOST_CHECK_THROW(wm.lock("test"), fc::exception);
   BOOST_CHECK_THROW(wm.unlock("test", "pw"), fc::exception);
   BOOST_CHECK_THROW(wm.import_key("test", "pw"), fc::exception);

   auto pw = wm.create("test");
   BOOST_CHECK(!pw.empty());
   BOOST_CHECK_EQUAL(0, pw.find("PW")); // starts with PW
   BOOST_CHECK_EQUAL(1, wm.list_wallets().size());
   // eosio key is imported automatically when a wallet is created
   BOOST_CHECK_EQUAL(1, wm.list_keys().size());
   BOOST_CHECK(wm.list_wallets().at(0).find("*") != std::string::npos);
   wm.lock("test");
   BOOST_CHECK(wm.list_wallets().at(0).find("*") == std::string::npos);
   wm.unlock("test", pw);
   BOOST_CHECK(wm.list_wallets().at(0).find("*") != std::string::npos);
   wm.import_key("test", key1);
   BOOST_CHECK_EQUAL(2, wm.list_keys().size());
   auto keys = wm.list_keys();
   
   auto pub_pri_pair = [](const char *key) -> auto {
       private_key_type prikey = private_key_type(std::string(key));
       return std::pair<const public_key_type, private_key_type>(prikey.get_public_key(), prikey);
   };

   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key1)) != keys.cend());

   wm.import_key("test", key2);
   keys = wm.list_keys();
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key1)) != keys.cend());
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key2)) != keys.cend());
   // key3 was automatically imported
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key3)) != keys.cend());
   wm.lock("test");
   BOOST_CHECK_EQUAL(0, wm.list_keys().size());
   wm.unlock("test", pw);
   BOOST_CHECK_EQUAL(3, wm.list_keys().size());
   wm.lock_all();
   BOOST_CHECK_EQUAL(0, wm.list_keys().size());
   BOOST_CHECK(wm.list_wallets().at(0).find("*") == std::string::npos);

   auto pw2 = wm.create("test2");
   BOOST_CHECK_EQUAL(2, wm.list_wallets().size());
   // eosio key is imported automatically when a wallet is created
   BOOST_CHECK_EQUAL(1, wm.list_keys().size());
   BOOST_CHECK_THROW(wm.import_key("test2", key3), fc::exception);
   keys = wm.list_keys();
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key1)) == keys.cend());
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key2)) == keys.cend());
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key3)) != keys.cend());
   wm.unlock("test", pw);
   keys = wm.list_keys();
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key1)) != keys.cend());
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key2)) != keys.cend());
   BOOST_CHECK(std::find(keys.cbegin(), keys.cend(), pub_pri_pair(key3)) != keys.cend());

   private_key_type pkey1{std::string(key1)};
   private_key_type pkey2{std::string(key2)};
   private_key_type pkey3{std::string(key3)};

   chain::signed_transaction trx;
   flat_set<public_key_type> pubkeys;
   pubkeys.emplace(pkey1.get_public_key());
   pubkeys.emplace(pkey2.get_public_key());
   pubkeys.emplace(pkey3.get_public_key());
   trx = wm.sign_transaction(trx, pubkeys, chain_id_type{});
   const auto& pks = trx.get_signature_keys(chain_id_type{});
   BOOST_CHECK_EQUAL(3, pks.size());
   BOOST_CHECK(find(pks.cbegin(), pks.cend(), pkey1.get_public_key()) != pks.cend());
   BOOST_CHECK(find(pks.cbegin(), pks.cend(), pkey2.get_public_key()) != pks.cend());
   BOOST_CHECK(find(pks.cbegin(), pks.cend(), pkey3.get_public_key()) != pks.cend());

   BOOST_CHECK_EQUAL(3, wm.list_keys().size());
   wm.set_timeout(chrono::seconds(0));
   BOOST_CHECK_EQUAL(0, wm.list_keys().size());


   BOOST_CHECK(fc::exists("test.wallet"));
   BOOST_CHECK(fc::exists("test2.wallet"));
   fc::remove("test.wallet");
   fc::remove("test2.wallet");

} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()

} // namespace eos
