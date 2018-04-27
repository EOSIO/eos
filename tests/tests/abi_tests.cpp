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
#include <fc/log/logger.hpp>

#include <eosio/chain/contracts/chain_initializer.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/abi_generator/abi_generator.hpp>

#include "config.hpp"
#include <boost/test/framework.hpp>
using namespace eosio;
using namespace chain;
using namespace chain::contracts;

BOOST_AUTO_TEST_SUITE(abi_tests)

// verify that round trip conversion, via bytes, reproduces the exact same data
fc::variant verify_byte_round_trip_conversion( const abi_serializer& abis, const type_name& type, const fc::variant& var )
{
   auto bytes = abis.variant_to_binary(type, var);

   auto var2 = abis.binary_to_variant(type, bytes);

   std::string r = fc::json::to_string(var2);

   auto bytes2 = abis.variant_to_binary(type, var2);

   BOOST_TEST( fc::to_hex(bytes) == fc::to_hex(bytes2) );

   return var2;
}

auto get_resolver(const contracts::abi_def& abi = contracts::abi_def())
{
   return [&abi](const account_name &name) -> optional<contracts::abi_serializer> {
      return abi_serializer(chain_initializer::eos_contract_abi(abi));
   };
}

// verify that round trip conversion, via actual class, reproduces the exact same data
template<typename T>
fc::variant verify_type_round_trip_conversion( const abi_serializer& abis, const type_name& type, const fc::variant& var )
{
   auto bytes = abis.variant_to_binary(type, var);

   T obj;
   abi_serializer::from_variant(var, obj, get_resolver());

   fc::variant var2;
   abi_serializer::to_variant(obj, var2, get_resolver());

   std::string r = fc::json::to_string(var2);


   auto bytes2 = abis.variant_to_binary(type, var2);

   BOOST_TEST( fc::to_hex(bytes) == fc::to_hex(bytes2) );

   return var2;
}

const char* my_abi = R"=====(
{
  "types": [],
  "structs": [{
      "name"  : "A",
      "base"  : "PublicKeyTypes",
      "fields": []
    },
    {
      "name": "PublicKeyTypes",
      "base" : "AssetTypes",
      "fields": [{
         "name": "publickey",
         "type": "public_key"
      },{
         "name": "publickey_arr",
         "type": "public_key[]"
      }]
    },{
      "name": "AssetTypes",
      "base" : "NativeTypes",
      "fields": [{
         "name": "asset",
         "type": "asset"
      },{
         "name": "asset_arr",
         "type": "asset[]"
      }]
    },{
      "name": "NativeTypes",
      "fields" : [{
         "name": "string",
         "type": "string"
      },{
         "name": "string_arr",
         "type": "string[]"
      },{
         "name": "time",
         "type": "time"
      },{
         "name": "time_arr",
         "type": "time[]"
      },{
         "name": "signature",
         "type": "signature"
      },{
         "name": "signature_arr",
         "type": "signature[]"
      },{
         "name": "checksum256",
         "type": "checksum256"
      },{
         "name": "checksum256_arr",
         "type": "checksum256[]"
      },{
         "name": "fieldname",
         "type": "field_name"
      },{
         "name": "fieldname_arr",
         "type": "field_name[]"
      },{
         "name": "fixedstring32",
         "type": "fixed_string32"
      },{
         "name": "fixedstring32_ar",
         "type": "fixed_string32[]"
      },{
         "name": "fixedstring16",
         "type": "fixed_string16"
      },{
         "name": "fixedstring16_ar",
         "type": "fixed_string16[]"
      },{
         "name": "typename",
         "type": "type_name"
      },{
         "name": "typename_arr",
         "type": "type_name[]"
      },{
         "name": "uint8",
         "type": "uint8"
      },{
         "name": "uint8_arr",
         "type": "uint8[]"
      },{
         "name": "uint16",
         "type": "uint16"
      },{
         "name": "uint16_arr",
         "type": "uint16[]"
      },{
         "name": "uint32",
         "type": "uint32"
      },{
         "name": "uint32_arr",
         "type": "uint32[]"
      },{
         "name": "uint64",
         "type": "uint64"
      },{
         "name": "uint64_arr",
         "type": "uint64[]"
      },{
         "name": "uint128",
         "type": "uint128"
      },{
         "name": "uint128_arr",
         "type": "uint128[]"
      },{
         "name": "uint256",
         "type": "uint256"
      },{
         "name": "uint256_arr",
         "type": "uint256[]"
      },{
         "name": "int8",
         "type": "int8"
      },{
         "name": "int8_arr",
         "type": "int8[]"
      },{
         "name": "int16",
         "type": "int16"
      },{
         "name": "int16_arr",
         "type": "int16[]"
      },{
         "name": "int32",
         "type": "int32"
      },{
         "name": "int32_arr",
         "type": "int32[]"
      },{
         "name": "int64",
         "type": "int64"
      },{
         "name": "int64_arr",
         "type": "int64[]"
      },{
         "name": "name",
         "type": "name"
      },{
         "name": "name_arr",
         "type": "name[]"
      },{
         "name": "field",
         "type": "field"
      },{
         "name": "field_arr",
         "type": "field[]"
      },{
         "name": "struct",
         "type": "struct_def"
      },{
         "name": "struct_arr",
         "type": "struct_def[]"
      },{
         "name": "fields",
         "type": "fields"
      },{
         "name": "fields_arr",
         "type": "fields[]"
      },{
         "name": "accountname",
         "type": "account_name"
      },{
         "name": "accountname_arr",
         "type": "account_name[]"
      },{
         "name": "permname",
         "type": "permission_name"
      },{
         "name": "permname_arr",
         "type": "permission_name[]"
      },{
         "name": "actionname",
         "type": "action_name"
      },{
         "name": "actionname_arr",
         "type": "action_name[]"
      },{
         "name": "scopename",
         "type": "scope_name"
      },{
         "name": "scopename_arr",
         "type": "scope_name[]"
      },{
         "name": "permlvl",
         "type": "permission_level"
      },{
         "name": "permlvl_arr",
         "type": "permission_level[]"
      },{
         "name": "action",
         "type": "action"
      },{
         "name": "action_arr",
         "type": "action[]"
      },{
         "name": "permlvlwgt",
         "type": "permission_level_weight"
      },{
         "name": "permlvlwgt_arr",
         "type": "permission_level_weight[]"
      },{
         "name": "transaction",
         "type": "transaction"
      },{
         "name": "transaction_arr",
         "type": "transaction[]"
      },{
         "name": "strx",
         "type": "signed_transaction"
      },{
         "name": "strx_arr",
         "type": "signed_transaction[]"
      },{
         "name": "keyweight",
         "type": "key_weight"
      },{
         "name": "keyweight_arr",
         "type": "key_weight[]"
      },{
         "name": "authority",
         "type": "authority"
      },{
         "name": "authority_arr",
         "type": "authority[]"
      },{
         "name": "typedef",
         "type": "type_def"
      },{
         "name": "typedef_arr",
         "type": "type_def[]"
      },{
         "name": "actiondef",
         "type": "action_def"
      },{
         "name": "actiondef_arr",
         "type": "action_def[]"
      },{
         "name": "tabledef",
         "type": "table_def"
      },{
         "name": "tabledef_arr",
         "type": "table_def[]"
      },{
         "name": "abidef",
         "type": "abi_def"
      },{
         "name": "abidef_arr",
         "type": "abi_def[]"
      }]
    }
  ],
  "actions": [],
  "tables": [],
  "ricardian_clauses": [{"id":"clause A","body":"clause body A"}, 
              {"id":"clause B","body":"clause body B"}]
}
)=====";

BOOST_AUTO_TEST_CASE(uint_types)
{ try {

   const char* currency_abi = R"=====(
   {
       "types": [],
       "structs": [{
           "name": "transfer",
           "base": "",
           "fields": [{
               "name": "amount64",
               "type": "uint64"
           },{
               "name": "amount32",
               "type": "uint32"
           },{
               "name": "amount16",
               "type": "uint16"
           },{
               "name": "amount8",
               "type": "uint8"
           }]
       }],
       "actions": [],
       "tables": [],
       "ricardian_clauses": []
   }
   )=====";

   auto abi = fc::json::from_string(currency_abi).as<abi_def>();

   abi_serializer abis(chain_initializer::eos_contract_abi(abi));
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
   verify_byte_round_trip_conversion(abi, "transfer", var);

} FC_LOG_AND_RETHROW() }

using namespace eosio::tests::config;

struct abi_gen_helper {

  abi_gen_helper() {}

  static bool is_abi_generation_exception(const eosio::abi_generation_exception& e) { return true; };

  bool generate_abi(const char* source, const char* abi, bool opt_sfs=false) {

    std::string include_param = std::string("-I") + eosiolib_path;
    std::string pfr_include_param = std::string("-I") + pfr_include_path;
    std::string boost_include_param = std::string("-I") + boost_include_path;
    std::string stdcpp_include_param = std::string("-I") + eosiolib_path + "/libc++/upstream/include";
    std::string stdc_include_param = std::string("-I") + eosiolib_path +  "/musl/upstream/include";

    abi_def output;

    std::string contract;
    std::vector<std::string> actions;

    bool res = runToolOnCodeWithArgs(new find_eosio_abi_macro_action(contract, actions, ""), source,
      {"-fparse-all-comments", "--std=c++14", "--target=wasm32", "-ffreestanding", "-nostdlib",
      "-nostdlibinc", "-fno-threadsafe-statics", "-fno-rtti",  "-fno-exceptions",
      include_param, boost_include_param, stdcpp_include_param,
      stdc_include_param, pfr_include_param }
    );
    FC_ASSERT(res == true);

    res = runToolOnCodeWithArgs(new generate_abi_action(false, opt_sfs, "", output, contract, actions), source,
      {"-fparse-all-comments", "--std=c++14", "--target=wasm32", "-ffreestanding", "-nostdlib",
      "-nostdlibinc", "-fno-threadsafe-statics", "-fno-rtti",  "-fno-exceptions",
      include_param, boost_include_param, stdcpp_include_param,
      stdc_include_param, pfr_include_param }
    );
    FC_ASSERT(res == true);

    abi_serializer(chain_initializer::eos_contract_abi(output)).validate();

    auto abi1 = fc::json::from_string(abi).as<abi_def>();

    auto e = fc::to_hex(fc::raw::pack(abi1)) == fc::to_hex(fc::raw::pack(output));

    if(!e) {
      BOOST_TEST_MESSAGE("Generate ABI:\n" <<
                        "expected: \n" << fc::json::to_pretty_string(abi1) << "\n" <<
                        "generated: \n" << fc::json::to_pretty_string(output));
    }

    return e;
  }

};

