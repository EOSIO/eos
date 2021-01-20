#define BOOST_TEST_MODULE static_variant
#include <boost/test/included/unit_test.hpp>
#include <fc/exception/exception.hpp>
#include <fc/static_variant.hpp>
#include <fc/variant.hpp>

BOOST_AUTO_TEST_SUITE(static_variant_test_suite)
   BOOST_AUTO_TEST_CASE(to_from_fc_variant)
   {
      using variant_type = std::variant<int32_t, bool>;
      auto std_variant_1 = variant_type{false};
      auto fc_variant = fc::variant{};

      fc::to_variant(std_variant_1, fc_variant);

      auto std_variant_2 = variant_type{};
      fc::from_variant(fc_variant, std_variant_2);

      BOOST_REQUIRE(std_variant_1 == std_variant_2);
   }

   BOOST_AUTO_TEST_CASE(get)
   {
     using variant_type = std::variant<int32_t, bool, std::string>;

     auto v1 = variant_type{std::string{"hello world"}};
     BOOST_CHECK_EXCEPTION(std::get<int32_t>(v1), std::bad_variant_access, [](const auto& e) { return true; });
     auto result1 = std::get<std::string>(v1);
     BOOST_REQUIRE(result1 == std::string{"hello world"});

     const auto v2 = variant_type{std::string{"hello world"}};
     BOOST_CHECK_EXCEPTION(std::get<int32_t>(v2), std::bad_variant_access, [](const auto& e) { return true; });
     const auto result2 = std::get<std::string>(v2);
     BOOST_REQUIRE(result2 == std::string{"hello world"});
   }

   BOOST_AUTO_TEST_CASE(static_variant_from_index)
   {
      using variant_type = std::variant<int32_t, bool, std::string>;
      auto v = variant_type{};

      BOOST_CHECK_EXCEPTION(fc::from_index(v, 3), fc::assert_exception, [](const auto& e) { return e.code() == fc::assert_exception_code; });

      fc::from_index(v, 2);
      BOOST_REQUIRE(std::string{} == std::get<std::string>(v));
   }

   BOOST_AUTO_TEST_CASE(static_variant_get_index)
   {
      using variant_type = std::variant<int32_t, bool, std::string>;
      BOOST_REQUIRE((fc::get_index<variant_type, int32_t>() == 0));
      BOOST_REQUIRE((fc::get_index<variant_type, bool>() == 1));
      BOOST_REQUIRE((fc::get_index<variant_type, std::string>() == 2));
      BOOST_REQUIRE((fc::get_index<variant_type, double>() == std::variant_size_v<variant_type>)); // Isn't a type contained in variant.
   }
BOOST_AUTO_TEST_SUITE_END()
