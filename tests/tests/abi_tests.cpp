/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/types/abi_serializer.hpp>
#include <eos/abi_generator/abi_generator.hpp>

#include "../common/database_fixture.hpp"

using namespace eosio;
using namespace chain;
using namespace eosio::types;

BOOST_AUTO_TEST_SUITE(abi_tests)

fc::variant verify_round_trip_conversion( const abi_serializer& abis, const type_name& type, const fc::variant& var )
{
   auto bytes = abis.variant_to_binary(type, var);

   auto var2 = abis.binary_to_variant(type, bytes);

   std::string r = fc::json::to_string(var2);

   //std::cout << r << std::endl;

   auto bytes2 = abis.variant_to_binary(type, var2);

   BOOST_CHECK_EQUAL( fc::to_hex(bytes), fc::to_hex(bytes2) );

   return var2;
}

const char* my_abi = R"=====(
{
  "types": [],
  "structs": [{
      "name"  : "A",
      "base"  : "PublicKeyTypes",
      "fields": {}
    },
    {
      "name": "PublicKeyTypes",
      "base" : "AssetTypes",
      "fields": {
        "publickey"      : "public_key",
        "publickey_arr"  : "public_key[]",
      }
    },{
      "name": "AssetTypes",
      "base" : "NativeTypes",
      "fields": {
        "asset"       : "asset",
        "asset_arr"   : "asset[]",
        "price"       : "price",
        "price_arr"   : "price[]",
      }
    },{
      "name": "NativeTypes",
      "base" : "GeneratedTypes",
      "fields" : {
        "string"            : "string",
        "string_arr"        : "string[]",
        "time"              : "time",
        "time_arr"          : "time[]",
        "signature"         : "signature",
        "signature_arr"     : "signature[]",
        "checksum"          : "checksum",
        "checksum_arr"      : "checksum[]",
        "fieldname"         : "field_name",
        "fieldname_arr"     : "field_name[]",
        "fixedstring32"     : "fixed_string32",
        "fixedstring32_ar"  : "fixed_string32[]",
        "fixedstring16"     : "fixed_string16",
        "fixedstring16_ar"  : "fixed_string16[]",
        "typename"          : "type_name",
        "typename_arr"      : "type_name[]",
        "bytes"             : "bytes",
        "bytes_arr"         : "bytes[]",
        "uint8"             : "uint8",
        "uint8_arr"         : "uint8[]",
        "uint16"            : "uint16",
        "uint16_arr"        : "uint16[]",
        "uint32"            : "uint32",
        "uint32_arr"        : "uint32[]",
        "uint64"            : "uint64",
        "uint64_arr"        : "uint64[]",
        "uint128"           : "uint128",
        "uint128_arr"       : "uint128[]",
        "uint256"           : "uint256",
        "uint256_arr"       : "uint256[]",
        "int8"              : "int8",
        "int8_arr"          : "int8[]",
        "int16"             : "int16",
        "int16_arr"         : "int16[]",
        "int32"             : "int32",
        "int32_arr"         : "int32[]",
        "int64"             : "int64",
        "int64_arr"         : "int64[]",
        "name"              : "name",
        "name_arr"          : "name[]",
        "field"             : "field",
        "field_arr"         : "field[]",
        "struct"            : "struct_t",
        "struct_arr"        : "struct_t[]",
        "fields"            : "fields",
        "fields_arr"        : "fields[]"

      }
    },{
      "name"   : "GeneratedTypes",
      "fields" : {
        "accountname":"account_name",
        "accountname_arr":"account_name[]",
        "permname":"permission_name",
        "permname_arr":"permission_name[]",
        "funcname":"func_name",
        "funcname_arr":"func_name[]",
        "messagename":"message_name",
        "messagename_arr":"message_name[]",
        "apermission":"account_permission",
        "apermission_arr":"account_permission[]",
        "message":"message",
        "message_arr":"message[]",
        "apweight":"account_permission_weight",
        "apweight_arr":"account_permission_weight[]",
        "transaction":"transaction",
        "transaction_arr":"transaction[]",
        "strx":"signed_transaction",
        "strx_arr":"signed_transaction[]",
        "kpweight":"key_permission_weight",
        "kpweight_arr":"key_permission_weight[]",
        "authority":"authority",
        "authority_arr":"authority[]",
        "blkcconfig":"blockchain_configuration",
        "blkcconfig_arr":"blockchain_configuration[]",
        "typedef":"type_def",
        "typedef_arr":"type_def[]",
        "action":"action",
        "action_arr":"action[]",
        "table":"table",
        "table_arr":"table[]",
        "abi":"abi",
        "abi_arr":"abi[]"
      }
    }
  ],
  "actions": [],
  "tables": []
}
)=====";