BOOST_FIXTURE_TEST_CASE(abigen_unknown_type, abi_gen_helper)
{ try {

   const char* unknown_type = R"=====(
   #include <eosiolib/types.h>
   //@abi action
   struct transfer {
      uint64_t param1;
      char*    param2;
   };
   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(unknown_type, ""), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_all_types, abi_gen_helper)
{
#if 0
   try {

   const char* all_types = R"=====(
    #include <eosiolib/types.hpp>
    #include <eosiolib/asset.hpp>
    #include <string>

    typedef int field;
    typedef int struct_def;
    typedef int fields;
    typedef int permission_level;
    typedef int action;
    typedef int permission_level_weight;
    typedef int transaction;
    typedef int signed_transaction;
    typedef int key_weight;
    typedef int authority;
    typedef int chain_config;
    typedef int type_def;
    typedef int action_def;
    typedef int table_def;
    typedef int abi_def;
    typedef int nonce;

   //@abi action
   struct test_struct {
      std::string             field1;
      time                    field2;
      signature               field3;
      checksum256             field4;
      field_name              field5;
      fixed_string32          field6;
      fixed_string16          field7;
      type_name               field8;
      uint8_t                 field9;
      uint16_t                field10;
      uint32_t                field11;
      uint64_t                field12;
      uint128_t               field13;
      //uint256                 field14;
      int8_t                  field15;
      int16_t                 field16;
      int32_t                 field17;
      int64_t                 field18;
      eosio::name             field19;
      field                   field20;
      struct_def              field21;
      fields                  field22;
      account_name            field23;
      permission_name         field24;
      action_name             field25;
      scope_name              field26;
      permission_level        field27;
      action                  field28;
      permission_level_weight field29;
      transaction             field30;
      signed_transaction      field31;
      key_weight              field32;
      authority               field33;
      chain_config            field34;
      type_def                field35;
      action_def              field36;
      table_def               field37;
      abi_def                 field38;
      public_key              field39;
      eosio::asset            field40;
   };
   )=====";

   const char* all_types_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "test_struct",
          "base" : "",
          "fields" : [{
               "name": "field1",
               "type": "string"
          },{
             "name": "field2",
             "type": "time"
          },{
             "name": "field3",
             "type": "signature"
          },{
             "name": "field4",
             "type": "checksum256"
          },{
             "name": "field5",
             "type": "field_name"
          },{
             "name": "field6",
             "type": "fixed_string32"
          },{
             "name": "field7",
             "type": "fixed_string16"
          },{
             "name": "field8",
             "type": "type_name"
          },{
             "name": "field9",
             "type": "uint8"
          },{
             "name": "field10",
             "type": "uint16"
          },{
             "name": "field11",
             "type": "uint32"
          },{
             "name": "field12",
             "type": "uint64"
          },{
             "name": "field13",
             "type": "uint128"
          },{
             "name": "field15",
             "type": "int8"
          },{
             "name": "field16",
             "type": "int16"
          },{
             "name": "field17",
             "type": "int32"
          },{
             "name": "field18",
             "type": "int64"
          },{
             "name": "field19",
             "type": "name"
          },{
             "name": "field20",
             "type": "field"
          },{
             "name": "field21",
             "type": "struct_def"
          },{
             "name": "field22",
             "type": "fields"
          },{
             "name": "field23",
             "type": "account_name"
          },{
             "name": "field24",
             "type": "permission_name"
          },{
             "name": "field25",
             "type": "action_name"
          },{
             "name": "field26",
             "type": "scope_name"
          },{
             "name": "field27",
             "type": "permission_level"
          },{
             "name": "field28",
             "type": "action"
          },{
             "name": "field29",
             "type": "permission_level_weight"
          },{
             "name": "field30",
             "type": "transaction"
          },{
             "name": "field31",
             "type": "signed_transaction"
          },{
             "name": "field32",
             "type": "key_weight"
          },{
             "name": "field33",
             "type": "authority"
          },{
             "name": "field34",
             "type": "chain_config"
          },{
             "name": "field35",
             "type": "type_def"
          },{
             "name": "field36",
             "type": "action_def"
          },{
             "name": "field37",
             "type": "table_def"
          },{
             "name": "field38",
             "type": "abi_def"
          },{
             "name": "field39",
             "type": "public_key"
          },{
             "name": "field40",
             "type": "asset"
          }]
           }],
           "actions": [{
              "name" : "teststruct",
              "type" : "test_struct"
           }],
       "tables": [],
       "ricardian_clauses": []
   }
   )=====";
   BOOST_TEST( generate_abi(all_types, all_types_abi) == true);

} FC_LOG_AND_RETHROW()
#endif
}

