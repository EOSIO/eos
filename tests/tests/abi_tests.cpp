#include <algorithm>
#include <vector>
#include <iterator>

#include <boost/test/unit_test.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/types/AbiSerializer.hpp>

#include "../common/database_fixture.hpp"

using namespace eos;
using namespace chain;
using namespace eos::types;

BOOST_AUTO_TEST_SUITE(abi_tests)

fc::variant verfiy_round_trip_conversion(const AbiSerializer& abis, const TypeName& type, const fc::variant& var)
{
   auto bytes = abis.variantToBinary(type, var);

   auto var2 = abis.binaryToVariant(type, bytes);

   std::string r = fc::json::to_string(var2);

   //std::cout << r << std::endl;

   auto bytes2 = abis.variantToBinary(type, var2);

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
        "abi_arr":"Abi[]"
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

BOOST_FIXTURE_TEST_CASE(transfer, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   const char* test_data = R"=====(
   {
     "from" : "from.acct",
     "to" : "to.acct",
     "amount" : 18446744073709551515,
     "memo" : "really important transfer"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto transfer = var.as<eos::types::transfer>();
   BOOST_CHECK_EQUAL("from.acct", transfer.from);
   BOOST_CHECK_EQUAL("to.acct", transfer.to);
   BOOST_CHECK_EQUAL(18446744073709551515u, transfer.amount);
   BOOST_CHECK_EQUAL("really important transfer", transfer.memo);

   auto var2 = verfiy_round_trip_conversion(abis, "transfer", var);
   auto transfer2 = var2.as<eos::types::transfer>();
   BOOST_CHECK_EQUAL(transfer.from, transfer2.from);
   BOOST_CHECK_EQUAL(transfer.to, transfer2.to);
   BOOST_CHECK_EQUAL(transfer.amount, transfer2.amount);
   BOOST_CHECK_EQUAL(transfer.memo, transfer2.memo);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(lock, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "from" : "from.acct",
     "to" : "to.acct",
     "amount" : -9223372036854775807,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto lock = var.as<eos::types::lock>();
   BOOST_CHECK_EQUAL("from.acct", lock.from);
   BOOST_CHECK_EQUAL("to.acct", lock.to);
   BOOST_CHECK_EQUAL(-9223372036854775807, lock.amount);

   auto var2 = verfiy_round_trip_conversion(abis, "lock", var);
   auto lock2 = var2.as<eos::types::lock>();
   BOOST_CHECK_EQUAL(lock.from, lock2.from);
   BOOST_CHECK_EQUAL(lock.to, lock2.to);
   BOOST_CHECK_EQUAL(lock.amount, lock2.amount);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(unlock, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "an.acct",
     "amount" : -9223372036854775807,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto unlock = var.as<eos::types::unlock>();
   BOOST_CHECK_EQUAL("an.acct", unlock.account);
   BOOST_CHECK_EQUAL(-9223372036854775807, unlock.amount);

   auto var2 = verfiy_round_trip_conversion(abis, "unlock", var);
   auto unlock2 = var2.as<eos::types::unlock>();
   BOOST_CHECK_EQUAL(unlock.account, unlock2.account);
   BOOST_CHECK_EQUAL(unlock.amount, unlock2.amount);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(claim, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "an.acct",
     "amount" : -9223372036854775807,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto claim = var.as<eos::types::claim>();
   BOOST_CHECK_EQUAL("an.acct", claim.account);
   BOOST_CHECK_EQUAL(-9223372036854775807, claim.amount);

   auto var2 = verfiy_round_trip_conversion(abis, "claim", var);
   auto claim2 = var2.as<eos::types::claim>();
   BOOST_CHECK_EQUAL(claim.account, claim2.account);
   BOOST_CHECK_EQUAL(claim.amount, claim2.amount);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(okproducer, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "voter" : "an.acct",
     "producer" : "an.acct2",
     "approve" : -128,
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto okproducer = var.as<eos::types::okproducer>();
   BOOST_CHECK_EQUAL("an.acct", okproducer.voter);
   BOOST_CHECK_EQUAL("an.acct2", okproducer.producer);
   BOOST_CHECK_EQUAL(-128, okproducer.approve);

   auto var2 = verfiy_round_trip_conversion(abis, "okproducer", var);
   auto okproducer2 = var2.as<eos::types::okproducer>();
   BOOST_CHECK_EQUAL(okproducer.voter, okproducer2.voter);
   BOOST_CHECK_EQUAL(okproducer.producer, okproducer2.producer);
   BOOST_CHECK_EQUAL(okproducer.approve, okproducer2.approve);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(setproducer, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   const char* test_data = R"=====(
   {
     "name" : "acct.name",
     "key" : "EOS5PnYq6BZn7H9GvL68cCLjWUZThRemTJoJmybCn1iEpVUXLb5Az",
     "configuration" : {
        "maxBlockSize" : 2147483135,
        "targetBlockSize" : 2147483145,
        "maxStorageSize" : 9223372036854775805,
        "electedPay" : -9223372036854775807,
        "runnerUpPay" : -9223372036854775717,
        "minEosBalance" : -9223372036854775707,
        "maxTrxLifetime" : 4294967071,
        "authDepthLimit" : 32777,
        "maxTrxRuntime" : 4294967007,
        "inlineDepthLimit" : 32770,
        "maxInlineMsgSize" : 4294966943,
        "maxGenTrxSize" : 4294966911
     }
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto setproducer = var.as<eos::types::setproducer>();
   BOOST_CHECK_EQUAL("acct.name", setproducer.name);
   BOOST_CHECK_EQUAL("EOS5PnYq6BZn7H9GvL68cCLjWUZThRemTJoJmybCn1iEpVUXLb5Az", (std::string)setproducer.key);
   BOOST_CHECK_EQUAL(2147483135u, setproducer.configuration.maxBlockSize);
   BOOST_CHECK_EQUAL(2147483145u, setproducer.configuration.targetBlockSize);
   BOOST_CHECK_EQUAL(9223372036854775805u, setproducer.configuration.maxStorageSize);
   BOOST_CHECK_EQUAL(-9223372036854775807, setproducer.configuration.electedPay);
   BOOST_CHECK_EQUAL(-9223372036854775717, setproducer.configuration.runnerUpPay);
   BOOST_CHECK_EQUAL(-9223372036854775707, setproducer.configuration.minEosBalance);
   BOOST_CHECK_EQUAL(4294967071u, setproducer.configuration.maxTrxLifetime);
   BOOST_CHECK_EQUAL(32777u, setproducer.configuration.authDepthLimit);
   BOOST_CHECK_EQUAL(4294967007u, setproducer.configuration.maxTrxRuntime);
   BOOST_CHECK_EQUAL(32770u, setproducer.configuration.inlineDepthLimit);
   BOOST_CHECK_EQUAL(4294966943u, setproducer.configuration.maxInlineMsgSize);
   BOOST_CHECK_EQUAL(4294966911u, setproducer.configuration.maxGenTrxSize);

   auto var2 = verfiy_round_trip_conversion(abis, "setproducer", var);
   auto setproducer2 = var2.as<eos::types::setproducer>();
   BOOST_CHECK_EQUAL(setproducer.configuration.maxBlockSize, setproducer2.configuration.maxBlockSize);
   BOOST_CHECK_EQUAL(setproducer.configuration.targetBlockSize, setproducer2.configuration.targetBlockSize);
   BOOST_CHECK_EQUAL(setproducer.configuration.maxStorageSize, setproducer2.configuration.maxStorageSize);
   BOOST_CHECK_EQUAL(setproducer.configuration.electedPay, setproducer2.configuration.electedPay);
   BOOST_CHECK_EQUAL(setproducer.configuration.runnerUpPay, setproducer2.configuration.runnerUpPay);
   BOOST_CHECK_EQUAL(setproducer.configuration.minEosBalance, setproducer2.configuration.minEosBalance);
   BOOST_CHECK_EQUAL(setproducer.configuration.maxTrxLifetime, setproducer2.configuration.maxTrxLifetime);
   BOOST_CHECK_EQUAL(setproducer.configuration.authDepthLimit, setproducer2.configuration.authDepthLimit);
   BOOST_CHECK_EQUAL(setproducer.configuration.maxTrxRuntime, setproducer2.configuration.maxTrxRuntime);
   BOOST_CHECK_EQUAL(setproducer.configuration.inlineDepthLimit, setproducer2.configuration.inlineDepthLimit);
   BOOST_CHECK_EQUAL(setproducer.configuration.maxInlineMsgSize, setproducer2.configuration.maxInlineMsgSize);
   BOOST_CHECK_EQUAL(setproducer.configuration.maxGenTrxSize, setproducer2.configuration.maxGenTrxSize);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(setproxy, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "stakeholder" : "stake.hldr",
     "proxy" : "stkhdr.prxy"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto setproxy = var.as<eos::types::setproxy>();
   BOOST_CHECK_EQUAL("stake.hldr", setproxy.stakeholder);
   BOOST_CHECK_EQUAL("stkhdr.prxy", setproxy.proxy);

   auto var2 = verfiy_round_trip_conversion(abis, "setproxy", var);
   auto setproxy2 = var2.as<eos::types::setproxy>();
   BOOST_CHECK_EQUAL(setproxy.stakeholder, setproxy2.stakeholder);
   BOOST_CHECK_EQUAL(setproxy.proxy, setproxy2.proxy);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(linkauth, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

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

   auto linkauth = var.as<eos::types::linkauth>();
   BOOST_CHECK_EQUAL("lnkauth.acct", linkauth.account);
   BOOST_CHECK_EQUAL("lnkauth.code", linkauth.code);
   BOOST_CHECK_EQUAL("lnkauth.type", linkauth.type);
   BOOST_CHECK_EQUAL("lnkauth.rqm", linkauth.requirement);

   auto var2 = verfiy_round_trip_conversion(abis, "linkauth", var);
   auto linkauth2 = var2.as<eos::types::linkauth>();
   BOOST_CHECK_EQUAL(linkauth.account, linkauth2.account);
   BOOST_CHECK_EQUAL(linkauth.code, linkauth2.code);
   BOOST_CHECK_EQUAL(linkauth.type, linkauth2.type);
   BOOST_CHECK_EQUAL(linkauth.requirement, linkauth2.requirement);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(unlinkauth, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "lnkauth.acct",
     "code" : "lnkauth.code",
     "type" : "lnkauth.type",
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto unlinkauth = var.as<eos::types::unlinkauth>();
   BOOST_CHECK_EQUAL("lnkauth.acct", unlinkauth.account);
   BOOST_CHECK_EQUAL("lnkauth.code", unlinkauth.code);
   BOOST_CHECK_EQUAL("lnkauth.type", unlinkauth.type);

   auto var2 = verfiy_round_trip_conversion(abis, "unlinkauth", var);
   auto unlinkauth2 = var2.as<eos::types::unlinkauth>();
   BOOST_CHECK_EQUAL(unlinkauth.account, unlinkauth2.account);
   BOOST_CHECK_EQUAL(unlinkauth.code, unlinkauth2.code);
   BOOST_CHECK_EQUAL(unlinkauth.type, unlinkauth2.type);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(updateauth, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "updauth.acct",
     "permission" : "updauth.prm",
     "parent" : "updauth.prnt",
     "authority" : {
        "threshold" : "2147483145",
        "keys" : [ {"key" : "EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", "weight" : 57005},
                   {"key" : "EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", "weight" : 57605} ],
        "accounts" : [ {"permission" : {"account" : "prm.acct1", "permission" : "prm.prm1"}, "weight" : 53005 },
                       {"permission" : {"account" : "prm.acct2", "permission" : "prm.prm2"}, "weight" : 53405 }]
     }
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto updateauth = var.as<eos::types::updateauth>();
   BOOST_CHECK_EQUAL("updauth.acct", updateauth.account);
   BOOST_CHECK_EQUAL("updauth.prm", updateauth.permission);
   BOOST_CHECK_EQUAL("updauth.prnt", updateauth.parent);
   BOOST_CHECK_EQUAL(2147483145u, updateauth.authority.threshold);

   BOOST_REQUIRE_EQUAL(2, updateauth.authority.keys.size());
   BOOST_CHECK_EQUAL("EOS65rXebLhtk2aTTzP4e9x1AQZs7c5NNXJp89W8R3HyaA6Zyd4im", (std::string)updateauth.authority.keys[0].key);
   BOOST_CHECK_EQUAL(57005u, updateauth.authority.keys[0].weight);
   BOOST_CHECK_EQUAL("EOS5eVr9TVnqwnUBNwf9kwMTbrHvX5aPyyEG97dz2b2TNeqWRzbJf", (std::string)updateauth.authority.keys[1].key);
   BOOST_CHECK_EQUAL(57605u, updateauth.authority.keys[1].weight);

   BOOST_REQUIRE_EQUAL(2, updateauth.authority.accounts.size());
   BOOST_CHECK_EQUAL("prm.acct1", updateauth.authority.accounts[0].permission.account);
   BOOST_CHECK_EQUAL("prm.prm1", updateauth.authority.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(53005u, updateauth.authority.accounts[0].weight);
   BOOST_CHECK_EQUAL("prm.acct2", updateauth.authority.accounts[1].permission.account);
   BOOST_CHECK_EQUAL("prm.prm2", updateauth.authority.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(53405u, updateauth.authority.accounts[1].weight);

   auto var2 = verfiy_round_trip_conversion(abis, "updateauth", var);
   auto updateauth2 = var2.as<eos::types::updateauth>();
   BOOST_CHECK_EQUAL(updateauth.account, updateauth2.account);
   BOOST_CHECK_EQUAL(updateauth.permission, updateauth2.permission);
   BOOST_CHECK_EQUAL(updateauth.parent, updateauth2.parent);

   BOOST_CHECK_EQUAL(updateauth.authority.threshold, updateauth2.authority.threshold);

   BOOST_REQUIRE_EQUAL(updateauth.authority.keys.size(), updateauth2.authority.keys.size());
   BOOST_CHECK_EQUAL(updateauth.authority.keys[0].key, updateauth2.authority.keys[0].key);
   BOOST_CHECK_EQUAL(updateauth.authority.keys[0].weight, updateauth2.authority.keys[0].weight);
   BOOST_CHECK_EQUAL(updateauth.authority.keys[1].key, updateauth2.authority.keys[1].key);
   BOOST_CHECK_EQUAL(updateauth.authority.keys[1].weight, updateauth2.authority.keys[1].weight);

   BOOST_REQUIRE_EQUAL(updateauth.authority.accounts.size(), updateauth2.authority.accounts.size());
   BOOST_CHECK_EQUAL(updateauth.authority.accounts[0].permission.account, updateauth2.authority.accounts[0].permission.account);
   BOOST_CHECK_EQUAL(updateauth.authority.accounts[0].permission.permission, updateauth2.authority.accounts[0].permission.permission);
   BOOST_CHECK_EQUAL(updateauth.authority.accounts[0].weight, updateauth2.authority.accounts[0].weight);
   BOOST_CHECK_EQUAL(updateauth.authority.accounts[1].permission.account, updateauth2.authority.accounts[1].permission.account);
   BOOST_CHECK_EQUAL(updateauth.authority.accounts[1].permission.permission, updateauth2.authority.accounts[1].permission.permission);
   BOOST_CHECK_EQUAL(updateauth.authority.accounts[1].weight, updateauth2.authority.accounts[1].weight);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(deleteauth, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

   BOOST_CHECK(true);
   const char* test_data = R"=====(
   {
     "account" : "delauth.acct",
     "permission" : "delauth.prm"
   }
   )=====";

   auto var = fc::json::from_string(test_data);

   auto deleteauth = var.as<eos::types::deleteauth>();
   BOOST_CHECK_EQUAL("delauth.acct", deleteauth.account);
   BOOST_CHECK_EQUAL("delauth.prm", deleteauth.permission);

   auto var2 = verfiy_round_trip_conversion(abis, "deleteauth", var);
   auto deleteauth2 = var2.as<eos::types::deleteauth>();
   BOOST_CHECK_EQUAL(deleteauth.account, deleteauth2.account);
   BOOST_CHECK_EQUAL(deleteauth.permission, deleteauth2.permission);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(newaccount, testing_fixture)
{ try {

   AbiSerializer abis(native_contract::native_contract_chain_initializer::eos_contract_abi());

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

   auto newaccount = var.as<eos::types::newaccount>();
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

   auto var2 = verfiy_round_trip_conversion(abis, "newaccount", var);
   auto newaccount2 = var2.as<eos::types::newaccount>();
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

BOOST_AUTO_TEST_SUITE_END()
