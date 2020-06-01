#define BOOST_TEST_MODULE io_json
#include <boost/test/included/unit_test.hpp>

#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

using namespace fc;

BOOST_AUTO_TEST_SUITE(json_test_suite)

namespace json_test_util {
   constexpr size_t exception_limit_size = 250;

   const json::yield_function_t yield_deadline_exception_at_start = [](std::ostream& os) {
      FC_CHECK_DEADLINE(fc::now() - fc::milliseconds(1));
   };

   const json::yield_function_t yield_deadline_exception_in_mid = [](std::ostream& os) {
      if (os.tellp() >= exception_limit_size) {
         throw fc::timeout_exception(fc::exception_code::timeout_exception_code, "timeout_exception", "execution timed out" );
      }
   };

   const json::yield_function_t yield_length_exception = [](std::ostream& os) {
      FC_ASSERT( os.tellp() <= exception_limit_size );
   };

   const json::yield_function_t yield_no_limitation = [](std::ostream& os) {
      // no limitation
   };

   const auto time_except_verf_func = [](const fc::timeout_exception& ex) -> bool {
      return (static_cast<int>(ex.code_value) == static_cast<int>(fc::exception_code::timeout_exception_code));
   };

   const auto length_limit_except_verf_func = [](const fc::assert_exception& ex) -> bool {
      return (static_cast<int>(ex.code_value) == static_cast<int>(fc::exception_code::assert_exception_code));
   };

   constexpr size_t repeat_char_num = 512;
   const std::string repeat_chars(repeat_char_num, 'a');
   const string escape_input_str = "\\b\\f\\n\\r\\t-\\-\\\\-\\x0\\x01\\x02\\x03\\x04\\x05\\x06\\x07\\x08\\x09\\x0a\\x0b\\x0c\\x0d\\x0e\\x0f"  \
                                   "\\x10\\x11\\x12\\x13\\x14\\x15\\x16\\x17\\x18\\x19\\x1a\\x1b\\x1c\\x1d\\x1e\\x1f-" + repeat_chars;

   const auto multiple_num = [](const size_t num, const size_t base) -> size_t {
      return (num / base + ((num % base == 0) ? 0 : 1)) * base;
   };

}  // namespace json_test_util