BOOST_FIXTURE_TEST_CASE(uint_types, testing_fixture)
{ try {
   
   const char* currency_abi = R"=====(
   {
       "types": [],
       "structs": [{
       "name": "transfer",
           "base": "",
           "fields": {
             "amount64": "uint64",
             "amount32": "uint32",
             "amount16": "uint16",
             "amount8" : "uint8"
           }
         }
       ],
       "actions": [],
       "tables": []
   }
   )=====";

   auto abi = fc::json::from_string(currency_abi).as<types::abi>();

   abi_serializer abis(abi);
   abis.validate();

   const char* test_data = R"=====(
   {
     "amount64" : 64,
     "amount32" : 32,
     "amount16" : 16,
     "amount8"  : 8,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   //std::cout << "var type =>" << var.get_type() << std::endl;
   
   auto bytes = abis.variant_to_binary("transfer", var);

   auto var2 = abis.binary_to_variant("transfer", bytes);
   
   std::string r = fc::json::to_string(var2);

   //std::cout << r << std::endl;
   
   auto bytes2 = abis.variant_to_binary("transfer", var2);

   BOOST_CHECK_EQUAL( fc::to_hex(bytes), fc::to_hex(bytes2) );

} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE(generator, testing_fixture)
{ try {

  if( std::getenv("EOSLIB") == nullptr ) {
    wlog("*************************************");
    wlog("*  EOSLIB env variable not defined  *");
    wlog("*  ABIGenerator tests will not run  *");
    wlog("*************************************");
    return;
  }

  auto is_abi_generation_exception =[](const eosio::abi_generation_exception& e) -> bool { return true; };

  auto generate_abi = [this](const char* source, const char* abi, bool opt_sfs=false) -> bool {
    
    const char* eoslib_path = std::getenv("EOSLIB");
    FC_ASSERT(eoslib_path != NULL);

    std::string include_param = std::string("-I") + eoslib_path;
    
    eosio::types::abi output;
    bool res = runToolOnCodeWithArgs(new generate_abi_action(false, opt_sfs, "", output), source, 
      {"-fparse-all-comments", "--std=c++14", "--target=wasm32", include_param});
    
    FC_ASSERT(res == true);
    abi_serializer(output).validate();

    auto abi1 = fc::json::from_string(abi).as<eosio::types::abi>();

    auto e = fc::to_hex(fc::raw::pack(abi1)) == fc::to_hex(fc::raw::pack(output));
    
    if(!e) {
      std::cout << "expected: " <<  std::endl << fc::json::to_pretty_string(abi1) << std::endl << std::endl; 
      std::cout << "generated: " <<  std::endl << fc::json::to_pretty_string(output) << std::endl << std::endl; 
    }

    return e;
  };

   const char* unknown_type = R"=====(
   #include <eoslib/types.h>
   //@abi action
   struct Transfer {
      uint64_t param1;
      char*    param2;
   };
   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(unknown_type, ""), eosio::abi_generation_exception, is_abi_generation_exception );
   
   const char* all_types = R"=====(
    #include <eoslib/types.hpp>
    #include <eoslib/string.hpp>

    typedef int field;
    typedef int struct_t;
    typedef int fields;
    typedef int message_name;
    typedef int account_permission;
    typedef int message;
    typedef int account_permission_weight;
    typedef int transaction;
    typedef int signed_transaction;
    typedef int key_permission_weight;
    typedef int authority;
    typedef int blockchain_configuration;
    typedef int type_def;
    typedef int action;
    typedef int table;
    typedef int abi;
   
   //@abi action
   struct test_struct {
      eosio::string           field1;
      time                    field2;
      signature               field3;
      checksum                field4;
      field_name              field5;
      fixed_string32          field6;
      fixed_string16          field7;
      type_name               field8;
      bytes                   field9;
      uint8_t                 field10;
      uint16_t                field11;
      uint32_t                field12;
      uint64_t                field13;
      uint128_t               field14;
      int8_t                  field16;
      int16_t                 field17;
      int32_t                 field18;
      int64_t                 field19;
      eosio::name             field20;
      field                   field21;
      struct_t                field22;
      fields                  field23;
      account_name            field24;
      permission_name         field25;
      func_name               field26;
      message_name            field27;
      account_permission      field28;
      message                 field29;
      account_permission_weight field30;
      transaction               field31;
      signed_transaction        field32;
      key_permission_weight     field33;
      authority                 field34;
      blockchain_configuration  field35;
      type_def                  field36;
      action                    field37;
      table                     field38;
      abi                       field39;
   };
   )=====";

   const char* all_types_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "test_struct",
          "base" : "",
          "fields" : {
            "field1" : "string",
            "field2" : "time",
            "field3" : "signature",
            "field4" : "checksum",
            "field5" : "field_name",
            "field6" : "fixed_string32",
            "field7" : "fixed_string16",
            "field8" : "type_name",
            "field9" : "bytes",
            "field10" : "uint8",
            "field11" : "uint16",
            "field12" : "uint32",
            "field13" : "uint64",
            "field14" : "uint128",
            "field16" : "int8",
            "field17" : "int16",
            "field18" : "int32",
            "field19" : "int64",
            "field20" : "name",
            "field21" : "field",
            "field22" : "struct_t",
            "field23" : "fields",
            "field24" : "account_name",
            "field25" : "permission_name",
            "field26" : "func_name",
            "field27" : "message_name",
            "field28" : "account_permission",
            "field29" : "message",
            "field30" : "account_permission_weight",
            "field31" : "transaction",
            "field32" : "signed_transaction",
            "field33" : "key_permission_weight",
            "field34" : "authority",
            "field35" : "blockchain_configuration",
            "field36" : "type_def",
            "field37" : "action",
            "field38" : "table",
            "field39" : "abi"
          }
       }],
       "actions": [{
          "action_name" : "teststruct",
          "type" : "test_struct"
       }],
       "tables": []
   }
   )=====";
  BOOST_CHECK_EQUAL( generate_abi(all_types, all_types_abi), true );

   const char* double_base = R"=====(
   #include <eoslib/types.h>

   struct A {
      uint64_t param3;
   };
   struct B {
      uint64_t param2;
   };
   
   //@abi action
   struct C : A,B {
      uint64_t param1;
   };
   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(double_base, ""), eosio::abi_generation_exception, is_abi_generation_exception );

   const char* double_action = R"=====(
   #include <eoslib/types.h>

   struct A {
      uint64_t param3;
   };
   struct B : A {
      uint64_t param2;
   };
   
   //@abi action action1 action2
   struct C : B {
      uint64_t param1;
   };
   )=====";

   const char* double_action_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "A",
          "base" : "",
          "fields" : {
            "param3" : "uint64",
          }
       },{
          "name" : "B",
          "base" : "A",
          "fields" : {
            "param2" : "uint64",
          }
       },{
          "name" : "C",
          "base" : "B",
          "fields" : {
            "param1" : "uint64",
          }
       }],
       "actions": [{
          "action_name" : "action1",
          "type" : "C"
       },{
          "action_name" : "action2",
          "type" : "C"
       }],
       "tables": []
   }
   )=====";


   BOOST_CHECK_EQUAL( generate_abi(double_action, double_action_abi), true );

   const char* all_indexes = R"=====(
   #include <eoslib/types.hpp>
   #include <eoslib/string.hpp>

   using namespace eosio;

   //@abi table
   struct Table1 {
      uint64_t field1;
   };

   //@abi table
   struct Table2 {
      uint128_t field1;
      uint128_t field2;
   };

   //@abi table
   struct Table3 {
      uint64_t field1;
      uint64_t field2;
      uint64_t field3;
   };

   struct MyComplexValue {
      uint64_t    a;
      account_name b;
   };

   //@abi table
   struct Table4 {
      eosio::string key;
      MyComplexValue value;
   };

   )=====";

   const char* all_indexes_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "Table1",
          "base" : "",
          "fields" : {
            "field1" : "uint64",
          }
       },{
          "name" : "Table2",
          "base" : "",
          "fields" : {
            "field1" : "uint128",
            "field2" : "uint128",
          }
       },{
          "name" : "Table3",
          "base" : "",
          "fields" : {
            "field1" : "uint64",
            "field2" : "uint64",
            "field3" : "uint64",
          }
       },{
          "name" : "MyComplexValue",
          "base" : "",
          "fields" : {
            "a" : "uint64",
            "b" : "account_name",
          }
       },{
          "name" : "Table4",
          "base" : "",
          "fields" : {
            "key" : "string",
            "value" : "MyComplexValue",
          }
       }],
       "actions": [],
       "tables": [
        {
          "table_name": "table1",
          "type": "Table1",
          "index_type": "i64",
          "key_names": [
            "field1"
          ],
          "key_types": [
            "uint64"
          ]
        },{
          "table_name": "table2",
          "type": "Table2",
          "index_type": "i128i128",
          "key_names": [
            "field1",
            "field2"
          ],
          "key_types": [
            "uint128",
            "uint128"
          ]
        },{
          "table_name": "table3",
          "type": "Table3",
          "index_type": "i64i64i64",
          "key_names": [
            "field1",
            "field2",
            "field3"
          ],
          "key_types": [
            "uint64",
            "uint64",
            "uint64"
          ]
        },{
          "table_name": "table4",
          "type": "Table4",
          "index_type": "str",
          "key_names": [
            "key",
          ],
          "key_types": [
            "string",
          ]
        },

       ]
   }
   )=====";

   BOOST_CHECK_EQUAL( generate_abi(all_indexes, all_indexes_abi), true );

   const char* unable_to_determine_index = R"=====(
   #include <eoslib/types.h>
   
   //@abi table
   struct PACKED(Table1) {
      uint32_t field1;
      uint64_t field2;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(unable_to_determine_index, ""), eosio::abi_generation_exception, is_abi_generation_exception );

   //TODO: full action / full table

  // typedef fixed_string16 FieldName;
   const char* long_field_name = R"=====(
   #include <eoslib/types.h>
   
   //@abi table
   struct PACKED(Table1) {
      uint64_t thisisaverylongfieldname;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(long_field_name, ""), eosio::abi_generation_exception, is_abi_generation_exception );

   const char* long_type_name = R"=====(
   #include <eoslib/types.h>

   struct ThisIsAVeryVeryVeryVeryLongTypeName {
      uint64_t field;
   };

   //@abi table
   struct PACKED(Table1) {
      ThisIsAVeryVeryVeryVeryLongTypeName filed1;
   };

   )=====";


   BOOST_CHECK_EXCEPTION( generate_abi(long_type_name, "{}"), eosio::abi_generation_exception, is_abi_generation_exception );

   const char* same_type_different_namespace = R"=====(
   #include <eoslib/types.h>

   namespace A {
     //@abi table
     struct Table1 {
        uint64_t field1;
     };
   }

   namespace B {
     //@abi table
     struct Table1 {
        uint64_t field2;
     };
   }

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(same_type_different_namespace, ""), eosio::abi_generation_exception, is_abi_generation_exception );

   const char* bad_index_type = R"=====(
   #include <eoslib/types.h>

   //@abi table i64
   struct Table1 {
      uint32_t key;
      uint64_t field1;
      uint64_t field2;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(bad_index_type, ""), eosio::abi_generation_exception, is_abi_generation_exception );

   const char* full_table_decl = R"=====(
   #include <eoslib/types.hpp>

   //@abi table i64
   class Table1 {
   public:
      uint64_t  id;
      eosio::name name;
      uint32_t  age;
   };

   )=====";

   const char* full_table_decl_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "Table1",
          "base" : "",
          "fields" : {
            "id" : "uint64",
            "name" : "name",
            "age" : "uint32"
          }
       }],
       "actions": [],
       "tables": [
        {
          "table_name": "table1",
          "type": "Table1",
          "index_type": "i64",
          "key_names": [
            "id"
          ],
          "key_types": [
            "uint64"
          ]
        }]
   }
   )=====";

   BOOST_CHECK_EQUAL( generate_abi(full_table_decl, full_table_decl_abi), true );

   const char* union_table = R"=====(
   #include <eoslib/types.h>

   //@abi table
   union Table1 {
      uint64_t field1;
      uint32_t field2;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(union_table, ""), eosio::abi_generation_exception, is_abi_generation_exception );

   const char* same_action_different_type = R"=====(
   #include <eoslib/types.h>

   //@abi action action1
   struct Table1 {
      uint64_t field1;
   };

   //@abi action action1
   struct Table2 {
      uint64_t field1;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(same_action_different_type, ""), eosio::abi_generation_exception, is_abi_generation_exception );

   const char* template_base = R"=====(
   #include <eoslib/types.h>

   template<typename T>
   class Base {
      T field;
   };

   typedef Base<uint32_t> Base32;

   //@abi table i64
   class Table1 : Base32 {
   public:
      uint64_t id;
   };

   )=====";

   const char* template_base_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "Base32",
          "base" : "",
          "fields" : {
            "field" : "uint32",
          }
       },{
          "name" : "Table1",
          "base" : "Base32",
          "fields" : {
            "id" : "uint64",
          }
       }],
       "actions": [],
       "tables": [
        {
          "table_name": "table1",
          "type": "Table1",
          "index_type": "i64",
          "key_names": [
            "id"
          ],
          "key_types": [
            "uint64"
          ]
        }]
   }
   )=====";

   BOOST_CHECK_EQUAL( generate_abi(template_base, template_base_abi), true );

   const char* action_and_table = R"=====(
   #include <eoslib/types.h>

  /* @abi table
   * @abi action
   */
   class TableAction {
   public:
      uint64_t id;
   };

   )=====";

   const char* action_and_table_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "TableAction",
          "base" : "",
          "fields" : {
            "id" : "uint64",
          }
       }],
       "actions": [{
          "action_name" : "tableaction",
          "type" : "TableAction"
       }],
       "tables": [
        {
          "table_name": "tableaction",
          "type": "TableAction",
          "index_type": "i64",
          "key_names": [
            "id"
          ],
          "key_types": [
            "uint64"
          ]
        }]
   }
   )=====";

   BOOST_CHECK_EQUAL( generate_abi(action_and_table, action_and_table_abi), true );

   const char* simple_typedef = R"=====(
   #include <eoslib/types.hpp>

   using namespace eosio;

   struct CommonParams {
      uint64_t c1;
      uint64_t c2;
      uint64_t c3;
   };

   typedef CommonParams MyBaseAlias;

   //@abi action
   struct MainAction : MyBaseAlias {
      uint64_t param1;
   };

   )=====";

   const char* simple_typedef_abi = R"=====(
   {
       "types": [{
          "new_type_name" : "MyBaseAlias",
          "type" : "CommonParams"
       }],
       "structs": [{
          "name" : "CommonParams",
          "base" : "",
          "fields" : {
            "c1" : "uint64",
            "c2" : "uint64",
            "c3" : "uint64"
          }
       },{
          "name" : "MainAction",
          "base" : "MyBaseAlias",
          "fields" : {
            "param1" : "uint64"
          }
       }],
       "actions": [{
          "action_name" : "mainaction",
          "type" : "MainAction"
       }],
       "tables": []
   }
   )=====";

   BOOST_CHECK_EQUAL( generate_abi(simple_typedef, simple_typedef_abi), true );

   const char* field_typedef = R"=====(
   #include <eoslib/types.hpp>

   using namespace eosio;

   typedef name MyNameAlias;

   struct ComplexField {
      uint64_t  f1;
      uint32_t  f2; 
   };

   typedef ComplexField MyComplexFieldAlias;

   //@abi table
   struct PACKED(Table1) {
      uint64_t            field1;
      MyComplexFieldAlias field2;
      MyNameAlias         name;
   };

   )=====";

   const char* field_typedef_abi = R"=====(
   {
       "types": [{
          "new_type_name" : "MyComplexFieldAlias",
          "type" : "ComplexField"
       },{
          "new_type_name" : "MyNameAlias",
          "type" : "name"
       }],
       "structs": [{
          "name" : "ComplexField",
          "base" : "",
          "fields" : {
            "f1" : "uint64",
            "f2" : "uint32",
          }
       },{
          "name" : "Table1",
          "base" : "",
          "fields" : {
            "field1" : "uint64",
            "field2" : "MyComplexFieldAlias",
            "name"   : "MyNameAlias"
          }
       }],
       "actions": [],
       "tables": [{
          "table_name": "table1",
          "type": "Table1",
          "index_type": "i64",
          "key_names": [
            "field1"
          ],
          "key_types": [
            "uint64"
          ]
        }]
   }
   )=====";

   BOOST_CHECK_EQUAL( generate_abi(field_typedef, field_typedef_abi), true );


} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(general, testing_fixture)
{ try {

   auto abi = fc::json::from_string(my_abi).as<types::abi>();

   abi_serializer abis(abi);
   abis.validate();

   const char *my_other = R"=====(
    {
      "publickey"     :  "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "publickey_arr" :  ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"],
      "asset"         : "100.00 EOS",
      "asset_arr"     : ["100.00 EOS","100.00 EOS"],
      "price"         : { "base" : "100.00 EOS", "quote" : "200.00 BTC" },
      "price_arr"     : [{ "base" : "100.00 EOS", "quote" : "200.00 BTC" },{ "base" : "100.00 EOS", "quote" : "200.00 BTC" }],

      "string"            : "ola ke ase",
      "string_arr"        : ["ola ke ase","ola ke desi"],
      "time"              : "2021-12-20T15:30",
      "time_arr"          : ["2021-12-20T15:30","2021-12-20T15:31"],
      "signature"         : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00",
      "signature_arr"     : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00"],
      "checksum"          : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
      "checksum_arr"      : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"],
      "fieldname"         : "name1",
      "fieldname_arr"     : ["name1","name2"],
      "fixedstring32"     : "1234567890abcdef1234567890abcdef",
      "fixedstring32_ar"  : ["1234567890abcdef1234567890abcdef","1234567890abcdef1234567890abcdea"],
      "fixedstring16"     : "1234567890abcdef",
      "fixedstring16_ar"  : ["1234567890abcdef","1234567890abcdea"],
      "typename"          : "name3",
      "typename_arr"      : ["name4","name5"],
      "bytes"             : "010203",
      "bytes_arr"         : ["010203","","040506"],
      "uint8"             : 8,
      "uint8_arr"         : [8,9],
      "uint16"            : 16,
      "uint16_arr"        : [16,17],
      "uint32"            : 32,
      "uint32_arr"        : [32,33],
      "uint64"            : 64,
      "uint64_arr"        : [64,65],
      "uint128"           : "128",
      "uint128_arr"       : ["128","129"],
      "uint256"           : "256",
      "uint256_arr"       : ["256","257"],
      "int8"              : 108,
      "int8_arr"          : [108,109],
      "int16"             : 116,
      "int16_arr"         : [116,117],
      "int32"             : 132,
      "int32_arr"         : [132,133],
      "int64"             : 164,
      "int64_arr"         : [164,165],
      "name"              : "xname1",
      "name_arr"          : ["xname2","xname3"],
      "field"             : {"name1":"type1"},
      "field_arr"         : {"name2":"type2", "name3":"type3"},
      "struct"            : {"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} },
      "struct_arr"        : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2"}},{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2"}}],
      "fields"            : {"name1":"type1", "name2":"type2"},
      "fields_arr"        : [{"name1":"type1", "name2":"type2"},{"name3":"type3", "name4":"type4"}],
      "accountname"       : "thename",
      "accountname_arr"   : ["name1","name2"],
      "permname"          : "pername",
      "permname_arr"      : ["pername1","pername2"],
      "funcname"          : "funname",
      "funcname_arr"      : ["funname1","funnname2"],
      "messagename"       : "msg1",
      "messagename_arr"   : ["msg1","msg2"],
      "apermission" : {"account":"acc1","permission":"permname1"},
      "apermission_arr": [{"account":"acc1","permission":"permname1"},{"account":"acc2","permission":"permname2"}],
      "message"           : {"code":"a1b2", "type":"type1", "data":"445566"},
      "message_arr"       : [{"code":"a1b2", "type":"type1", "data":"445566"},{"code":"2233", "type":"type2", "data":""}],
      "apweight": {"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},
      "apweight_arr": [{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}],
      "transaction"       : { 
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}]
      },
      "transaction_arr": [
      { 
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}]
      },
      { 
        "ref_block_num":"2",
        "ref_block_prefix":"3",
        "expiration":"2021-12-20T15:40",
        "scope":["acc3","acc4"],
        "messages":[{"code":"3344", "type":"type2", "data":"778899"}]
      }],
      "strx": {
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}],
        "signatures" : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00"],
        "authorizations" : [{"account":"acc1","permission":"permname1"},{"account":"acc2","permission":"permname2"}]
      },
      "strx_arr": [{
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}],
        "signatures" : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00"],
        "authorizations" : [{"account":"acc1","permission":"permname1"},{"account":"acc2","permission":"permname2"}]
      },{
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}],
        "signatures" : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00"],
        "authorizations" : [{"account":"acc1","permission":"permname1"},{"account":"acc2","permission":"permname2"}]
      }],
      "kpweight": {"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},
      "kpweight_arr": [{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}],
      "authority": { 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
       },
      "authority_arr": [{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
       },{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
       }],
      "blkcconfig": {"max_blk_size": "100","target_blk_size" : "200", "max_storage_size":"300","elected_pay" : "400", "runner_up_pay" : "500", "min_eos_balance" : "600", "max_trx_lifetime"  : "700"},
      "blkcconfig_arr": [
        {"max_blk_size": "100","target_blk_size" : "200", "max_storage_size":"300","elected_pay" : "400", "runner_up_pay" : "500", "min_eos_balance" : "600", "max_trx_lifetime"  : "700"},
        {"max_blk_size": "100","target_blk_size" : "200", "max_storage_size":"300","elected_pay" : "400", "runner_up_pay" : "500", "min_eos_balance" : "600", "max_trx_lifetime"  : "700"}
      ],
      "typedef" : {"new_type_name":"new", "type":"old"},
      "typedef_arr": [{"new_type_name":"new", "type":"old"},{"new_type_name":"new", "type":"old"}],
      "action": {"action_name":"action1","type":"type1"},
      "action_arr": [{"action_name":"action1","type":"type1"},{"action_name":"action2","type":"type2"}],
      "table": {"table_name":"table1","type":"type1"},
      "table_arr": [{"table_name":"table1","type":"type1"},{"table_name":"table1","type":"type1"}],
      "abi":{
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
        "actions" : [{"action_name":"action1","type":"type1"}],
        "tables" : [{"table_name":"table1","type":"type1"}]
      },
      "abi_arr": [{
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
        "actions" : [{"action_name":"action1","type":"type1"}],
        "tables" : [{"table_name":"table1","type":"type1"}]
      },{
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
        "actions" : [{"action_name":"action1","type":"type1"}],
        "tables" : [{"table_name":"table1","type":"type1"}]
      }]
    }
   )=====";

   auto var = fc::json::from_string(my_other);

   auto bytes = abis.variant_to_binary("A", var);
   auto var2 = abis.binary_to_variant("A", bytes);
   std::string r = fc::json::to_string(var2);

   std::cout << r << std::endl;
   
   auto bytes2 = abis.variant_to_binary("A", var2);

   BOOST_CHECK_EQUAL( fc::to_hex(bytes), fc::to_hex(bytes2) );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abi_cycle, testing_fixture)
{ try {
  
   const char* typedef_cycle_abi = R"=====(
   {
       "types": [{
          "new_type_name": "A",
          "type": "name"
        },{
          "new_type_name": "name",
          "type": "A"
        }],
       "structs": [],
       "actions": [],
       "tables": []
   }
   )=====";

   const char* struct_cycle_abi = R"=====(
   {
       "types": [],
       "structs": [{
         "name": "A",
         "base": "B",
         "fields": {}
       },{
         "name": "B",
         "base": "C",
         "fields": {}
       },{
         "name": "C",
         "base": "A",
         "fields": {}
       }],
       "actions": [],
       "tables": []
   }
   )=====";

   auto abi = fc::json::from_string(typedef_cycle_abi).as<types::abi>();
   abi_serializer abis(abi);
   
   auto is_assert_exception = [](fc::assert_exception const & e) -> bool { std::cout << e.to_string() << std::endl; return true; };
   BOOST_CHECK_EXCEPTION( abis.validate(), fc::assert_exception, is_assert_exception );

   abi = fc::json::from_string(struct_cycle_abi).as<types::abi>();
   abis.set_abi(abi);
   BOOST_CHECK_EXCEPTION( abis.validate(), fc::assert_exception, is_assert_exception );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(transfer, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   const char* test_data = R"=====(
   {
     "from" : "from.acct",
     "to" : "to.acct",
     "amount" : 18446744073709551515,
     "memo" : "really important transfer"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto transfer = var.as<eosio::types::transfer>();
   BOOST_CHECK_EQUAL("from.acct", transfer.from);
   BOOST_CHECK_EQUAL("to.acct", transfer.to);
   BOOST_CHECK_EQUAL(18446744073709551515u, transfer.amount);
   BOOST_CHECK_EQUAL("really important transfer", transfer.memo);

   auto var2 = verify_round_trip_conversion( abis, "transfer", var );
   auto transfer2 = var2.as<eosio::types::transfer>();
   BOOST_CHECK_EQUAL(transfer.from, transfer2.from);
   BOOST_CHECK_EQUAL(transfer.to, transfer2.to);
   BOOST_CHECK_EQUAL(transfer.amount, transfer2.amount);
   BOOST_CHECK_EQUAL(transfer.memo, transfer2.memo);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(lock, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "from" : "from.acct",
     "to" : "to.acct",
     "amount" : -9223372036854775807,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto lock = var.as<eosio::types::lock>();
   BOOST_CHECK_EQUAL("from.acct", lock.from);
   BOOST_CHECK_EQUAL("to.acct", lock.to);
   BOOST_CHECK_EQUAL(-9223372036854775807, lock.amount);

   auto var2 = verify_round_trip_conversion( abis, "lock", var );
   auto lock2 = var2.as<eosio::types::lock>();
   BOOST_CHECK_EQUAL(lock.from, lock2.from);
   BOOST_CHECK_EQUAL(lock.to, lock2.to);
   BOOST_CHECK_EQUAL(lock.amount, lock2.amount);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(unlock, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "an.acct",
     "amount" : -9223372036854775807,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto unlock = var.as<eosio::types::unlock>();
   BOOST_CHECK_EQUAL("an.acct", unlock.account);
   BOOST_CHECK_EQUAL(-9223372036854775807, unlock.amount);

   auto var2 = verify_round_trip_conversion( abis, "unlock", var );
   auto unlock2 = var2.as<eosio::types::unlock>();
   BOOST_CHECK_EQUAL(unlock.account, unlock2.account);
   BOOST_CHECK_EQUAL(unlock.amount, unlock2.amount);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(claim, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "an.acct",
     "amount" : -9223372036854775807,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto claim = var.as<eosio::types::claim>();
   BOOST_CHECK_EQUAL("an.acct", claim.account);
   BOOST_CHECK_EQUAL(-9223372036854775807, claim.amount);

   auto var2 = verify_round_trip_conversion( abis, "claim", var );
   auto claim2 = var2.as<eosio::types::claim>();
   BOOST_CHECK_EQUAL(claim.account, claim2.account);
   BOOST_CHECK_EQUAL(claim.amount, claim2.amount);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(okproducer, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "voter" : "an.acct",
     "producer" : "an.acct2",
     "approve" : -128,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto okproducer = var.as<eosio::types::okproducer>();
   BOOST_CHECK_EQUAL("an.acct", okproducer.voter);
   BOOST_CHECK_EQUAL("an.acct2", okproducer.producer);
   BOOST_CHECK_EQUAL(-128, okproducer.approve);

   auto var2 = verify_round_trip_conversion( abis, "okproducer", var );
   auto okproducer2 = var2.as<eosio::types::okproducer>();
   BOOST_CHECK_EQUAL(okproducer.voter, okproducer2.voter);
   BOOST_CHECK_EQUAL(okproducer.producer, okproducer2.producer);
   BOOST_CHECK_EQUAL(okproducer.approve, okproducer2.approve);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(setproducer, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   const char* test_data = R"=====(
   {
     "name" : "acct.name",
     "key" : "EOS5PnYq6BZn7H9GvL68cCLjWUZThRemTJoJmybCn1iEpVUXLb5Az",
     "configuration" : {
        "max_blk_size" : 2147483135,
        "target_blk_size" : 2147483145,
        "max_storage_size" : 9223372036854775805,
        "elected_pay" : -9223372036854775807,
        "runner_up_pay" : -9223372036854775717,
        "min_eos_balance" : -9223372036854775707,
        "max_trx_lifetime" : 4294967071,
        "auth_depth_limit" : 32777,
        "max_trx_runtime" : 4294967007,
        "in_depth_limit" : 32770,
        "max_in_msg_size" : 4294966943,
        "max_gen_trx_size" : 4294966911
     }
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto setproducer = var.as<eosio::types::setproducer>();
   BOOST_CHECK_EQUAL("acct.name", setproducer.name);
   BOOST_CHECK_EQUAL("EOS5PnYq6BZn7H9GvL68cCLjWUZThRemTJoJmybCn1iEpVUXLb5Az", (std::string)setproducer.key);
   BOOST_CHECK_EQUAL(2147483135u, setproducer.configuration.max_blk_size);
   BOOST_CHECK_EQUAL(2147483145u, setproducer.configuration.target_blk_size);
   BOOST_CHECK_EQUAL(9223372036854775805u, setproducer.configuration.max_storage_size);
   BOOST_CHECK_EQUAL(-9223372036854775807, setproducer.configuration.elected_pay);
   BOOST_CHECK_EQUAL(-9223372036854775717, setproducer.configuration.runner_up_pay);
   BOOST_CHECK_EQUAL(-9223372036854775707, setproducer.configuration.min_eos_balance);
   BOOST_CHECK_EQUAL(4294967071u, setproducer.configuration.max_trx_lifetime);
   BOOST_CHECK_EQUAL(32777u, setproducer.configuration.auth_depth_limit);
   BOOST_CHECK_EQUAL(4294967007u, setproducer.configuration.max_trx_runtime);
   BOOST_CHECK_EQUAL(32770u, setproducer.configuration.in_depth_limit);
   BOOST_CHECK_EQUAL(4294966943u, setproducer.configuration.max_in_msg_size);
   BOOST_CHECK_EQUAL(4294966911u, setproducer.configuration.max_gen_trx_size);

   auto var2 = verify_round_trip_conversion( abis, "setproducer", var );
   auto setproducer2 = var2.as<eosio::types::setproducer>();
   BOOST_CHECK_EQUAL(setproducer.configuration.max_blk_size, setproducer2.configuration.max_blk_size);
   BOOST_CHECK_EQUAL(setproducer.configuration.target_blk_size, setproducer2.configuration.target_blk_size);
   BOOST_CHECK_EQUAL(setproducer.configuration.max_storage_size, setproducer2.configuration.max_storage_size);
   BOOST_CHECK_EQUAL(setproducer.configuration.elected_pay, setproducer2.configuration.elected_pay);
   BOOST_CHECK_EQUAL(setproducer.configuration.runner_up_pay, setproducer2.configuration.runner_up_pay);
   BOOST_CHECK_EQUAL(setproducer.configuration.min_eos_balance, setproducer2.configuration.min_eos_balance);
   BOOST_CHECK_EQUAL(setproducer.configuration.max_trx_lifetime, setproducer2.configuration.max_trx_lifetime);
   BOOST_CHECK_EQUAL(setproducer.configuration.auth_depth_limit, setproducer2.configuration.auth_depth_limit);
   BOOST_CHECK_EQUAL(setproducer.configuration.max_trx_runtime, setproducer2.configuration.max_trx_runtime);
   BOOST_CHECK_EQUAL(setproducer.configuration.in_depth_limit, setproducer2.configuration.in_depth_limit);
   BOOST_CHECK_EQUAL(setproducer.configuration.max_in_msg_size, setproducer2.configuration.max_in_msg_size);
   BOOST_CHECK_EQUAL(setproducer.configuration.max_gen_trx_size, setproducer2.configuration.max_gen_trx_size);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(setproxy, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "stakeholder" : "stake.hldr",
     "proxy" : "stkhdr.prxy"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto setproxy = var.as<eosio::types::setproxy>();
   BOOST_CHECK_EQUAL("stake.hldr", setproxy.stakeholder);
   BOOST_CHECK_EQUAL("stkhdr.prxy", setproxy.proxy);

   auto var2 = verify_round_trip_conversion( abis, "setproxy", var );
   auto setproxy2 = var2.as<eosio::types::setproxy>();
   BOOST_CHECK_EQUAL(setproxy.stakeholder, setproxy2.stakeholder);
   BOOST_CHECK_EQUAL(setproxy.proxy, setproxy2.proxy);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(linkauth, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "lnkauth.acct",
     "code" : "lnkauth.code",
     "type" : "lnkauth.type",
     "requirement" : "lnkauth.rqm",
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto linkauth = var.as<eosio::types::linkauth>();
   BOOST_CHECK_EQUAL("lnkauth.acct", linkauth.account);
   BOOST_CHECK_EQUAL("lnkauth.code", linkauth.code);
   BOOST_CHECK_EQUAL("lnkauth.type", linkauth.type);
   BOOST_CHECK_EQUAL("lnkauth.rqm", linkauth.requirement);

   auto var2 = verify_round_trip_conversion( abis, "linkauth", var );
   auto linkauth2 = var2.as<eosio::types::linkauth>();
   BOOST_CHECK_EQUAL(linkauth.account, linkauth2.account);
   BOOST_CHECK_EQUAL(linkauth.code, linkauth2.code);
   BOOST_CHECK_EQUAL(linkauth.type, linkauth2.type);
   BOOST_CHECK_EQUAL(linkauth.requirement, linkauth2.requirement);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(unlinkauth, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "lnkauth.acct",
     "code" : "lnkauth.code",
     "type" : "lnkauth.type",
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto unlinkauth = var.as<eosio::types::unlinkauth>();
   BOOST_CHECK_EQUAL("lnkauth.acct", unlinkauth.account);
   BOOST_CHECK_EQUAL("lnkauth.code", unlinkauth.code);
   BOOST_CHECK_EQUAL("lnkauth.type", unlinkauth.type);

   auto var2 = verify_round_trip_conversion( abis, "unlinkauth", var );
   auto unlinkauth2 = var2.as<eosio::types::unlinkauth>();
   BOOST_CHECK_EQUAL(unlinkauth.account, unlinkauth2.account);
   BOOST_CHECK_EQUAL(unlinkauth.code, unlinkauth2.code);
   BOOST_CHECK_EQUAL(unlinkauth.type, unlinkauth2.type);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(updateauth, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "updauth.acct",
     "permission" : "updauth.prm",
     "parent" : "updauth.prnt",
     "new_authority" : {
        "threshold" : "2147483145",
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"account" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"account" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     }
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto updateauth = var.as<eosio::types::updateauth>();
   BOOST_CHECK_EQUAL("updauth.acct", updateauth.account);
   BOOST_CHECK_EQUAL("updauth.prm", updateauth.permission);
   BOOST_CHECK_EQUAL("updauth.prnt", updateauth.parent);
   BOOST_CHECK_EQUAL(2147483145u, updateauth.new_authority.threshold);

   BOOST_REQUIRE_EQUAL(2, updateauth.new_authority.keys.size());
   BOOST_CHECK_EQUAL("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", (std::string)updateauth.new_authority.keys[0].key);
   BOOST_CHECK_EQUAL(57005u, updateauth.new_authority.keys[0].weight);
   BOOST_CHECK_EQUAL("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", (std::string)updateauth.new_authority.keys[1].key);
   BOOST_CHECK_EQUAL(57605u, updateauth.new_authority.keys[1].weight);

   BOOST_REQUIRE_EQUAL(2, updateauth.new_authority.accounts.size());
   BOOST_CHECK_EQUAL("prm.acct1", updateauth.new_authority.accounts[0].permission.account);
   BOOST_CHECK_EQUAL("prm.prm1", updateauth.new_authority.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(53005u, updateauth.new_authority.accounts[0].weight);
   BOOST_CHECK_EQUAL("prm.acct2", updateauth.new_authority.accounts[1].permission.account);
   BOOST_CHECK_EQUAL("prm.prm2", updateauth.new_authority.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(53405u, updateauth.new_authority.accounts[1].weight);

   auto var2 = verify_round_trip_conversion( abis, "updateauth", var );
   auto updateauth2 = var2.as<eosio::types::updateauth>();
   BOOST_CHECK_EQUAL(updateauth.account, updateauth2.account);
   BOOST_CHECK_EQUAL(updateauth.permission, updateauth2.permission);
   BOOST_CHECK_EQUAL(updateauth.parent, updateauth2.parent);

   BOOST_CHECK_EQUAL(updateauth.new_authority.threshold, updateauth2.new_authority.threshold);

   BOOST_REQUIRE_EQUAL(updateauth.new_authority.keys.size(), updateauth2.new_authority.keys.size());
   BOOST_CHECK_EQUAL(updateauth.new_authority.keys[0].key, updateauth2.new_authority.keys[0].key);
   BOOST_CHECK_EQUAL(updateauth.new_authority.keys[0].weight, updateauth2.new_authority.keys[0].weight);
   BOOST_CHECK_EQUAL(updateauth.new_authority.keys[1].key, updateauth2.new_authority.keys[1].key);
   BOOST_CHECK_EQUAL(updateauth.new_authority.keys[1].weight, updateauth2.new_authority.keys[1].weight);

   BOOST_REQUIRE_EQUAL(updateauth.new_authority.accounts.size(), updateauth2.new_authority.accounts.size());
   BOOST_CHECK_EQUAL(updateauth.new_authority.accounts[0].permission.account, updateauth2.new_authority.accounts[0].permission.account);
   BOOST_CHECK_EQUAL(updateauth.new_authority.accounts[0].permission.permission, updateauth2.new_authority.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(updateauth.new_authority.accounts[0].weight, updateauth2.new_authority.accounts[0].weight);
   BOOST_CHECK_EQUAL(updateauth.new_authority.accounts[1].permission.account, updateauth2.new_authority.accounts[1].permission.account);
   BOOST_CHECK_EQUAL(updateauth.new_authority.accounts[1].permission.permission, updateauth2.new_authority.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(updateauth.new_authority.accounts[1].weight, updateauth2.new_authority.accounts[1].weight);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(deleteauth, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "delauth.acct",
     "permission" : "delauth.prm"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto deleteauth = var.as<eosio::types::deleteauth>();
   BOOST_CHECK_EQUAL("delauth.acct", deleteauth.account);
   BOOST_CHECK_EQUAL("delauth.prm", deleteauth.permission);

   auto var2 = verify_round_trip_conversion( abis, "deleteauth", var );
   auto deleteauth2 = var2.as<eosio::types::deleteauth>();
   BOOST_CHECK_EQUAL(deleteauth.account, deleteauth2.account);
   BOOST_CHECK_EQUAL(deleteauth.permission, deleteauth2.permission);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(newaccount, testing_fixture)
{ try {

   abi_serializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "creator" : "newacct.crtr",
     "name" : "newacct.name",
     "owner" : {
        "threshold" : 2147483145,
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"account" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"account" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     },
     "active" : {
        "threshold" : 2146483145,
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"account" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"account" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     },
     "recovery" : {
        "threshold" : 2145483145,
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"account" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"account" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     },
     "deposit" : "-90000000.0000 EOS"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto newaccount = var.as<eosio::types::newaccount>();
   BOOST_CHECK_EQUAL("newacct.crtr", newaccount.creator);
   BOOST_CHECK_EQUAL("newacct.name", newaccount.name);

   BOOST_CHECK_EQUAL(2147483145u, newaccount.owner.threshold);

   BOOST_REQUIRE_EQUAL(2, newaccount.owner.keys.size());
   BOOST_CHECK_EQUAL("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", (std::string)newaccount.owner.keys[0].key);
   BOOST_CHECK_EQUAL(57005u, newaccount.owner.keys[0].weight);
   BOOST_CHECK_EQUAL("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", (std::string)newaccount.owner.keys[1].key);
   BOOST_CHECK_EQUAL(57605u, newaccount.owner.keys[1].weight);

   BOOST_REQUIRE_EQUAL(2, newaccount.owner.accounts.size());
   BOOST_CHECK_EQUAL("prm.acct1", newaccount.owner.accounts[0].permission.account);
   BOOST_CHECK_EQUAL("prm.prm1", newaccount.owner.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(53005u, newaccount.owner.accounts[0].weight);
   BOOST_CHECK_EQUAL("prm.acct2", newaccount.owner.accounts[1].permission.account);
   BOOST_CHECK_EQUAL("prm.prm2", newaccount.owner.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(53405u, newaccount.owner.accounts[1].weight);

   BOOST_CHECK_EQUAL(2146483145u, newaccount.active.threshold);

   BOOST_REQUIRE_EQUAL(2, newaccount.active.keys.size());
   BOOST_CHECK_EQUAL("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", (std::string)newaccount.active.keys[0].key);
   BOOST_CHECK_EQUAL(57005u, newaccount.active.keys[0].weight);
   BOOST_CHECK_EQUAL("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", (std::string)newaccount.active.keys[1].key);
   BOOST_CHECK_EQUAL(57605u, newaccount.active.keys[1].weight);

   BOOST_REQUIRE_EQUAL(2, newaccount.active.accounts.size());
   BOOST_CHECK_EQUAL("prm.acct1", newaccount.active.accounts[0].permission.account);
   BOOST_CHECK_EQUAL("prm.prm1", newaccount.active.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(53005u, newaccount.active.accounts[0].weight);
   BOOST_CHECK_EQUAL("prm.acct2", newaccount.active.accounts[1].permission.account);
   BOOST_CHECK_EQUAL("prm.prm2", newaccount.active.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(53405u, newaccount.active.accounts[1].weight);

   BOOST_CHECK_EQUAL(2145483145u, newaccount.recovery.threshold);

   BOOST_REQUIRE_EQUAL(2, newaccount.recovery.keys.size());
   BOOST_CHECK_EQUAL("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", (std::string)newaccount.recovery.keys[0].key);
   BOOST_CHECK_EQUAL(57005u, newaccount.recovery.keys[0].weight);
   BOOST_CHECK_EQUAL("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", (std::string)newaccount.recovery.keys[1].key);
   BOOST_CHECK_EQUAL(57605u, newaccount.recovery.keys[1].weight);

   BOOST_REQUIRE_EQUAL(2, newaccount.recovery.accounts.size());
   BOOST_CHECK_EQUAL("prm.acct1", newaccount.recovery.accounts[0].permission.account);
   BOOST_CHECK_EQUAL("prm.prm1", newaccount.recovery.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(53005u, newaccount.recovery.accounts[0].weight);
   BOOST_CHECK_EQUAL("prm.acct2", newaccount.recovery.accounts[1].permission.account);
   BOOST_CHECK_EQUAL("prm.prm2", newaccount.recovery.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(53405u, newaccount.recovery.accounts[1].weight);

   BOOST_CHECK_EQUAL(-900000000000, newaccount.deposit.amount);
   BOOST_CHECK_EQUAL(EOS_SYMBOL, newaccount.deposit.symbol);

   auto var2 = verify_round_trip_conversion( abis, "newaccount", var );
   auto newaccount2 = var2.as<eosio::types::newaccount>();
   BOOST_CHECK_EQUAL(newaccount.creator, newaccount2.creator);
   BOOST_CHECK_EQUAL(newaccount.name, newaccount2.name);

   BOOST_CHECK_EQUAL(newaccount.owner.threshold, newaccount2.owner.threshold);

   BOOST_REQUIRE_EQUAL(newaccount.owner.keys.size(), newaccount2.owner.keys.size());
   BOOST_CHECK_EQUAL(newaccount.owner.keys[0].key, newaccount2.owner.keys[0].key);
   BOOST_CHECK_EQUAL(newaccount.owner.keys[0].weight, newaccount2.owner.keys[0].weight);
   BOOST_CHECK_EQUAL(newaccount.owner.keys[1].key, newaccount2.owner.keys[1].key);
   BOOST_CHECK_EQUAL(newaccount.owner.keys[1].weight, newaccount2.owner.keys[1].weight);

   BOOST_REQUIRE_EQUAL(newaccount.owner.accounts.size(), newaccount2.owner.accounts.size());
   BOOST_CHECK_EQUAL(newaccount.owner.accounts[0].permission.account, newaccount2.owner.accounts[0].permission.account);
   BOOST_CHECK_EQUAL(newaccount.owner.accounts[0].permission.permission, newaccount2.owner.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(newaccount.owner.accounts[0].weight, newaccount2.owner.accounts[0].weight);
   BOOST_CHECK_EQUAL(newaccount.owner.accounts[1].permission.account, newaccount2.owner.accounts[1].permission.account);
   BOOST_CHECK_EQUAL(newaccount.owner.accounts[1].permission.permission, newaccount2.owner.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(newaccount.owner.accounts[1].weight, newaccount2.owner.accounts[1].weight);

   BOOST_CHECK_EQUAL(newaccount.active.threshold, newaccount2.active.threshold);

   BOOST_REQUIRE_EQUAL(newaccount.active.keys.size(), newaccount2.active.keys.size());
   BOOST_CHECK_EQUAL(newaccount.active.keys[0].key, newaccount2.active.keys[0].key);
   BOOST_CHECK_EQUAL(newaccount.active.keys[0].weight, newaccount2.active.keys[0].weight);
   BOOST_CHECK_EQUAL(newaccount.active.keys[1].key, newaccount2.active.keys[1].key);
   BOOST_CHECK_EQUAL(newaccount.active.keys[1].weight, newaccount2.active.keys[1].weight);

   BOOST_REQUIRE_EQUAL(newaccount.active.accounts.size(), newaccount2.active.accounts.size());
   BOOST_CHECK_EQUAL(newaccount.active.accounts[0].permission.account, newaccount2.active.accounts[0].permission.account);
   BOOST_CHECK_EQUAL(newaccount.active.accounts[0].permission.permission, newaccount2.active.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(newaccount.active.accounts[0].weight, newaccount2.active.accounts[0].weight);
   BOOST_CHECK_EQUAL(newaccount.active.accounts[1].permission.account, newaccount2.active.accounts[1].permission.account);
   BOOST_CHECK_EQUAL(newaccount.active.accounts[1].permission.permission, newaccount2.active.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(newaccount.active.accounts[1].weight, newaccount2.active.accounts[1].weight);

   BOOST_CHECK_EQUAL(newaccount.recovery.threshold, newaccount2.recovery.threshold);

   BOOST_REQUIRE_EQUAL(newaccount.recovery.keys.size(), newaccount2.recovery.keys.size());
   BOOST_CHECK_EQUAL(newaccount.recovery.keys[0].key, newaccount2.recovery.keys[0].key);
   BOOST_CHECK_EQUAL(newaccount.recovery.keys[0].weight, newaccount2.recovery.keys[0].weight);
   BOOST_CHECK_EQUAL(newaccount.recovery.keys[1].key, newaccount2.recovery.keys[1].key);
   BOOST_CHECK_EQUAL(newaccount.recovery.keys[1].weight, newaccount2.recovery.keys[1].weight);

   BOOST_REQUIRE_EQUAL(newaccount.recovery.accounts.size(), newaccount2.recovery.accounts.size());
   BOOST_CHECK_EQUAL(newaccount.recovery.accounts[0].permission.account, newaccount2.recovery.accounts[0].permission.account);
   BOOST_CHECK_EQUAL(newaccount.recovery.accounts[0].permission.permission, newaccount2.recovery.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(newaccount.recovery.accounts[0].weight, newaccount2.recovery.accounts[0].weight);
   BOOST_CHECK_EQUAL(newaccount.recovery.accounts[1].permission.account, newaccount2.recovery.accounts[1].permission.account);
   BOOST_CHECK_EQUAL(newaccount.recovery.accounts[1].permission.permission, newaccount2.recovery.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(newaccount.recovery.accounts[1].weight, newaccount2.recovery.accounts[1].weight);

   BOOST_CHECK_EQUAL(newaccount.deposit.amount, newaccount2.deposit.amount);
   BOOST_CHECK_EQUAL(newaccount.deposit.symbol, newaccount2.deposit.symbol);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abi_type_repeat, testing_fixture)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       },{
         "new_type_name": "account_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": {
           "from": "account_name",
           "to": "account_name",
           "amount": "uint64"
         }
       },{
         "name": "account",
         "base": "",
         "fields": {
           "account": "name",
           "balance": "uint64"
         }
       }
     ],
     "actions": [{
         "action_name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "table_name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ]
   }
   )=====";

   auto abi = fc::json::from_string(repeat_abi).as<types::abi>();
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("types.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abi_struct_repeat, testing_fixture)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": {
           "from": "account_name",
           "to": "account_name",
           "amount": "uint64"
         }
       },{
         "name": "transfer",
         "base": "",
         "fields": {
           "account": "name",
           "balance": "uint64"
         }
       }
     ],
     "actions": [{
         "action_name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "table_name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ]
   }
   )=====";

   auto abi = fc::json::from_string(repeat_abi).as<types::abi>();
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("structs.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abi_action_repeat, testing_fixture)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": {
           "from": "account_name",
           "to": "account_name",
           "amount": "uint64"
         }
       },{
         "name": "account",
         "base": "",
         "fields": {
           "account": "name",
           "balance": "uint64"
         }
       }
     ],
     "actions": [{
         "action_name": "transfer",
         "type": "transfer"
       },{
         "action_name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "table_name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ]
   }
   )=====";

   auto abi = fc::json::from_string(repeat_abi).as<types::abi>();
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("actions.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abi_table_repeat, testing_fixture)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": {
           "from": "account_name",
           "to": "account_name",
           "amount": "uint64"
         }
       },{
         "name": "account",
         "base": "",
         "fields": {
           "account": "name",
           "balance": "uint64"
         }
       }
     ],
     "actions": [{
         "action_name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "table_name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       },{
         "table_name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ]
   }
   )=====";

   auto abi = fc::json::from_string(repeat_abi).as<types::abi>();
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("tables.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
