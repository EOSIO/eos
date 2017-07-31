#include <eos/types/TypeParser.hpp>

#include <boost/test/unit_test.hpp>

namespace eos { namespace types {

BOOST_AUTO_TEST_SUITE(types_tests)


/// Put the SimpleSymbolTable through its paces
BOOST_AUTO_TEST_CASE(basic_simple_symbol_table_test)
{ try {
      SimpleSymbolTable table;

      BOOST_CHECK_THROW(table.getType("Struct"), type_exception);
      BOOST_CHECK_THROW(table.getType("Foo"), unknown_type_exception);

      BOOST_CHECK_THROW(table.addType({"test type", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Testtype[]", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Testtype]", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Testtype[", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Test[]type", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Test]type", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Test[type", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Test!type", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"!Testtype", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"!testtype", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Testtype!", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"_Testtype", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Testtype_", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"Test_type", "", {{"f1", "String"}}}), invalid_type_name_exception);
      table.addType({"Testtype", "", {{"f1", "String"}}});
      table.addType({"Testtype1", "", {{"f1", "String"}}});
      table.addType({"Test2type", "", {{"f1", "String"}}});
      BOOST_CHECK_THROW(table.addType({"Testtype", "", {{"f1", "String"}}}), duplicate_type_exception);

      BOOST_CHECK_THROW(table.addTypeDef("Testtype", "Testtype"), duplicate_type_exception);
      BOOST_CHECK_THROW(table.addTypeDef("Testtype5", "NewType"), unknown_type_exception);
      table.addTypeDef("Testtype", "TestType");
      BOOST_CHECK_THROW(table.addTypeDef("Testtype", "TestType"), duplicate_type_exception);
      BOOST_CHECK_THROW(table.addTypeDef("Testtype", "NewType[]"), type_exception);

      BOOST_CHECK(table.isKnownType("Testtype"));
      BOOST_CHECK(table.isKnownType("Testtype1"));
      BOOST_CHECK(table.isKnownType("Test2type"));
      BOOST_CHECK(table.isKnownType("TestType"));
      BOOST_CHECK(table.isKnownType("Testtype[]"));
      BOOST_CHECK(!table.isKnownType("Testtype5"));
      BOOST_CHECK(!table.isKnownType("NewType"));
      BOOST_CHECK(!table.isKnownType("NewType[]"));

      BOOST_CHECK_THROW(table.addType({"Foo", "Bar", {{"f1", "String"}}}), unknown_type_exception);
      BOOST_CHECK_THROW(table.addType({"Foo", "Foo", {{"f1", "String"}}}), unknown_type_exception);
      BOOST_CHECK_THROW(table.addType({"Foo", "", {{"f1", "Bar"}}}), unknown_type_exception);
      BOOST_CHECK_THROW(table.addType({"Foo", "", {{"f1", "Foo"}}}), unknown_type_exception);
      table.addType({"Foo", "", {{"f1", "String"}}});
      table.addType({"FooBar", "Foo", {{"f2", "Int32"}}});
      BOOST_CHECK(table.isKnownType("Foo"));
      BOOST_CHECK(table.isKnownType("FooBar"));
      BOOST_CHECK(!table.isKnownType("Bar"));

      BOOST_CHECK_THROW(table.addType({"Bar", "", {{"F1", "Struct"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.addType({"Bar", "", {{"f!", "Struct"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.addType({"Bar", "", {{"fg ", "Struct"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.addType({"Bar", "", {{"f g", "Struct"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.addType({"Bar", "", {{" fg", "Struct"}}}), invalid_field_name_exception);
      BOOST_CHECK_THROW(table.addType({"Bar", "", {{"0fg", "Struct"}}}), invalid_field_name_exception);
      table.addType({"Bar", "", {{"f0g", "Struct"}}});
      BOOST_CHECK(table.isKnownType("Bar"));
} FC_LOG_AND_RETHROW() }

/// Check that SimpleSymbolTable can handle a basic schema parse
BOOST_AUTO_TEST_CASE(simple_symbol_table_parse_test)
{ try {
      SimpleSymbolTable table;
      std::string schema = R"x(
struct Foo
   bar String
   baz Int8
struct Bar
   foo Int32
   baz Signature
typedef Bar Baz
typedef Baz Qux
)x";
      std::istringstream schema_string(schema);
      table.parse(schema_string);

      BOOST_CHECK(table.isKnownType("Foo"));
      BOOST_CHECK(table.isKnownType("Bar"));
      BOOST_CHECK(table.isKnownType("Baz"));

      Struct foo = {"Foo", "", {{"bar", "String"}, {"baz", "Int8"}}};
      Struct bar = {"Bar", "", {{"foo", "Int32"}, {"baz", "Signature"}}};
      BOOST_CHECK(table.getType("Foo") == foo);
      BOOST_CHECK(table.getType("Bar") == bar);
      BOOST_CHECK_EQUAL(table.resolveTypedef("Baz"), "Bar");
      BOOST_CHECK_EQUAL(table.resolveTypedef("Qux"), "Bar");
      BOOST_CHECK(table.getType("Baz") == bar);
      BOOST_CHECK(table.getType("Qux") == bar);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

}} // namespace eos::types