BOOST_AUTO_TEST_CASE(to_stream_test)
{
   {
      // to_stream( ostream& out, const fc::string&, const yield_function_t& yield );
      std::stringstream escape_out_str_ss;
      fc::escape_string(json_test_util::escape_input_str, escape_out_str_ss, json_test_util::yield_no_limitation);
      const auto size_different = escape_out_str_ss.str().size() - json_test_util::escape_input_str.size();
      const auto expected_out_str_size = json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count) + size_different - 1;
      BOOST_CHECK_LT(json_test_util::repeat_char_num, json_test_util::escape_input_str.size());
      BOOST_CHECK_LT(json_test_util::escape_input_str.size() - json_test_util::repeat_char_num, json_test_util::exception_limit_size);  // by using size_different to calculate expected string
      {
         std::stringstream deadline_exception_at_start_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_at_start_ss, json_test_util::escape_input_str, json_test_util::yield_deadline_exception_at_start),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_ss.str(), "\"");
      }
      {
         std::stringstream deadline_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_in_mid_ss, json_test_util::escape_input_str, json_test_util::yield_deadline_exception_in_mid),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_in_mid_ss.str(), escape_out_str_ss.str().substr(0, expected_out_str_size));
      }
      {
         std::stringstream length_except_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream(length_except_mid_ss, json_test_util::escape_input_str, json_test_util::yield_length_exception),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(length_except_mid_ss.str(), escape_out_str_ss.str().substr(0, expected_out_str_size));
      }
   }
   {
      // to_stream( ostream& out, const variant& v, const fc::time_point& deadline, const uint64_t max_len = max_length_limit, output_formatting format = stringify_large_ints_and_doubles );
      {
         variant v(json_test_util::escape_input_str);
         std::stringstream deadline_exception_at_start_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_at_start_ss, v, fc::time_point(), json::output_formatting::stringify_large_ints_and_doubles, json::max_length_limit),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_ss.str().empty(), true);
      }
      {
         constexpr size_t max_len = 10;
         variant v(json_test_util::repeat_chars);
         std::stringstream length_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( length_exception_in_mid_ss, v, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, max_len),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(length_exception_in_mid_ss.str(),
                           "\"" + json_test_util::repeat_chars.substr(0, json_test_util::multiple_num(max_len, json::escape_string_yeild_check_count)));
      }
      {
         variant v(json_test_util::repeat_chars);
         std::stringstream length_exception_in_mid_ss;
         BOOST_CHECK_NO_THROW(json::to_stream( length_exception_in_mid_ss, v, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, json::max_length_limit));
         BOOST_CHECK_EQUAL(length_exception_in_mid_ss.str(), "\"" + json_test_util::repeat_chars + "\"");
      }
   }
   {
      // to_stream( ostream& out, const variant& v, const yield_function_t& yield, output_formatting format = stringify_large_ints_and_doubles );
      const variant v(json_test_util::repeat_chars);
      BOOST_CHECK_LT(json_test_util::exception_limit_size, json_test_util::repeat_chars.size());
      {
         std::stringstream deadline_exception_at_start_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_at_start_ss, v, json_test_util::yield_deadline_exception_at_start),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_ss.str().empty(), true);
      }
      {
         std::stringstream deadline_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_in_mid_ss, v, json_test_util::yield_deadline_exception_in_mid),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_in_mid_ss.str(),
                           "\"" + json_test_util::repeat_chars.substr(0, json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count)));
      }
      {
         std::stringstream length_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( length_exception_in_mid_ss, v, json_test_util::yield_length_exception),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(length_exception_in_mid_ss.str(),
                           "\"" + json_test_util::repeat_chars.substr(0, json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count)));
      }
      {
         std::stringstream no_exception_ss;
         BOOST_CHECK_NO_THROW(json::to_stream( no_exception_ss, v, json_test_util::yield_no_limitation));
         BOOST_CHECK_EQUAL(no_exception_ss.str(), "\"" + json_test_util::repeat_chars + "\"");
      }
   }
   {
      // to_stream( ostream& out, const variants& v, const yield_function_t& yield, output_formatting format = stringify_large_ints_and_doubles );
      const std::string a_list(json_test_util::repeat_char_num, 'a');
      const std::string b_list(json_test_util::repeat_char_num, 'b');
      const variants variant_list{variant(a_list), variant(b_list)};
      BOOST_CHECK_LT(json_test_util::exception_limit_size, a_list.size() + b_list.size());
      {
         std::stringstream deadline_exception_at_start_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_at_start_ss, variant_list, json_test_util::yield_deadline_exception_at_start),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_ss.str().empty(), true);
      }
      {
         std::stringstream deadline_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_in_mid_ss, variant_list, json_test_util::yield_deadline_exception_in_mid),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_in_mid_ss.str(),
                           "[\"" + a_list.substr(0, json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count)));
      }
      {
         std::stringstream length_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( length_exception_in_mid_ss, variant_list, json_test_util::yield_length_exception),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(length_exception_in_mid_ss.str(),
                           "[\"" + a_list.substr(0, json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count)));
      }
      {
         std::stringstream no_exception_ss;
         BOOST_CHECK_NO_THROW(json::to_stream( no_exception_ss, variant_list, json_test_util::yield_no_limitation));
         BOOST_CHECK_EQUAL(no_exception_ss.str(), "[\"" + a_list + "\",\"" + b_list + "\"]");
      }
   }
   {
      // to_stream( ostream& out, const variant_object& v, const yield_function_t& yield, output_formatting format = stringify_large_ints_and_doubles );
      const std::string a_list(json_test_util::repeat_char_num, 'a');
      const std::string b_list(json_test_util::repeat_char_num, 'b');
      const variant_object vo(mutable_variant_object()("a", a_list)("b", b_list));
      BOOST_CHECK_LT(json_test_util::exception_limit_size, a_list.size() + b_list.size());
      {
         std::stringstream deadline_exception_at_start_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_at_start_ss, vo, json_test_util::yield_deadline_exception_at_start),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_ss.str().empty(), true);
      }
      {
         std::stringstream deadline_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( deadline_exception_in_mid_ss, vo, json_test_util::yield_deadline_exception_in_mid),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_in_mid_ss.str(),
                           "{\"a\":\"" + a_list.substr(0, json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count)));
      }
      {
         std::stringstream length_exception_in_mid_ss;
         BOOST_CHECK_EXCEPTION(json::to_stream( length_exception_in_mid_ss, vo, json_test_util::yield_length_exception),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(length_exception_in_mid_ss.str(),
                           "{\"a\":\"" + a_list.substr(0, json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count)));
      }
      {
         std::stringstream no_exception_ss;
         BOOST_CHECK_NO_THROW(json::to_stream( no_exception_ss, vo, json_test_util::yield_no_limitation));
         BOOST_CHECK_EQUAL(no_exception_ss.str(), "{\"a\":\"" + a_list + "\",\"b\":\"" + b_list + "\"}");
      }
   }
}

