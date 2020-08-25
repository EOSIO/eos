#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <fc/scoped_exit.hpp>

#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/testing/tester.hpp>

#include <boost/test/framework.hpp>

#include <deep_nested.abi.hpp>
#include <large_nested.abi.hpp>

using namespace eosio;
using namespace chain;

BOOST_AUTO_TEST_SUITE(abi_tests)

fc::microseconds max_serialization_time = fc::seconds(1); // some test machines are very slow

// verify that round trip conversion, via bytes, reproduces the exact same data
fc::variant verify_byte_round_trip_conversion( const abi_serializer& abis, const type_name& type, const fc::variant& var )
{
   auto bytes = abis.variant_to_binary(type, var, abi_serializer::create_yield_function( max_serialization_time ));

   auto var2 = abis.binary_to_variant(type, bytes, abi_serializer::create_yield_function( max_serialization_time ));

   std::string r = fc::json::to_string(var2, fc::time_point::now() + max_serialization_time);

   auto bytes2 = abis.variant_to_binary(type, var2, abi_serializer::create_yield_function( max_serialization_time ));

   BOOST_TEST( fc::to_hex(bytes) == fc::to_hex(bytes2) );

   return var2;
}

void verify_round_trip_conversion( const abi_serializer& abis, const type_name& type, const std::string& json, const std::string& hex, const std::string& expected_json )
{
   auto var = fc::json::from_string(json);
   auto bytes = abis.variant_to_binary(type, var, abi_serializer::create_yield_function( max_serialization_time ));
   BOOST_REQUIRE_EQUAL(fc::to_hex(bytes), hex);
   auto var2 = abis.binary_to_variant(type, bytes, abi_serializer::create_yield_function( max_serialization_time ));
   BOOST_REQUIRE_EQUAL(fc::json::to_string(var2, fc::time_point::now() + max_serialization_time), expected_json);
   auto bytes2 = abis.variant_to_binary(type, var2, abi_serializer::create_yield_function( max_serialization_time ));
   BOOST_REQUIRE_EQUAL(fc::to_hex(bytes2), hex);
}

void verify_round_trip_conversion( const abi_serializer& abis, const type_name& type, const std::string& json, const std::string& hex )
{
   verify_round_trip_conversion( abis, type, json, hex, json );
}

auto get_resolver(const abi_def& abi = abi_def())
{
   return [&abi](const account_name &name) -> optional<abi_serializer> {
      return abi_serializer(eosio_contract_abi(abi), abi_serializer::create_yield_function( max_serialization_time ));
   };
}

// verify that round trip conversion, via actual class, reproduces the exact same data
template<typename T>
fc::variant verify_type_round_trip_conversion( const abi_serializer& abis, const type_name& type, const fc::variant& var )
{ try {

   auto bytes = abis.variant_to_binary(type, var, abi_serializer::create_yield_function( max_serialization_time ));

   T obj;
   abi_serializer::from_variant(var, obj, get_resolver(), abi_serializer::create_yield_function( max_serialization_time ));

   fc::variant var2;
   abi_serializer::to_variant(obj, var2, get_resolver(), abi_serializer::create_yield_function( max_serialization_time ));

   std::string r = fc::json::to_string(var2, fc::time_point::now() + max_serialization_time);


   auto bytes2 = abis.variant_to_binary(type, var2, abi_serializer::create_yield_function( max_serialization_time ));

   BOOST_TEST( fc::to_hex(bytes) == fc::to_hex(bytes2) );

   return var2;
} FC_LOG_AND_RETHROW() }


    const char* my_abi = R"=====(
{
   "version": "eosio::abi/1.0",
   "types": [{
      "new_type_name": "type_name",
      "type": "string"
   },{
      "new_type_name": "field_name",
      "type": "string"
   },{
      "new_type_name": "fields",
      "type": "field_def[]"
   },{
      "new_type_name": "scope_name",
      "type": "name"
   }],
   "structs": [{
      "name": "abi_extension",
      "base": "",
      "fields": [{
         "name": "type",
         "type": "uint16"
      },{
         "name": "data",
         "type": "bytes"
      }]
   },{
      "name": "type_def",
      "base": "",
      "fields": [{
         "name": "new_type_name",
         "type": "type_name"
      },{
         "name": "type",
         "type": "type_name"
      }]
   },{
      "name": "field_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "field_name"
      },{
         "name": "type",
         "type": "type_name"
      }]
   },{
      "name": "struct_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "type_name"
      },{
         "name": "base",
         "type": "type_name"
      }{
         "name": "fields",
         "type": "field_def[]"
      }]
   },{
      "name": "action_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "action_name"
      },{
         "name": "type",
         "type": "type_name"
      },{
         "name": "ricardian_contract",
         "type": "string"
      }]
   },{
      "name": "table_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "table_name"
      },{
         "name": "index_type",
         "type": "type_name"
      },{
         "name": "key_names",
         "type": "field_name[]"
      },{
         "name": "key_types",
         "type": "type_name[]"
      },{
         "name": "type",
         "type": "type_name"
      }]
   },{
     "name": "clause_pair",
     "base": "",
     "fields": [{
         "name": "id",
         "type": "string"
     },{
         "name": "body",
         "type": "string"
     }]
   },{
      "name": "abi_def",
      "base": "",
      "fields": [{
         "name": "version",
         "type": "string"
      },{
         "name": "types",
         "type": "type_def[]"
      },{
         "name": "structs",
         "type": "struct_def[]"
      },{
         "name": "actions",
         "type": "action_def[]"
      },{
         "name": "tables",
         "type": "table_def[]"
      },{
         "name": "ricardian_clauses",
         "type": "clause_pair[]"
      },{
         "name": "abi_extensions",
         "type": "abi_extension[]"
      }]
   },{
      "name"  : "A",
      "base"  : "PublicKeyTypes",
      "fields": []
   },{
      "name": "signed_transaction",
      "base": "transaction",
      "fields": [{
         "name": "signatures",
         "type": "signature[]"
      },{
         "name": "context_free_data",
         "type": "bytes[]"
      }]
   },{
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
         "name": "block_timestamp_type",
         "type": "block_timestamp_type"
      },{
         "name": "time_point",
         "type": "time_point"
      },{
         "name": "time_point_arr",
         "type": "time_point[]"
      },{
         "name": "time_point_sec",
         "type": "time_point_sec"
      },{
         "name": "time_point_sec_arr",
         "type": "time_point_sec[]"
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
         "name": "int128",
         "type": "int128"
      },{
         "name": "int128_arr",
         "type": "int128[]"
      },{
         "name": "name",
         "type": "name"
      },{
         "name": "name_arr",
         "type": "name[]"
      },{
         "name": "field",
         "type": "field_def"
      },{
         "name": "field_arr",
         "type": "field_def[]"
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
              {"id":"clause B","body":"clause body B"}],
  "abi_extensions": []
}
)=====";

