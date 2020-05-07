#define BOOST_TEST_MODULE webauthn_test_mod
#include <boost/test/included/unit_test.hpp>
#include <boost/algorithm/string.hpp>

#include <fc/crypto/public_key.hpp>
#include <fc/crypto/private_key.hpp>
#include <fc/crypto/signature.hpp>
#include <fc/utility.hpp>

using namespace fc::crypto;
using namespace fc;
using namespace std::literals;

static fc::crypto::webauthn::signature make_webauthn_sig(const fc::crypto::r1::private_key& priv_key,
                                                         std::vector<uint8_t>& auth_data,
                                                         const std::string& json) {

   //webauthn signature is sha256(auth_data || client_data_hash)
   fc::sha256 client_data_hash = fc::sha256::hash(json);
   fc::sha256::encoder e;
   e.write((char*)auth_data.data(), auth_data.size());
   e.write(client_data_hash.data(), client_data_hash.data_size());

   r1::compact_signature sig = priv_key.sign_compact(e.result());

   char buff[8192];
   datastream<char*> ds(buff, sizeof(buff));
   fc::raw::pack(ds, sig);
   fc::raw::pack(ds, auth_data);
   fc::raw::pack(ds, json);
   ds.seekp(0);

   fc::crypto::webauthn::signature ret;
   fc::raw::unpack(ds, ret);

   return ret;
}

//used for many below
static const r1::private_key priv = fc::crypto::r1::private_key::generate();
static const r1::public_key pub = priv.get_public_key();
static const fc::sha256 d = fc::sha256::hash("sup"s);
static const fc::sha256 origin_hash = fc::sha256::hash("fctesting.invalid"s);

BOOST_AUTO_TEST_SUITE(webauthn_suite)

