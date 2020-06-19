#define BOOST_TEST_MODULE static_variant
#include <boost/test/included/unit_test.hpp>

#include <fc/static_variant.hpp>
#include <fc/exception/exception.hpp>

using namespace fc;

template<bool Eq = false, bool Neq = false, bool Lt = false, bool Lte = false, bool Gt= false, bool Gte = false>
struct comparable_type {

   int value;

   template<bool Enabled = Eq, typename = std::enable_if_t<Enabled>>
   bool operator== (const comparable_type& other ) const {
      return value == other.value;
   }

   template<bool Enabled = Neq, typename = std::enable_if_t<Enabled>>
   bool operator!= (const comparable_type& other ) const {
      return value != other.value;
   }

   template<bool Enabled = Lt, typename = std::enable_if_t<Enabled>>
   bool operator< (const comparable_type& other ) const {
      return value < other.value;
   }

   template<bool Enabled = Lte, typename = std::enable_if_t<Enabled>>
   bool operator<= (const comparable_type& other ) const {
      return value <= other.value;
   }

   template<bool Enabled = Gt, typename = std::enable_if_t<Enabled>>
   bool operator> (const comparable_type& other ) const {
      return value > other.value;
   }

   template<bool Enabled = Gte, typename = std::enable_if_t<Enabled>>
   bool operator>= (const comparable_type& other ) const {
      return value >= other.value;
   }
};

template<template<typename> class Op, typename Variant>
std::is_convertible<std::invoke_result_t<Op<void>, const Variant&, const Variant&>, bool> variant_has_op_test(int);

template<template<typename> class Op, typename Variant>
std::false_type variant_has_op_test(...);

template<template<typename> class Op, typename Variant>
constexpr bool variant_has_op_v = decltype(variant_has_op_test<Op, Variant>(0))::value;

using not_comparable = comparable_type<>;
using full_comparable = comparable_type<true, true, true, true, true, true>;
using eq_only  = comparable_type<true>;
using neq_only = comparable_type<false, true>;
using lt_only  = comparable_type<false, false, true>;
using lte_only = comparable_type<false, false, false, true>;
using gt_only  = comparable_type<false, false, false, false, true>;
using gte_only = comparable_type<false, false, false, false, false, true>;