BOOST_AUTO_TEST_CASE(uint_types)
{ try {

   const char* currency_abi = R"=====(
   {
       "version": "eosio::abi/1.0",
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

   abi_serializer abis(eosio_contract_abi(abi), abi_serializer::create_yield_function( max_serialization_time ));

   const char* test_data = R"=====(
   {
     "amount64" : 64,
     "amount32" : 32,
     "amount16" : 16,
     "amount8"  : 8,
   }
   )=====";


   auto var = fc::json::from_string(test_data);
   verify_byte_round_trip_conversion(abi_serializer{abi, abi_serializer::create_yield_function( max_serialization_time )}, "transfer", var);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(general)
{ try {

   auto abi = eosio_contract_abi(fc::json::from_string(my_abi).as<abi_def>());

   abi_serializer abis(abi, abi_serializer::create_yield_function( max_serialization_time ));

   const char *my_other = R"=====(
    {
      "publickey"     :  "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "publickey_arr" :  ["EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"],
      "asset"         : "100.0000 SYS",
      "asset_arr"     : ["100.0000 SYS","100.0000 SYS"],

      "string"            : "ola ke ase",
      "string_arr"        : ["ola ke ase","ola ke desi"],
      "block_timestamp_type" : "2021-12-20T15",
      "time_point"        : "2021-12-20T15:30",
      "time_point_arr"    : ["2021-12-20T15:30","2021-12-20T15:31"],
      "time_point_sec"    : "2021-12-20T15:30:21",
      "time_point_sec_arr": ["2021-12-20T15:30:21","2021-12-20T15:31:21"],
      "signature"         : "SIG_K1_Jzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwV1v4G5",
      "signature_arr"     : ["SIG_K1_Jzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwV1v4G5","SIG_K1_Jzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwV1v4G5"],
      "checksum256"       : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
      "checksum256_arr"      : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"],
      "fieldname"         : "name1",
      "fieldname_arr"     : ["name1","name2"],
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
      "uint128"           : 128,
      "uint128_arr"       : ["0x00000000000000000000000000000080",129],
      "int8"              : 108,
      "int8_arr"          : [108,109],
      "int16"             : 116,
      "int16_arr"         : [116,117],
      "int32"             : 132,
      "int32_arr"         : [132,133],
      "int64"             : 164,
      "int64_arr"         : [164,165],
      "int128"            : -128,
      "int128_arr"        : ["0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF80",-129],
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
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"accountname1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_cpu_usage_ms":43,
        "delay_sec":0,
        "transaction_extensions": []
      },
      "transaction_arr": [{
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"acc1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_cpu_usage_ms":43,
        "delay_sec":0,
        "transaction_extensions": []
      },{
        "ref_block_num":"2",
        "ref_block_prefix":"3",
        "expiration":"2021-12-20T15:40",
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"acc2", "name":"actionname2", "authorization":[{"actor":"acc2","permission":"permname2"}], "data":""}],
        "max_net_usage_words":21,
        "max_cpu_usage_ms":87,
        "delay_sec":0,
        "transaction_extensions": []
      }],
      "strx": {
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "region": "1",
        "signatures" : ["SIG_K1_Jzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwV1v4G5"],
        "context_free_data" : ["abcdef","0123456789","ABCDEF0123456789abcdef"],
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"accountname1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_cpu_usage_ms":43,
        "delay_sec":0,
        "transaction_extensions": []
      },
      "strx_arr": [{
        "ref_block_num":"1",
        "ref_block_prefix":"2",
        "expiration":"2021-12-20T15:30",
        "region": "1",
        "signatures" : ["SIG_K1_Jzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwV1v4G5"],
        "context_free_data" : ["abcdef","0123456789","ABCDEF0123456789abcdef"],
        "context_free_actions":[{"account":"contextfree1", "name":"cfactionname1", "authorization":[{"actor":"cfacc1","permission":"cfpermname1"}], "data":"778899"}],
        "actions":[{"account":"acc1", "name":"actionname1", "authorization":[{"actor":"acc1","permission":"permname1"}], "data":"445566"}],
        "max_net_usage_words":15,
        "max_cpu_usage_ms":43,
        "delay_sec":0,
        "transaction_extensions": []
      },{
        "ref_block_num":"2",
        "ref_block_prefix":"3",
        "expiration":"2021-12-20T15:40",
        "region": "1",
        "signatures" : ["SIG_K1_Jzdpi5RCzHLGsQbpGhndXBzcFs8vT5LHAtWLMxPzBdwRHSmJkcCdVu6oqPUQn1hbGUdErHvxtdSTS1YA73BThQFwV1v4G5"],
        "context_free_data" : ["abcdef","0123456789","ABCDEF0123456789abcdef"],
        "context_free_actions":[{"account":"contextfree2", "name":"cfactionname2", "authorization":[{"actor":"cfacc2","permission":"cfpermname2"}], "data":"667788"}],
        "actions":[{"account":"acc2", "name":"actionname2", "authorization":[{"actor":"acc2","permission":"permname2"}], "data":""}],
        "max_net_usage_words":15,
        "max_cpu_usage_ms":43,
        "delay_sec":0,
        "transaction_extensions": []
      }],
      "keyweight": {"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},
      "keyweight_arr": [{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}],
      "authority": {
         "threshold":"10",
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":100},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":200}],
         "accounts":[{"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"actor":"acc2","permission":"permname2"},"weight":"2"}],
         "waits":[]
       },
      "authority_arr": [{
         "threshold":"10",
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}],
         "accounts":[{"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"actor":"acc2","permission":"permname2"},"weight":"2"}],
         "waits":[]
       },{
         "threshold":"10",
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}],
         "accounts":[{"permission":{"actor":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"actor":"acc2","permission":"permname2"},"weight":"2"}],
         "waits":[]
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
        "version": "eosio::abi/1.0",
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract":""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": [],
        "abi_extensions": []
      },
      "abidef_arr": [{
        "version": "eosio::abi/1.0",
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract":""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": [],
        "abi_extensions": []
      },{
        "version": "eosio::abi/1.0",
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract": ""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": [],
        "abi_extensions": []
      }]
    }
   )=====";

   auto var = fc::json::from_string(my_other);
   verify_byte_round_trip_conversion(abi_serializer{abi, abi_serializer::create_yield_function( max_serialization_time )}, "A", var);

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
       "version": "eosio::abi/1.0",
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

   auto abi = eosio_contract_abi(fc::json::from_string(typedef_cycle_abi).as<abi_def>());

   auto is_assert_exception = [](const auto& e) -> bool {
      wlog(e.to_string()); return true;
   };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi, abi_serializer::create_yield_function( max_serialization_time )), duplicate_abi_type_def_exception, is_assert_exception);

   abi = fc::json::from_string(struct_cycle_abi).as<abi_def>();
   abi_serializer abis;
   BOOST_CHECK_EXCEPTION( abis.set_abi(abi, abi_serializer::create_yield_function( max_serialization_time )), abi_circular_def_exception, is_assert_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(linkauth_test)
{ try {

   abi_serializer abis(eosio_contract_abi(abi_def()), abi_serializer::create_yield_function( max_serialization_time ));

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

   auto lauth = var.as<linkauth>();
   BOOST_TEST(name("lnkauth.acct") == lauth.account);
   BOOST_TEST(name("lnkauth.code") == lauth.code);
   BOOST_TEST(name("lnkauth.type") == lauth.type);
   BOOST_TEST(name("lnkauth.rqm") == lauth.requirement);

   auto var2 = verify_byte_round_trip_conversion( abis, "linkauth", var );
   auto linkauth2 = var2.as<linkauth>();
   BOOST_TEST(lauth.account == linkauth2.account);
   BOOST_TEST(lauth.code == linkauth2.code);
   BOOST_TEST(lauth.type == linkauth2.type);
   BOOST_TEST(lauth.requirement == linkauth2.requirement);

   verify_type_round_trip_conversion<linkauth>( abis, "linkauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(unlinkauth_test)
{ try {

   abi_serializer abis(eosio_contract_abi(abi_def()), abi_serializer::create_yield_function( max_serialization_time ));

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "lnkauth.acct",
     "code" : "lnkauth.code",
     "type" : "lnkauth.type",
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto unlauth = var.as<unlinkauth>();
   BOOST_TEST(name("lnkauth.acct") == unlauth.account);
   BOOST_TEST(name("lnkauth.code") == unlauth.code);
   BOOST_TEST(name("lnkauth.type") == unlauth.type);

   auto var2 = verify_byte_round_trip_conversion( abis, "unlinkauth", var );
   auto unlinkauth2 = var2.as<unlinkauth>();
   BOOST_TEST(unlauth.account == unlinkauth2.account);
   BOOST_TEST(unlauth.code == unlinkauth2.code);
   BOOST_TEST(unlauth.type == unlinkauth2.type);

   verify_type_round_trip_conversion<unlinkauth>( abis, "unlinkauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(updateauth_test)
{ try {

   abi_serializer abis(eosio_contract_abi(abi_def()), abi_serializer::create_yield_function( max_serialization_time ));

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "updauth.acct",
     "permission" : "updauth.prm",
     "parent" : "updauth.prnt",
     "auth" : {
        "threshold" : "2147483145",
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"actor" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"actor" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 } ],
        "waits" : []
     }
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto updauth = var.as<updateauth>();
   BOOST_TEST(name("updauth.acct") == updauth.account);
   BOOST_TEST(name("updauth.prm") == updauth.permission);
   BOOST_TEST(name("updauth.prnt") == updauth.parent);
   BOOST_TEST(2147483145u == updauth.auth.threshold);

   BOOST_TEST_REQUIRE(2u == updauth.auth.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == updauth.auth.keys[0].key.to_string());
   BOOST_TEST(57005u == updauth.auth.keys[0].weight);
   BOOST_TEST("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf" == updauth.auth.keys[1].key.to_string());
   BOOST_TEST(57605u == updauth.auth.keys[1].weight);

   BOOST_TEST_REQUIRE(2u == updauth.auth.accounts.size());
   BOOST_TEST(name("prm.acct1") == updauth.auth.accounts[0].permission.actor);
   BOOST_TEST(name("prm.prm1") == updauth.auth.accounts[0].permission.permission);
   BOOST_TEST(53005u == updauth.auth.accounts[0].weight);
   BOOST_TEST(name("prm.acct2") == updauth.auth.accounts[1].permission.actor);
   BOOST_TEST(name("prm.prm2") == updauth.auth.accounts[1].permission.permission);
   BOOST_TEST(53405u == updauth.auth.accounts[1].weight);

   auto var2 = verify_byte_round_trip_conversion( abis, "updateauth", var );
   auto updateauth2 = var2.as<updateauth>();
   BOOST_TEST(updauth.account == updateauth2.account);
   BOOST_TEST(updauth.permission == updateauth2.permission);
   BOOST_TEST(updauth.parent == updateauth2.parent);

   BOOST_TEST(updauth.auth.threshold == updateauth2.auth.threshold);

   BOOST_TEST_REQUIRE(updauth.auth.keys.size() == updateauth2.auth.keys.size());
   BOOST_TEST(updauth.auth.keys[0].key == updateauth2.auth.keys[0].key);
   BOOST_TEST(updauth.auth.keys[0].weight == updateauth2.auth.keys[0].weight);
   BOOST_TEST(updauth.auth.keys[1].key == updateauth2.auth.keys[1].key);
   BOOST_TEST(updauth.auth.keys[1].weight == updateauth2.auth.keys[1].weight);

   BOOST_TEST_REQUIRE(updauth.auth.accounts.size() == updateauth2.auth.accounts.size());
   BOOST_TEST(updauth.auth.accounts[0].permission.actor == updateauth2.auth.accounts[0].permission.actor);
   BOOST_TEST(updauth.auth.accounts[0].permission.permission == updateauth2.auth.accounts[0].permission.permission);
   BOOST_TEST(updauth.auth.accounts[0].weight == updateauth2.auth.accounts[0].weight);
   BOOST_TEST(updauth.auth.accounts[1].permission.actor == updateauth2.auth.accounts[1].permission.actor);
   BOOST_TEST(updauth.auth.accounts[1].permission.permission == updateauth2.auth.accounts[1].permission.permission);
   BOOST_TEST(updauth.auth.accounts[1].weight == updateauth2.auth.accounts[1].weight);

   verify_type_round_trip_conversion<updateauth>( abis, "updateauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(deleteauth_test)
{ try {

   abi_serializer abis(eosio_contract_abi(abi_def()), abi_serializer::create_yield_function( max_serialization_time ));

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "delauth.acct",
     "permission" : "delauth.prm"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto delauth = var.as<deleteauth>();
   BOOST_TEST(name("delauth.acct") == delauth.account);
   BOOST_TEST(name("delauth.prm") == delauth.permission);

   auto var2 = verify_byte_round_trip_conversion( abis, "deleteauth", var );
   auto deleteauth2 = var2.as<deleteauth>();
   BOOST_TEST(delauth.account == deleteauth2.account);
   BOOST_TEST(delauth.permission == deleteauth2.permission);

   verify_type_round_trip_conversion<deleteauth>( abis, "deleteauth", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(newaccount_test)
{ try {

   abi_serializer abis(eosio_contract_abi(abi_def()), abi_serializer::create_yield_function( max_serialization_time ));

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
                       {"permission" : {"actor" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }],
        "waits" : []
     },
     "active" : {
        "threshold" : 2146483145,
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"actor" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"actor" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }],
        "waits" : []
     }   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto newacct = var.as<newaccount>();
   BOOST_TEST(name("newacct.crtr") == newacct.creator);
   BOOST_TEST(name("newacct.name") == newacct.name);

   BOOST_TEST(2147483145u == newacct.owner.threshold);

   BOOST_TEST_REQUIRE(2u == newacct.owner.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == newacct.owner.keys[0].key.to_string());
   BOOST_TEST(57005u == newacct.owner.keys[0].weight);
   BOOST_TEST("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf" == newacct.owner.keys[1].key.to_string());
   BOOST_TEST(57605u == newacct.owner.keys[1].weight);

   BOOST_TEST_REQUIRE(2u == newacct.owner.accounts.size());
   BOOST_TEST(name("prm.acct1") == newacct.owner.accounts[0].permission.actor);
   BOOST_TEST(name("prm.prm1") == newacct.owner.accounts[0].permission.permission);
   BOOST_TEST(53005u == newacct.owner.accounts[0].weight);
   BOOST_TEST(name("prm.acct2") == newacct.owner.accounts[1].permission.actor);
   BOOST_TEST(name("prm.prm2") == newacct.owner.accounts[1].permission.permission);
   BOOST_TEST(53405u == newacct.owner.accounts[1].weight);

   BOOST_TEST(2146483145u == newacct.active.threshold);

   BOOST_TEST_REQUIRE(2u == newacct.active.keys.size());
   BOOST_TEST("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im" == newacct.active.keys[0].key.to_string());
   BOOST_TEST(57005u == newacct.active.keys[0].weight);
   BOOST_TEST("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf" == newacct.active.keys[1].key.to_string());
   BOOST_TEST(57605u == newacct.active.keys[1].weight);

   BOOST_TEST_REQUIRE(2u == newacct.active.accounts.size());
   BOOST_TEST(name("prm.acct1") == newacct.active.accounts[0].permission.actor);
   BOOST_TEST(name("prm.prm1") == newacct.active.accounts[0].permission.permission);
   BOOST_TEST(53005u == newacct.active.accounts[0].weight);
   BOOST_TEST(name("prm.acct2") == newacct.active.accounts[1].permission.actor);
   BOOST_TEST(name("prm.prm2") == newacct.active.accounts[1].permission.permission);
   BOOST_TEST(53405u == newacct.active.accounts[1].weight);


   auto var2 = verify_byte_round_trip_conversion( abis, "newaccount", var );
   auto newaccount2 = var2.as<newaccount>();
   BOOST_TEST(newacct.creator == newaccount2.creator);
   BOOST_TEST(newacct.name == newaccount2.name);

   BOOST_TEST(newacct.owner.threshold == newaccount2.owner.threshold);

   BOOST_TEST_REQUIRE(newacct.owner.keys.size() == newaccount2.owner.keys.size());
   BOOST_TEST(newacct.owner.keys[0].key == newaccount2.owner.keys[0].key);
   BOOST_TEST(newacct.owner.keys[0].weight == newaccount2.owner.keys[0].weight);
   BOOST_TEST(newacct.owner.keys[1].key == newaccount2.owner.keys[1].key);
   BOOST_TEST(newacct.owner.keys[1].weight == newaccount2.owner.keys[1].weight);

   BOOST_TEST_REQUIRE(newacct.owner.accounts.size() == newaccount2.owner.accounts.size());
   BOOST_TEST(newacct.owner.accounts[0].permission.actor == newaccount2.owner.accounts[0].permission.actor);
   BOOST_TEST(newacct.owner.accounts[0].permission.permission == newaccount2.owner.accounts[0].permission.permission);
   BOOST_TEST(newacct.owner.accounts[0].weight == newaccount2.owner.accounts[0].weight);
   BOOST_TEST(newacct.owner.accounts[1].permission.actor == newaccount2.owner.accounts[1].permission.actor);
   BOOST_TEST(newacct.owner.accounts[1].permission.permission == newaccount2.owner.accounts[1].permission.permission);
   BOOST_TEST(newacct.owner.accounts[1].weight == newaccount2.owner.accounts[1].weight);

   BOOST_TEST(newacct.active.threshold == newaccount2.active.threshold);

   BOOST_TEST_REQUIRE(newacct.active.keys.size() == newaccount2.active.keys.size());
   BOOST_TEST(newacct.active.keys[0].key == newaccount2.active.keys[0].key);
   BOOST_TEST(newacct.active.keys[0].weight == newaccount2.active.keys[0].weight);
   BOOST_TEST(newacct.active.keys[1].key == newaccount2.active.keys[1].key);
   BOOST_TEST(newacct.active.keys[1].weight == newaccount2.active.keys[1].weight);

   BOOST_TEST_REQUIRE(newacct.active.accounts.size() == newaccount2.active.accounts.size());
   BOOST_TEST(newacct.active.accounts[0].permission.actor == newaccount2.active.accounts[0].permission.actor);
   BOOST_TEST(newacct.active.accounts[0].permission.permission == newaccount2.active.accounts[0].permission.permission);
   BOOST_TEST(newacct.active.accounts[0].weight == newaccount2.active.accounts[0].weight);
   BOOST_TEST(newacct.active.accounts[1].permission.actor == newaccount2.active.accounts[1].permission.actor);
   BOOST_TEST(newacct.active.accounts[1].permission.permission == newaccount2.active.accounts[1].permission.permission);
   BOOST_TEST(newacct.active.accounts[1].weight == newaccount2.active.accounts[1].weight);


   verify_type_round_trip_conversion<newaccount>( abis, "newaccount", var);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(setcode_test)
{ try {

   abi_serializer abis(eosio_contract_abi(abi_def()), abi_serializer::create_yield_function( max_serialization_time ));

   const char* test_data = R"=====(
   {
     "account" : "setcode.acc",
     "vmtype" : "0",
     "vmversion" : "0",
     "code" : "0061736d0100000001390a60037e7e7f017f60047e7e7f7f017f60017e0060057e7e7e7f7f"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto set_code = var.as<setcode>();
   BOOST_TEST(name("setcode.acc") == set_code.account);
   BOOST_TEST(0 == set_code.vmtype);
   BOOST_TEST(0 == set_code.vmversion);
   BOOST_TEST("0061736d0100000001390a60037e7e7f017f60047e7e7f7f017f60017e0060057e7e7e7f7f" == fc::to_hex(set_code.code.data(), set_code.code.size()));

   auto var2 = verify_byte_round_trip_conversion( abis, "setcode", var );
   auto setcode2 = var2.as<setcode>();
   BOOST_TEST(set_code.account == setcode2.account);
   BOOST_TEST(set_code.vmtype == setcode2.vmtype);
   BOOST_TEST(set_code.vmversion == setcode2.vmversion);
   BOOST_TEST(set_code.code == setcode2.code);

   verify_type_round_trip_conversion<setcode>( abis, "setcode", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(setabi_test)
{ try {

   const char* abi_def_abi = R"=====(
      {
         "version": "eosio::abi/1.0",
         "types": [{
            "new_type_name": "type_name",
            "type": "string"
         },{
            "new_type_name": "field_name",
            "type": "string"
         }],
         "structs": [{
            "name": "abi_extension",
            "base": "",
            "fields": [{
               "name": "type",
               "type": "uint16"
            },{
               "name": "data",
               "type": "bytes"
            }]
         },{
            "name": "type_def",
            "base": "",
            "fields": [{
               "name": "new_type_name",
               "type": "type_name"
            },{
               "name": "type",
               "type": "type_name"
            }]
         },{
            "name": "field_def",
            "base": "",
            "fields": [{
               "name": "name",
               "type": "field_name"
            },{
               "name": "type",
               "type": "type_name"
            }]
         },{
            "name": "struct_def",
            "base": "",
            "fields": [{
               "name": "name",
               "type": "type_name"
            },{
               "name": "base",
               "type": "type_name"
            }{
               "name": "fields",
               "type": "field_def[]"
            }]
         },{
               "name": "action_def",
               "base": "",
               "fields": [{
                  "name": "name",
                  "type": "action_name"
               },{
                  "name": "type",
                  "type": "type_name"
               },{
                  "name": "ricardian_contract",
                  "type": "string"
               }]
         },{
               "name": "table_def",
               "base": "",
               "fields": [{
                  "name": "name",
                  "type": "table_name"
               },{
                  "name": "index_type",
                  "type": "type_name"
               },{
                  "name": "key_names",
                  "type": "field_name[]"
               },{
                  "name": "key_types",
                  "type": "type_name[]"
               },{
                  "name": "type",
                  "type": "type_name"
               }]
         },{
            "name": "clause_pair",
            "base": "",
            "fields": [{
               "name": "id",
               "type": "string"
            },{
               "name": "body",
               "type": "string"
            }]
         },{
               "name": "abi_def",
               "base": "",
               "fields": [{
                  "name": "version",
                  "type": "string"
               },{
                  "name": "types",
                  "type": "type_def[]"
               },{
                  "name": "structs",
                  "type": "struct_def[]"
               },{
                  "name": "actions",
                  "type": "action_def[]"
               },{
                  "name": "tables",
                  "type": "table_def[]"
               },{
                  "name": "ricardian_clauses",
                  "type": "clause_pair[]"
               },{
                  "name": "abi_extensions",
                  "type": "abi_extension[]"
               }]
         }],
         "actions": [],
         "tables": [],
         "ricardian_clauses": [],
         "abi_extensions": []
      }
   )=====";

   auto v = fc::json::from_string(abi_def_abi);

   abi_serializer abis(eosio_contract_abi(v.as<abi_def>()), abi_serializer::create_yield_function( max_serialization_time ));

   const char* abi_string = R"=====(
      {
        "version": "eosio::abi/1.0",
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
       "ricardian_clauses": [],
       "abi_extensions": []
      }
   )=====";

   auto var = fc::json::from_string(abi_string);
   auto abi = var.as<abi_def>();

   BOOST_TEST_REQUIRE(1u == abi.types.size());

   BOOST_TEST("account_name" == abi.types[0].new_type_name);
   BOOST_TEST("name" == abi.types[0].type);

   BOOST_TEST_REQUIRE(3u == abi.structs.size());

   BOOST_TEST("transfer_base" == abi.structs[0].name);
   BOOST_TEST("" == abi.structs[0].base);
   BOOST_TEST_REQUIRE(1u == abi.structs[0].fields.size());
   BOOST_TEST("memo" == abi.structs[0].fields[0].name);
   BOOST_TEST("string" == abi.structs[0].fields[0].type);

   BOOST_TEST("transfer" == abi.structs[1].name);
   BOOST_TEST("transfer_base" == abi.structs[1].base);
   BOOST_TEST_REQUIRE(3u == abi.structs[1].fields.size());
   BOOST_TEST("from" == abi.structs[1].fields[0].name);
   BOOST_TEST("account_name" == abi.structs[1].fields[0].type);
   BOOST_TEST("to" == abi.structs[1].fields[1].name);
   BOOST_TEST("account_name" == abi.structs[1].fields[1].type);
   BOOST_TEST("amount" == abi.structs[1].fields[2].name);
   BOOST_TEST("uint64" == abi.structs[1].fields[2].type);

   BOOST_TEST("account" == abi.structs[2].name);
   BOOST_TEST("" == abi.structs[2].base);
   BOOST_TEST_REQUIRE(2u == abi.structs[2].fields.size());
   BOOST_TEST("account" == abi.structs[2].fields[0].name);
   BOOST_TEST("name" == abi.structs[2].fields[0].type);
   BOOST_TEST("balance" == abi.structs[2].fields[1].name);
   BOOST_TEST("uint64" == abi.structs[2].fields[1].type);

   BOOST_TEST_REQUIRE(1u == abi.actions.size());
   BOOST_TEST(name("transfer") == abi.actions[0].name);
   BOOST_TEST("transfer" == abi.actions[0].type);

   BOOST_TEST_REQUIRE(1u == abi.tables.size());
   BOOST_TEST(name("account") == abi.tables[0].name);
   BOOST_TEST("account" == abi.tables[0].type);
   BOOST_TEST("i64" == abi.tables[0].index_type);
   BOOST_TEST_REQUIRE(1u == abi.tables[0].key_names.size());
   BOOST_TEST("account" == abi.tables[0].key_names[0]);
   BOOST_TEST_REQUIRE(1u == abi.tables[0].key_types.size());
   BOOST_TEST("name" == abi.tables[0].key_types[0]);

   auto var2 = verify_byte_round_trip_conversion( abis, "abi_def", var );
   auto abi2 = var2.as<abi_def>();

   BOOST_TEST_REQUIRE(abi.types.size() == abi2.types.size());

   BOOST_TEST(abi.types[0].new_type_name == abi2.types[0].new_type_name);
   BOOST_TEST(abi.types[0].type == abi2.types[0].type);

   BOOST_TEST_REQUIRE(abi.structs.size() == abi2.structs.size());

   BOOST_TEST(abi.structs[0].name == abi2.structs[0].name);
   BOOST_TEST(abi.structs[0].base == abi2.structs[0].base);
   BOOST_TEST_REQUIRE(abi.structs[0].fields.size() == abi2.structs[0].fields.size());
   BOOST_TEST(abi.structs[0].fields[0].name == abi2.structs[0].fields[0].name);
   BOOST_TEST(abi.structs[0].fields[0].type == abi2.structs[0].fields[0].type);

   BOOST_TEST(abi.structs[1].name == abi2.structs[1].name);
   BOOST_TEST(abi.structs[1].base == abi2.structs[1].base);
   BOOST_TEST_REQUIRE(abi.structs[1].fields.size() == abi2.structs[1].fields.size());
   BOOST_TEST(abi.structs[1].fields[0].name == abi2.structs[1].fields[0].name);
   BOOST_TEST(abi.structs[1].fields[0].type == abi2.structs[1].fields[0].type);
   BOOST_TEST(abi.structs[1].fields[1].name == abi2.structs[1].fields[1].name);
   BOOST_TEST(abi.structs[1].fields[1].type == abi2.structs[1].fields[1].type);
   BOOST_TEST(abi.structs[1].fields[2].name == abi2.structs[1].fields[2].name);
   BOOST_TEST(abi.structs[1].fields[2].type == abi2.structs[1].fields[2].type);

   BOOST_TEST(abi.structs[2].name == abi2.structs[2].name);
   BOOST_TEST(abi.structs[2].base == abi2.structs[2].base);
   BOOST_TEST_REQUIRE(abi.structs[2].fields.size() == abi2.structs[2].fields.size());
   BOOST_TEST(abi.structs[2].fields[0].name == abi2.structs[2].fields[0].name);
   BOOST_TEST(abi.structs[2].fields[0].type == abi2.structs[2].fields[0].type);
   BOOST_TEST(abi.structs[2].fields[1].name == abi2.structs[2].fields[1].name);
   BOOST_TEST(abi.structs[2].fields[1].type == abi2.structs[2].fields[1].type);

   BOOST_TEST_REQUIRE(abi.actions.size() == abi2.actions.size());
   BOOST_TEST(abi.actions[0].name == abi2.actions[0].name);
   BOOST_TEST(abi.actions[0].type == abi2.actions[0].type);

   BOOST_TEST_REQUIRE(abi.tables.size() == abi2.tables.size());
   BOOST_TEST(abi.tables[0].name == abi2.tables[0].name);
   BOOST_TEST(abi.tables[0].type == abi2.tables[0].type);
   BOOST_TEST(abi.tables[0].index_type == abi2.tables[0].index_type);
   BOOST_TEST_REQUIRE(abi.tables[0].key_names.size() == abi2.tables[0].key_names.size());
   BOOST_TEST(abi.tables[0].key_names[0] == abi2.tables[0].key_names[0]);
   BOOST_TEST_REQUIRE(abi.tables[0].key_types.size() == abi2.tables[0].key_types.size());
   BOOST_TEST(abi.tables[0].key_types[0] == abi2.tables[0].key_types[0]);

} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_CASE(setabi_kv_tables_test)
{ try {
   // mystruct doesn't exists in structs
   const char* abi_def_abi = R"=====(
   {
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.2",
    "types": [],
    "structs": [],
    "actions": [],
    "tables": [],
    "kv_tables": {
        "testtable1": {
            "type": "mystruct",
            "primary_index": {
                "name": "pid1",
                "type": "name"
            }
        }
    },
    "ricardian_clauses": [],
    "variants": [],
    "action_results": []
   }
   )=====";

   auto var = fc::json::from_string(abi_def_abi);
   BOOST_CHECK_THROW( abi_serializer abis(fc::json::from_string(abi_def_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), fc::exception );

   // testtable1_INVALID_NAME is not a valid eosio::name
   const char* abi_def_abi2 = R"=====(
   {
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.2",
    "types": [],
    "structs": [],
    "actions": [],
    "tables": [],
    "kv_tables": {
        "testtable1_INVALID_NAME": {
            "type": "mystruct",
            "primary_index": {
                "name": "pid1",
                "type": "name"
            }
        }
    },
    "ricardian_clauses": [],
    "variants": [],
    "action_results": []
   }
   )=====";

   var = fc::json::from_string(abi_def_abi2);
   BOOST_CHECK_THROW( abi_serializer abis(fc::json::from_string(abi_def_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), fc::exception );

   // testtable1.type is empty
   const char *abi_def_abi3 = R"=====(
   {
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.2",
    "types": [],
    "structs": [
        {
            "name": "mystruct",
            "base": "",
            "fields": [
                {
                    "name": "primarykey",
                    "type": "name"
                },
                {
                    "name": "foo",
                    "type": "string"
                }
            ]
        }
    ],
    "actions": [],
    "tables": [],
    "kv_tables": {
        "testtable1": {
            "type": "",
            "primary_index": {
                "name": "pid1",
                "type": "name"
            }
        }
    },
    "ricardian_clauses": [],
    "variants": [],
    "action_results": []
   }
   )=====";
   var = fc::json::from_string(abi_def_abi3);
   BOOST_CHECK_THROW(abi_serializer abis(fc::json::from_string(abi_def_abi).as<abi_def>(), abi_serializer::create_yield_function(max_serialization_time)), fc::exception);

   // testtable1.primary_index is missing
   const char *abi_def_abi4 = R"=====(
   {
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.2",
    "types": [],
    "structs": [
        {
            "name": "mystruct",
            "base": "",
            "fields": [
                {
                    "name": "primarykey",
                    "type": "name"
                },
                {
                    "name": "foo",
                    "type": "string"
                }
            ]
        }
    ],
    "actions": [],
    "tables": [],
    "kv_tables": {
        "testtable1": {
            "type": "mystruct",
            "primary_index": {}
        }
    },
    "ricardian_clauses": [],
    "variants": [],
    "action_results": []
   }
   )=====";
   var = fc::json::from_string(abi_def_abi3);
   BOOST_CHECK_THROW(abi_serializer abis(fc::json::from_string(abi_def_abi).as<abi_def>(), abi_serializer::create_yield_function(max_serialization_time)), fc::exception);

   const char* abi_def_abi5 = R"=====(

   {
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.2",
    "types": [],
    "structs": [],
    "actions": [],
    "tables": [],
    "kv_tables": {},
    "ricardian_clauses": [],
    "variants": [],
    "action_results": []
   }
   )=====";

   var = fc::json::from_string(abi_def_abi5);
   auto abi = var.as<abi_def>();

   BOOST_CHECK_NO_THROW();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(setabi_test3)
{ try {

      const char *abi_def_abi = R"=====(

   {
    "____comment": "This file was generated with eosio-abigen. DO NOT EDIT ",
    "version": "eosio::abi/1.2",
    "types": [],
    "structs": [
        {
            "name": "get",
            "base": "",
            "fields": []
        },
        {
            "name": "iteration",
            "base": "",
            "fields": []
        },
        {
            "name": "mystruct",
            "base": "",
            "fields": [
                {
                    "name": "primarykey",
                    "type": "name"
                },
                {
                    "name": "foo",
                    "type": "string"
                },
                {
                    "name": "bar",
                    "type": "uint64"
                },
                {
                    "name": "fullname",
                    "type": "string"
                },
                {
                    "name": "combo",
                    "type": "tuple_string_uint32"
                },
                {
                    "name": "age",
                    "type": "uint32"
                },
                {
                    "name": "rate",
                    "type": "float64"
                }
            ]
        },
        {
            "name": "nonunique",
            "base": "",
            "fields": []
        },
        {
            "name": "setup",
            "base": "",
            "fields": []
        },
        {
            "name": "tuple_string_uint32",
            "base": "",
            "fields": [
                {
                    "name": "field0",
                    "type": "string"
                },
                {
                    "name": "field1",
                    "type": "uint32"
                }
            ]
        },
        {
            "name": "update",
            "base": "",
            "fields": []
        },
        {
            "name": "updateerr1",
            "base": "",
            "fields": []
        },
        {
            "name": "updateerr2",
            "base": "",
            "fields": []
        }
    ],
    "actions": [
        {
            "name": "get",
            "type": "get",
            "ricardian_contract": ""
        },
        {
            "name": "iteration",
            "type": "iteration",
            "ricardian_contract": ""
        },
        {
            "name": "nonunique",
            "type": "nonunique",
            "ricardian_contract": ""
        },
        {
            "name": "setup",
            "type": "setup",
            "ricardian_contract": ""
        },
        {
            "name": "update",
            "type": "update",
            "ricardian_contract": ""
        },
        {
            "name": "updateerr1",
            "type": "updateerr1",
            "ricardian_contract": ""
        },
        {
            "name": "updateerr2",
            "type": "updateerr2",
            "ricardian_contract": ""
        }
    ],
    "tables": [],
    "kv_tables": {
        "testtable1": {
            "type": "mystruct",
            "primary_index": {
                "name": "pid1",
                "type": "name"
            },
            "secondary_indices": {
                "foo": {
                    "type": "string"
                },
                "bar": {
                    "type": "uint64"
                },
                "combo": {
                    "type": "tuple_string_uint32"
                },
                "rate": {
                    "type": "float64"
                }
            }
        },
        "testtable2": {
            "type": "mystruct",
            "primary_index": {
                "name": "pid2",
                "type": "name"
            },
            "secondary_indices": {
                "sid1": {
                    "type": "string"
                },
                "sid2": {
                    "type": "uint64"
                }
            }
        }

    },
    "ricardian_clauses": [],
    "variants": [],
    "action_results": []
   }
   )=====";

      auto var = fc::json::from_string(abi_def_abi);
      auto abi = var.as<abi_def>();

      BOOST_TEST(2u == abi.kv_tables.value.size());
      name tbl_name{"testtable1"};
      auto &kv_tbl_def = abi.kv_tables.value[tbl_name];
      BOOST_TEST("pid1" == kv_tbl_def.primary_index.name.to_string());
      BOOST_TEST("name" == kv_tbl_def.primary_index.type);
      BOOST_TEST(4u == kv_tbl_def.secondary_indices.size());
      BOOST_TEST("string" == kv_tbl_def.secondary_indices[name("foo")].type);
      BOOST_TEST("uint64" == kv_tbl_def.secondary_indices[name("bar")].type);
      BOOST_TEST("tuple_string_uint32" == kv_tbl_def.secondary_indices[name("combo")].type);

      tbl_name = name("testtable2");
      auto &kv_tbl_def2 = abi.kv_tables.value[tbl_name];
      BOOST_TEST("pid2" == kv_tbl_def2.primary_index.name.to_string());
      BOOST_TEST("name" == kv_tbl_def2.primary_index.type);
      BOOST_TEST(2u == kv_tbl_def2.secondary_indices.size());

      abi_serializer abis(abi, abi_serializer::create_yield_function(max_serialization_time));

      const char *test_data = R"=====(
   {
      "type": "mystruct",
      "primarykey": "hello",
      "foo":  "World",
      "bar":  1000,
      "fullname": "Hello World",
      "age":  30,
      "combo": {
         "field0": "welcome",
         "field1": 99
      },
      "rate": 123.567
   }
   )=====";

      auto var_data = fc::json::from_string(test_data);
      auto var2 = verify_byte_round_trip_conversion(abis, "testtable1", var_data);

      fc::variant v1;
      fc::variant v2;
      kv_tables_as_object<map<table_name, kv_table_def>> kv_tables_obj;

      to_variant(abi.kv_tables, v1);
      from_variant(v1, kv_tables_obj);
      to_variant(kv_tables_obj, v2);

      std::stringstream ss1;
      std::stringstream ss2;
      ss1 << v1;
      ss2 << v2;
      string str1 = ss1.str();
      string str2 = ss2.str();

      BOOST_TEST(str1 == str2);
     
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
   BOOST_REQUIRE_EQUAL(exp.account.to_string(), act.account.to_string());
   BOOST_REQUIRE_EQUAL(exp.name.to_string(), act.name.to_string());
   BOOST_REQUIRE_EQUAL(exp.authorization.size(), act.authorization.size());
   for(unsigned int i = 0; i < exp.authorization.size(); ++i)
   {
      BOOST_REQUIRE_EQUAL(exp.authorization[i].actor.to_string(), act.authorization[i].actor.to_string());
      BOOST_REQUIRE_EQUAL(exp.authorization[i].permission.to_string(), act.authorization[i].permission.to_string());
   }
   BOOST_REQUIRE_EQUAL(exp.data.size(), act.data.size());
   BOOST_REQUIRE(!memcmp(exp.data.data(), act.data.data(), exp.data.size()));
}

private_key_type get_private_key( name keyname, string role ) {
   return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(keyname.to_string()+role));
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

   chain::signed_transaction txn;
   txn.ref_block_num = 1;
   txn.ref_block_prefix = 2;
   txn.expiration.from_iso_string("2021-12-20T15:30");
   name a = N(alice);
   txn.context_free_actions.emplace_back(
         vector<permission_level>{{N(testapi1), config::active_name}},
         newaccount{
               .creator  = config::system_account_name,
               .name     = a,
               .owner    = authority( get_public_key( a, "owner" )),
               .active   = authority( get_public_key( a, "active" ) )
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
   txn.max_cpu_usage_ms = 43;

   // pack the transaction to verify that the var unpacking logic is correct
   auto packed_txn = chain::packed_transaction(signed_transaction(txn), true);

   const char* packed_transaction_abi = R"=====(
   {
       "version": "eosio::abi/1.0",
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
   abi_serializer::to_variant(packed_txn, var, get_resolver(fc::json::from_string(packed_transaction_abi).as<abi_def>()), abi_serializer::create_yield_function( max_serialization_time ));

   chain::packed_transaction_v0 packed_txn2;
   abi_serializer::from_variant(var, packed_txn2, get_resolver(fc::json::from_string(packed_transaction_abi).as<abi_def>()), abi_serializer::create_yield_function( max_serialization_time ));

   const auto& txn2 = packed_txn2.get_transaction();

   BOOST_REQUIRE_EQUAL(txn.ref_block_num, txn2.ref_block_num);
   BOOST_REQUIRE_EQUAL(txn.ref_block_prefix, txn2.ref_block_prefix);
   BOOST_REQUIRE(txn.expiration == txn2.expiration);
   BOOST_REQUIRE_EQUAL(txn.context_free_actions.size(), txn2.context_free_actions.size());
   for (unsigned int i = 0; i < txn.context_free_actions.size(); ++i)
      verify_action_equal<action1>(txn.context_free_actions[i], txn2.context_free_actions[i]);
   BOOST_REQUIRE_EQUAL(txn.actions.size(), txn2.actions.size());
   for (unsigned int i = 0; i < txn.actions.size(); ++i)
      verify_action_equal<action2>(txn.actions[i], txn2.actions[i]);
   BOOST_REQUIRE_EQUAL(txn.max_net_usage_words.value, txn2.max_net_usage_words.value);
   BOOST_REQUIRE_EQUAL(txn.max_cpu_usage_ms, txn2.max_cpu_usage_ms);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_type_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
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

   auto abi = eosio_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   auto is_table_exception = [](fc::exception const & e) -> bool { return e.to_detail_string().find("type already exists") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(abi, abi_serializer::create_yield_function( max_serialization_time )), duplicate_abi_type_def_exception, is_table_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_struct_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
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

   auto abi = eosio_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   BOOST_CHECK_THROW( abi_serializer abis(abi, abi_serializer::create_yield_function( max_serialization_time )), duplicate_abi_struct_def_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_action_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
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

   auto abi = eosio_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   BOOST_CHECK_THROW( abi_serializer abis(abi, abi_serializer::create_yield_function( max_serialization_time )), duplicate_abi_action_def_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_table_repeat)
{ try {

   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
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

   auto abi = eosio_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   BOOST_CHECK_THROW( abi_serializer abis(abi, abi_serializer::create_yield_function( max_serialization_time )), duplicate_abi_table_def_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_type_def)
{ try {
   // inifinite loop in types
   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "account_name"
         },{
            "name": "to",
            "type": "name"
         },{
            "name": "amount",
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
     "tables": []
   }
   )=====";

   abi_serializer abis(fc::json::from_string(repeat_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ));
   BOOST_CHECK(abis.is_type("name", abi_serializer::create_yield_function( max_serialization_time )));
   BOOST_CHECK(abis.is_type("account_name", abi_serializer::create_yield_function( max_serialization_time )));

   const char* test_data = R"=====(
   {
     "from" : "kevin",
     "to" : "dan",
     "amount" : 16
   }
   )=====";

   auto var = fc::json::from_string(test_data);
   verify_byte_round_trip_conversion(abis, "transfer", var);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_type_loop)
{ try {
   // inifinite loop in types
   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       },{
         "new_type_name": "name",
         "type": "account_name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "account_name"
         },{
            "name": "to",
            "type": "name"
         },{
            "name": "amount",
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
     "tables": []
   }
   )=====";

   auto is_type_exception = [](fc::exception const & e) -> bool { return e.to_detail_string().find("type already exists") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(fc::json::from_string(repeat_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), duplicate_abi_type_def_exception, is_type_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_type_redefine)
{ try {
   // inifinite loop in types
   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
     "types": [{
         "new_type_name": "account_name",
         "type": "account_name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "account_name"
         },{
            "name": "to",
            "type": "name"
         },{
            "name": "amount",
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
     "tables": []
   }
   )=====";

   auto is_type_exception = [](fc::exception const & e) -> bool { return e.to_detail_string().find("Circular reference in type account_name") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(fc::json::from_string(repeat_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), abi_circular_def_exception, is_type_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_type_redefine_to_name)
{ try {
      // inifinite loop in types
      const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
     "types": [{
         "new_type_name": "name",
         "type": "name"
       }
     ],
     "structs": [],
     "actions": [],
     "tables": []
   }
   )=====";

   auto is_type_exception = [](fc::exception const & e) -> bool { return e.to_detail_string().find("type already exists") != std::string::npos; };
   BOOST_CHECK_EXCEPTION( abi_serializer abis(fc::json::from_string(repeat_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), duplicate_abi_type_def_exception, is_type_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_type_nested_in_vector)
{ try {
      // inifinite loop in types
      const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
     "types": [],
     "structs": [{
         "name": "store_t",
         "base": "",
         "fields": [{
            "name": "id",
            "type": "uint64"
         },{
            "name": "childs",
            "type": "store_t[]"
         }],
     "actions": [],
     "tables": []
   }
   )=====";

   BOOST_CHECK_THROW( abi_serializer abis(fc::json::from_string(repeat_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), fc::exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(abi_account_name_in_eosio_abi)
{ try {
   // inifinite loop in types
   const char* repeat_abi = R"=====(
   {
     "version": "eosio::abi/1.0",
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "account_name"
         },{
            "name": "to",
            "type": "name"
         },{
            "name": "amount",
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
     "tables": []
   }
   )=====";

   auto abi = eosio_contract_abi(fc::json::from_string(repeat_abi).as<abi_def>());
   auto is_type_exception = [](fc::assert_exception const & e) -> bool { return e.to_detail_string().find("abi.types.size") != std::string::npos; };

} FC_LOG_AND_RETHROW() }


// Unlimited array size during abi serialization can exhaust memory and crash the process
BOOST_AUTO_TEST_CASE(abi_large_array)
{
   try {
      const char* abi_str = R"=====(
      {
        "version": "eosio::abi/1.0",
        "types": [],
        "structs": [{
           "name": "hi",
           "base": "",
           "fields": [
           ]
         }
       ],
       "actions": [{
           "name": "hi",
           "type": "hi[]",
           "ricardian_contract": ""
         }
       ],
       "tables": []
      }
      )=====";

      abi_serializer abis( fc::json::from_string( abi_str ).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );
      // indicate a very large array, but don't actually provide a large array
      // curl http://127.0.0.1:8888/v1/chain/abi_bin_to_json -X POST -d '{"code":"eosio", "action":"hi", "binargs":"ffffffff08"}'
      bytes bin = {static_cast<char>(0xff),
                   static_cast<char>(0xff),
                   static_cast<char>(0xff),
                   static_cast<char>(0xff),
                   static_cast<char>(0x08)};
      BOOST_CHECK_THROW( abis.binary_to_variant( "hi[]", bin, abi_serializer::create_yield_function( max_serialization_time ) );, fc::exception );

   } FC_LOG_AND_RETHROW()
}

// Infinite recursion of abi_serializer is_type
BOOST_AUTO_TEST_CASE(abi_is_type_recursion)
{
   try {
      const char* abi_str = R"=====(
      {
       "version": "eosio::abi/1.0",
       "types": [
        {
            "new_type_name": "a[]",
            "type": "a[][]",
        },
        ],
        "structs": [
         {
            "name": "a[]",
            "base": "",
            "fields": []
         },
         {
            "name": "hi",
            "base": "",
            "fields": [{
                "name": "user",
                "type": "name"
              }
            ]
          }
        ],
        "actions": [{
            "name": "hi",
            "type": "hi",
            "ricardian_contract": ""
          }
        ],
        "tables": []
      }
      )=====";

      BOOST_CHECK_THROW( abi_serializer abis(fc::json::from_string(abi_str).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), fc::exception );

   } FC_LOG_AND_RETHROW()
}

// Infinite recursion of abi_serializer in struct definitions
BOOST_AUTO_TEST_CASE(abi_recursive_structs)
{
   try {
      const char* abi_str = R"=====(
      {
        "version": "eosio::abi/1.0",
        "types": [],
        "structs": [
          {
            "name": "a"
            "base": "",
            "fields": [
              {
              "name": "user",
              "type": "b"
              }
            ]
          },
          {
            "name": "b"
            "base": "",
            "fields": [
             {
               "name": "user",
               "type": "a"
             }
            ]
          },
          {
            "name": "hi",
            "base": "",
            "fields": [{
                "name": "user",
                "type": "name"
              },
              {
                "name": "arg2",
                "type": "a"
              }
            ]
         },
         {
           "name": "hi2",
           "base": "",
           "fields": [{
               "name": "user",
               "type": "name"
             }
           ]
         }
        ],
        "actions": [{
            "name": "hi",
            "type": "hi",
            "ricardian_contract": ""
          }
        ],
        "tables": []
      }
      )=====";

      abi_serializer abis(fc::json::from_string(abi_str).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ));
      string hi_data = "{\"user\":\"eosio\"}";
      auto bin = abis.variant_to_binary("hi2", fc::json::from_string(hi_data), abi_serializer::create_yield_function( max_serialization_time ));
      BOOST_CHECK_THROW( abis.binary_to_variant("hi", bin, abi_serializer::create_yield_function( max_serialization_time ));, fc::exception );

   } FC_LOG_AND_RETHROW()
}

// Infinite recursion of abi_serializer in struct definitions
BOOST_AUTO_TEST_CASE(abi_very_deep_structs)
{
   try {
      abi_serializer abis( fc::json::from_string( large_nested_abi ).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );
      string hi_data = "{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":0}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}";
      BOOST_CHECK_THROW( abis.variant_to_binary( "s98", fc::json::from_string( hi_data ), abi_serializer::create_yield_function( max_serialization_time ) ), fc::exception );
   } FC_LOG_AND_RETHROW()
}

// Infinite recursion of abi_serializer in struct definitions
BOOST_AUTO_TEST_CASE(abi_very_deep_structs_1ms)
{
   try {
      BOOST_CHECK_THROW(
            abi_serializer abis( fc::json::from_string( large_nested_abi ).as<abi_def>(), abi_serializer::create_yield_function( fc::microseconds( 1 ) ) ),
            fc::exception );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(abi_deep_structs_validate)
{
   try {
      BOOST_CHECK_THROW(
            abi_serializer abis( fc::json::from_string( deep_nested_abi ).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) ),
            fc::exception );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(variants)
{
   using eosio::testing::fc_exception_message_starts_with;

   auto duplicate_variant_abi = R"({
      "version": "eosio::abi/1.1",
      "variants": [
         {"name": "v1", "types": ["int8", "string", "bool"]},
         {"name": "v1", "types": ["int8", "string", "bool"]},
      ],
   })";

   auto variant_abi_invalid_type = R"({
      "version": "eosio::abi/1.1",
      "variants": [
         {"name": "v1", "types": ["int91", "string", "bool"]},
      ],
   })";

   auto variant_abi = R"({
      "version": "eosio::abi/1.1",
      "types": [
         {"new_type_name": "foo", "type": "s"},
         {"new_type_name": "bar", "type": "s"},
      ],
      "structs": [
         {"name": "s", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"},
         ]}
      ],
      "variants": [
         {"name": "v1", "types": ["int8", "string", "int16"]},
         {"name": "v2", "types": ["foo", "bar"]},
      ],
   })";

   try {
      // round-trip abi through multiple formats
      // json -> variant -> abi_def -> bin
      auto bin = fc::raw::pack(fc::json::from_string(variant_abi).as<abi_def>());
      // bin -> abi_def -> variant -> abi_def
      abi_serializer abis(variant(fc::raw::unpack<abi_def>(bin)).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      // duplicate variant definition detected
      BOOST_CHECK_THROW( abi_serializer( fc::json::from_string(duplicate_variant_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) ), duplicate_abi_variant_def_exception );

      // invalid_type_inside_abi
      BOOST_CHECK_THROW( abi_serializer( fc::json::from_string(variant_abi_invalid_type).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) ), invalid_type_inside_abi );

      // expected array containing variant
      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"(9)"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Expected input to be an array of two items while processing variant 'v1'") );
      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"([4])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Expected input to be an array of two items while processing variant 'v1") );
      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"([4, 5])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Encountered non-string as first item of input array while processing variant 'v1") );
      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"(["4", 5, 6])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Expected input to be an array of two items while processing variant 'v1'") );

      // type is not valid within this variant
      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"(["int9", 21])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Specified type 'int9' in input array is not valid within the variant 'v1'") );

      verify_round_trip_conversion(abis, "v1", R"(["int8",21])", "0015");
      verify_round_trip_conversion(abis, "v1", R"(["string","abcd"])", "010461626364");
      verify_round_trip_conversion(abis, "v1", R"(["int16",3])", "020300");
      verify_round_trip_conversion(abis, "v1", R"(["int16",4])", "020400");
      verify_round_trip_conversion(abis, "v2", R"(["foo",{"i0":5,"i1":6}])", "000506");
      verify_round_trip_conversion(abis, "v2", R"(["bar",{"i0":5,"i1":6}])", "010506");
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(aliased_variants)
{
   using eosio::testing::fc_exception_message_starts_with;

   auto aliased_variant = R"({
      "version": "eosio::abi/1.1",
      "types": [
         { "new_type_name": "foo", "type": "foo_variant" }
      ],
      "variants": [
         {"name": "foo_variant", "types": ["int8", "string"]}
      ],
   })";

   try {
      // round-trip abi through multiple formats
      // json -> variant -> abi_def -> bin
      auto bin = fc::raw::pack(fc::json::from_string(aliased_variant).as<abi_def>());
      // bin -> abi_def -> variant -> abi_def
      abi_serializer abis(variant(fc::raw::unpack<abi_def>(bin)).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      verify_round_trip_conversion(abis, "foo", R"(["int8",21])", "0015");
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(variant_of_aliases)
{
   using eosio::testing::fc_exception_message_starts_with;

   auto aliased_variant = R"({
      "version": "eosio::abi/1.1",
      "types": [
         { "new_type_name": "foo_0", "type": "int8" },
         { "new_type_name": "foo_1", "type": "string" }
      ],
      "variants": [
         {"name": "foo", "types": ["foo_0", "foo_1"]}
      ],
   })";

   try {
      // round-trip abi through multiple formats
      // json -> variant -> abi_def -> bin
      auto bin = fc::raw::pack(fc::json::from_string(aliased_variant).as<abi_def>());
      // bin -> abi_def -> variant -> abi_def
      abi_serializer abis(variant(fc::raw::unpack<abi_def>(bin)).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      verify_round_trip_conversion(abis, "foo", R"(["foo_0",21])", "0015");
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(action_results)
{
   auto action_results_abi = R"({
      "version": "eosio::abi/1.2",
      "action_results": [
         {"name": "act1", "result_type": "string"},
         {"name": "act2", "result_type": "uint16"},
      ],
   })";

   auto duplicate_action_results_abi = R"({
      "version": "eosio::abi/1.2",
      "action_results": [
         {"name": "act1", "result_type": "string"},
         {"name": "act1", "result_type": "string"},
      ],
   })";

   auto action_results_abi_invalid_type = R"({
      "version": "eosio::abi/1.2",
      "action_results": [
         {"name": "act1", "result_type": "string"},
         {"name": "act2", "result_type": "uint9000"},
      ],
   })";

   try {
      // duplicate action_results definition detected
      BOOST_CHECK_THROW( abi_serializer( fc::json::from_string(duplicate_action_results_abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) ), duplicate_abi_action_results_def_exception );

      // invalid_type_inside_abi
      BOOST_CHECK_THROW( abi_serializer( fc::json::from_string(action_results_abi_invalid_type).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) ), invalid_type_inside_abi );

      // round-trip abi through multiple formats
      // json -> variant -> abi_def -> bin
      auto bin = fc::raw::pack(fc::json::from_string(action_results_abi).as<abi_def>());
      // bin -> abi_def -> variant -> abi_def
      auto def = variant(fc::raw::unpack<abi_def>(bin)).as<abi_def>();

      BOOST_REQUIRE_EQUAL(def.action_results.value.size(), 2);
      BOOST_REQUIRE_EQUAL(def.action_results.value[0].name, name{"act1"});
      BOOST_REQUIRE_EQUAL(def.action_results.value[0].result_type, "string");
      BOOST_REQUIRE_EQUAL(def.action_results.value[1].name, name{"act2"});
      BOOST_REQUIRE_EQUAL(def.action_results.value[1].result_type, "uint16");
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(extend)
{
   using eosio::testing::fc_exception_message_starts_with;

   auto abi = R"({
      "version": "eosio::abi/1.1",
      "structs": [
         {"name": "s", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"},
            {"name": "i2", "type": "int8$"},
            {"name": "a", "type": "int8[]$"},
            {"name": "o", "type": "int8?$"},
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8$"},
            {"name": "i2", "type": "int8"},
         ]}
      ],
   })";
   // NOTE: Ideally this ABI would be rejected during validation for an improper definition for struct "s2".
   //       Such a check is not yet implemented during validation, but it can check during serialization.

   try {
      abi_serializer abis(fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      // missing i1
      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s", fc::json::from_string(R"({"i0":5})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Missing field 'i1' in input object while processing struct") );

      // Unexpected 'a'
      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s", fc::json::from_string(R"({"i0":5,"i1":6,"a":[8,9,10]})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Unexpected field 'a' found in input object while processing struct") );

      verify_round_trip_conversion(abis, "s", R"({"i0":5,"i1":6})", "0506");
      verify_round_trip_conversion(abis, "s", R"({"i0":5,"i1":6,"i2":7})", "050607");
      verify_round_trip_conversion(abis, "s", R"({"i0":5,"i1":6,"i2":7,"a":[8,9,10]})", "0506070308090a");
      verify_round_trip_conversion(abis, "s", R"({"i0":5,"i1":6,"i2":7,"a":[8,9,10],"o":null})", "0506070308090a00");
      verify_round_trip_conversion(abis, "s", R"({"i0":5,"i1":6,"i2":7,"a":[8,9,10],"o":31})", "0506070308090a011f");

      verify_round_trip_conversion(abis, "s", R"([5,6])", "0506", R"({"i0":5,"i1":6})");
      verify_round_trip_conversion(abis, "s", R"([5,6,7])", "050607", R"({"i0":5,"i1":6,"i2":7})");
      verify_round_trip_conversion(abis, "s", R"([5,6,7,[8,9,10]])", "0506070308090a", R"({"i0":5,"i1":6,"i2":7,"a":[8,9,10]})");
      verify_round_trip_conversion(abis, "s", R"([5,6,7,[8,9,10],null])", "0506070308090a00", R"({"i0":5,"i1":6,"i2":7,"a":[8,9,10],"o":null})");
      verify_round_trip_conversion(abis, "s", R"([5,6,7,[8,9,10],31])", "0506070308090a011f", R"({"i0":5,"i1":6,"i2":7,"a":[8,9,10],"o":31})");

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s2", fc::json::from_string(R"({"i0":1})"), abi_serializer::create_yield_function( max_serialization_time )),
                             abi_exception, fc_exception_message_starts_with("Encountered field 'i2' without binary extension designation while processing struct") );


   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(version)
{
   try {
      BOOST_CHECK_THROW( abi_serializer(fc::json::from_string(R"({})").as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), unsupported_abi_version_exception );
      BOOST_CHECK_THROW( abi_serializer(fc::json::from_string(R"({"version": ""})").as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), unsupported_abi_version_exception );
      BOOST_CHECK_THROW( abi_serializer(fc::json::from_string(R"({"version": "eosio::abi/9.0"})").as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time )), unsupported_abi_version_exception );
      abi_serializer(fc::json::from_string(R"({"version": "eosio::abi/1.0"})").as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ));
      abi_serializer(fc::json::from_string(R"({"version": "eosio::abi/1.1"})").as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ));
      abi_serializer(fc::json::from_string(R"({"version": "eosio::abi/1.2"})").as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ));
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(abi_serialize_incomplete_json_array)
{
   using eosio::testing::fc_exception_message_starts_with;

   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {"name": "s", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"},
            {"name": "i2", "type": "int8"}
         ]}
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s", fc::json::from_string(R"([])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Early end to input array specifying the fields of struct") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s", fc::json::from_string(R"([1,2])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Early end to input array specifying the fields of struct") );

      verify_round_trip_conversion(abis, "s", R"([1,2,3])", "010203", R"({"i0":1,"i1":2,"i2":3})");

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(abi_serialize_incomplete_json_object)
{
   using eosio::testing::fc_exception_message_starts_with;

   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"}
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "f0", "type": "s1"}
            {"name": "i2", "type": "int8"}
         ]}
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s2", fc::json::from_string(R"({})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Missing field 'f0' in input object") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s2", fc::json::from_string(R"({"f0":{"i0":1}})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_starts_with("Missing field 'i1' in input object") );

      verify_round_trip_conversion(abis, "s2", R"({"f0":{"i0":1,"i1":2},"i2":3})", "010203");

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(abi_serialize_json_mismatching_type)
{
   using eosio::testing::fc_exception_message_is;

   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "f0", "type": "s1"}
            {"name": "i1", "type": "int8"}
         ]}
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s2", fc::json::from_string(R"({"f0":1,"i1":2})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Unexpected input encountered while processing struct 's2.f0'") );

      verify_round_trip_conversion(abis, "s2", R"({"f0":{"i0":1},"i1":2})", "0102");

   } FC_LOG_AND_RETHROW()
}

