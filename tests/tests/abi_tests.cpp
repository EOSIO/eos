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
  "types": [{
      "newTypeName": "SameAsName",
      "type": "Name"
    },{
      "newTypeName": "SameAsB",
      "type": "B"
    },{
      "newTypeName": "SameAsTime",
      "type": "Time"
    }
  ],
  "structs": [{
      "name": "A",
      "base" : "SameAsB",
      "fields": {
        "u_int8"   : "UInt8",
        "u_int16"  : "UInt16",
        "u_int32"  : "UInt32",
        "u_int64"  : "UInt64",
        "u_int128" : "UInt128",
        "u_int256" : "UInt256"
      }
    },{
      "name": "B",
      "fields": {
        "s_int8"   : "Int8",
        "s_int16"  : "Int16",
        "s_int32"  : "Int32",
        "s_int64"  : "Int64",
        "other"    : "Other"
      }
    },{
      "name": "Other",
      "fields" : {
        "o_name"  : "SameAsName",
        "o_time"  : "SameAsTime",
        "o_chk"   : "Checksum",
        "o_sig"   : "Signature",
        "o_fix16" : "FixedString16",
        "o_fix32" : "FixedString32"
      }
    }
  ],
  "actions": [],
  "tables": []
}
)=====";

BOOST_FIXTURE_TEST_CASE(general, testing_fixture)
{ try {

   auto abi = fc::json::from_string(my_abi).as<Abi>();

   AbiSerializer abis(abi);
   abis.validate();

   const char *my_other = R"=====(
    {
      "u_int8"   : 1,
      "u_int16"  : 2,
      "u_int32"  : 3,
      "u_int64"  : 4,
      "u_int128" : "5",
      "u_int256" : "6",
      "s_int8"   : 7,
      "s_int16"  : 8,
      "s_int32"  : 9,
      "s_int64"  : 10,
      "other" : {
         "o_name"  : "test.name",
         "o_time"  : "2021-12-20T15:30",
         "o_chk"   : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
         "o_sig"   : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad00",
         "o_fix16" : "1234567890abcdef",
         "o_fix32" : "1234567890abcdef1234567890abcdef",
      }
    }
   )=====";

   auto var = fc::json::from_string(my_other);
   auto bytes = abis.variantToBinary("A", var);

   auto var2 = abis.binaryToVariant("A", bytes);
   std::string r = fc::json::to_string(var2);

   std::cout << r << std::endl;
   
   auto bytes2 = abis.variantToBinary("A", var2);

   BOOST_CHECK_EQUAL( fc::to_hex(bytes), fc::to_hex(bytes2) );
   //BOOST_CHECK_EXCEPTION( , fc::assert_exception, is_assert_exception );

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