BOOST_AUTO_TEST_SUITE(static_variant_test_suite)
   BOOST_AUTO_TEST_CASE(test_eq)
   {
      // ensure that the given comparisons are present IFF the types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::equal_to, static_variant<not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::equal_to, static_variant<eq_only> > ));

      // ensure that given comparisons are present IFF the ALL types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::equal_to, static_variant<eq_only, not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::equal_to, static_variant<eq_only, full_comparable> > ));

      // ensure the operator returns expected values
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) == static_variant<full_comparable>(full_comparable{1})  ));
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) == static_variant<full_comparable>(full_comparable{2})) ));

      // ensure the operator respects binary equivalent but different types
      BOOST_REQUIRE(( !(static_variant<full_comparable, eq_only>(full_comparable{1}) == static_variant<full_comparable, eq_only>(eq_only{1})) ));
   }

   BOOST_AUTO_TEST_CASE(test_neq)
   {
      // ensure that the given comparisons are present IFF the types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::not_equal_to, static_variant<not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::not_equal_to, static_variant<neq_only> > ));

      // ensure that given comparisons are present IFF the ALL types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::not_equal_to, static_variant<neq_only, not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::not_equal_to, static_variant<neq_only, full_comparable> > ));

      // ensure the operator returns expected values
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) != static_variant<full_comparable>(full_comparable{2})  ));
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) != static_variant<full_comparable>(full_comparable{1})) ));

      // ensure the operator respects binary equivalent but different types
      BOOST_REQUIRE(( static_variant<full_comparable, neq_only>(full_comparable{1}) != static_variant<full_comparable, neq_only>(neq_only{1}) ));
   }

   BOOST_AUTO_TEST_CASE(test_lt)
   {
      // ensure that the given comparisons are present IFF the types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::less, static_variant<not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::less, static_variant<lt_only> > ));

      // ensure that given comparisons are present IFF the ALL types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::less, static_variant<lt_only, not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::less, static_variant<lt_only, full_comparable> > ));

      // ensure the operator returns expected values
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) < static_variant<full_comparable>(full_comparable{2})  ));
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) < static_variant<full_comparable>(full_comparable{1})) ));
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) < static_variant<full_comparable>(full_comparable{0})) ));

      // ensure the operator respects binary equivalent but different types
      BOOST_REQUIRE((   static_variant<full_comparable, lt_only>(full_comparable{1}) < static_variant<full_comparable, lt_only>(lt_only{1})          ));
      BOOST_REQUIRE(( !(static_variant<full_comparable, lt_only>(full_comparable{1}) < static_variant<full_comparable, lt_only>(full_comparable{1})) ));
      BOOST_REQUIRE(( !(static_variant<full_comparable, lt_only>(lt_only{1})         < static_variant<full_comparable, lt_only>(full_comparable{1})) ));
   }

   BOOST_AUTO_TEST_CASE(test_lte)
   {
      // ensure that the given comparisons are present IFF the types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::less_equal, static_variant<not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::less_equal, static_variant<lte_only> > ));

      // ensure that given comparisons are present IFF the ALL types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::less_equal, static_variant<lte_only, not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::less_equal, static_variant<lte_only, full_comparable> > ));

      // ensure the operator returns expected values
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) <= static_variant<full_comparable>(full_comparable{2})  ));
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) <= static_variant<full_comparable>(full_comparable{1})  ));
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) <= static_variant<full_comparable>(full_comparable{0})) ));

      // ensure the operator respects binary equivalent but different types
      BOOST_REQUIRE((   static_variant<full_comparable, lte_only>(full_comparable{1}) <= static_variant<full_comparable, lte_only>(lte_only{1})         ));
      BOOST_REQUIRE((   static_variant<full_comparable, lte_only>(full_comparable{1}) <= static_variant<full_comparable, lte_only>(full_comparable{1})  ));
      BOOST_REQUIRE(( !(static_variant<full_comparable, lte_only>(lte_only{1})        <= static_variant<full_comparable, lte_only>(full_comparable{1})) ));
   }

   BOOST_AUTO_TEST_CASE(test_gt)
   {
      // ensure that the given comparisons are present IFF the types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::greater, static_variant<not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::greater, static_variant<gt_only> > ));

      // ensure that given comparisons are present IFF the ALL types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::greater, static_variant<gt_only, not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::greater, static_variant<gt_only, full_comparable> > ));

      // ensure the operator returns expected values
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) > static_variant<full_comparable>(full_comparable{2})) ));
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) > static_variant<full_comparable>(full_comparable{1})) ));
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) > static_variant<full_comparable>(full_comparable{0})  ));

      // ensure the operator respects binary equivalent but different types
      BOOST_REQUIRE(( !(static_variant<full_comparable, gt_only>(full_comparable{1}) > static_variant<full_comparable, gt_only>(gt_only{1}))         ));
      BOOST_REQUIRE(( !(static_variant<full_comparable, gt_only>(full_comparable{1}) > static_variant<full_comparable, gt_only>(full_comparable{1})) ));
      BOOST_REQUIRE((   static_variant<full_comparable, gt_only>(gt_only{1})         > static_variant<full_comparable, gt_only>(full_comparable{1})  ));
   }

   BOOST_AUTO_TEST_CASE(test_gte)
   {
      // ensure that the given comparisons are present IFF the types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::greater_equal, static_variant<not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::greater_equal, static_variant<gte_only> > ));

      // ensure that given comparisons are present IFF the ALL types support it
      BOOST_REQUIRE(( !variant_has_op_v<std::greater_equal, static_variant<gte_only, not_comparable> > ));
      BOOST_REQUIRE(( variant_has_op_v<std::greater_equal, static_variant<gte_only, full_comparable> > ));

      // ensure the operator returns expected values
      BOOST_REQUIRE(( !(static_variant<full_comparable>(full_comparable{1}) >= static_variant<full_comparable>(full_comparable{2})) ));
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) >= static_variant<full_comparable>(full_comparable{1})  ));
      BOOST_REQUIRE((   static_variant<full_comparable>(full_comparable{1}) >= static_variant<full_comparable>(full_comparable{0})  ));

      // ensure the operator respects binary equivalent but different types
      BOOST_REQUIRE(( !(static_variant<full_comparable, gte_only>(full_comparable{1}) >= static_variant<full_comparable, gte_only>(gte_only{1}))        ));
      BOOST_REQUIRE((   static_variant<full_comparable, gte_only>(full_comparable{1}) >= static_variant<full_comparable, gte_only>(full_comparable{1})  ));
      BOOST_REQUIRE((   static_variant<full_comparable, gte_only>(gte_only{1})        >= static_variant<full_comparable, gte_only>(full_comparable{1})  ));
   }

   enum class selected_op {
      STRING_LVALUE_REF,
      STRING_RVALUE_REF,
      STRING_CONST_REF,
      VECTOR_LVALUE_REF,
      VECTOR_RVALUE_REF,
      VECTOR_CONST_REF
   };

   struct test_visitor_type {
      selected_op operator()(string&) const { return selected_op::STRING_LVALUE_REF; }
      selected_op operator()(string&&) const { return selected_op::STRING_RVALUE_REF; }
      selected_op operator()(const string&) const { return selected_op::STRING_CONST_REF; }
      
      selected_op operator()(std::vector<char>&) const { return selected_op::VECTOR_LVALUE_REF; }
      selected_op operator()(std::vector<char>&&) const { return selected_op::VECTOR_RVALUE_REF; }
      selected_op operator()(const std::vector<char>&) const { return selected_op::VECTOR_CONST_REF; }
   };

   BOOST_AUTO_TEST_CASE(test_visitor) 
   {
      test_visitor_type visitor;
      using var_type = static_variant<string, std::vector<char>>;
      var_type string_lvalue = string("abcd");
      const var_type string_const_value = string("abcd");

      BOOST_CHECK(string_lvalue.visit(visitor) == selected_op::STRING_LVALUE_REF);
      BOOST_CHECK(var_type(string("abcd")).visit(visitor) == selected_op::STRING_RVALUE_REF);
      BOOST_CHECK(string_const_value.visit(visitor) == selected_op::STRING_CONST_REF);

      var_type vector_lvalue = std::vector<char>{};
      const var_type vector_const_value = std::vector<char>{};

      BOOST_CHECK(vector_lvalue.visit(visitor) == selected_op::VECTOR_LVALUE_REF);
      BOOST_CHECK(var_type(std::vector<char>{}).visit(visitor) == selected_op::VECTOR_LVALUE_REF);
      BOOST_CHECK(vector_const_value.visit(visitor) == selected_op::VECTOR_CONST_REF);
   }

   BOOST_AUTO_TEST_CASE(test_get) {
      test_visitor_type visitor;

      using var_type = static_variant<string, std::vector<char>>;
      var_type lvalue(string("abc"));
      const var_type const_value(string("abc"));

      BOOST_CHECK(visitor(lvalue.get<string>()) == selected_op::STRING_LVALUE_REF);
      BOOST_CHECK(visitor(const_value.get<string>()) == selected_op::STRING_CONST_REF);
      BOOST_CHECK(visitor(var_type(string("abc")).get<string>()) == selected_op::STRING_RVALUE_REF);
   }

BOOST_AUTO_TEST_SUITE_END()