// it is a bit odd to have an empty name for a field, but json seems to allow it
BOOST_AUTO_TEST_CASE(abi_serialize_json_empty_name)
{
   using eosio::testing::fc_exception_message_is;

   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "", "type": "int8"},
         ]}
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      auto bin = abis.variant_to_binary("s1", fc::json::from_string(R"({"":1})"), abi_serializer::create_yield_function( max_serialization_time ));

      verify_round_trip_conversion(abis, "s1", R"({"":1})", "01");

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(abi_serialize_detailed_error_messages)
{
   using eosio::testing::fc_exception_message_is;

   auto abi = R"({
      "version": "eosio::abi/1.1",
      "types": [
         {"new_type_name": "foo", "type": "s2"},
         {"new_type_name": "bar", "type": "foo"},
         {"new_type_name": "s1array", "type": "s1[]"},
         {"new_type_name": "s1arrayarray", "type": "s1array[]"}
      ],
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"}
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "f0", "type": "s1"},
            {"name": "i2", "type": "int8"}
         ]},
         {"name": "s3", "base": "s1", "fields": [
            {"name": "i2", "type": "int8"},
            {"name": "f3", "type": "v2"},
            {"name": "f4", "type": "foo$"},
            {"name": "f5", "type": "s1$"}
         ]},
         {"name": "s4", "base": "", "fields": [
            {"name": "f0", "type": "int8[]"},
            {"name": "f1", "type": "s1[]"}
         ]},
         {"name": "s5", "base": "", "fields": [
            {"name": "f0", "type": "v2[]"},
         ]},
      ],
      "variants": [
         {"name": "v1", "types": ["s3", "int8", "s4"]},
         {"name": "v2", "types": ["foo", "bar"]},
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("bar", fc::json::from_string(R"({"f0":{"i0":1},"i2":3})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's2.f0'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s3", fc::json::from_string(R"({"i0":1,"i2":3})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's3'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s3", fc::json::from_string(R"({"i0":1,"i1":2,"i2":3,"f3":["s2",{}]})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Specified type 's2' in input array is not valid within the variant 's3.f3'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s3", fc::json::from_string(R"({"i0":1,"i1":2,"i2":3,"f3":["bar",{"f0":{"i0":11},"i2":13}]})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's3.f3.<variant(1)=bar>.f0'") );

      verify_round_trip_conversion(abis, "s3", R"({"i0":1,"i1":2,"i2":3,"f3":["bar",{"f0":{"i0":11,"i1":12},"i2":13}]})", "010203010b0c0d");

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"(["s3",{"i0":1,"i1":2,"i2":3,"f3":["bar",{"f0":{"i0":11,"i1":12},"i2":13}],"f5":0}])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Unexpected field 'f5' found in input object while processing struct 'v1.<variant(0)=s3>'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"(["s4",{"f0":[0,1],"f1":[{"i0":2,"i1":3},{"i1":5}]}])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Missing field 'i0' in input object while processing struct 'v1.<variant(2)=s4>.f1[1]'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s2[]", fc::json::from_string(R"([{"f0":{"i0":1,"i1":2},"i2":3},{"f0":{"i0":4},"i2":6}])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 'ARRAY[1].f0'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s5", fc::json::from_string(R"({"f0":[["bar",{"f0":{"i0":1,"i1":2},"i2":3}],["foo",{"f0":{"i0":4},"i2":6}]]})"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's5.f0[1].<variant(0)=foo>.f0'") );

      verify_round_trip_conversion( abis, "s1arrayarray", R"([[{"i0":1,"i1":2},{"i0":3,"i1":4}],[{"i0":5,"i1":6},{"i0":7,"i1":8},{"i0":9,"i1":10}]])", "0202010203040305060708090a");

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s1arrayarray", fc::json::from_string(R"([[{"i0":1,"i1":2},{"i0":3,"i1":4}],[{"i0":6,"i1":6},{"i0":7,"i1":8},{"i1":10}]])"), abi_serializer::create_yield_function( max_serialization_time )),
                             pack_exception, fc_exception_message_is("Missing field 'i0' in input object while processing struct 'ARRAY[1][2]'") );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(abi_serialize_short_error_messages)
{
   using eosio::testing::fc_exception_message_is;

   auto abi = R"({
      "version": "eosio::abi/1.1",
      "types": [
         {"new_type_name": "foo", "type": "s2"},
         {"new_type_name": "bar", "type": "foo"},
         {"new_type_name": "s1array", "type": "s1[]"},
         {"new_type_name": "s1arrayarray", "type": "s1array[]"}
      ],
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"}
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "f0", "type": "s1"},
            {"name": "i2", "type": "int8"}
         ]},
         {"name": "very_very_very_very_very_very_very_very_very_very_long_struct_name_s3", "base": "s1", "fields": [
            {"name": "i2", "type": "int8"},
            {"name": "f3", "type": "v2"},
            {"name": "f4", "type": "foo$"},
            {"name": "very_very_very_very_very_very_very_very_very_very_long_field_name_f5", "type": "s1$"}
         ]},
         {"name": "s4", "base": "", "fields": [
            {"name": "f0", "type": "int8[]"},
            {"name": "f1", "type": "s1[]"}
         ]},
         {"name": "s5", "base": "", "fields": [
            {"name": "f0", "type": "v2[]"},
         ]},
      ],
      "variants": [
         {"name": "v1", "types": ["very_very_very_very_very_very_very_very_very_very_long_struct_name_s3", "int8", "s4"]},
         {"name": "v2", "types": ["foo", "bar"]},
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("bar", fc::json::from_string(R"({"f0":{"i0":1},"i2":3})"), abi_serializer::create_yield_function( max_serialization_time ), true),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's1'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary( "very_very_very_very_very_very_very_very_very_very_long_struct_name_s3",
                                                     fc::json::from_string(R"({"i0":1,"i2":3})"), abi_serializer::create_yield_function( max_serialization_time ), true ),
                             pack_exception,
                             fc_exception_message_is("Missing field 'i1' in input object while processing struct 'very_very_very_very_very_very_very_very_very_very_long_...ame_s3'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary( "very_very_very_very_very_very_very_very_very_very_long_struct_name_s3",
                                                     fc::json::from_string(R"({"i0":1,"i1":2,"i2":3,"f3":["s2",{}]})"), abi_serializer::create_yield_function( max_serialization_time ), true ),
                             pack_exception, fc_exception_message_is("Specified type 's2' in input array is not valid within the variant 'v2'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary( "very_very_very_very_very_very_very_very_very_very_long_struct_name_s3",
                                                     fc::json::from_string(R"({"i0":1,"i1":2,"i2":3,"f3":["bar",{"f0":{"i0":11},"i2":13}]})"), abi_serializer::create_yield_function( max_serialization_time ), true ),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's1'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary( "v1",
         fc::json::from_string(R"(["very_very_very_very_very_very_very_very_very_very_long_struct_name_s3",{"i0":1,"i1":2,"i2":3,"f3":["bar",{"f0":{"i0":11,"i1":12},"i2":13}],"very_very_very_very_very_very_very_very_very_very_long_field_name_f5":0}])"),
                                                     abi_serializer::create_yield_function( max_serialization_time ), true ),
                             pack_exception,
                             fc_exception_message_is("Unexpected field 'very_very_very_very_very_very_very_very_very_very_long_...ame_f5' found in input object while processing struct 'very_very_very_very_very_very_very_very_very_very_long_...ame_s3'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("v1", fc::json::from_string(R"(["s4",{"f0":[0,1],"f1":[{"i0":2,"i1":3},{"i1":5}]}])"), abi_serializer::create_yield_function( max_serialization_time ), true),
                             pack_exception, fc_exception_message_is("Missing field 'i0' in input object while processing struct 's1'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s2[]", fc::json::from_string(R"([{"f0":{"i0":1,"i1":2},"i2":3},{"f0":{"i0":4},"i2":6}])"), abi_serializer::create_yield_function( max_serialization_time ), true),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's1'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s5", fc::json::from_string(R"({"f0":[["bar",{"f0":{"i0":1,"i1":2},"i2":3}],["foo",{"f0":{"i0":4},"i2":6}]]})"), abi_serializer::create_yield_function( max_serialization_time ), true),
                             pack_exception, fc_exception_message_is("Missing field 'i1' in input object while processing struct 's1'") );

      BOOST_CHECK_EXCEPTION( abis.variant_to_binary("s1arrayarray", fc::json::from_string(R"([[{"i0":1,"i1":2},{"i0":3,"i1":4}],[{"i0":6,"i1":6},{"i0":7,"i1":8},{"i1":10}]])"), abi_serializer::create_yield_function( max_serialization_time ), true),
                             pack_exception, fc_exception_message_is("Missing field 'i0' in input object while processing struct 's1'") );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(abi_deserialize_detailed_error_messages)
{
   using eosio::testing::fc_exception_message_is;

   auto abi = R"({
      "version": "eosio::abi/1.1",
      "types": [
         {"new_type_name": "oint", "type": "int8?"},
         {"new_type_name": "os1", "type": "s1?"}
      ],
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"}
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "f0", "type": "int8[]"},
            {"name": "f1", "type": "s1[]"}
         ]},
         {"name": "s3", "base": "s1", "fields": [
            {"name": "i3", "type": "int8"},
            {"name": "i4", "type": "int8$"},
            {"name": "i5", "type": "int8"}
         ]},
         {"name": "s4", "base": "", "fields": [
            {"name": "f0", "type": "oint[]"}
         ]},
         {"name": "s5", "base": "", "fields": [
            {"name": "f0", "type": "os1[]"},
            {"name": "f1", "type": "v1[]"},
         ]},
         {"name": "s6", "base": "", "fields": [
         ]},
      ],
      "variants": [
         {"name": "v1", "types": ["int8", "s1"]},
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s2", fc::variant("020102").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Stream unexpectedly ended; unable to unpack field 'f1' of struct 's2'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s2", fc::variant("0201020103").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Stream unexpectedly ended; unable to unpack field 'i1' of struct 's2.f1[0]'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s2", fc::variant("020102ff").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Unable to unpack size of array 's2.f1'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s3", fc::variant("010203").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             abi_exception, fc_exception_message_is("Encountered field 'i5' without binary extension designation while processing struct 's3'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s3", fc::variant("02010304").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             abi_exception, fc_exception_message_is("Encountered field 'i5' without binary extension designation while processing struct 's3'") );

      // This check actually points to a problem with the current abi_serializer.
      // An array of optionals (which is unfortunately not rejected in validation) leads to an unpack_exception here because one of the optional elements is not present.
      // However, abis.binary_to_variant("s4", fc::variant("03010101020103").as<bytes>(), max_serialization_time) would work just fine!
      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s4", fc::variant("030101000103").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Invalid packed array 's4.f0[1]'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s4", fc::variant("020101").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Unable to unpack optional of built-in type 'int8' while processing 's4.f0[1]'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s5", fc::variant("02010102").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Unable to unpack presence flag of optional 's5.f0[1]'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s5", fc::variant("0001").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Unable to unpack tag of variant 's5.f1[0]'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s5", fc::variant("00010501").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Unpacked invalid tag (5) for variant 's5.f1[0]'") );

      BOOST_CHECK_EXCEPTION( abis.binary_to_variant("s5", fc::variant("00010101").as<bytes>(), abi_serializer::create_yield_function( max_serialization_time )),
                             unpack_exception, fc_exception_message_is("Stream unexpectedly ended; unable to unpack field 'i1' of struct 's5.f1[0].<variant(1)=s1>'") );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(serialize_optional_struct_type)
{
   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {"name": "s", "base": "", "fields": [
            {"name": "i0", "type": "int8"}
         ]},
      ],
   })";

   try {
      abi_serializer abis( fc::json::from_string(abi).as<abi_def>(), abi_serializer::create_yield_function( max_serialization_time ) );

      verify_round_trip_conversion(abis, "s?", R"({"i0":5})", "0105");
      verify_round_trip_conversion(abis, "s?", R"(null)", "00");

   } FC_LOG_AND_RETHROW()
}

