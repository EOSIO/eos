#define BOOST_TEST_MODULE variant
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>
#include <fc/exception/exception.hpp>
#include <fc/crypto/base64.hpp>
#include <string>

using namespace fc;

BOOST_AUTO_TEST_SUITE(variant_test_suite)
BOOST_AUTO_TEST_CASE(mutable_variant_object_test)
{
  // no BOOST_CHECK / BOOST_REQUIRE, just see that this compiles on all supported platforms
  try {
    variant v(42);
    variant_object vo;
    mutable_variant_object mvo;
    variants vs;
    vs.push_back(mutable_variant_object("level", "debug")("color", v));
    vs.push_back(mutable_variant_object()("level", "debug")("color", v));
    vs.push_back(mutable_variant_object("level", "debug")("color", "green"));
    vs.push_back(mutable_variant_object()("level", "debug")("color", "green"));
    vs.push_back(mutable_variant_object("level", "debug")(vo));
    vs.push_back(mutable_variant_object()("level", "debug")(mvo));
    vs.push_back(mutable_variant_object("level", "debug").set("color", v));
    vs.push_back(mutable_variant_object()("level", "debug").set("color", v));
    vs.push_back(mutable_variant_object("level", "debug").set("color", "green"));
    vs.push_back(mutable_variant_object()("level", "debug").set("color", "green"));
  }
  FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE(variant_format_string_limited)
{
   constexpr size_t long_rep_char_num = 1024;
   const std::string a_long_list = std::string(long_rep_char_num, 'a');
   const std::string b_long_list = std::string(long_rep_char_num, 'b');
   {
      const string format = "${a} ${b} ${c}";
      fc::mutable_variant_object mu;
      mu( "a", string( long_rep_char_num, 'a' ) );
      mu( "b", string( long_rep_char_num, 'b' ) );
      mu( "c", string( long_rep_char_num, 'c' ) );
      const string result = fc::format_string( format, mu, true );
      BOOST_CHECK_LT(0, mu.size());
      const auto arg_limit_size = (1024 - format.size()) / mu.size();
      BOOST_CHECK_EQUAL( result, string(arg_limit_size, 'a' ) + "... " + string(arg_limit_size, 'b' ) + "... " + string(arg_limit_size, 'c' ) + "..." );
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());
   }
   {  // verify object, array, blob, and string, all exceeds limits, fold display for each
      fc::mutable_variant_object mu;
      mu( "str", a_long_list );
      mu( "obj", variant_object(mutable_variant_object()("a", a_long_list)("b", b_long_list)) );
      mu( "arr", variants{variant(a_long_list), variant(b_long_list)} );
      mu( "blob", blob({std::vector<char>(a_long_list.begin(), a_long_list.end())}) );
      const string format_prefix = "Format string test: ";
      const string format_str = format_prefix + "${str} ${obj} ${arr} {blob}";
      const string result = fc::format_string( format_str, mu, true );
      BOOST_CHECK_LT(0, mu.size());
      const auto arg_limit_size = (1024 - format_str.size()) / mu.size();
      BOOST_CHECK_EQUAL( result, format_prefix + a_long_list.substr(0, arg_limit_size) + "..." + " ${obj} ${arr} {blob}");
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());
   }
   {  // verify object, array can be displayed properly
      const string format_prefix = "Format string test: ";
      const string format_str = format_prefix + "${str} ${obj} ${arr} ${blob} ${var}";
      BOOST_CHECK_LT(format_str.size(), 1024);
      const size_t short_rep_char_num = (1024 - format_str.size()) / 5 - 1;
      const std::string a_short_list = std::string(short_rep_char_num, 'a');
      const std::string b_short_list = std::string(short_rep_char_num / 3, 'b');
      const std::string c_short_list = std::string(short_rep_char_num / 3, 'c');
      const std::string d_short_list = std::string(short_rep_char_num / 3, 'd');
      const std::string e_short_list = std::string(short_rep_char_num / 3, 'e');
      const std::string f_short_list = std::string(short_rep_char_num, 'f');
      const std::string g_short_list = std::string(short_rep_char_num, 'g');
      fc::mutable_variant_object mu;
      const variant_object vo(mutable_variant_object()("b", b_short_list)("c", c_short_list));
      const variants variant_list{variant(d_short_list), variant(e_short_list)};
      const blob a_blob({std::vector<char>(f_short_list.begin(), f_short_list.end())});
      const variant a_variant(g_short_list);
      mu( "str",  a_short_list );
      mu( "obj",  vo);
      mu( "arr",  variant_list);
      mu( "blob", a_blob);
      mu( "var",  a_variant);
      const string result = fc::format_string( format_str, mu, true );
      const string target_result = format_prefix + a_short_list + " " +
                                   "{" + "\"b\":\"" + b_short_list + "\",\"c\":\"" + c_short_list + "\"}" + " " +
                                   "[\"" + d_short_list + "\",\"" + e_short_list + "\"]" + " " +
                                   base64_encode( a_blob.data.data(), a_blob.data.size() ) + "=" + " " +
                                   g_short_list;

      BOOST_CHECK_EQUAL( result, target_result);
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());
   }
}
BOOST_AUTO_TEST_SUITE_END()
