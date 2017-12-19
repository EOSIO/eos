/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/types/type_parser.hpp>

#include <boost/test/unit_test.hpp>

namespace eosio { namespace types {

BOOST_AUTO_TEST_SUITE(types_tests)


/// Put the SimpleSymbolTable through its paces
BOOST_AUTO_TEST_CASE(basic_simple_symbol_table_test)
{ try {
      simple_symbol_table table;

      BOOST_CHECK_THROW(table.get_type("struct_t"), type_exception);
      BOOST_CHECK_THROW(table.get_type("Foo"), unknown_type_exception);

      BOOST_CHECK_THROW(table.add_type({"test type", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Testtype[]", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Testtype]", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Testtype[", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Test[]type", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Test]type", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Test[type", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Test!type", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"!Testtype", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"!testtype", "", {{"f1", "string"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Testtype!", "", {{"f1", "string"}}}), invalid_type_name_exception);
      table.add_type({"Testtype", "", {{"f1", "string"}}});
      table.add_type({"Testtype1", "", {{"f1", "string"}}});
      table.add_type({"Test2type", "", {{"f1", "string"}}});
      table.add_type({"test_type", "", {{"f1", "string"}}});
      table.add_type({"_test_type", "", {{"f1", "string"}}});
      table.add_type({"test_type_", "", {{"f1", "string"}}});
      BOOST_CHECK_THROW(table.add_type({"Testtype", "", {{"f1", "string"}}}), duplicate_type_exception);

      BOOST_CHECK_THROW(table.add_type_def("Testtype", "Testtype"), duplicate_type_exception);
      BOOST_CHECK_THROW(table.add_type_def("Testtype5", "NewType"), unknown_type_exception);
            table.add_type_def("Testtype", "TestType");
      BOOST_CHECK_THROW(table.add_type_def("Testtype", "TestType"), duplicate_type_exception);
      BOOST_CHECK_THROW(table.add_type_def("Testtype", "NewType[]"), type_exception);

      BOOST_CHECK(table.is_known_type("Testtype"));
      BOOST_CHECK(table.is_known_type("Testtype1"));
      BOOST_CHECK(table.is_known_type("Test2type"));
      BOOST_CHECK(table.is_known_type("TestType"));
      BOOST_CHECK(table.is_known_type("Testtype[]"));
      BOOST_CHECK(!table.is_known_type("Testtype5"));
      BOOST_CHECK(!table.is_known_type("NewType"));
      BOOST_CHECK(!table.is_known_type("NewType[]"));

      BOOST_CHECK_THROW(table.add_type({"Foo", "Bar", {{"f1", "string"}}}), unknown_type_exception);
      BOOST_CHECK_THROW(table.add_type({"Foo", "Foo", {{"f1", "string"}}}), unknown_type_exception);
      BOOST_CHECK_THROW(table.add_type({"Foo", "", {{"f1", "Bar"}}}), unknown_type_exception);
      BOOST_CHECK_THROW(table.add_type({"Foo", "", {{"f1", "Foo"}}}), unknown_type_exception);
      table.add_type({"Foo", "", {{"f1", "string"}}});
      table.add_type({"FooBar", "Foo", {{"f2", "int32"}}});
      BOOST_CHECK(table.is_known_type("Foo"));
      BOOST_CHECK(table.is_known_type("FooBar"));
      BOOST_CHECK(!table.is_known_type("Bar"));

      BOOST_CHECK_THROW(table.add_type({"Bar", "", {{"F1", "struct_t"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Bar", "", {{"f!", "struct_t"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Bar", "", {{"fg ", "struct_t"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Bar", "", {{"f g", "struct_t"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Bar", "", {{" fg", "struct_t"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.add_type({"Bar", "", {{"0fg", "struct_t"}}}), invalid_field_name_exception);
      table.add_type({"Bar", "", {{"f0g", "struct_t"}}});
      BOOST_CHECK(table.is_known_type("Bar"));
} FC_LOG_AND_RETHROW() }

/// Check that SimpleSymbolTable can handle a basic schema parse
BOOST_AUTO_TEST_CASE(simple_symbol_table_parse_test)
{ try {
      simple_symbol_table table;
      std::string schema = R"x(
struct foo
   bar string
   baz int8
struct Bar
   foo int32
   baz signature
typedef Bar Baz
typedef Baz Qux
)x";
      std::istringstream schema_string(schema);
      table.parse(schema_string);

      BOOST_CHECK(table.is_known_type("foo"));
      BOOST_CHECK(table.is_known_type("Bar"));
      BOOST_CHECK(table.is_known_type("Baz"));

      struct_t foo = {"foo", "", {{"bar", "string"}, {"baz", "int8"}}};
      struct_t bar = {"Bar", "", {{"foo", "int32"}, {"baz", "signature"}}};
      BOOST_CHECK(table.get_type("foo") == foo);
      BOOST_CHECK(table.get_type("Bar") == bar);
      BOOST_CHECK_EQUAL(table.resolve_type_def("Baz"), "Bar");
      BOOST_CHECK_EQUAL(table.resolve_type_def("Qux"), "Bar");
      BOOST_CHECK(table.get_type("Baz") == bar);
      BOOST_CHECK(table.get_type("Qux") == bar);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

}} // namespace eosio::types
