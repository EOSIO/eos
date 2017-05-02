#include <eos/types/TypeParser.hpp>

#include <boost/test/unit_test.hpp>

namespace eos { namespace types {

BOOST_AUTO_TEST_SUITE(types_tests)

/// Put the SimpleSymbolTable through its paces
BOOST_AUTO_TEST_CASE(basic_simple_symbol_table_test)
{ try {
      SimpleSymbolTable table;

      BOOST_CHECK_THROW(table.addType({"testtype", "", {{"f1", "String"}}}), invalid_type_name_exception);
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
      BOOST_CHECK_THROW(table.addType({"0Testtype", "", {{"f1", "String"}}}), invalid_type_name_exception);
      BOOST_CHECK_THROW(table.addType({"0testtype", "", {{"f1", "String"}}}), invalid_type_name_exception);
      table.addType({"Testtype", "", {{"f1", "String"}}});
      table.addType({"Testtype1", "", {{"f1", "String"}}});
      table.addType({"Test2type", "", {{"f1", "String"}}});
      BOOST_CHECK_THROW(table.addType({"Testtype", "", {{"f1", "String"}}}), duplicate_type_exception);

      BOOST_CHECK_THROW(table.addTypeDef("Testtype", "Testtype"), duplicate_type_exception);
      BOOST_CHECK_THROW(table.addTypeDef("Testtype5", "NewType"), unknown_type_exception);
      table.addTypeDef("Testtype", "TestType");
      BOOST_CHECK_THROW(table.addTypeDef("Testtype", "TestType"), duplicate_type_exception);
      table.addTypeDef("Testtype[]", "TestTypeArray");
      BOOST_CHECK_THROW(table.addTypeDef("Testtype", "NewType[]"), invalid_type_name_exception);

      BOOST_CHECK(table.isKnownType("Testtype"));
      BOOST_CHECK(table.isKnownType("Testtype1"));
      BOOST_CHECK(table.isKnownType("Test2type"));
      BOOST_CHECK(table.isKnownType("TestType"));
      BOOST_CHECK(table.isKnownType("Testtype[]"));
      BOOST_CHECK(table.isKnownType("TestTypeArray"));
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

      BOOST_CHECK_THROW(table.addType({"Bar", "", {{"f1", "Struct"}}}), unknown_type_exception);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

}} // namespace eos::types