BOOST_AUTO_TEST_CASE(to_string_test)
{
   {  // to_string( const variant& v, const fc::time_point& deadline, const uint64_t max_len = max_length_limit, output_formatting format = stringify_large_ints_and_doubles);
      {
         variant v(json_test_util::escape_input_str);
         std::string deadline_exception_at_start_str;
         BOOST_CHECK_EXCEPTION(deadline_exception_at_start_str = json::to_string( v, fc::time_point(), json::output_formatting::stringify_large_ints_and_doubles, json::max_length_limit),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
      }
      {
         constexpr size_t max_len = 10;
         variant v(json_test_util::repeat_chars);
         std::string deadline_exception_at_start_str;
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
         BOOST_CHECK_EXCEPTION(deadline_exception_at_start_str = json::to_string( v, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, max_len),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
      }
      {
         variant v(json_test_util::repeat_chars);
         std::string length_exception_in_mid_str;
         BOOST_CHECK_NO_THROW(length_exception_in_mid_str = json::to_string( v, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, json::max_length_limit));
         BOOST_CHECK_EQUAL(length_exception_in_mid_str, "\"" + json_test_util::repeat_chars + "\"");
      }
   }
   {  // to_string( const variant& v, const yield_function_t& yield, output_formatting format = stringify_large_ints_and_doubles);
      const variant v(json_test_util::repeat_chars);
      BOOST_CHECK_LT(json_test_util::exception_limit_size, json_test_util::repeat_chars.size());
      {
         std::string deadline_exception_at_start_str;
         BOOST_CHECK_EXCEPTION(deadline_exception_at_start_str = json::to_string( v, json_test_util::yield_deadline_exception_at_start),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
      }
      {
         std::string deadline_exception_in_mid_str;
         BOOST_CHECK_EXCEPTION(deadline_exception_in_mid_str = json::to_string( v, json_test_util::yield_deadline_exception_in_mid),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_in_mid_str.empty(), true);
      }
      {
         std::string length_exception_in_mid_str;
         BOOST_CHECK_EXCEPTION(length_exception_in_mid_str = json::to_string( v, json_test_util::yield_length_exception),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(length_exception_in_mid_str.empty(), true);
      }
      {
         std::string no_exception_str;
         BOOST_CHECK_NO_THROW(no_exception_str = json::to_string( v, json_test_util::yield_no_limitation));
         BOOST_CHECK_EQUAL(no_exception_str, "\"" + json_test_util::repeat_chars + "\"");
      }
   }
   { // to_string template call
      const uint16_t id = 1000;
      const uint64_t len = 3;
      const std::string id_ret_1 = json::to_string( id, fc::time_point::max());
      BOOST_CHECK_EQUAL(std::to_string(id), id_ret_1);

      // exceed length
      std::string id_ret_2;
      BOOST_REQUIRE_THROW(id_ret_2 = json::to_string( id, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, len), fc::assert_exception);
      BOOST_CHECK_EQUAL(id_ret_2.empty(), true);

      // time_out
      std::string id_ret_3;
      BOOST_REQUIRE_THROW(id_ret_3 = json::to_string( id, fc::now() - fc::milliseconds(1) ), fc::timeout_exception);
      BOOST_CHECK_EQUAL(id_ret_3.empty(), true);
   }
}

BOOST_AUTO_TEST_CASE(to_pretty_string_test)
{
   {  // to_pretty_string( const variant& v, const fc::time_point& deadline, const uint64_t max_len = max_length_limit, output_formatting format = stringify_large_ints_and_doubles );
      {
         variant v(json_test_util::escape_input_str);
         std::string deadline_exception_at_start_str;
         BOOST_CHECK_EXCEPTION(deadline_exception_at_start_str = json::to_pretty_string( v, fc::time_point(), json::output_formatting::stringify_large_ints_and_doubles, json::max_length_limit),
         fc::timeout_exception,
         json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
      }
      {
         constexpr size_t max_len = 10;
         variant v(json_test_util::repeat_chars);
         std::string deadline_exception_at_start_str;
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
         BOOST_CHECK_EXCEPTION(deadline_exception_at_start_str = json::to_pretty_string( v, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, max_len),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
      }
      {
         variant v(json_test_util::repeat_chars);
         std::string length_exception_in_mid_str;
         BOOST_CHECK_NO_THROW(length_exception_in_mid_str = json::to_pretty_string( v, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, json::max_length_limit));
         BOOST_CHECK_EQUAL(length_exception_in_mid_str, "\"" + json_test_util::repeat_chars + "\"");
      }
   }
   {  // to_pretty_string( const variant& v, const yield_function_t& yield, output_formatting format = stringify_large_ints_and_doubles );
      const variant v(json_test_util::repeat_chars);
      BOOST_CHECK_LT(json_test_util::exception_limit_size, json_test_util::repeat_chars.size());
      {
         std::string deadline_exception_at_start_str;
         BOOST_CHECK_EXCEPTION(deadline_exception_at_start_str = json::to_pretty_string( v, json_test_util::yield_deadline_exception_at_start),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_at_start_str.empty(), true);
      }
      {
         std::string deadline_exception_in_mid_str;
         BOOST_CHECK_EXCEPTION(deadline_exception_in_mid_str = json::to_pretty_string( v, json_test_util::yield_deadline_exception_in_mid),
                               fc::timeout_exception,
                               json_test_util::time_except_verf_func);
         BOOST_CHECK_EQUAL(deadline_exception_in_mid_str.empty(), true);
      }
      {
         std::string length_exception_in_mid_str;
         BOOST_CHECK_EXCEPTION(length_exception_in_mid_str = json::to_pretty_string( v, json_test_util::yield_length_exception),
                               fc::assert_exception,
                               json_test_util::length_limit_except_verf_func);
         BOOST_CHECK_EQUAL(length_exception_in_mid_str.empty(), true);
      }
      {
         std::string no_exception_str;
         BOOST_CHECK_NO_THROW(no_exception_str = json::to_pretty_string( v, json_test_util::yield_no_limitation));
         BOOST_CHECK_EQUAL(no_exception_str, "\"" + json_test_util::repeat_chars + "\"");
      }
   }
   { // to_pretty_string template call
      const uint16_t id = 1000;
      const uint64_t len = 3;
      const std::string id_ret_1 = json::to_pretty_string( id, fc::time_point::max());
      BOOST_CHECK_EQUAL(std::to_string(id), id_ret_1);

      // exceed length
      std::string id_ret_2;
      BOOST_REQUIRE_THROW(id_ret_2 = json::to_pretty_string( id, fc::time_point::max(), json::output_formatting::stringify_large_ints_and_doubles, len), fc::assert_exception);
      BOOST_CHECK_EQUAL(id_ret_2.empty(), true);

      // time_out
      std::string id_ret_3;
      BOOST_REQUIRE_THROW(id_ret_3 = json::to_pretty_string( id, fc::now() - fc::milliseconds(1) ), fc::timeout_exception);
      BOOST_CHECK_EQUAL(id_ret_3.empty(), true);
   }
}

BOOST_AUTO_TEST_CASE(escape_string_test)
{
   std::stringstream escape_out_str_ss;
   fc::escape_string(json_test_util::escape_input_str, escape_out_str_ss, json_test_util::yield_no_limitation);
   const auto size_different = escape_out_str_ss.str().size() - json_test_util::escape_input_str.size();
   const auto expected_out_str_size = json_test_util::multiple_num(json_test_util::exception_limit_size, json::escape_string_yeild_check_count) + size_different - 1;
   BOOST_CHECK_LT(json_test_util::repeat_char_num, json_test_util::escape_input_str.size());
   BOOST_CHECK_LT(json_test_util::escape_input_str.size() - json_test_util::repeat_char_num, json_test_util::exception_limit_size);  // by using size_different to calculate expected string
   {
      // simulate exceed time exception at the beginning of processing
      std::stringstream escape_time_except_begin_ss;
      BOOST_CHECK_EXCEPTION(escape_string(json_test_util::escape_input_str, escape_time_except_begin_ss, json_test_util::yield_deadline_exception_at_start),
                            fc::timeout_exception,
                            json_test_util::time_except_verf_func);
      BOOST_CHECK_EQUAL(escape_time_except_begin_ss.str(), "\"");
   }
   {
      // simulate exceed time exception in the middle of processing
      std::stringstream escape_time_except_mid_ss;
      BOOST_CHECK_EXCEPTION(escape_string(json_test_util::escape_input_str, escape_time_except_mid_ss, json_test_util::yield_deadline_exception_in_mid),
                            fc::timeout_exception,
                            json_test_util::time_except_verf_func);
      BOOST_CHECK_EQUAL(escape_time_except_mid_ss.str(), escape_out_str_ss.str().substr(0, expected_out_str_size));
   }
   {
      // length limitation exception in the middle of processing
      std::stringstream length_except_mid_ss;
      BOOST_CHECK_EXCEPTION(escape_string(json_test_util::escape_input_str, length_except_mid_ss, json_test_util::yield_length_exception),
                            fc::assert_exception,
                            json_test_util::length_limit_except_verf_func);
      BOOST_CHECK_EQUAL(length_except_mid_ss.str(), escape_out_str_ss.str().substr(0, expected_out_str_size));
   }
}

BOOST_AUTO_TEST_SUITE_END()
