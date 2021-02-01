#define BOOST_TEST_MODULE tpm_tests
#include <boost/test/included/unit_test.hpp>

#include <eosio/tpm-helpers/tpm-helpers.hpp>

#include <fc/crypto/public_key.hpp>
#include <fc/crypto/signature.hpp>

#include <openssl/rand.h>

#include <boost/process.hpp>

using namespace std::literals::string_literals;
using namespace eosio::tpm;

BOOST_AUTO_TEST_SUITE(tpm_tests)

BOOST_AUTO_TEST_CASE(basic_test) try {
   swtpm tpm;

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 0);

   fc::crypto::public_key pubkey = create_key(tpm.tcti(), {});
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 1);

   {
      tpm_key key(tpm.tcti(), pubkey, {});
      for(unsigned i = 0; i < 16; ++i) {
         fc::sha256 digest;
         RAND_bytes((unsigned char*)digest.data(), sizeof(digest));

         fc::crypto::signature sig = key.sign(digest);
         BOOST_CHECK_EQUAL(pubkey, fc::crypto::public_key(sig, digest));
      }
   }

   {
      //can have two tpm_keys referring to same underlying key
      tpm_key key1(tpm.tcti(), pubkey, {});
      tpm_key key2(tpm.tcti(), pubkey, {});

      fc::crypto::signature sig1 = key1.sign(fc::sha256());
      fc::crypto::signature sig2 = key2.sign(fc::sha256());

      BOOST_CHECK_EQUAL(pubkey, fc::crypto::public_key(sig1, fc::sha256()));
      BOOST_CHECK_EQUAL(pubkey, fc::crypto::public_key(sig2, fc::sha256()));
   }

   //make a second key and ensure can sign correctly with both
   fc::crypto::public_key pubkey2 = create_key(tpm.tcti(), {});
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 2);

   {
      tpm_key key1(tpm.tcti(), pubkey, {});
      tpm_key key2(tpm.tcti(), pubkey2, {});

      fc::crypto::signature sig1 = key1.sign(fc::sha256());
      fc::crypto::signature sig2 = key2.sign(fc::sha256());

      BOOST_CHECK_EQUAL(pubkey, fc::crypto::public_key(sig1, fc::sha256()));
      BOOST_CHECK_EQUAL(pubkey2, fc::crypto::public_key(sig2, fc::sha256()));
   }

   //try and get a key that won't exist
   BOOST_CHECK_THROW(tpm_key(tpm.tcti(), fc::crypto::public_key(), {}), fc::exception);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(bad_tcti) try {
   BOOST_CHECK_THROW(tpm_key("lalalalala", fc::crypto::public_key(), {}), fc::exception);
   BOOST_CHECK_THROW(get_all_persistent_keys("trolololololol").size(), fc::exception);
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(existing_non_ecc) try {
   swtpm tpm;
   fc::temp_file primary_ctx;

   //place an RSA key in to persistent handle 0x81000001
   {
      std::stringstream ss;
      ss << "tpm2_createprimary -c " << primary_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_evictcontrol -c " << primary_ctx.path().generic_string() << " -T " << tpm.tcti() << " 0x81000001";
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_flushcontext -t -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 0);

   //create one key (will end up in 0x81000000)
   fc::crypto::public_key pubkey = create_key(tpm.tcti(), {});
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 1);

   //this second key will end up in 0x81000002
   fc::crypto::public_key pubkey2 = create_key(tpm.tcti(), {});
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 2);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(non_p256) try {
   swtpm tpm;
   fc::temp_file primary_ctx;
   fc::temp_file created_ctx;

   {
      std::stringstream ss;
      ss << "tpm2_createprimary -c " << primary_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_create -G ecc384 -C " << primary_ctx.path().generic_string() << " -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_flushcontext -t -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_evictcontrol -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_flushcontext -t -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 0);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(high_handle) try {
   swtpm tpm;
   fc::temp_file primary_ctx;
   fc::temp_file created_ctx;

   {
      std::stringstream ss;
      ss << "tpm2_createprimary -c " << primary_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_create -G ecc -C " << primary_ctx.path().generic_string() << " -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_flushcontext -t -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_evictcontrol -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti() << " 0x8100ffff";
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 1);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(no_count_transient) try {
   swtpm tpm;
   fc::temp_file primary_ctx;
   fc::temp_file created_ctx;

   {
      std::stringstream ss;
      ss << "tpm2_createprimary -c " << primary_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_create -G ecc -C " << primary_ctx.path().generic_string() << " -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 0);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(destroy_during_use) try {
   swtpm tpm;

   fc::crypto::public_key pubkey = create_key(tpm.tcti(), {});
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 1);

   tpm_key key(tpm.tcti(), pubkey, {});
   key.sign(fc::sha256());

   {
      std::stringstream ss;
      ss << "tpm2_evictcontrol -T " << tpm.tcti() << " -c 0x81000000";
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }

   BOOST_CHECK_THROW(key.sign(fc::sha256()), fc::exception);

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 0);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(two_tpms) try {
   swtpm tpm1;
   swtpm tpm2;

   fc::crypto::public_key pubkey1 = create_key(tpm1.tcti(), {});

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm1.tcti()).size(), 1);
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm2.tcti()).size(), 0);

   fc::crypto::public_key pubkey2 = create_key(tpm2.tcti(), {});

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm1.tcti()).size(), 1);
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm2.tcti()).size(), 1);
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(fill_it_up) try {
   swtpm tpm;

   std::list<fc::crypto::public_key> pubs;

   for(unsigned i = 0 ; i < 128; i++) {
      try {
         pubs.emplace_back(create_key(tpm.tcti(), {}));
      } catch(...) {
         break;
      }
   }

   tpm_key key(tpm.tcti(), pubs.front(), {});
   key.sign(fc::sha256());

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(basic_pcr) try {
   swtpm tpm;

   fc::crypto::public_key pubkey = create_key(tpm.tcti(), {0, 1, 7});
   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 1);

   {
      tpm_key key(tpm.tcti(), pubkey, {0, 1, 7});
      key.sign(fc::sha256());
      key.sign(fc::sha256());
      key.sign(fc::sha256());
   }

   {
      tpm_key key(tpm.tcti(), pubkey, {0, 7});
      //authorization not checked until signing
      BOOST_CHECK_THROW(key.sign(fc::sha256()), fc::exception);
   }

   {
      tpm_key key(tpm.tcti(), pubkey, {});
      //authorization not checked until signing
      BOOST_CHECK_THROW(key.sign(fc::sha256()), fc::exception);
   }

   fc::crypto::public_key pubkey2 = create_key(tpm.tcti(), {});
   tpm_key key(tpm.tcti(), pubkey2, {0, 7});
   BOOST_CHECK_THROW(key.sign(fc::sha256()), fc::exception);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(no_sessions_avail) try {
   swtpm tpm;
   fc::temp_file session_file;

   for(unsigned i = 0; i < 128; ++i) {
      std::stringstream ss;
      ss << "tpm2_startauthsession -S " << session_file.path().generic_string() << " -T " << tpm.tcti();
      if(boost::process::system(ss.str()))
         break;
   }

   BOOST_CHECK_THROW(create_key(tpm.tcti(), {0, 1, 7}), fc::exception);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(no_count_platform_persistent) try {
   swtpm tpm;
   fc::temp_file primary_ctx;
   fc::temp_file created_ctx;

   {
      std::stringstream ss;
      ss << "tpm2_createprimary -C p -c " << primary_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_create -G ecc -C " << primary_ctx.path().generic_string() << " -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_flushcontext 0x80000000 -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_evictcontrol -C p -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_flushcontext -t -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 0);

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(externally_create_pcr) try {
   swtpm tpm;
   fc::temp_file policy_file;
   fc::temp_file primary_ctx;
   fc::temp_file created_ctx;

   {
      std::stringstream ss;
      ss << "tpm2_createpolicy --policy-pcr -l sha256:0,1 -L " << policy_file.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_createprimary -c " << primary_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_create -G ecc -C " << primary_ctx.path().generic_string() << " -L " << policy_file.path().generic_string() << " -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_flushcontext -t -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }
   {
      std::stringstream ss;
      ss << "tpm2_evictcontrol -c " << created_ctx.path().generic_string() << " -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);
   }

   BOOST_CHECK_EQUAL(get_all_persistent_keys(tpm.tcti()).size(), 1);

   {
      tpm_key k(tpm.tcti(), *get_all_persistent_keys(tpm.tcti()).begin(), {});
      BOOST_CHECK_THROW(k.sign(fc::sha256()), fc::exception);
   }
   {
      tpm_key k(tpm.tcti(), *get_all_persistent_keys(tpm.tcti()).begin(), {0, 1});
      k.sign(fc::sha256());
   }

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(basic_attest) try {
   swtpm tpm;

   fc::crypto::public_key ak = create_key(tpm.tcti(), {});
   const uint32_t ak_handle = 0x81000000u;  //first owner owned persistent handle

   attested_key new_key = create_key_attested(tpm.tcti(), {}, ak_handle);
   BOOST_CHECK_EQUAL(ak, verify_attestation(new_key));

   //key was created with no policy; so verifying with a policy must fail
   const std::map<unsigned, fc::sha256>& pcr_policy = {{0u, {}}};
   BOOST_CHECK_THROW(verify_attestation(new_key, pcr_policy), fc::exception);
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(basic_attest_with_policy) try {
   swtpm tpm;

   fc::crypto::public_key ak = create_key(tpm.tcti(), {});
   const uint32_t ak_handle = 0x81000000u;  //first owner owned persistent handle

   attested_key new_key = create_key_attested(tpm.tcti(), {0u, 7u}, ak_handle);

   BOOST_CHECK_THROW(verify_attestation(new_key), fc::exception);

   //default PCR values in a TPM are 0x0000...
   const std::map<unsigned, fc::sha256>& pcr_policy = {{0u, fc::sha256()}, {7u, fc::sha256()}};
   BOOST_CHECK_EQUAL(ak, verify_attestation(new_key, pcr_policy));

   const std::map<unsigned, fc::sha256>& wrong_pcr_hash = {{0u, fc::sha256::hash(std::string("banana"))}, {7u, fc::sha256()}};
   BOOST_CHECK_THROW(verify_attestation(new_key, wrong_pcr_hash), fc::exception);

   const std::map<unsigned, fc::sha256>& wrong_pcrs_policy = {{2u, fc::sha256()}, {7u, fc::sha256()}};
   BOOST_CHECK_THROW(verify_attestation(new_key, wrong_pcrs_policy), fc::exception);
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(test_attest_checks) try {
   swtpm tpm;

   fc::crypto::public_key ak = create_key(tpm.tcti(), {});
   const uint32_t ak_handle = 0x81000000u;  //first owner owned persistent handle

   attested_key new_key = create_key_attested(tpm.tcti(), {}, ak_handle);
   BOOST_CHECK_EQUAL(ak, verify_attestation(new_key));

   attested_key damaged_attested_key;

   damaged_attested_key = new_key;
   damaged_attested_key.pub_key = fc::crypto::public_key();
   BOOST_CHECK_THROW(verify_attestation(damaged_attested_key), fc::exception);

   damaged_attested_key = new_key;
   char serialized_signature[sizeof(fc::crypto::r1::compact_signature) + 1] = {fc::get_index<fc::crypto::signature::storage_type, fc::crypto::r1::signature_shim>(), 0x27};
   damaged_attested_key.certification_signature = fc::raw::unpack<fc::crypto::signature>(serialized_signature, sizeof(serialized_signature));
   BOOST_CHECK_THROW(verify_attestation(damaged_attested_key), fc::exception);

   damaged_attested_key = new_key;
   damaged_attested_key.public_area.data[4]++;
   BOOST_CHECK_THROW(verify_attestation(damaged_attested_key), fc::exception);

   damaged_attested_key = new_key;
   damaged_attested_key.creation_certification.data[4]++;
   BOOST_CHECK_THROW(verify_attestation(damaged_attested_key), fc::exception);
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(basic_nv) try {
   swtpm tpm;

   std::vector<char> mydata(2048);
   RAND_bytes((unsigned char*)mydata.data(), mydata.size());
   {
      nv_data d(tpm.tcti(), 0, {1,2});
      BOOST_CHECK(!d.data());
      d.set_data(mydata);
      BOOST_CHECK(d.data());
   }
   {
      nv_data d(tpm.tcti(), 0, {1,2});
      BOOST_CHECK(d.data());
      auto stored_val = *d.data();
      BOOST_CHECK(stored_val == mydata);
   }
   {
      nv_data d(tpm.tcti(), 0, {});
      BOOST_CHECK_THROW(d.data(), fc::exception);
   }

   //check w/o PCR policy
   {
      nv_data d(tpm.tcti(), 1, {});
      BOOST_CHECK(!d.data());
      d.set_data(mydata);
      BOOST_CHECK(d.data());
   }
   {
      nv_data d(tpm.tcti(), 1, {});
      BOOST_CHECK(d.data());
      auto stored_val = *d.data();
      BOOST_CHECK(stored_val == mydata);
   }

   //small items get padded out to 2048. it's just how it it's going to be. deal with it
   std::vector<char> small_data(412);
   RAND_bytes((unsigned char*)small_data.data(), small_data.size());
   {
      nv_data d(tpm.tcti(), 2, {7});
      BOOST_CHECK(!d.data());
      d.set_data(small_data);
      BOOST_CHECK(d.data());
   }
   {
      nv_data d(tpm.tcti(), 2, {7});
      BOOST_CHECK(d.data());
      auto stored_val = *d.data();
      stored_val.resize(small_data.size());
      BOOST_CHECK(stored_val == small_data);
   }

   //well, ACTUALLY, it's whatever the public area defines it as. Create NV index with the tools with a weirdo size defined
   {
      std::stringstream ss;
      ss << "tpm2_nvdefine 3 -s 1244 -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);

      nv_data d(tpm.tcti(), 3, {});
      BOOST_CHECK(!d.data());
      d.set_data(std::vector<char>());
      auto stored_val = *d.data();
      BOOST_CHECK(stored_val.size() == 1244);
   }

   //check size 0 while we're at it
   {
      std::stringstream ss;
      ss << "tpm2_nvdefine 4 -s 0 -T " << tpm.tcti();
      BOOST_CHECK(boost::process::system(ss.str()) == 0);

      nv_data d(tpm.tcti(), 4, {});
      BOOST_CHECK(!d.data());
      BOOST_CHECK_THROW(d.set_data(std::vector<char>(6)), fc::exception);
      d.set_data(std::vector<char>());
      auto stored_val = *d.data();
      BOOST_CHECK(stored_val.size() == 0);
   }

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_SUITE_END()