template<class T>
inline std::pair<action_trace, std::string> generate_action_trace(const fc::optional<T> &  return_value, const std::string &  return_value_hex, bool parsable = true)
{
   action_trace at;
   at.action_ordinal = 0;
   at.creator_action_ordinal = 1;
   at.closest_unnotified_ancestor_action_ordinal = 2;
   at.receipt = fc::optional<action_receipt>{};
   at.receiver = action_name{"test"};
   at.act = eosio::chain::action(
      std::vector<eosio::chain::permission_level>{
         eosio::chain::permission_level{
            account_name{"acctest"},
            permission_name{"active"}}},
      account_name{"acctest"},
      action_name{"acttest"},
      bytes{fc::raw::pack(std::string{"test_data"})});
   at.elapsed = fc::microseconds{3};
   at.console = "console line";
   at.trx_id = transaction_id_type{"5d039021cf3262c5036a6ad40a809ae1440ae6c6792a48e6e95abf083b108d5f"};
   at.block_num = 4;
   at.block_time = block_timestamp_type{5};
   at.producer_block_id = fc::optional<block_id_type>{};
   if (return_value.valid()) {
      at.return_value = fc::raw::pack(*return_value);
   }
   std::stringstream expected_json;
   expected_json
      << "{"
      <<     "\"action_traces\":{"
      <<         "\"action_ordinal\":0,"
      <<         "\"creator_action_ordinal\":1,"
      <<         "\"closest_unnotified_ancestor_action_ordinal\":2,"
      <<         "\"receipt\":null,"
      <<         "\"receiver\":\"test\","
      <<         "\"act\":{"
      <<             "\"account\":\"acctest\","
      <<             "\"name\":\"acttest\","
      <<             "\"authorization\":[{"
      <<                 "\"actor\":\"acctest\","
      <<                 "\"permission\":\"active\""
      <<             "}],"
      <<             "\"data\":\"09746573745f64617461\""
      <<         "},"
      <<         "\"context_free\":false,"
      <<         "\"elapsed\":3,"
      <<         "\"console\":\"console line\","
      <<         "\"trx_id\":\"5d039021cf3262c5036a6ad40a809ae1440ae6c6792a48e6e95abf083b108d5f\","
      <<         "\"block_num\":4,"
      <<         "\"block_time\":\"2000-01-01T00:00:02.500\","
      <<         "\"producer_block_id\":null,"
      <<         "\"account_ram_deltas\":[],"
      <<         "\"account_disk_deltas\":[],"
      <<         "\"except\":null,"
      <<         "\"error_code\":null,"
      <<         "\"return_value_hex_data\":\"" << return_value_hex << "\"";
   if (return_value.valid() && parsable) {
      if (std::is_same<T, std::string>::value) {
         expected_json
            <<   ",\"return_value_data\":\"" << *return_value << "\"";
      }
      else {
         expected_json
             <<  ",\"return_value_data\":" << *return_value;
      }
   }
   expected_json
      <<     "}"
      << "}";

   return std::make_pair(at, expected_json.str());
}

