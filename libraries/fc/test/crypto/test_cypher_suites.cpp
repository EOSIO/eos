#define BOOST_TEST_MODULE cypher_suites
#include <boost/test/unit_test.hpp>

#include <fc/crypto/public_key.hpp>
#include <fc/crypto/private_key.hpp>
#include <fc/crypto/signature.hpp>
#include <fc/utility.hpp>

using namespace fc::crypto;
using namespace fc;

BOOST_AUTO_TEST_SUITE(cypher_suites)
BOOST_AUTO_TEST_CASE(test_k1) try {
   auto private_key_string = std::string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3");
   auto expected_public_key = std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV");
   auto test_private_key = private_key(private_key_string);
   auto test_public_key = test_private_key.get_public_key();

   BOOST_CHECK_EQUAL(private_key_string, std::string(test_private_key));
   BOOST_CHECK_EQUAL(expected_public_key, std::string(test_public_key));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(test_r1) try {
   auto private_key_string = std::string("EOSR1iyQmnyPEGvFd8uffnk152WC2WryBjgTrg22fXQryuGL9mU6qW");
   auto expected_public_key = std::string("EOSR16EPHFSKVYHBjQgxVGQPrwCxTg7BbZ69H9i4gztN9deKTEXYne4");
   auto test_private_key = private_key(private_key_string);
   auto test_public_key = test_private_key.get_public_key();

   BOOST_CHECK_EQUAL(private_key_string, std::string(test_private_key));
   BOOST_CHECK_EQUAL(expected_public_key, std::string(test_public_key));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(test_k1_recovery) try {
   auto payload = "Test Cases";
   auto digest = sha256::hash(payload, const_strlen(payload));
   auto key = private_key::generate<ecc::private_key_shim>();
   auto pub = key.get_public_key();
   auto sig = key.sign(digest);

   auto recovered_pub = public_key(sig, digest);
   std::cout << recovered_pub << std::endl;

   BOOST_CHECK_EQUAL(std::string(recovered_pub), std::string(pub));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(test_r1_recovery) try {
   auto payload = "Test Cases";
   auto digest = sha256::hash(payload, const_strlen(payload));
   auto key = private_key::generate<r1::private_key_shim>();
   auto pub = key.get_public_key();
   auto sig = key.sign(digest);

   auto recovered_pub = public_key(sig, digest);
   std::cout << recovered_pub << std::endl;

   BOOST_CHECK_EQUAL(std::string(recovered_pub), std::string(pub));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(test_k1_recyle) try {
   auto key = private_key::generate<ecc::private_key_shim>();
   auto pub = key.get_public_key();
   auto pub_str = std::string(pub);
   auto recycled_pub = public_key(pub_str);

   std::cout << pub << " -> " << recycled_pub << std::endl;

   BOOST_CHECK_EQUAL(std::string(pub), std::string(recycled_pub));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(test_r1_recyle) try {
   auto key = private_key::generate<r1::private_key_shim>();
   auto pub = key.get_public_key();
   auto pub_str = std::string(pub);
   auto recycled_pub = public_key(pub_str);

   std::cout << pub << " -> " << recycled_pub << std::endl;

   BOOST_CHECK_EQUAL(std::string(pub), std::string(recycled_pub));
} FC_LOG_AND_RETHROW();


BOOST_AUTO_TEST_SUITE_END()