//Good signature
BOOST_AUTO_TEST_CASE(good) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);

   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EQUAL(wa_pub, make_webauthn_sig(priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//A valid signature but shouldn't match public key due to presence difference
BOOST_AUTO_TEST_CASE(mismatch_presence) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_PRESENT, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));
   auth_data[32] |= 0x04; //User presence verified

   BOOST_CHECK_NE(wa_pub, make_webauthn_sig(priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//A valid signature but shouldn't match public key due to origin difference
BOOST_AUTO_TEST_CASE(mismatch_origin) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_PRESENT, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://mallory.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   fc::sha256 mallory_origin_hash = fc::sha256::hash("mallory.invalid"s);
   memcpy(auth_data.data(), mallory_origin_hash.data(), sizeof(mallory_origin_hash));

   BOOST_CHECK_NE(wa_pub, make_webauthn_sig(priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//A valid signature but shouldn't recover because http was in use instead of https
BOOST_AUTO_TEST_CASE(non_https) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"http://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn origin must begin with https://") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//A valid signature but shouldn't recover because there is no origin scheme
BOOST_AUTO_TEST_CASE(lacking_scheme) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn origin must begin with https://") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//A valid signature but shouldn't recover because empty origin
BOOST_AUTO_TEST_CASE(empty_origin) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn origin must begin with https://") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//Good signature with a port
BOOST_AUTO_TEST_CASE(good_port) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid:123456\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EQUAL(wa_pub, make_webauthn_sig(priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//Good signature with an empty port
BOOST_AUTO_TEST_CASE(empty_port) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid:\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EQUAL(wa_pub, make_webauthn_sig(priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//valid signature but with misc junk in challenge
BOOST_AUTO_TEST_CASE(challenge_junk) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + "blahBLAH"s + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::exception, [](const fc::exception& e) {
      return e.to_detail_string().find("sha256: size mismatch") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//valid signature but with non base64 in challenge
BOOST_AUTO_TEST_CASE(challenge_non_base64) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + "hello@world$"s + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::exception, [](const fc::exception& e) {
      return e.to_detail_string().find("encountered non-base64 character") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//valid signature but with some other digest in the challenge that is not the one we are recovering from
BOOST_AUTO_TEST_CASE(challenge_wrong) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   fc::sha256 other_digest = fc::sha256::hash("yo"s);
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(other_digest.data(), other_digest.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("Wrong webauthn challenge") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//valid signature but wrong webauthn type
BOOST_AUTO_TEST_CASE(wrong_type) try {

   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.meh\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn signature type not an assertion") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//signature if good but the auth_data rpid hash is not what is expected for origin
BOOST_AUTO_TEST_CASE(auth_data_rpid_hash_bad) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   fc::sha256 origin_hash_corrupt = fc::sha256::hash("fctesting.invalid"s);
   memcpy(auth_data.data(), origin_hash_corrupt.data(), sizeof(origin_hash_corrupt));
   auth_data[4]++;

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn rpid hash doesn't match origin") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//signature if good but auth_data too short to store what needs to be stored
BOOST_AUTO_TEST_CASE(auth_data_too_short) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(1);

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("auth_data not as large as required") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//Good signature but missing origin completely
BOOST_AUTO_TEST_CASE(missing_origin) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn origin must begin with https://") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//Good signature but missing type completely
BOOST_AUTO_TEST_CASE(missing_type) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn signature type not an assertion") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//Good signature but missing challenge completely
BOOST_AUTO_TEST_CASE(missing_challenge) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::exception, [](const fc::exception& e) {
      return e.to_detail_string().find("sha256: size mismatch") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//Good signature with some extra stuff sprinkled in the json
BOOST_AUTO_TEST_CASE(good_extrajunk) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"cool\":\"beans\",\"obj\":{\"array\":[4, 5, 6]},"
                      "\"type\":\"webauthn.get\",\"answer\": 42 ,\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EQUAL(wa_pub, make_webauthn_sig(priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//Good signature but it's not a JSON object!
BOOST_AUTO_TEST_CASE(not_json_object) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "hey man"s;

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("Failed to parse client data JSON") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//damage the r/s portion of the signature
BOOST_AUTO_TEST_CASE(damage_sig) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   webauthn::signature sig = make_webauthn_sig(priv, auth_data, json);
   char buff[8192];
   datastream<char*> ds(buff, sizeof(buff));
   fc::raw::pack(ds, sig);
   buff[4]++;
   ds.seekp(0);
   fc::raw::unpack(ds, sig);

   bool failed_recovery = false;
   bool failed_compare = false;
   webauthn::public_key recovered_pub;

   try {
      recovered_pub = sig.recover(d, true);
      failed_compare = !(wa_pub == recovered_pub);
   }
   catch(fc::exception& e) {
      failed_recovery = e.to_detail_string().find("unable to reconstruct public key from signature") != std::string::npos;
   }

   //We can fail either by failing to recover any key, or recovering the wrong key. Check that at least one of these failed
   BOOST_CHECK_EQUAL(failed_recovery || failed_compare, true);
} FC_LOG_AND_RETHROW();

//damage the recovery index portion of the sig
BOOST_AUTO_TEST_CASE(damage_sig_idx) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   webauthn::signature sig = make_webauthn_sig(priv, auth_data, json);
   char buff[8192];
   datastream<char*> ds(buff, sizeof(buff));
   fc::raw::pack(ds, sig);
   buff[0]++;
   ds.seekp(0);
   fc::raw::unpack(ds, sig);

   bool failed_recovery = false;
   bool failed_compare = false;
   webauthn::public_key recovered_pub;

   try {
      recovered_pub = sig.recover(d, true);
      failed_compare = !(wa_pub == recovered_pub);
   }
   catch(fc::exception& e) {
      failed_recovery = e.to_detail_string().find("unable to reconstruct public key from signature") != std::string::npos;
   }

   //We can fail either by failing to recover any key, or recovering the wrong key. Check that at least one of these failed
   BOOST_CHECK_EQUAL(failed_recovery || failed_compare, true);
} FC_LOG_AND_RETHROW();

//Good signature but with a different private key than was expecting
BOOST_AUTO_TEST_CASE(different_priv_key) try {
   r1::private_key other_priv = fc::crypto::r1::private_key::generate();

   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_NE(wa_pub, make_webauthn_sig(other_priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//Good signature but has empty json
BOOST_AUTO_TEST_CASE(empty_json) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");
   std::string json;

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("Failed to parse client data JSON") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//empty rpid not allowed for pub key
BOOST_AUTO_TEST_CASE(empty_rpid) try {
   char data[67] = {};
   datastream<char*> ds(data, sizeof(data));
   webauthn::public_key pubkey;

   BOOST_CHECK_EXCEPTION(fc::raw::unpack(ds, pubkey), fc::assert_exception, [](const fc::assert_exception& e) {
      return e.to_detail_string().find("webauthn pubkey must have non empty rpid") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

//good sig but remove the trailing =, should still be accepted
BOOST_AUTO_TEST_CASE(good_no_trailing_equal) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");

   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";
   boost::erase_all(json, "=");

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EQUAL(wa_pub, make_webauthn_sig(priv, auth_data, json).recover(d, true));
} FC_LOG_AND_RETHROW();

//Before the base64 decoder was adjusted to throw in error (pre-webauthn-merge), it would simply stop when encountering
//a non-base64 char. This means it was possible for the following JSON & challenge to validate completely correctly. Now it
//should not pass.
BOOST_AUTO_TEST_CASE(base64_wonky) try {
   webauthn::public_key wa_pub(pub.serialize(), webauthn::public_key::user_presence_t::USER_PRESENCE_NONE, "fctesting.invalid");

   std::string json = "{\"origin\":\"https://fctesting.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "!@#$\"}";
   boost::erase_all(json, "=");

   std::vector<uint8_t> auth_data(37);
   memcpy(auth_data.data(), origin_hash.data(), sizeof(origin_hash));

   BOOST_CHECK_EXCEPTION(make_webauthn_sig(priv, auth_data, json).recover(d, true), fc::exception, [](const fc::exception& e) {
      return e.to_detail_string().find("encountered non-base64 character") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_SUITE_END()
