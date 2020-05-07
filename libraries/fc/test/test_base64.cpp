#define BOOST_TEST_MODULE base64
#include <boost/test/included/unit_test.hpp>

#include <fc/crypto/base64.hpp>
#include <fc/exception/exception.hpp>

using namespace fc;
using namespace std::literals;

BOOST_AUTO_TEST_SUITE(base64)

BOOST_AUTO_TEST_CASE(base64enc) try {
   auto input = "abc123$&()'?\xb4\xf5\x01\xfa~a"s;
   auto expected_output = "YWJjMTIzJCYoKSc/tPUB+n5h"s;

   BOOST_CHECK_EQUAL(expected_output, base64_encode(input));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(base64urlenc) try {
   auto input = "abc123$&()'?\xb4\xf5\x01\xfa~a"s;
   auto expected_output = "YWJjMTIzJCYoKSc_tPUB-n5h"s;

   BOOST_CHECK_EQUAL(expected_output, base64url_encode(input));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(base64dec) try {
   auto input = "YWJjMTIzJCYoKSc/tPUB+n5h"s;
   auto expected_output = "abc123$&()'?\xb4\xf5\x01\xfa~a"s;

   BOOST_CHECK_EQUAL(expected_output, base64_decode(input));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(base64urldec) try {
   auto input = "YWJjMTIzJCYoKSc_tPUB-n5h"s;
   auto expected_output = "abc123$&()'?\xb4\xf5\x01\xfa~a"s;

   BOOST_CHECK_EQUAL(expected_output, base64url_decode(input));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(base64dec_extraequals) try {
   auto input = "YWJjMTIzJCYoKSc/tPUB+n5h========="s;
   auto expected_output = "abc123$&()'?\xb4\xf5\x01\xfa~a"s;

   BOOST_CHECK_EQUAL(expected_output, base64_decode(input));
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(base64dec_bad_stuff) try {
   auto input = "YWJjMTIzJCYoKSc/tPU$B+n5h="s;

   BOOST_CHECK_EXCEPTION(base64_decode(input), fc::exception, [](const fc::exception& e) {
      return e.to_detail_string().find("encountered non-base64 character") != std::string::npos;
   });
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_SUITE_END()