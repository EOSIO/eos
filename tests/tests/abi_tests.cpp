#include <algorithm>
#include <vector>
#include <iterator>

#include <boost/test/unit_test.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/types/AbiSerializer.hpp>

#include "../common/database_fixture.hpp"

using namespace eos;
using namespace chain;
using namespace eos::types;

BOOST_AUTO_TEST_SUITE(abi_tests)

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
        "publickey"      : "PublicKey",
        "publickey_arr"  : "PublicKey[]",
      }
    },{
      "name": "AssetTypes",
      "base" : "NativeTypes",
      "fields": {
        "asset"       : "Asset",
        "asset_arr"   : "Asset[]",
        "price"       : "Price",
        "price_arr"   : "Price[]",
      }
    },{
      "name": "NativeTypes",
      "base" : "GeneratedTypes",
      "fields" : {
        "string"            : "String",
        "string_arr"        : "String[]",
        "time"              : "Time",
        "time_arr"          : "Time[]",
        "signature"         : "Signature",
        "signature_arr"     : "Signature[]",
        "checksum"          : "Checksum",
        "checksum_arr"      : "Checksum[]",
        "fieldname"         : "FieldName",
        "fieldname_arr"     : "FieldName[]",
        "fixedstring32"     : "FixedString32",
        "fixedstring32_ar"  : "FixedString32[]",
        "fixedstring16"     : "FixedString16",
        "fixedstring16_ar"  : "FixedString16[]",
        "typename"          : "TypeName",
        "typename_arr"      : "TypeName[]",
        "bytes"             : "Bytes",
        "bytes_arr"         : "Bytes[]",
        "uint8"             : "UInt8",
        "uint8_arr"         : "UInt8[]",
        "uint16"            : "UInt16",
        "uint16_arr"        : "UInt16[]",
        "uint32"            : "UInt32",
        "uint32_arr"        : "UInt32[]",
        "uint64"            : "UInt64",
        "uint64_arr"        : "UInt64[]",
        "uint128"           : "UInt128",
        "uint128_arr"       : "UInt128[]",
        "uint256"           : "UInt256",
        "uint256_arr"       : "UInt256[]",
        "int8"              : "Int8",
        "int8_arr"          : "Int8[]",
        "int16"             : "Int16",
        "int16_arr"         : "Int16[]",
        "int32"             : "Int32",
        "int32_arr"         : "Int32[]",
        "int64"             : "Int64",
        "int64_arr"         : "Int64[]",
        "name"              : "Name",
        "name_arr"          : "Name[]",
        "field"             : "Field",
        "field_arr"         : "Field[]",
        "struct"            : "Struct",
        "struct_arr"        : "Struct[]",
        "fields"            : "Fields",
        "fields_arr"        : "Fields[]"

      }
    },{
      "name"   : "GeneratedTypes",
      "fields" : {
        "accountname":"AccountName",
        "accountname_arr":"AccountName[]",
        "permname":"PermissionName",
        "permname_arr":"PermissionName[]",
        "funcname":"FuncName",
        "funcname_arr":"FuncName[]",
        "messagename":"MessageName",
        "messagename_arr":"MessageName[]",
        "apermission":"AccountPermission",
        "apermission_arr":"AccountPermission[]",
        "message":"Message",
        "message_arr":"Message[]",
        "apweight":"AccountPermissionWeight",
        "apweight_arr":"AccountPermissionWeight[]",
        "transaction":"Transaction",
        "transaction_arr":"Transaction[]",
        "strx":"SignedTransaction",
        "strx_arr":"SignedTransaction[]",
        "kpweight":"KeyPermissionWeight",
        "kpweight_arr":"KeyPermissionWeight[]",
        "authority":"Authority",
        "authority_arr":"Authority[]",
        "blkcconfig":"BlockchainConfiguration",
        "blkcconfig_arr":"BlockchainConfiguration[]",
        "typedef":"TypeDef",
        "typedef_arr":"TypeDef[]",
        "action":"Action",
        "action_arr":"Action[]",
        "table":"Table",
        "table_arr":"Table[]",
        "abi":"Abi",
        "abi_arr":"Abi[]",
        "transfer":"transfer",
        "transfer_arr":"transfer[]",
        "lock":"lock",
        "lock_arr":"lock[]",
        "unlock":"unlock",
        "unlock_arr":"unlock[]",
        "claim":"claim",
        "claim_arr":"claim[]",
        "newaccount":"newaccount",
        "newaccount_arr":"newaccount[]",
        "setcode":"setcode",
        "setcode_arr":"setcode[]",
        "setproducer":"setproducer",
        "setproducer_arr":"setproducer[]",
        "okproducer":"okproducer",
        "okproducer_arr":"okproducer[]",
        "setproxy":"setproxy",
        "setproxy_arr":"setproxy[]",
        "uauth":"updateauth",
        "uauth_arr":"updateauth[]",
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
             "amount64": "UInt64",
             "amount32": "UInt32",
             "amount16": "UInt16",
             "amount8" : "UInt8"
           }
         }
       ],
       "actions": [],
       "tables": []
   }
   )=====";

   auto abi = fc::json::from_string(currency_abi).as<Abi>();

   AbiSerializer abis(abi);
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
   
   auto bytes = abis.variantToBinary("transfer", var);

   auto var2 = abis.binaryToVariant("transfer", bytes);
   
   std::string r = fc::json::to_string(var2);

   //std::cout << r << std::endl;
   
   auto bytes2 = abis.variantToBinary("transfer", var2);

   BOOST_CHECK_EQUAL( fc::to_hex(bytes), fc::to_hex(bytes2) );

} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE(general, testing_fixture)
{ try {

   auto abi = fc::json::from_string(my_abi).as<Abi>();

   AbiSerializer abis(abi);
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
        "refBlockNum":"1",
        "refBlockPrefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}]
      },
      "transaction_arr": [
      { 
        "refBlockNum":"1",
        "refBlockPrefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}]
      },
      { 
        "refBlockNum":"2",
        "refBlockPrefix":"3",
        "expiration":"2021-12-20T15:40",
        "scope":["acc3","acc4"],
        "messages":[{"code":"3344", "type":"type2", "data":"778899"}]
      }],
      "strx": {
        "refBlockNum":"1",
        "refBlockPrefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}],
        "signatures" : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00"],
        "authorizations" : [{"account":"acc1","permission":"permname1"},{"account":"acc2","permission":"permname2"}]
      },
      "strx_arr": [{
        "refBlockNum":"1",
        "refBlockPrefix":"2",
        "expiration":"2021-12-20T15:30",
        "scope":["acc1","acc2"],
        "messages":[{"code":"a1b2", "type":"type1", "data":"445566"}],
        "signatures" : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00"],
        "authorizations" : [{"account":"acc1","permission":"permname1"},{"account":"acc2","permission":"permname2"}]
      },{
        "refBlockNum":"1",
        "refBlockPrefix":"2",
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
      "blkcconfig": {"maxBlockSize": "100","targetBlockSize" : "200", "maxStorageSize":"300","electedPay" : "400", "runnerUpPay" : "500", "minEosBalance" : "600", "maxTrxLifetime"  : "700"},
      "blkcconfig_arr": [
        {"maxBlockSize": "100","targetBlockSize" : "200", "maxStorageSize":"300","electedPay" : "400", "runnerUpPay" : "500", "minEosBalance" : "600", "maxTrxLifetime"  : "700"},
        {"maxBlockSize": "100","targetBlockSize" : "200", "maxStorageSize":"300","electedPay" : "400", "runnerUpPay" : "500", "minEosBalance" : "600", "maxTrxLifetime"  : "700"}
      ],
      "typedef" : {"newTypeName":"new", "type":"old"},
      "typedef_arr": [{"newTypeName":"new", "type":"old"},{"newTypeName":"new", "type":"old"}],
      "action": {"action":"action1","type":"type1"},
      "action_arr": [{"action":"action1","type":"type1"},{"action":"action2","type":"type2"}],
      "table": {"table":"table1","type":"type1"},
      "table_arr": [{"table":"table1","type":"type1"},{"table":"table1","type":"type1"}],
      "abi":{
        "types" : [{"newTypeName":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
        "actions" : [{"action":"action1","type":"type1"}],
        "tables" : [{"table":"table1","type":"type1"}]
      },
      "abi_arr": [{
        "types" : [{"newTypeName":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
        "actions" : [{"action":"action1","type":"type1"}],
        "tables" : [{"table":"table1","type":"type1"}]
      },{
        "types" : [{"newTypeName":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
        "actions" : [{"action":"action1","type":"type1"}],
        "tables" : [{"table":"table1","type":"type1"}]
      }],
      "transfer": {"from":"acc1", "to":"acc2", "amount":"10"},
      "transfer_arr": [
        {"from":"acc1", "to":"acc2", "amount":"10"},
        {"from":"acc2", "to":"acc3", "amount":"20"}
      ],
      "lock": {"from":"acc1", "to":"acc2", "amount":"10"},
      "lock_arr": [
        {"from":"acc1", "to":"acc2", "amount":"10"},
        {"from":"acc1", "to":"acc2", "amount":"10"},
      ],
      "unlock": {"account":"aaa", "amount":111},
      "unlock_arr": [
        {"account":"aaa", "amount":111},
        {"account":"bbb", "amount":222}
      ],
      "claim": {"account":"acc1","amount":222},
      "claim_arr": [
        {"account":"acc2","amount":333},
        {"account":"acc2","amount":444},
      ],
      "newaccount": {
        "creator" : "cre1",
        "name"    : "name1",
        "owner"   : { 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "active":{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "recovery":{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "deposit" : "100.00 EOS"
      },
      "newaccount_arr": [{
        "creator" : "cre1",
        "name"    : "name1",
        "owner"   : { 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "active":{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "recovery":{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "deposit" : "100.00 EOS"
      },{
        "creator" : "cre1",
        "name"    : "name1",
        "owner"   : { 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "active":{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "recovery":{ 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        },
        "deposit" : "100.00 EOS"
      }],
      "setcode": {
        "account" : "xxx",
        "vmtype"  :  "44",
        "vmversion" : "55",
        "code"    : "556677",
        "abi"     : {
          "types" : [{"newTypeName":"new", "type":"old"}],
          "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
          "actions" : [{"action":"action1","type":"type1"}],
          "tables" : [{"table":"table1","type":"type1"}]
        }
      },
      "setcode_arr": [{
        "account" : "xxx",
        "vmtype"  :   "44",
        "vmversion" : "55",
        "code"    : "556677",
        "abi"     : {
          "types" : [{"newTypeName":"new", "type":"old"}],
          "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
          "actions" : [{"action":"action1","type":"type1"}],
          "tables" : [{"table":"table1","type":"type1"}]
        }
      },{
        "account" : "xxx",
        "vmtype"  : "44",
        "vmversion" : "55",
        "code"    : "556677",
        "abi"     : {
          "types" : [{"newTypeName":"new", "type":"old"}],
          "structs" : [{"name":"struct1", "base":"base1", "fields": {"name1":"type1", "name2":"type2", "name3":"type3", "name4":"type4"} }],
          "actions" : [{"action":"action1","type":"type1"}],
          "tables" : [{"table":"table1","type":"type1"}]
        }
      }],
      "setproducer": {
        "name" : "prodname",
        "key"  : "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
        "configuration" : {"maxBlockSize": "100","targetBlockSize" : "200", "maxStorageSize":"300","electedPay" : "400", "runnerUpPay" : "500", "minEosBalance" : "600", "maxTrxLifetime"  : "700"}
      },
      "setproducer_arr": [{
        "name" : "prodname",
        "key"  : "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
        "configuration" : {"maxBlockSize": "100","targetBlockSize" : "200", "maxStorageSize":"300","electedPay" : "400", "runnerUpPay" : "500", "minEosBalance" : "600", "maxTrxLifetime"  : "700"}
      },{
        "name" : "prodname2",
        "key"  : "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
        "configuration" : {"maxBlockSize": "100","targetBlockSize" : "300", "maxStorageSize":"300","electedPay" : "400", "runnerUpPay" : "500", "minEosBalance" : "600", "maxTrxLifetime"  : "700"}
      }],
      "okproducer": {
        "voter" : "voter1",
        "producer" : "prodnme1",
        "approve" : 1
      },
      "okproducer_arr": [{
        "voter" : "voter1",
        "producer" : "prodnme1",
        "approve" : 1
      },{
        "voter" : "voter2",
        "producer" : "prodnme2",
        "approve" : 0
      }],
      "setproxy":{
        "stakeholder" : "sholder",
        "proxy" : "proxyname"
      },
      "setproxy_arr":[{
        "stakeholder" : "sholder",
        "proxy" : "proxyname"
      },{
        "stakeholder" : "sholder2",
        "proxy" : "proxyname2"
      }],
      "uauth":{
        "account" : "acc1",
        "permission" : "perm1",
        "parent" : "parent1",
        "authority" : { 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
       }
      },
      "uauth_arr": [{
        "account" : "acc1",
        "permission" : "perm1",
        "parent" : "parent1",
        "authority" : { 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        }},{
        "account" : "acc1",
        "permission" : "perm1",
        "parent" : "parent1",
        "authority" : { 
         "threshold":"10", 
         "keys":[{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"100"},{"key":"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "weight":"200"}], 
         "accounts":[{"permission":{"account":"acc1","permission":"permname1"},"weight":"1"},{"permission":{"account":"acc2","permission":"permname2"},"weight":"2"}]
        }
      }],
      "deletepermission": {
        "account" : "acc1",
        "permission" : "perm1"
      },
      "deletepermission_arr": [{
        "account" : "acc1",
        "permission" : "perm1"
      },{
        "account" : "acc2",
        "permission" : "perm2"
      }]
    }
   )=====";

   auto var = fc::json::from_string(my_other);

   auto bytes = abis.variantToBinary("A", var);
   auto var2 = abis.binaryToVariant("A", bytes);
   std::string r = fc::json::to_string(var2);

   std::cout << r << std::endl;
   
   auto bytes2 = abis.variantToBinary("A", var2);

   BOOST_CHECK_EQUAL( fc::to_hex(bytes), fc::to_hex(bytes2) );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
