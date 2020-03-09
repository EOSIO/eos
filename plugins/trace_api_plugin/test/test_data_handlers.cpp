#define BOOST_TEST_MODULE trace_data_handlers
#include <boost/test/included/unit_test.hpp>

#include <eosio/trace_api/base64_data_handler.hpp>
#include <eosio/trace_api/abi_data_handler.hpp>

#include <eosio/trace_api/test_common.hpp>

using namespace eosio;
using namespace eosio::trace_api;
using namespace eosio::trace_api::test_common;

BOOST_AUTO_TEST_SUITE(abi_data_handler_tests)
   BOOST_AUTO_TEST_CASE(empty_data)
   {
      auto action = action_trace_v0 {
         0, "alice"_n, "alice"_n, "foo"_n, {}, {}
      };
      abi_data_handler handler;

      auto expected = fc::variant();
      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(no_abi)
   {
      auto action = action_trace_v0 {
         0, "alice"_n, "alice"_n, "foo"_n, {}, {0x00, 0x01, 0x02, 0x03}
      };
      abi_data_handler handler;

      auto expected = fc::variant();
      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(basic_abi)
   {
      auto action = action_trace_v0 {
            0, "alice"_n, "alice"_n, "foo"_n, {}, {0x00, 0x01, 0x02, 0x03}
      };

      auto abi = chain::abi_def ( {},
         {
            { "foo", "", { {"a", "varuint32"}, {"b", "varuint32"}, {"c", "varuint32"}, {"d", "varuint32"} } }
         },
         {
            { "foo"_n, "foo", ""}
         },
         {}, {}, {}
      );
      abi.version = "eosio::abi/1.";
      
      abi_data_handler handler;
      handler.add_abi("alice"_n, abi);

      fc::variant expected = fc::mutable_variant_object()
         ("a", 0)
         ("b", 1)
         ("c", 2)
         ("d", 3);

      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(basic_abi_wrong_type)
   {
      auto action = action_trace_v0 {
            0, "alice"_n, "alice"_n, "foo"_n, {}, {0x00, 0x01, 0x02, 0x03}
      };

      auto abi = chain::abi_def ( {},
         {
            { "foo", "", { {"a", "varuint32"}, {"b", "varuint32"}, {"c", "varuint32"}, {"d", "varuint32"} } }
         },
         {
            { "bar"_n, "foo", ""}
         },
         {}, {}, {}
      );
      abi.version = "eosio::abi/1.";

      abi_data_handler handler;
      handler.add_abi("alice"_n, abi);

      auto expected = fc::variant();

      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(basic_abi_insufficient_data)
   {
      auto action = action_trace_v0 {
            0, "alice"_n, "alice"_n, "foo"_n, {}, {0x00, 0x01, 0x02}
      };

      auto abi = chain::abi_def ( {},
         {
            { "foo", "", { {"a", "varuint32"}, {"b", "varuint32"}, {"c", "varuint32"}, {"d", "varuint32"} } }
         },
         {
            { "foo"_n, "foo", ""}
         },
         {}, {}, {}
      );
      abi.version = "eosio::abi/1.";

      bool log_called = false;
      abi_data_handler handler([&log_called](const exception_with_context& ){log_called = true;});
      handler.add_abi("alice"_n, abi);

      auto expected = fc::variant();

      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
      BOOST_TEST(log_called);
   }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(base64_data_handler_tests)
   BOOST_AUTO_TEST_CASE(empty_data)
   {
      auto action = action_trace_v0 {
         0, "alice"_n, "alice"_n, "foo"_n, {}, {}
      };
      base64_data_handler handler;

      auto expected = fc::variant("");
      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(aligned_data)
   {
      auto action = action_trace_v0 {
         0, "alice"_n, "alice"_n, "foo"_n, {}, { 0x00, 0x01, 0x02 }
      };
      base64_data_handler handler;

      auto expected = fc::variant("AAEC");
      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(single_padded_data)
   {
      auto action = action_trace_v0 {
            0, "alice"_n, "alice"_n, "foo"_n, {}, { 0x00, 0x01 }
      };
      base64_data_handler handler;

      auto expected = fc::variant("AAE=");
      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(double_padded_data)
   {
      auto action = action_trace_v0 {
            0, "alice"_n, "alice"_n, "foo"_n, {}, { 0x00 }
      };
      base64_data_handler handler;

      auto expected = fc::variant("AA==");
      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

   BOOST_AUTO_TEST_CASE(special_chars_data)
   {
      auto action = action_trace_v0 {
            0, "alice"_n, "alice"_n, "foo"_n, {}, { static_cast<char>(0xFF), static_cast<char>(0xE0) }
      };
      base64_data_handler handler;

      auto expected = fc::variant("/+A=");
      auto actual = handler.process_data(action);

      BOOST_TEST(to_kv(expected) == to_kv(actual), boost::test_tools::per_element());
   }

BOOST_AUTO_TEST_SUITE_END()