inline std::pair<action_trace, std::string> generate_action_trace() {
   return generate_action_trace(fc::optional<char>(), "");
}

BOOST_AUTO_TEST_CASE(abi_to_variant__add_action__good_return_value)
{
   action_trace at;
   std::string expected_json;
   std::tie(at, expected_json) = generate_action_trace(fc::optional<uint16_t>{6}, "0600");

   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {"name": "acttest", "base": "", "fields": [
            {"name": "str", "type": "string"}
         ]},
      ],
      "action_results": [
         {
            "name": "acttest",
            "result_type": "uint16"
         }
      ]
   })";
   auto abidef = fc::json::from_string(abi).as<abi_def>();
   abi_serializer abis(abidef, abi_serializer::create_yield_function(max_serialization_time));

   mutable_variant_object mvo;
   eosio::chain::impl::abi_traverse_context ctx(abi_serializer::create_yield_function(max_serialization_time));
   eosio::chain::impl::abi_to_variant::add(mvo, "action_traces", at, get_resolver(abidef), ctx);
   std::string res = fc::json::to_string(mvo, fc::time_point::now() + max_serialization_time);

   BOOST_CHECK_EQUAL(res, expected_json);
}

BOOST_AUTO_TEST_CASE(abi_to_variant__add_action__bad_return_value)
{
   action_trace at;
   std::string expected_json;
   std::tie(at, expected_json) = generate_action_trace(fc::optional<std::string>{"no return"}, "096e6f2072657475726e", false);

   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {"name": "acttest", "base": "", "fields": [
            {"name": "str", "type": "string"}
         ]},
      ]
   })";
   auto abidef = fc::json::from_string(abi).as<abi_def>();
   abi_serializer abis(abidef, abi_serializer::create_yield_function(max_serialization_time));

   mutable_variant_object mvo;
   eosio::chain::impl::abi_traverse_context ctx(abi_serializer::create_yield_function(max_serialization_time));
   eosio::chain::impl::abi_to_variant::add(mvo, "action_traces", at, get_resolver(abidef), ctx);
   std::string res = fc::json::to_string(mvo, fc::time_point::now() + max_serialization_time);

   BOOST_CHECK_EQUAL(res, expected_json);
}

BOOST_AUTO_TEST_CASE(abi_to_variant__add_action__no_return_value)
{
   action_trace at;
   std::string expected_json;
   std::tie(at, expected_json) = generate_action_trace();

   auto abi = R"({
      "version": "eosio::abi/1.0",
      "structs": [
         {
            "name": "acttest",
            "base": "",
            "fields": [
               {"name": "str", "type": "string"}
            ]
         },
      ],
      "action_results": [
         {
            "name": "acttest",
            "result_type": "uint16"
         }
      ]
   })";
   auto abidef = fc::json::from_string(abi).as<abi_def>();
   abi_serializer abis(abidef, abi_serializer::create_yield_function(max_serialization_time));

   mutable_variant_object mvo;
   eosio::chain::impl::abi_traverse_context ctx(abi_serializer::create_yield_function(max_serialization_time));
   eosio::chain::impl::abi_to_variant::add(mvo, "action_traces", at, get_resolver(abidef), ctx);
   std::string res = fc::json::to_string(mvo, fc::time_point::now() + max_serialization_time);

   BOOST_CHECK_EQUAL(res, expected_json);
}

BOOST_AUTO_TEST_SUITE_END()