BOOST_FIXTURE_TEST_CASE(abigen_double_base, abi_gen_helper)
{ try {

   const char* double_base = R"=====(
   #include <eosiolib/types.h>

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

   BOOST_CHECK_EXCEPTION( generate_abi(double_base, ""), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );

} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE(abigen_double_action, abi_gen_helper)
{ try {

   const char* double_action = R"=====(
   #include <eosiolib/types.h>

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
          "fields" : [{
            "name" : "param3",
            "type" : "uint64"
          }]
       },{
          "name" : "B",
          "base" : "A",
          "fields" : [{
            "name" : "param2",
            "type" : "uint64"
          }]
       },{
          "name" : "C",
          "base" : "B",
          "fields" : [{
            "name" : "param1",
            "type" : "uint64"
          }]
       }],
       "actions": [{
          "name" : "action1",
          "type" : "C",
          "ricardian_contract" : ""
       },{
          "name" : "action2",
          "type" : "C",
          "ricardian_contract" : ""
       }],
       "tables": [],
       "ricardian_clauses":[]
   }
   )=====";


   BOOST_TEST( generate_abi(double_action, double_action_abi) == true );

} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE(abigen_all_indexes, abi_gen_helper)
{ try {

   const char* all_indexes = R"=====(
   #include <eosiolib/types.hpp>
   #include <string>

   using namespace eosio;

   //@abi table
   struct table1 {
      uint64_t field1;
   };

   //@abi table
   struct table2 {
      uint128_t field1;
      uint128_t field2;
   };

   //@abi table
   struct table3 {
      uint64_t field1;
      uint64_t field2;
      uint64_t field3;
   };

   struct my_complex_value {
      uint64_t    a;
      account_name b;
   };

   //@abi table
   struct table4 {
      std::string key;
      my_complex_value value;
   };

   )=====";

   const char* all_indexes_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "table1",
          "base" : "",
          "fields" : [{
            "name" : "field1",
            "type" : "uint64"
          }]
       },{
          "name" : "table2",
          "base" : "",
          "fields" : [{
            "name" : "field1",
            "type" : "uint128"
          },{
            "name" : "field2",
            "type" : "uint128"
          }]
       },{
          "name" : "table3",
          "base" : "",
          "fields" : [{
            "name" : "field1",
            "type" : "uint64"
          },{
            "name" : "field2",
            "type" : "uint64"
          },{
            "name" : "field3",
            "type" : "uint64"
          }]
       },{
          "name" : "my_complex_value",
          "base" : "",
          "fields" : [{
            "name" : "a",
            "type" : "uint64"
          },{
            "name" : "b",
            "type" : "account_name"
          }]
       },{
          "name" : "table4",
          "base" : "",
          "fields" : [{
            "name" : "key",
            "type" : "string"
          },{
            "name" : "value",
            "type" : "my_complex_value"
          }]
       }],
       "actions": [],
       "tables": [
        {
          "name": "table1",
          "type": "table1",
          "index_type": "i64",
          "key_names": [
            "field1"
          ],
          "key_types": [
            "uint64"
          ]
        },{
          "name": "table2",
          "type": "table2",
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
          "name": "table3",
          "type": "table3",
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
          "name": "table4",
          "type": "table4",
          "index_type": "str",
          "key_names": [
            "key",
          ],
          "key_types": [
            "string",
          ]
        },

       ],
       "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(all_indexes, all_indexes_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_unable_to_determine_index, abi_gen_helper)
{ try {

   const char* unable_to_determine_index = R"=====(
   #include <eosiolib/types.h>

   //@abi table
   struct table1 {
      uint32_t field1;
      uint64_t field2;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(unable_to_determine_index, ""), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_long_field_name, abi_gen_helper)
{ try {

   //TODO: full action / full table
  // typedef fixed_string16 FieldName;
   const char* long_field_name = R"=====(
   #include <eosiolib/types.h>

   //@abi table
   struct table1 {
      uint64_t thisisaverylongfieldname;
   };

   )=====";

   BOOST_TEST( generate_abi(long_field_name, "{}") == false );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_long_type_name, abi_gen_helper)
{ try {

   const char* long_type_name = R"=====(
   #include <eosiolib/types.h>

   struct this_is_a_very_very_very_very_long_type_name {
      uint64_t field;
   };

   //@abi table
   struct table1 {
      this_is_a_very_very_very_very_long_type_name field1;
   };

   )=====";


   BOOST_TEST( generate_abi(long_type_name, "{}") == false );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_same_type_different_namespace, abi_gen_helper)
{ try {

   const char* same_type_different_namespace = R"=====(
   #include <eosiolib/types.h>

   namespace A {
     //@abi table
     struct table1 {
        uint64_t field1;
     };
   }

   namespace B {
     //@abi table
     struct table1 {
        uint64_t field2;
     };
   }

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(same_type_different_namespace, ""), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_bad_index_type, abi_gen_helper)
{ try {

   const char* bad_index_type = R"=====(
   #include <eosiolib/types.h>

   //@abi table table1 i128i128
   struct table1 {
      uint32_t key;
      uint64_t field1;
      uint64_t field2;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(bad_index_type, "{}"), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_full_table_decl, abi_gen_helper)
{ try {

   const char* full_table_decl = R"=====(
   #include <eosiolib/types.hpp>

   //@abi table table1 i64
   class table1 {
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
          "name" : "table1",
          "base" : "",
          "fields" : [{
            "name" : "id",
            "type" : "uint64"
          },{
            "name" : "name",
            "type" : "name"
          },{
            "name" : "age",
            "type" : "uint32"
          }]
       }],
       "actions": [],
       "tables": [
        {
          "name": "table1",
          "type": "table1",
          "index_type": "i64",
          "key_names": [
            "id"
          ],
          "key_types": [
            "uint64"
          ]
        }],
       "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(full_table_decl, full_table_decl_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_str_table_decl, abi_gen_helper)
{ try {

   const char* str_table_decl = R"=====(
   #include <eosiolib/types.hpp>
   #include <string>

   //@abi table
   class table1 {
   public:
      std::string name;
      uint32_t age;
   };

   )=====";

   const char* str_table_decl_abi = R"=====(
   {
     "types": [],
     "structs": [{
         "name": "table1",
         "base": "",
         "fields": [{
            "name" : "name",
            "type" : "string"
          },{
            "name" : "age",
            "type" : "uint32"
          }]
       }
     ],
     "actions": [],
     "tables": [{
         "name": "table1",
         "index_type": "str",
         "key_names": [
           "name"
         ],
         "key_types": [
           "string"
         ],
         "type": "table1"
       }
     ],
     "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(str_table_decl, str_table_decl_abi) == true );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_union_table, abi_gen_helper)
{ try {

   const char* union_table = R"=====(
   #include <eosiolib/types.h>

   //@abi table
   union table1 {
      uint64_t field1;
      uint32_t field2;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(union_table, ""), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_same_action_different_type, abi_gen_helper)
{ try {

   const char* same_action_different_type = R"=====(
   #include <eosiolib/types.h>

   //@abi action action1
   struct table1 {
      uint64_t field1;
   };

   //@abi action action1
   struct table2 {
      uint64_t field1;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(same_action_different_type, ""), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_template_base, abi_gen_helper)
{ try {

   const char* template_base = R"=====(
   #include <eosiolib/types.h>

   template<typename T>
   class base {
      T field;
   };

   typedef base<uint32_t> base32;

   //@abi table table1 i64
   class table1 : base32 {
   public:
      uint64_t id;
   };

   )=====";

   const char* template_base_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "base32",
          "base" : "",
          "fields" : [{
            "name" : "field",
            "type" : "uint32"
          }]
       },{
          "name" : "table1",
          "base" : "base32",
          "fields" : [{
            "name" : "id",
            "type" : "uint64"
          }]
       }],
       "actions": [],
       "tables": [
        {
          "name": "table1",
          "type": "table1",
          "index_type": "i64",
          "key_names": [
            "id"
          ],
          "key_types": [
            "uint64"
          ]
        }],
       "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(template_base, template_base_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_action_and_table, abi_gen_helper)
{ try {

   const char* action_and_table = R"=====(
   #include <eosiolib/types.h>

  /* @abi table
   * @abi action
   */
   class table_action {
   public:
      uint64_t id;
   };

   )=====";

   const char* action_and_table_abi = R"=====(
   {
       "types": [],
       "structs": [{
          "name" : "table_action",
          "base" : "",
          "fields" : [{
            "name" : "id",
            "type" : "uint64"
          }]
       }],
       "actions": [{
          "name" : "tableaction",
          "type" : "table_action",
          "ricardian_contract" : ""
       }],
       "tables": [
        {
          "name": "tableaction",
          "type": "table_action",
          "index_type": "i64",
          "key_names": [
            "id"
          ],
          "key_types": [
            "uint64"
          ]
        }],
       "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(action_and_table, action_and_table_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_simple_typedef, abi_gen_helper)
{ try {

   const char* simple_typedef = R"=====(
   #include <eosiolib/types.hpp>

   using namespace eosio;

   struct common_params {
      uint64_t c1;
      uint64_t c2;
      uint64_t c3;
   };

   typedef common_params my_base_alias;

   //@abi action
   struct main_action : my_base_alias {
      uint64_t param1;
   };

   )=====";

   const char* simple_typedef_abi = R"=====(
   {
       "types": [{
          "new_type_name" : "my_base_alias",
          "type" : "common_params"
       }],
       "structs": [{
          "name" : "common_params",
          "base" : "",
          "fields" : [{
            "name" : "c1",
            "type" : "uint64"
          },{
            "name" : "c2",
            "type" : "uint64"
          },{
            "name" : "c3",
            "type" : "uint64"
          }]
       },{
          "name" : "main_action",
          "base" : "my_base_alias",
          "fields" : [{
            "name" : "param1",
            "type" : "uint64"
          }]
       }],
       "actions": [{
          "name" : "mainaction",
          "type" : "main_action",
          "ricardian_contract" : ""
       }],
       "tables": [],
       "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(simple_typedef, simple_typedef_abi) == true );
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_field_typedef, abi_gen_helper)
{ try {

   const char* field_typedef = R"=====(
   #include <eosiolib/types.hpp>

   using namespace eosio;

   typedef name my_name_alias;

   struct complex_field {
      uint64_t  f1;
      uint32_t  f2;
   };

   typedef complex_field my_complex_field_alias;

   //@abi table
   struct table1 {
      uint64_t               field1;
      my_complex_field_alias field2;
      my_name_alias          name;
   };

   )=====";

   const char* field_typedef_abi = R"=====(
   {
       "types": [{
          "new_type_name" : "my_complex_field_alias",
          "type" : "complex_field"
       },{
          "new_type_name" : "my_name_alias",
          "type" : "name"
       }],
       "structs": [{
          "name" : "complex_field",
          "base" : "",
          "fields" : [{
            "name": "f1",
            "type": "uint64"
          }, {
            "name": "f2",
            "type": "uint32"
          }]
       },{
          "name" : "table1",
          "base" : "",
          "fields" : [{
            "name": "field1",
            "type": "uint64"
          },{
            "name": "field2",
            "type": "my_complex_field_alias"
          },{
            "name": "name",
            "type": "my_name_alias"
          }]
       }],
       "actions": [],
       "tables": [{
          "name": "table1",
          "type": "table1",
          "index_type": "i64",
          "key_names": [
            "field1"
          ],
          "key_types": [
            "uint64"
          ]
        }],
       "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(field_typedef, field_typedef_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_vector_of_POD, abi_gen_helper)
{ try {

   const char* abigen_vector_of_POD = R"=====(
   #include <vector>
   #include <string>
   #include <eosiolib/types.hpp>

   using namespace eosio;
   using namespace std;

   //@abi table
   struct table1 {
      uint64_t         field1;
      vector<uint64_t> uints64;
      vector<uint32_t> uints32;
      vector<uint16_t> uints16;
      vector<uint8_t>  uints8;
   };

   )=====";

   const char* abigen_vector_of_POD_abi = R"=====(
   {
     "types": [],
     "structs": [{
         "name": "table1",
         "base": "",
         "fields": [{
             "name": "field1",
             "type": "uint64"
           },{
             "name": "uints64",
             "type": "uint64[]"
           },{
             "name": "uints32",
             "type": "uint32[]"
           },{
             "name": "uints16",
             "type": "uint16[]"
           },{
             "name": "uints8",
             "type": "uint8[]"
           }
         ]
       }
     ],
     "actions": [],
     "tables": [{
         "name": "table1",
         "index_type": "i64",
         "key_names": [
           "field1"
         ],
         "key_types": [
           "uint64"
         ],
         "type": "table1"
       }
     ],
    "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(abigen_vector_of_POD, abigen_vector_of_POD_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_vector_of_structs, abi_gen_helper)
{ try {

   const char* abigen_vector_of_structs = R"=====(
   #include <vector>
   #include <string>
   #include <eosiolib/types.hpp>

   using namespace eosio;
   using namespace std;

   struct my_struct {
      vector<uint64_t> uints64;
      vector<uint32_t> uints32;
      vector<uint16_t> uints16;
      vector<uint8_t>  uints8;
      string           str;
   };

   //@abi table
   struct table1 {
      uint64_t          field1;
      vector<my_struct> field2;
   };

   )=====";

   const char* abigen_vector_of_structs_abi = R"=====(
   {
     "types": [],
     "structs": [{
         "name": "my_struct",
         "base": "",
         "fields": [{
             "name": "uints64",
             "type": "uint64[]"
           },{
             "name": "uints32",
             "type": "uint32[]"
           },{
             "name": "uints16",
             "type": "uint16[]"
           },{
             "name": "uints8",
             "type": "uint8[]"
           },{
             "name": "str",
             "type": "string"
           }
         ]
       },{
         "name": "table1",
         "base": "",
         "fields": [{
             "name": "field1",
             "type": "uint64"
           },{
             "name": "field2",
             "type": "my_struct[]"
           }
         ]
       }
     ],
     "actions": [],
     "tables": [{
         "name": "table1",
         "index_type": "i64",
         "key_names": [
           "field1"
         ],
         "key_types": [
           "uint64"
         ],
         "type": "table1"
       }
     ],
    "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(abigen_vector_of_structs, abigen_vector_of_structs_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abigen_vector_multidimension, abi_gen_helper)
{ try {

   const char* abigen_vector_multidimension = R"=====(
   #include <vector>
   #include <string>
   #include <eosiolib/types.hpp>

   using namespace eosio;
   using namespace std;

   //@abi table
   struct table1 {
      uint64_t                 field1;
      vector<vector<uint64_t>> field2;
   };

   )=====";

   BOOST_CHECK_EXCEPTION( generate_abi(abigen_vector_multidimension, ""), eosio::abi_generation_exception, abi_gen_helper::is_abi_generation_exception );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abgigen_vector_alias, abi_gen_helper)
{ try {

   const char* abgigen_vector_alias = R"=====(
   #include <string>
   #include <vector>
   #include <eosiolib/types.hpp>
   #include <eosiolib/print.hpp>

   using namespace std;

   struct row {
    std::vector<uint32_t> cells;
   };

   typedef vector<row> array_of_rows;

   //@abi action
   struct my_action {
     uint64_t id;
     array_of_rows rows;
   };

   )=====";

   const char* abgigen_vector_alias_abi = R"=====(
   {
     "types": [{
         "new_type_name": "array_of_rows",
         "type": "row[]"
       }
     ],
     "structs": [{
         "name": "row",
         "base": "",
         "fields": [{
             "name": "cells",
             "type": "uint32[]"
           }
         ]
       },{
         "name": "my_action",
         "base": "",
         "fields": [{
             "name": "id",
             "type": "uint64"
           },{
             "name": "rows",
             "type": "array_of_rows"
           }
         ]
       }
     ],
     "actions": [{
         "name": "myaction",
         "type": "my_action",
         "ricardian_contract": ""
       }
     ],
     "tables": [],
     "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(abgigen_vector_alias, abgigen_vector_alias_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abgigen_eosioabi_macro, abi_gen_helper)
{ try {

   const char* abgigen_eosioabi_macro = R"=====(

      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wpointer-bool-conversion"

      #include <eosiolib/eosio.hpp>
      #include <eosiolib/print.hpp>


      using namespace eosio;

      struct hello : public eosio::contract {
        public:
            using contract::contract;

            void hi( account_name user ) {
               print( "Hello, ", name{user} );
            }

            void bye( account_name user ) {
               print( "Bye, ", name{user} );
            }
      };

      EOSIO_ABI(hello,(hi))

      #pragma GCC diagnostic pop

   )=====";

   const char* abgigen_eosioabi_macro_abi = R"=====(
   {
     "types": [],
     "structs": [{
         "name": "hi",
         "base": "",
         "fields": [{
             "name": "user",
             "type": "account_name"
           }
         ]
       }
     ],
     "actions": [{
         "name": "hi",
         "type": "hi"
       }
     ],
     "tables": [],
     "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(abgigen_eosioabi_macro, abgigen_eosioabi_macro_abi) == true );

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(abgigen_contract_inheritance, abi_gen_helper)
{ try {

   const char* abgigen_contract_inheritance = R"=====(
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wpointer-bool-conversion"

      #include <eosiolib/eosio.hpp>
      #include <eosiolib/print.hpp>


      using namespace eosio;

      struct hello : public eosio::contract {
        public:
            using contract::contract;

            void hi( account_name user ) {
               print( "Hello, ", name{user} );
            }
      };

      struct new_hello : hello {
        public:
            new_hello(account_name self) : hello(self) {}
            void bye( account_name user ) {
               print( "Bye, ", name{user} );
            }
      };

      EOSIO_ABI(new_hello,(hi)(bye))

      #pragma GCC diagnostic pop
   )=====";

   const char* abgigen_contract_inheritance_abi = R"=====(
   {
     "types": [],
     "structs": [{
         "name": "hi",
         "base": "",
         "fields": [{
             "name": "user",
             "type": "account_name"
           }
         ]
       },{
         "name": "bye",
         "base": "",
         "fields": [{
             "name": "user",
             "type": "account_name"
           }
         ]
       }
     ],
     "actions": [{
         "name": "hi",
         "type": "hi"
       },{
         "name": "bye",
         "type": "bye"
       }
     ],
     "tables": [],
     "ricardian_clauses": []
   }
   )=====";

   BOOST_TEST( generate_abi(abgigen_contract_inheritance, abgigen_contract_inheritance_abi) == true );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(general)
{ try {

   auto abi = chain_initializer::eos_contract_abi(fc::json::from_string(my_abi).as<abi_def>());

   abi_serializer abis(abi);
   abis.validate();

   const char *my_other = R"=====(
    {
      "publickey"     :  "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "publickey_arr" :  ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"],
      "asset"         : "100.0000 EOS",
      "asset_arr"     : ["100.0000 EOS","100.0000 EOS"],

      "string"            : "ola ke ase",
      "string_arr"        : ["ola ke ase","ola ke desi"],
      "time"              : "2021-12-20T15:30",
      "time_arr"          : ["2021-12-20T15:30","2021-12-20T15:31"],
      "signature"         : "EOSJzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwT77X1U",
      "signature_arr"     : ["EOSJzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwT77X1U","EOSJzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwT77X1U"],
      "checksum256"       : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
      "checksum256_arr"      : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"],
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
      "field"             : {"name":"name1", "type":"type1"},
      "field_arr"         : [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}],
      "struct"            : {"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}]},
      "struct_arr"        : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}]},{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}]}],
      "fields"            : [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}],
      "fields_arr"        : [[{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}],[{"name":"name3", "type":"type3"}, {"name":"name4", "type":"type4"}]],
      "accountname"       : "acc1",
      "accountname_arr"   : ["acc1","acc2"],
      "permname"          : "pername",
      "permname_arr"      : ["pername1","pername2"],
      "actionname"        : "actionname",
      "actionname_arr"    : ["actionname1","actionname2"],
      "scopename"         : "acc1",
      "scopename_arr"     : ["acc1","acc2"],
      "permlvl"           : {"actor":"acc1","permission":"permname1"},
      "permlvl_arr"       : [{"actor":"acc1","permission":"permname1"},{"actor":"acc2","permission":"permname2"}],
      "action"            : {"account":"acc1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"},
      "action_arr"        : [{"account":"acc1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"},{"account":"acc2", "name":"actionname2", "authorization":[{"actor":"acc2","permission":"permname2"}], "data":""}],
      "permlvlwgt"        : {"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},
      "permlvlwgt_arr"    : [{"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"actor":"acc2","permission":"permname2"},"weight":"2"}],
      "transaction"       : {
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "region": "1",
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"accountname1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_kcpu_usage":43,
        "delay_sec":0
      },
      "transaction_arr": [{
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "region": "1",
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"acc1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_kcpu_usage":43,
        "delay_sec":0
      },{
        "ref_block_num":"2",
        "ref_block_prefix":"3",
        "expiration":"2021-12-20T15:40",
        "region": "1",
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"acc2", "name":"actionname2", "authorization":[{"actor":"acc2","permission":"permname2"}], "data":""}],
        "max_net_usage_words":21,
        "max_kcpu_usage":87,
        "delay_sec":0
      }],
      "strx": {
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "region": "1",
        "signatures" : ["EOSJzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwT77X1U"],
        "context_free_data" : ["abcdef","0123456789","ABCDEF0123456789abcdef"],
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"accountname1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_kcpu_usage":43,
        "delay_sec":0
      },
      "strx_arr": [{
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "region": "1",
        "signatures" : ["EOSJzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwT77X1U"],
        "context_free_data" : ["abcdef","0123456789","ABCDEF0123456789abcdef"],
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"acc1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_kcpu_usage":43,
        "delay_sec":0
      },{
        "ref_block_num":"2",
        "ref_block_prefix":"3",
        "expiration":"2021-12-20T15:40",
        "region": "1",
        "signatures" : ["EOSJzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwT77X1U"],
        "context_free_data" : ["abcdef","0123456789","ABCDEF0123456789abcdef"],
        "context_free_actions":[{"account":"contextfree2", "name":"cfactionname2", "authorization":[{"actor":"cfacc2","permission":"cfpermname2"}], "data":"667788"}],
        "actions":[{"account":"acc2", "name":"actionname2", "authorization":[{"actor":"acc2","permission":"permname2"}], "data":""}],
        "max_net_usage_words":15,
        "max_kcpu_usage":43,
        "delay_sec":0
      }],
      "keyweight": {"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},
      "keyweight_arr": [{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}],
      "authority": {
         "threshold":"10",
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":100},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":200}],
         "accounts":[{"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"actor":"acc2","permission":"permname2"},"weight":"2"}]
       },
      "authority_arr": [{
         "threshold":"10",
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}],
         "accounts":[{"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"actor":"acc2","permission":"permname2"},"weight":"2"}]
       },{
         "threshold":"10",
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}],
         "accounts":[{"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"actor":"acc2","permission":"permname2"},"weight":"2"}]
       }],
      "typedef" : {"new_type_name":"new", "type":"old"},
      "typedef_arr": [{"new_type_name":"new", "type":"old"},{"new_type_name":"new", "type":"old"}],
      "actiondef"       : {"name":"actionname1", "type":"type1", "ricardian_contract":"ricardian1"},
      "actiondef_arr"   : [{"name":"actionname1", "type":"type1","ricardian_contract":"ricardian1"},{"name":"actionname2", "type":"type2","ricardian_contract":"ricardian2"}],
      "tabledef": {"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"},
      "tabledef_arr": [
         {"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"},
         {"name":"table2","index_type":"indextype2","key_names":["keyname2"],"key_types":["typename2"],"type":"type2"}
      ],
      "abidef":{
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract":""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": []
      },
      "abidef_arr": [{
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract":""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": []
      },{
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract": ""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": []
      }]
    }
   )=====";

   auto var = fc::json::from_string(my_other);
   verify_byte_round_trip_conversion(abi, "A", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_cycle)
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
       "tables": [],
       "ricardian_clauses": []
   }
   )=====";

   const char* struct_cycle_abi = R"=====(
   {
       "types": [],
       "structs": [{
         "name": "A",
         "base": "B",
         "fields": []
       },{
         "name": "B",
         "base": "C",
         "fields": []
       },{
         "name": "C",
         "base": "A",
         "fields": []
       }],
       "actions": [],
       "tables": [],
       "ricardian_clauses": []
   }
   )=====";

   auto abi = chain_initializer::eos_contract_abi(fc::json::from_string(typedef_cycle_abi).as<abi_def>());
   abi_serializer abis(abi);

   auto is_assert_exception = [](fc::assert_exception const & e) -> bool {
      wlog(e.to_string()); return true;
   };
   BOOST_CHECK_EXCEPTION( abis.validate(), fc::assert_exception, is_assert_exception );

   abi = fc::json::from_string(struct_cycle_abi).as<abi_def>();
   abis.set_abi(abi);
   BOOST_CHECK_EXCEPTION( abis.validate(), fc::assert_exception, is_assert_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(linkauth)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

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

   auto linkauth = var.as<contracts::linkauth>();
   BOOST_TEST("lnkauth.acct" == linkauth.account);
   BOOST_TEST("lnkauth.code" == linkauth.code);
   BOOST_TEST("lnkauth.type" == linkauth.type);
   BOOST_TEST("lnkauth.rqm" == linkauth.requirement);

   auto var2 = verify_byte_round_trip_conversion( abis, "linkauth", var );
   auto linkauth2 = var2.as<contracts::linkauth>();
   BOOST_TEST(linkauth.account == linkauth2.account);
   BOOST_TEST(linkauth.code == linkauth2.code);
   BOOST_TEST(linkauth.type == linkauth2.type);
   BOOST_TEST(linkauth.requirement == linkauth2.requirement);

   verify_type_round_trip_conversion<contracts::linkauth>( abis, "linkauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(unlinkauth)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "lnkauth.acct",
     "code" : "lnkauth.code",
     "type" : "lnkauth.type",
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto unlinkauth = var.as<contracts::unlinkauth>();
   BOOST_TEST("lnkauth.acct" == unlinkauth.account);
   BOOST_TEST("lnkauth.code" == unlinkauth.code);
   BOOST_TEST("lnkauth.type" == unlinkauth.type);

   auto var2 = verify_byte_round_trip_conversion( abis, "unlinkauth", var );
   auto unlinkauth2 = var2.as<contracts::unlinkauth>();
   BOOST_TEST(unlinkauth.account == unlinkauth2.account);
   BOOST_TEST(unlinkauth.code == unlinkauth2.code);
   BOOST_TEST(unlinkauth.type == unlinkauth2.type);

   verify_type_round_trip_conversion<contracts::unlinkauth>( abis, "unlinkauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(updateauth)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "updauth.acct",
     "permission" : "updauth.prm",
     "parent" : "updauth.prnt",
     "data" : {
        "threshold" : "2147483145",
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"actor" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"actor" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     },
     "delay" : 0
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto updateauth = var.as<contracts::updateauth>();
   BOOST_TEST("updauth.acct" == updateauth.account);
   BOOST_TEST("updauth.prm" == updateauth.permission);
   BOOST_TEST("updauth.prnt" == updateauth.parent);
   BOOST_TEST(2147483145u == updateauth.data.threshold);

   BOOST_TEST_REQUIRE(2 == updateauth.data.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == (std::string)updateauth.data.keys[0].key);
   BOOST_TEST(57005u == updateauth.data.keys[0].weight);
   BOOST_TEST("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf" == (std::string)updateauth.data.keys[1].key);
   BOOST_TEST(57605u == updateauth.data.keys[1].weight);

   BOOST_TEST_REQUIRE(2 == updateauth.data.accounts.size());
   BOOST_TEST("prm.acct1" == updateauth.data.accounts[0].permission.actor);
   BOOST_TEST("prm.prm1" == updateauth.data.accounts[0].permission.permission);
   BOOST_TEST(53005u == updateauth.data.accounts[0].weight);
   BOOST_TEST("prm.acct2" == updateauth.data.accounts[1].permission.actor);
   BOOST_TEST("prm.prm2" == updateauth.data.accounts[1].permission.permission);
   BOOST_TEST(53405u == updateauth.data.accounts[1].weight);

   auto var2 = verify_byte_round_trip_conversion( abis, "updateauth", var );
   auto updateauth2 = var2.as<contracts::updateauth>();
   BOOST_TEST(updateauth.account == updateauth2.account);
   BOOST_TEST(updateauth.permission == updateauth2.permission);
   BOOST_TEST(updateauth.parent == updateauth2.parent);

   BOOST_TEST(updateauth.data.threshold == updateauth2.data.threshold);

   BOOST_TEST_REQUIRE(updateauth.data.keys.size() == updateauth2.data.keys.size());
   BOOST_TEST(updateauth.data.keys[0].key == updateauth2.data.keys[0].key);
   BOOST_TEST(updateauth.data.keys[0].weight == updateauth2.data.keys[0].weight);
   BOOST_TEST(updateauth.data.keys[1].key == updateauth2.data.keys[1].key);
   BOOST_TEST(updateauth.data.keys[1].weight == updateauth2.data.keys[1].weight);

   BOOST_TEST_REQUIRE(updateauth.data.accounts.size() == updateauth2.data.accounts.size());
   BOOST_TEST(updateauth.data.accounts[0].permission.actor == updateauth2.data.accounts[0].permission.actor);
   BOOST_TEST(updateauth.data.accounts[0].permission.permission == updateauth2.data.accounts[0].permission.permission);
   BOOST_TEST(updateauth.data.accounts[0].weight == updateauth2.data.accounts[0].weight);
   BOOST_TEST(updateauth.data.accounts[1].permission.actor == updateauth2.data.accounts[1].permission.actor);
   BOOST_TEST(updateauth.data.accounts[1].permission.permission == updateauth2.data.accounts[1].permission.permission);
   BOOST_TEST(updateauth.data.accounts[1].weight == updateauth2.data.accounts[1].weight);

   verify_type_round_trip_conversion<contracts::updateauth>( abis, "updateauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(deleteauth)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "delauth.acct",
     "permission" : "delauth.prm"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto deleteauth = var.as<contracts::deleteauth>();
   BOOST_TEST("delauth.acct" == deleteauth.account);
   BOOST_TEST("delauth.prm" == deleteauth.permission);

   auto var2 = verify_byte_round_trip_conversion( abis, "deleteauth", var );
   auto deleteauth2 = var2.as<contracts::deleteauth>();
   BOOST_TEST(deleteauth.account == deleteauth2.account);
   BOOST_TEST(deleteauth.permission == deleteauth2.permission);

   verify_type_round_trip_conversion<contracts::deleteauth>( abis, "deleteauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(newaccount)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "creator" : "newacct.crtr",
     "name" : "newacct.name",
     "owner" : {
        "threshold" : 2147483145,
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"actor" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"actor" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     },
     "active" : {
        "threshold" : 2146483145,
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"actor" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"actor" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     },
     "recovery" : {
        "threshold" : 2145483145,
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"actor" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"actor" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     }
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto newaccount = var.as<contracts::newaccount>();
   BOOST_TEST("newacct.crtr" == newaccount.creator);
   BOOST_TEST("newacct.name" == newaccount.name);

   BOOST_TEST(2147483145u == newaccount.owner.threshold);

   BOOST_TEST_REQUIRE(2 == newaccount.owner.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == (std::string)newaccount.owner.keys[0].key);
   BOOST_TEST(57005u == newaccount.owner.keys[0].weight);
   BOOST_TEST("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf" == (std::string)newaccount.owner.keys[1].key);
   BOOST_TEST(57605u == newaccount.owner.keys[1].weight);

   BOOST_TEST_REQUIRE(2 == newaccount.owner.accounts.size());
   BOOST_TEST("prm.acct1" == newaccount.owner.accounts[0].permission.actor);
   BOOST_TEST("prm.prm1" == newaccount.owner.accounts[0].permission.permission);
   BOOST_TEST(53005u == newaccount.owner.accounts[0].weight);
   BOOST_TEST("prm.acct2" == newaccount.owner.accounts[1].permission.actor);
   BOOST_TEST("prm.prm2" == newaccount.owner.accounts[1].permission.permission);
   BOOST_TEST(53405u == newaccount.owner.accounts[1].weight);

   BOOST_TEST(2146483145u == newaccount.active.threshold);

   BOOST_TEST_REQUIRE(2 == newaccount.active.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == (std::string)newaccount.active.keys[0].key);
   BOOST_TEST(57005u == newaccount.active.keys[0].weight);
   BOOST_TEST("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf" == (std::string)newaccount.active.keys[1].key);
   BOOST_TEST(57605u == newaccount.active.keys[1].weight);

   BOOST_TEST_REQUIRE(2 == newaccount.active.accounts.size());
   BOOST_TEST("prm.acct1" == newaccount.active.accounts[0].permission.actor);
   BOOST_TEST("prm.prm1" == newaccount.active.accounts[0].permission.permission);
   BOOST_TEST(53005u == newaccount.active.accounts[0].weight);
   BOOST_TEST("prm.acct2" == newaccount.active.accounts[1].permission.actor);
   BOOST_TEST("prm.prm2" == newaccount.active.accounts[1].permission.permission);
   BOOST_TEST(53405u == newaccount.active.accounts[1].weight);

   BOOST_TEST(2145483145u == newaccount.recovery.threshold);

   BOOST_TEST_REQUIRE(2 == newaccount.recovery.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == (std::string)newaccount.recovery.keys[0].key);
   BOOST_TEST(57005u == newaccount.recovery.keys[0].weight);
   BOOST_TEST("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf" == (std::string)newaccount.recovery.keys[1].key);
   BOOST_TEST(57605u == newaccount.recovery.keys[1].weight);

   BOOST_TEST_REQUIRE(2 == newaccount.recovery.accounts.size());
   BOOST_TEST("prm.acct1" == newaccount.recovery.accounts[0].permission.actor);
   BOOST_TEST("prm.prm1" == newaccount.recovery.accounts[0].permission.permission);
   BOOST_TEST(53005u == newaccount.recovery.accounts[0].weight);
   BOOST_TEST("prm.acct2" == newaccount.recovery.accounts[1].permission.actor);
   BOOST_TEST("prm.prm2" == newaccount.recovery.accounts[1].permission.permission);
   BOOST_TEST(53405u == newaccount.recovery.accounts[1].weight);

   auto var2 = verify_byte_round_trip_conversion( abis, "newaccount", var );
   auto newaccount2 = var2.as<contracts::newaccount>();
   BOOST_TEST(newaccount.creator == newaccount2.creator);
   BOOST_TEST(newaccount.name == newaccount2.name);

   BOOST_TEST(newaccount.owner.threshold == newaccount2.owner.threshold);

   BOOST_TEST_REQUIRE(newaccount.owner.keys.size() == newaccount2.owner.keys.size());
   BOOST_TEST(newaccount.owner.keys[0].key == newaccount2.owner.keys[0].key);
   BOOST_TEST(newaccount.owner.keys[0].weight == newaccount2.owner.keys[0].weight);
   BOOST_TEST(newaccount.owner.keys[1].key == newaccount2.owner.keys[1].key);
   BOOST_TEST(newaccount.owner.keys[1].weight == newaccount2.owner.keys[1].weight);

   BOOST_TEST_REQUIRE(newaccount.owner.accounts.size() == newaccount2.owner.accounts.size());
   BOOST_TEST(newaccount.owner.accounts[0].permission.actor == newaccount2.owner.accounts[0].permission.actor);
   BOOST_TEST(newaccount.owner.accounts[0].permission.permission == newaccount2.owner.accounts[0].permission.permission);
   BOOST_TEST(newaccount.owner.accounts[0].weight == newaccount2.owner.accounts[0].weight);
   BOOST_TEST(newaccount.owner.accounts[1].permission.actor == newaccount2.owner.accounts[1].permission.actor);
   BOOST_TEST(newaccount.owner.accounts[1].permission.permission == newaccount2.owner.accounts[1].permission.permission);
   BOOST_TEST(newaccount.owner.accounts[1].weight == newaccount2.owner.accounts[1].weight);

   BOOST_TEST(newaccount.active.threshold == newaccount2.active.threshold);

   BOOST_TEST_REQUIRE(newaccount.active.keys.size() == newaccount2.active.keys.size());
   BOOST_TEST(newaccount.active.keys[0].key == newaccount2.active.keys[0].key);
   BOOST_TEST(newaccount.active.keys[0].weight == newaccount2.active.keys[0].weight);
   BOOST_TEST(newaccount.active.keys[1].key == newaccount2.active.keys[1].key);
   BOOST_TEST(newaccount.active.keys[1].weight == newaccount2.active.keys[1].weight);

   BOOST_TEST_REQUIRE(newaccount.active.accounts.size() == newaccount2.active.accounts.size());
   BOOST_TEST(newaccount.active.accounts[0].permission.actor == newaccount2.active.accounts[0].permission.actor);
   BOOST_TEST(newaccount.active.accounts[0].permission.permission == newaccount2.active.accounts[0].permission.permission);
   BOOST_TEST(newaccount.active.accounts[0].weight == newaccount2.active.accounts[0].weight);
   BOOST_TEST(newaccount.active.accounts[1].permission.actor == newaccount2.active.accounts[1].permission.actor);
   BOOST_TEST(newaccount.active.accounts[1].permission.permission == newaccount2.active.accounts[1].permission.permission);
   BOOST_TEST(newaccount.active.accounts[1].weight == newaccount2.active.accounts[1].weight);

   BOOST_TEST(newaccount.recovery.threshold == newaccount2.recovery.threshold);

   BOOST_TEST_REQUIRE(newaccount.recovery.keys.size() == newaccount2.recovery.keys.size());
   BOOST_TEST(newaccount.recovery.keys[0].key == newaccount2.recovery.keys[0].key);
   BOOST_TEST(newaccount.recovery.keys[0].weight == newaccount2.recovery.keys[0].weight);
   BOOST_TEST(newaccount.recovery.keys[1].key == newaccount2.recovery.keys[1].key);
   BOOST_TEST(newaccount.recovery.keys[1].weight == newaccount2.recovery.keys[1].weight);

   BOOST_TEST_REQUIRE(newaccount.recovery.accounts.size() == newaccount2.recovery.accounts.size());
   BOOST_TEST(newaccount.recovery.accounts[0].permission.actor == newaccount2.recovery.accounts[0].permission.actor);
   BOOST_TEST(newaccount.recovery.accounts[0].permission.permission == newaccount2.recovery.accounts[0].permission.permission);
   BOOST_TEST(newaccount.recovery.accounts[0].weight == newaccount2.recovery.accounts[0].weight);
   BOOST_TEST(newaccount.recovery.accounts[1].permission.actor == newaccount2.recovery.accounts[1].permission.actor);
   BOOST_TEST(newaccount.recovery.accounts[1].permission.permission == newaccount2.recovery.accounts[1].permission.permission);
   BOOST_TEST(newaccount.recovery.accounts[1].weight == newaccount2.recovery.accounts[1].weight);

   verify_type_round_trip_conversion<contracts::newaccount>( abis, "newaccount", var);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(setcode)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   const char* test_data = R"=====(
   {
     "account" : "setcode.acc",
     "vmtype" : "0",
     "vmversion" : "0",
     "code" : "0061736d0100000001390a60037e7e7f017f60047e7e7f7f017f60017e0060057e7e7e7f7f"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto setcode = var.as<contracts::setcode>();
   BOOST_TEST("setcode.acc" == setcode.account);
   BOOST_TEST(0 == setcode.vmtype);
   BOOST_TEST(0 == setcode.vmversion);
   BOOST_TEST("0061736d0100000001390a60037e7e7f017f60047e7e7f7f017f60017e0060057e7e7e7f7f" == fc::to_hex(setcode.code.data(), setcode.code.size()));

   auto var2 = verify_byte_round_trip_conversion( abis, "setcode", var );
   auto setcode2 = var2.as<contracts::setcode>();
   BOOST_TEST(setcode.account == setcode2.account);
   BOOST_TEST(setcode.vmtype == setcode2.vmtype);
   BOOST_TEST(setcode.vmversion == setcode2.vmversion);
   BOOST_TEST(setcode.code == setcode2.code);

   verify_type_round_trip_conversion<contracts::setcode>( abis, "setcode", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(setabi)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   const char* test_data = R"=====(
   {
      "account": "setabi.acc",
      "abi":  {
        "types": [{
            "new_type_name": "account_name",
            "type": "name"
          }
        ],
        "structs": [{
            "name": "transfer_base",
            "base": "",
            "fields": [{
               "name": "memo",
               "type": "string"
            }]
          },{
            "name": "transfer",
            "base": "transfer_base",
            "fields": [{
               "name": "from",
               "type": "account_name"
            },{
               "name": "to",
               "type": "account_name"
            },{
               "name": "amount",
               "type": "uint64"
            }]
          },{
            "name": "account",
            "base": "",
            "fields": [{
               "name": "account",
               "type": "name"
            },{
               "name": "balance",
               "type": "uint64"
            }]
          }
        ],
        "actions": [{
            "name": "transfer",
            "type": "transfer",
            "ricardian_contract": "transfer contract"
          }
        ],
        "tables": [{
            "name": "account",
            "type": "account",
            "index_type": "i64",
            "key_names" : ["account"],
            "key_types" : ["name"]
          }
        ],
       "ricardian_clauses": []
      }
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto setabi = var.as<contracts::setabi>();
   BOOST_TEST("setabi.acc" == setabi.account);

   BOOST_TEST_REQUIRE(1 == setabi.abi.types.size());

   BOOST_TEST("account_name" == setabi.abi.types[0].new_type_name);
   BOOST_TEST("name" == setabi.abi.types[0].type);

   BOOST_TEST_REQUIRE(3 == setabi.abi.structs.size());

   BOOST_TEST("transfer_base" == setabi.abi.structs[0].name);
   BOOST_TEST("" == setabi.abi.structs[0].base);
   BOOST_TEST_REQUIRE(1 == setabi.abi.structs[0].fields.size());
   BOOST_TEST("memo" == setabi.abi.structs[0].fields[0].name);
   BOOST_TEST("string" == setabi.abi.structs[0].fields[0].type);

   BOOST_TEST("transfer" == setabi.abi.structs[1].name);
   BOOST_TEST("transfer_base" == setabi.abi.structs[1].base);
   BOOST_TEST_REQUIRE(3 == setabi.abi.structs[1].fields.size());
   BOOST_TEST("from" == setabi.abi.structs[1].fields[0].name);
   BOOST_TEST("account_name" == setabi.abi.structs[1].fields[0].type);
   BOOST_TEST("to" == setabi.abi.structs[1].fields[1].name);
   BOOST_TEST("account_name" == setabi.abi.structs[1].fields[1].type);
   BOOST_TEST("amount" == setabi.abi.structs[1].fields[2].name);
   BOOST_TEST("uint64" == setabi.abi.structs[1].fields[2].type);

   BOOST_TEST("account" == setabi.abi.structs[2].name);
   BOOST_TEST("" == setabi.abi.structs[2].base);
   BOOST_TEST_REQUIRE(2 == setabi.abi.structs[2].fields.size());
   BOOST_TEST("account" == setabi.abi.structs[2].fields[0].name);
   BOOST_TEST("name" == setabi.abi.structs[2].fields[0].type);
   BOOST_TEST("balance" == setabi.abi.structs[2].fields[1].name);
   BOOST_TEST("uint64" == setabi.abi.structs[2].fields[1].type);

   BOOST_TEST_REQUIRE(1 == setabi.abi.actions.size());
   BOOST_TEST("transfer" == setabi.abi.actions[0].name);
   BOOST_TEST("transfer" == setabi.abi.actions[0].type);

   BOOST_TEST_REQUIRE(1 == setabi.abi.tables.size());
   BOOST_TEST("account" == setabi.abi.tables[0].name);
   BOOST_TEST("account" == setabi.abi.tables[0].type);
   BOOST_TEST("i64" == setabi.abi.tables[0].index_type);
   BOOST_TEST_REQUIRE(1 == setabi.abi.tables[0].key_names.size());
   BOOST_TEST("account" == setabi.abi.tables[0].key_names[0]);
   BOOST_TEST_REQUIRE(1 == setabi.abi.tables[0].key_types.size());
   BOOST_TEST("name" == setabi.abi.tables[0].key_types[0]);

   auto var2 = verify_byte_round_trip_conversion( abis, "setabi", var );
   auto setabi2 = var2.as<contracts::setabi>();

   BOOST_TEST(setabi.account == setabi2.account);

   BOOST_TEST_REQUIRE(setabi.abi.types.size() == setabi2.abi.types.size());

   BOOST_TEST(setabi.abi.types[0].new_type_name == setabi2.abi.types[0].new_type_name);
   BOOST_TEST(setabi.abi.types[0].type == setabi2.abi.types[0].type);

   BOOST_TEST_REQUIRE(setabi.abi.structs.size() == setabi2.abi.structs.size());

   BOOST_TEST(setabi.abi.structs[0].name == setabi2.abi.structs[0].name);
   BOOST_TEST(setabi.abi.structs[0].base == setabi2.abi.structs[0].base);
   BOOST_TEST_REQUIRE(setabi.abi.structs[0].fields.size() == setabi2.abi.structs[0].fields.size());
   BOOST_TEST(setabi.abi.structs[0].fields[0].name == setabi2.abi.structs[0].fields[0].name);
   BOOST_TEST(setabi.abi.structs[0].fields[0].type == setabi2.abi.structs[0].fields[0].type);

   BOOST_TEST(setabi.abi.structs[1].name == setabi2.abi.structs[1].name);
   BOOST_TEST(setabi.abi.structs[1].base == setabi2.abi.structs[1].base);
   BOOST_TEST_REQUIRE(setabi.abi.structs[1].fields.size() == setabi2.abi.structs[1].fields.size());
   BOOST_TEST(setabi.abi.structs[1].fields[0].name == setabi2.abi.structs[1].fields[0].name);
   BOOST_TEST(setabi.abi.structs[1].fields[0].type == setabi2.abi.structs[1].fields[0].type);
   BOOST_TEST(setabi.abi.structs[1].fields[1].name == setabi2.abi.structs[1].fields[1].name);
   BOOST_TEST(setabi.abi.structs[1].fields[1].type == setabi2.abi.structs[1].fields[1].type);
   BOOST_TEST(setabi.abi.structs[1].fields[2].name == setabi2.abi.structs[1].fields[2].name);
   BOOST_TEST(setabi.abi.structs[1].fields[2].type == setabi2.abi.structs[1].fields[2].type);

   BOOST_TEST(setabi.abi.structs[2].name == setabi2.abi.structs[2].name);
   BOOST_TEST(setabi.abi.structs[2].base == setabi2.abi.structs[2].base);
   BOOST_TEST_REQUIRE(setabi.abi.structs[2].fields.size() == setabi2.abi.structs[2].fields.size());
   BOOST_TEST(setabi.abi.structs[2].fields[0].name == setabi2.abi.structs[2].fields[0].name);
   BOOST_TEST(setabi.abi.structs[2].fields[0].type == setabi2.abi.structs[2].fields[0].type);
   BOOST_TEST(setabi.abi.structs[2].fields[1].name == setabi2.abi.structs[2].fields[1].name);
   BOOST_TEST(setabi.abi.structs[2].fields[1].type == setabi2.abi.structs[2].fields[1].type);

   BOOST_TEST_REQUIRE(setabi.abi.actions.size() == setabi2.abi.actions.size());
   BOOST_TEST(setabi.abi.actions[0].name == setabi2.abi.actions[0].name);
   BOOST_TEST(setabi.abi.actions[0].type == setabi2.abi.actions[0].type);

   BOOST_TEST_REQUIRE(setabi.abi.tables.size() == setabi2.abi.tables.size());
   BOOST_TEST(setabi.abi.tables[0].name == setabi2.abi.tables[0].name);
   BOOST_TEST(setabi.abi.tables[0].type == setabi2.abi.tables[0].type);
   BOOST_TEST(setabi.abi.tables[0].index_type == setabi2.abi.tables[0].index_type);
   BOOST_TEST_REQUIRE(setabi.abi.tables[0].key_names.size() == setabi2.abi.tables[0].key_names.size());
   BOOST_TEST(setabi.abi.tables[0].key_names[0] == setabi2.abi.tables[0].key_names[0]);
   BOOST_TEST_REQUIRE(setabi.abi.tables[0].key_types.size() == setabi2.abi.tables[0].key_types.size());
   BOOST_TEST(setabi.abi.tables[0].key_types[0] == setabi2.abi.tables[0].key_types[0]);

   verify_type_round_trip_conversion<contracts::setabi>( abis, "setabi", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(postrecovery)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   const char* test_data = R"=====(
   {
     "account" : "postrec.acc",
     "data": {
        "threshold" : "2147483145",
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005} ],
        "accounts" : [ {"permission" : {"actor" : "postrec.acc", "permission" : "prm.prm1"}, "weight" : 57005 } ]
     }
     "memo": "postrec.memo"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto postrecovery = var.as<contracts::postrecovery>();
   BOOST_TEST("postrec.acc" == postrecovery.account);
   BOOST_TEST(2147483145u == postrecovery.data.threshold);

   BOOST_TEST_REQUIRE(1 == postrecovery.data.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == (std::string)postrecovery.data.keys[0].key);
   BOOST_TEST(57005u == postrecovery.data.keys[0].weight);

   BOOST_TEST_REQUIRE(1 == postrecovery.data.accounts.size());
   BOOST_TEST("postrec.acc" == postrecovery.data.accounts[0].permission.actor);
   BOOST_TEST("prm.prm1" == postrecovery.data.accounts[0].permission.permission);
   BOOST_TEST(57005u == postrecovery.data.accounts[0].weight);
   BOOST_TEST("postrec.memo" == postrecovery.memo);

   auto var2 = verify_byte_round_trip_conversion( abis, "postrecovery", var );
   auto postrecovery2 = var2.as<contracts::postrecovery>();
   BOOST_TEST(postrecovery.account == postrecovery2.account);
   BOOST_TEST(postrecovery.data.threshold == postrecovery2.data.threshold);

   BOOST_TEST_REQUIRE(postrecovery.data.keys.size() == postrecovery2.data.keys.size());
   BOOST_TEST(postrecovery.data.keys[0].key == postrecovery2.data.keys[0].key);
   BOOST_TEST(postrecovery.data.keys[0].weight == postrecovery2.data.keys[0].weight);

   BOOST_TEST_REQUIRE(postrecovery.data.accounts.size() == postrecovery2.data.accounts.size());
   BOOST_TEST(postrecovery.data.accounts[0].permission.actor == postrecovery2.data.accounts[0].permission.actor);
   BOOST_TEST(postrecovery.data.accounts[0].permission.permission == postrecovery2.data.accounts[0].permission.permission);
   BOOST_TEST(postrecovery.data.accounts[0].weight == postrecovery2.data.accounts[0].weight);
   BOOST_TEST(postrecovery.memo == postrecovery2.memo);

   verify_type_round_trip_conversion<contracts::postrecovery>( abis, "postrecovery", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(passrecovery)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   const char* test_data = R"=====(
   {
     "account" : "passrec.acc"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto passrecovery = var.as<contracts::passrecovery>();
   BOOST_TEST("passrec.acc" == passrecovery.account);

   auto var2 = verify_byte_round_trip_conversion( abis, "passrecovery", var );
   auto passrecovery2 = var2.as<contracts::passrecovery>();
   BOOST_TEST(passrecovery.account == passrecovery2.account);

   verify_type_round_trip_conversion<contracts::passrecovery>( abis, "passrecovery", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(vetorecovery)
{ try {

   abi_serializer abis(chain_initializer::eos_contract_abi(abi_def()));

   const char* test_data = R"=====(
   {
     "account" : "vetorec.acc"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto vetorecovery = var.as<contracts::vetorecovery>();
   BOOST_TEST("vetorec.acc" == vetorecovery.account);

   auto var2 = verify_byte_round_trip_conversion( abis, "vetorecovery", var );
   auto vetorecovery2 = var2.as<contracts::vetorecovery>();
   BOOST_TEST(vetorecovery.account == vetorecovery2.account);

   verify_type_round_trip_conversion<contracts::vetorecovery>( abis, "vetorecovery", var);

} FC_LOG_AND_RETHROW() }

struct action1 {
   action1() = default;
   action1(uint64_t b1, uint32_t b2, uint8_t b3) : blah1(b1), blah2(b2), blah3(b3) {}
   uint64_t blah1;
   uint32_t blah2;
   uint8_t blah3;
   static account_name get_account() { return N(acount1); }
   static account_name get_name() { return N(action1); }

   template<typename Stream>
   friend Stream& operator<<( Stream& ds, const action1& act ) {
     ds << act.blah1 << act.blah2 << act.blah3;
     return ds;
   }

   template<typename Stream>
   friend Stream& operator>>( Stream& ds, action1& act ) {
      ds >> act.blah1 >> act.blah2 >> act.blah3;
     return ds;
   }
};

struct action2 {
   action2() = default;
   action2(uint32_t b1, uint64_t b2, uint8_t b3) : blah1(b1), blah2(b2), blah3(b3) {}
   uint32_t blah1;
   uint64_t blah2;
   uint8_t blah3;
   static account_name get_account() { return N(acount2); }
   static account_name get_name() { return N(action2); }

   template<typename Stream>
   friend Stream& operator<<( Stream& ds, const action2& act ) {
     ds << act.blah1 << act.blah2 << act.blah3;
     return ds;
   }

   template<typename Stream>
   friend Stream& operator>>( Stream& ds, action2& act ) {
      ds >> act.blah1 >> act.blah2 >> act.blah3;
     return ds;
   }
};

template<typename T>
void verify_action_equal(const chain::action& exp, const chain::action& act)
{
   BOOST_REQUIRE_EQUAL((std::string)exp.account, (std::string)act.account);
   BOOST_REQUIRE_EQUAL((std::string)exp.name, (std::string)act.name);
   BOOST_REQUIRE_EQUAL(exp.authorization.size(), act.authorization.size());
   for(unsigned int i = 0; i < exp.authorization.size(); ++i)
   {
      BOOST_REQUIRE_EQUAL((std::string)exp.authorization[i].actor, (std::string)act.authorization[i].actor);
      BOOST_REQUIRE_EQUAL((std::string)exp.authorization[i].permission, (std::string)act.authorization[i].permission);
   }
   BOOST_REQUIRE_EQUAL(exp.data.size(), act.data.size());
   BOOST_REQUIRE(!memcmp(exp.data.data(), act.data.data(), exp.data.size()));
}

private_key_type get_private_key( name keyname, string role ) {
   return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(string(keyname)+role));
}

public_key_type  get_public_key( name keyname, string role ) {
   return get_private_key( keyname, role ).get_public_key();
}

// This test causes the pack logic performed using the FC_REFLECT defined packing (because of
// packed_transaction::data), to be combined with the unpack logic performed using the abi_serializer,
// and thus the abi_def for non-built-in-types.  This test will expose if any of the transaction and
// its sub-types have different packing/unpacking orders in FC_REFLECT vs. their abi_def
BOOST_AUTO_TEST_CASE(packed_transaction)
{ try {

   chain::transaction txn;
   txn.ref_block_num = 1;
   txn.ref_block_prefix = 2;
   txn.expiration.from_iso_string("2021-12-20T15:30");
   txn.region = 1;
   name a = N(alice);
   txn.context_free_actions.emplace_back(
         vector<permission_level>{{N(testapi1), config::active_name}},
         contracts::newaccount{
               .creator  = config::system_account_name,
               .name     = a,
               .owner    = authority( get_public_key( a, "owner" )),
               .active   = authority( get_public_key( a, "active" ) ),
               .recovery = authority( get_public_key( a, "recovery" ) ),
         });
   txn.context_free_actions.emplace_back(
         vector<permission_level>{{N(testapi2), config::active_name}},
         action1{ 15, 23, (uint8_t)3});
   txn.actions.emplace_back(
         vector<permission_level>{{N(testapi3), config::active_name}},
         action2{ 42, 67, (uint8_t)1});
   txn.actions.emplace_back(
         vector<permission_level>{{N(testapi4), config::active_name}},
         action2{ 61, 23, (uint8_t)2});
   txn.max_net_usage_words = 15;
   txn.max_kcpu_usage = 43;

   // pack the transaction to verify that the var unpacking logic is correct
   auto packed_txn = chain::packed_transaction(txn);

   const char* packed_transaction_abi = R"=====(
   {
       "types": [{
          "new_type_name": "compression_type",
          "type": "int64"
        }],
       "structs": [{
          "name": "packed_transaction",
          "base": "",
          "fields": [{
             "name": "signatures",
             "type": "signature[]"
          },{
             "name": "compression",
             "type": "compression_type"
          },{
             "name": "data",
             "type": "bytes"
          }]
       },{
          "name": "action1",
          "base": "",
          "fields": [{
             "name": "blah1",
             "type": "uint64"
          },{
             "name": "blah2",
             "type": "uint32"
          },{
             "name": "blah3",
             "type": "uint8"
          }]
       },{
          "name": "action2",
          "base": "",
          "fields": [{
             "name": "blah1",
             "type": "uint32"
          },{
             "name": "blah2",
             "type": "uint64"
          },{
             "name": "blah3",
             "type": "uint8"
          }]
       }]
       "actions": [{
           "name": "action1",
           "type": "action1"
         },{
           "name": "action2",
           "type": "action2"
         }
       ],
       "tables": [],
       "ricardian_clauses": []
   }
   )=====";
   fc::variant var;
   abi_serializer::to_variant(packed_txn, var, get_resolver(fc::json::from_string(packed_transaction_abi).as<abi_def>()));

   chain::packed_transaction packed_txn2;
   abi_serializer::from_variant(var, packed_txn2, get_resolver(fc::json::from_string(packed_transaction_abi).as<abi_def>()));

   const auto txn2 = packed_txn2.get_transaction();

   BOOST_REQUIRE_EQUAL(txn.ref_block_num, txn2.ref_block_num);
   BOOST_REQUIRE_EQUAL(txn.ref_block_prefix, txn2.ref_block_prefix);
   BOOST_REQUIRE(txn.expiration == txn2.expiration);
   BOOST_REQUIRE_EQUAL(txn.region, txn2.region);
   BOOST_REQUIRE_EQUAL(txn.context_free_actions.size(), txn2.context_free_actions.size());
   for (unsigned int i = 0; i < txn.context_free_actions.size(); ++i)
      verify_action_equal<action1>(txn.context_free_actions[i], txn2.context_free_actions[i]);
   BOOST_REQUIRE_EQUAL(txn.actions.size(), txn2.actions.size());
   for (unsigned int i = 0; i < txn.actions.size(); ++i)
      verify_action_equal<action2>(txn.actions[i], txn2.actions[i]);
   BOOST_REQUIRE_EQUAL(txn.max_net_usage_words.value, txn2.max_net_usage_words.value);
   BOOST_REQUIRE_EQUAL(txn.max_kcpu_usage.value, txn2.max_kcpu_usage.value);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_type_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "actor_name",
         "type": "name"
       },{
         "new_type_name": "actor_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "actor_name"
         },{
            "name": "to",
            "type": "actor_name"
         },{
            "name": "amount",
            "type": "uint64"
         }]
       },{
         "name": "account",
         "base": "",
         "fields": [{
            "name": "account",
            "type": "name"
         },{
            "name": "balance",
            "type": "uint64"
         }]
       }
     ],
     "actions": [{
         "name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ],
    "ricardian_clauses": []
   }
   )=====";

   auto abi = chain_initializer::eos_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("types.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_struct_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "actor_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "actor_name"
         },{
            "name": "to",
            "type": "actor_name"
         },{
            "name": "amount",
            "type": "uint64"
         }]
       },{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "account",
            "type": "name"
         },{
            "name": "balance",
            "type": "uint64"
         }]
       }
     ],
     "actions": [{
         "name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ],
     "ricardian_clauses": []
   }
   )=====";

   auto abi = chain_initializer::eos_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("structs.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_action_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "actor_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "actor_name"
         },{
            "name": "to",
            "type": "actor_name"
         },{
            "name": "amount",
            "type": "uint64"
         }]
       },{
         "name": "account",
         "base": "",
         "fields": [{
            "name": "account",
            "type": "name"
         },{
            "name": "balance",
            "type": "uint64"
         }]
       }
     ],
     "actions": [{
         "name": "transfer",
         "type": "transfer"
       },{
         "name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ],
    "ricardian_clauses": []
   }
   )=====";

   auto abi = chain_initializer::eos_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("actions.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_table_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "types": [{
         "new_type_name": "actor_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "actor_name"
         },{
            "name": "to",
            "type": "actor_name"
         },{
            "name": "amount",
            "type": "uint64"
         }]
       },{
         "name": "account",
         "base": "",
         "fields": [{
            "name": "account",
            "type": "name"
         },{
            "name": "balance",
            "type": "uint64"
         }]
       }
     ],
     "actions": [{
         "name": "transfer",
         "type": "transfer",
         "ricardian_contract": "transfer contract"
       }
     ],
     "tables": [{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       },{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ]
   }
   )=====";

   auto abi = chain_initializer::eos_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   auto is_table_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("tables.size") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi), fc::assert_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
