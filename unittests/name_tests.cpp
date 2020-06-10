#include <boost/test/unit_test.hpp>
#include <eosio/chain/name.hpp>

using namespace eosio::chain;

BOOST_AUTO_TEST_SUITE(name_test)

static constexpr uint64_t u64min = std::numeric_limits<uint64_t>::min(); // 0ULL
static constexpr uint64_t u64max = std::numeric_limits<uint64_t>::max(); // 18446744073709551615ULL

BOOST_AUTO_TEST_CASE(ctor_test) {
try {

   BOOST_TEST( name{}.to_uint64_t() == u64min );

   //// constexpr explicit name(uint64_t);
   BOOST_TEST( name{u64min}.to_uint64_t() == u64min );
   BOOST_TEST( name{1ULL}.to_uint64_t() == 1ULL );
   BOOST_TEST( name{u64max}.to_uint64_t() == u64max );

   //// explicit name(string_view);
   // Note:
   // These are the exact `uint64_t` value representations of the given string
   BOOST_TEST( name{"1"}.to_uint64_t() == 576460752303423488ULL );
   BOOST_TEST( name{"5"}.to_uint64_t() == 2882303761517117440ULL );
   BOOST_TEST( name{"a"}.to_uint64_t() == 3458764513820540928ULL );
   BOOST_TEST( name{"z"}.to_uint64_t() == 17870283321406128128ULL );

   BOOST_TEST( name{"abc"}.to_uint64_t() == 3589368903014285312ULL );
   BOOST_TEST( name{"123"}.to_uint64_t() == 614178399182651392ULL );

   BOOST_TEST( name{".abc"}.to_uint64_t() == 112167778219196416ULL );
   BOOST_TEST( name{".........abc"}.to_uint64_t() == 102016ULL );
   BOOST_TEST( name{"123."}.to_uint64_t() == 614178399182651392ULL );
   BOOST_TEST( name{"123........."}.to_uint64_t() == 614178399182651392ULL );
   BOOST_TEST( name{".a.b.c.1.2.3."}.to_uint64_t() == 108209673814966320ULL );

   BOOST_TEST( name{"abc.123"}.to_uint64_t() == 3589369488740450304ULL );
   BOOST_TEST( name{"123.abc"}.to_uint64_t() == 614181822271586304ULL );

   BOOST_TEST( name{"12345abcdefgj"}.to_uint64_t() == 614251623682315983ULL );
   BOOST_TEST( name{"hijklmnopqrsj"}.to_uint64_t() == 7754926748989239183ULL );
   BOOST_TEST( name{"tuvwxyz.1234j"}.to_uint64_t() == 14895601873741973071ULL );

   BOOST_TEST( name{"111111111111j"}.to_uint64_t() == 595056260442243615ULL );
   BOOST_TEST( name{"555555555555j"}.to_uint64_t() == 2975281302211218015ULL );
   BOOST_TEST( name{"aaaaaaaaaaaaj"}.to_uint64_t() == 3570337562653461615ULL );
   BOOST_TEST( name{"zzzzzzzzzzzzj"}.to_uint64_t() == u64max );

   BOOST_CHECK_THROW( name{"-1"}, name_type_exception );
   BOOST_CHECK_THROW( name{"0"}, name_type_exception );
   BOOST_CHECK_THROW( name{"6"}, name_type_exception );
   BOOST_CHECK_THROW( name{"111111111111k"}, name_type_exception );
   BOOST_CHECK_THROW( name{"zzzzzzzzzzzzk"}, name_type_exception );
   BOOST_CHECK_THROW( name{"12345abcdefghj"}, name_type_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(char_to_symbol_test) {
try {
   // --------------------------------------------
   // static constexpr uint8_t char_to_value(char);
   char c{'.'};
   uint8_t expected_value{}; // Will increment to the expected correct value in the set [0,32);
   BOOST_TEST( char_to_symbol(c) == expected_value );
   ++expected_value;

   for(c = '1'; c <= '5'; ++c) {
      BOOST_TEST( char_to_symbol(c) == expected_value );
      ++expected_value;
   }

   for(c = 'a'; c <= 'z'; ++c) {
      BOOST_TEST( char_to_symbol(c) == expected_value );
      ++expected_value;
   }

   BOOST_CHECK_THROW( char_to_symbol(char{'-'}), name_type_exception );
   BOOST_CHECK_THROW( char_to_symbol(char{'/'}), name_type_exception );
   BOOST_CHECK_THROW( char_to_symbol(char{'6'}), name_type_exception );
   BOOST_CHECK_THROW( char_to_symbol(char{'A'}), name_type_exception );
   BOOST_CHECK_THROW( char_to_symbol(char{'Z'}), name_type_exception );
   BOOST_CHECK_THROW( char_to_symbol(char{'`'}), name_type_exception );
   BOOST_CHECK_THROW( char_to_symbol(char{'{'}), name_type_exception );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(to_string_test) {
try {
   // -------------------------------
   // std::string to_string()const
   BOOST_TEST( name{"1"}.to_string() == "1" );
   BOOST_TEST( name{"5"}.to_string() == "5" );
   BOOST_TEST( name{"a"}.to_string() == "a" );
   BOOST_TEST( name{"z"}.to_string() == "z" );

   BOOST_TEST( name{"abc"}.to_string() == "abc" );
   BOOST_TEST( name{"123"}.to_string() == "123" );

   BOOST_TEST( name{".abc"}.to_string() == ".abc" );
   BOOST_TEST( name{".........abc"}.to_string() == ".........abc" );
   BOOST_TEST( name{"123."}.to_string() == "123" );
   BOOST_TEST( name{"123........."}.to_string() == "123" );
   BOOST_TEST( name{".a.b.c.1.2.3."}.to_string() == ".a.b.c.1.2.3" );

   BOOST_TEST( name{"abc.123"}.to_string() == "abc.123" );
   BOOST_TEST( name{"123.abc"}.to_string() == "123.abc" );

   BOOST_TEST( name{"12345abcdefgj"}.to_string() == "12345abcdefgj" );
   BOOST_TEST( name{"hijklmnopqrsj"}.to_string() == "hijklmnopqrsj" );
   BOOST_TEST( name{"tuvwxyz.1234j"}.to_string() == "tuvwxyz.1234j" );

   BOOST_TEST( name{"111111111111j"}.to_string() == "111111111111j" );
   BOOST_TEST( name{"555555555555j"}.to_string() == "555555555555j" );
   BOOST_TEST( name{"aaaaaaaaaaaaj"}.to_string() == "aaaaaaaaaaaaj" );
   BOOST_TEST( name{"zzzzzzzzzzzzj"}.to_string() == "zzzzzzzzzzzzj" );
   BOOST_TEST( name{""}.to_string() == "" );
   BOOST_TEST( name{"e"}.to_string() == "e" );
   BOOST_TEST( name{"eo"}.to_string() == "eo" );
   BOOST_TEST( name{"eos"}.to_string() == "eos" );
   BOOST_TEST( name{"eosi"}.to_string() == "eosi" );
   BOOST_TEST( name{"eosio"}.to_string() == "eosio" );
   BOOST_TEST( name{"eosioa"}.to_string() == "eosioa" );
   BOOST_TEST( name{"eosioac"}.to_string() == "eosioac" );
   BOOST_TEST( name{"eosioacc"}.to_string() == "eosioacc" );
   BOOST_TEST( name{"eosioacco"}.to_string() == "eosioacco" );
   BOOST_TEST( name{"eosioaccou"}.to_string() == "eosioaccou" );
   BOOST_TEST( name{"eosioaccoun"}.to_string() == "eosioaccoun" );
   BOOST_TEST( name{"eosioaccount"}.to_string() == "eosioaccount" );
   BOOST_TEST( name{"eosioaccountj"}.to_string() == "eosioaccountj" );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(operators_test) {
try {
      // ---------------------------------------
   // constexpr explicit operator bool()const
   // Note that I must be explicit about calling the operator because it is defined as `explicit`
   BOOST_TEST( name{0}.operator bool() == false );
   BOOST_TEST( !name{0}.operator bool() == true );
   BOOST_TEST( name{0}.empty() == true );
   BOOST_TEST( name{0}.good() == false );
   BOOST_TEST( name{1}.operator bool() == true );
   BOOST_TEST( !name{1}.operator bool() == false );
   BOOST_TEST( name{1}.empty() == false );
   BOOST_TEST( name{1}.good() == true );

   BOOST_TEST( name{""}.operator bool() == false );
   BOOST_TEST( !name{""}.operator bool() == true );
   BOOST_TEST( name{""}.empty() == true );
   BOOST_TEST( name{""}.good() == false );
   BOOST_TEST( name{"1"}.operator bool() == true );
   BOOST_TEST( !name{"1"}.operator bool() == false );
   BOOST_TEST( name{"1"}.empty() == false );
   BOOST_TEST( name{"1"}.good() == true );

   // ----------------------------------------------------------
   // friend constexpr bool operator == ( const name& a, const name& b )
   BOOST_TEST( name{"1"} == name{"1"} );
   BOOST_TEST( name{"5"} == name{"5"} );
   BOOST_TEST( name{"a"} == name{"a"} );
   BOOST_TEST( name{"z"} == name{"z"} );

   BOOST_TEST( name{"abc"} == name{"abc"} );
   BOOST_TEST( name{"123"} == name{"123"} );

   BOOST_TEST( name{".abc"} == name{".abc"} );
   BOOST_TEST( name{".........abc"} == name{".........abc"} );
   BOOST_TEST( name{"123."} == name{"123"} );
   BOOST_TEST( name{"123........."} == name{"123"} );
   BOOST_TEST( name{".a.b.c.1.2.3."} == name{".a.b.c.1.2.3"} );

   BOOST_TEST( name{"abc.123"} == name{"abc.123"} );
   BOOST_TEST( name{"123.abc"} == name{"123.abc"} );

   BOOST_TEST( name{"12345abcdefgj"} == name{"12345abcdefgj"} );
   BOOST_TEST( name{"hijklmnopqrsj"} == name{"hijklmnopqrsj"} );
   BOOST_TEST( name{"tuvwxyz.1234j"} == name{"tuvwxyz.1234j"} );

   BOOST_TEST( name{"111111111111j"} == name{"111111111111j"} );
   BOOST_TEST( name{"555555555555j"} == name{"555555555555j"} );
   BOOST_TEST( name{"aaaaaaaaaaaaj"} == name{"aaaaaaaaaaaaj"} );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} == name{"zzzzzzzzzzzzj"} );

   // -----------------------------------------------------------
   // friend constexpr bool operator != ( const name& a, const name& b )
   BOOST_TEST( name{"1"} != name{} );
   BOOST_TEST( name{"5"} != name{} );
   BOOST_TEST( name{"a"} != name{} );
   BOOST_TEST( name{"z"} != name{} );

   BOOST_TEST( name{"abc"} != name{} );
   BOOST_TEST( name{"123"} != name{} );

   BOOST_TEST( name{".abc"} != name{} );
   BOOST_TEST( name{".........abc"} != name{} );
   BOOST_TEST( name{"123."} != name{} );
   BOOST_TEST( name{"123........."} != name{} );
   BOOST_TEST( name{".a.b.c.1.2.3."} != name{} );

   BOOST_TEST( name{"abc.123"} != name{} );
   BOOST_TEST( name{"123.abc"} != name{} );

   BOOST_TEST( name{"12345abcdefgj"} != name{} );
   BOOST_TEST( name{"hijklmnopqrsj"} != name{} );
   BOOST_TEST( name{"tuvwxyz.1234j"} != name{} );

   BOOST_TEST( name{"111111111111j"} != name{} );
   BOOST_TEST( name{"555555555555j"} != name{} );
   BOOST_TEST( name{"aaaaaaaaaaaaj"} != name{} );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} != name{} );

   // ---------------------------------------------------------
   // friend constexpr bool operator < ( const name& a, const name& b )
   BOOST_TEST( name{} < name{"1"} );
   BOOST_TEST( name{"4"} < name{"5"} );
   BOOST_TEST( name{"1"} < name{"a"} );
   BOOST_TEST( name{"y"} < name{"z"} );

   BOOST_TEST( name{"aaa"} < name{"abc"} );
   BOOST_TEST( name{"122"} < name{"123"} );

   BOOST_TEST( name{".111"} < name{".abc"} );
   BOOST_TEST( name{"."} < name{".........abc"} );
   BOOST_TEST( name{"122."} < name{"123."} );
   BOOST_TEST( name{"122."} < name{"123........."} );
   BOOST_TEST( name{".1"} < name{".a.b.c.1.2.3."} );

   BOOST_TEST( name{"abb"} < name{"abc.123"} );
   BOOST_TEST( name{"123.aaa"} < name{"123.abc"} );

   BOOST_TEST( name{"12345abcdefga"} < name{"12345abcdefgj"} );
   BOOST_TEST( name{"hijklmnopqrsh"} < name{"hijklmnopqrsj"} );
   BOOST_TEST( name{"tuvwxyz.1234d"} < name{"tuvwxyz.1234j"} );

   BOOST_TEST( name{"111111111111a"} < name{"111111111111j"} );
   BOOST_TEST( name{"555555555555b"} < name{"555555555555j"} );
   BOOST_TEST( name{"aaaaaaaaaaaai"} < name{"aaaaaaaaaaaaj"} );
   BOOST_TEST( name{"zzzzzzzzzzzzi"} < name{"zzzzzzzzzzzzj"} );

   // ---------------------------------------------------------
   // friend constexpr bool operator <= ( const name& a, const name& b )
   BOOST_TEST( name{} <= name{"1"} );
   BOOST_TEST( name{"4"} <= name{"5"} );
   BOOST_TEST( name{"1"} <= name{"a"} );
   BOOST_TEST( name{"y"} <= name{"z"} );

   BOOST_TEST( name{"aaa"} <= name{"abc"} );
   BOOST_TEST( name{"122"} <= name{"123"} );

   BOOST_TEST( name{".111"} <= name{".abc"} );
   BOOST_TEST( name{"."} <= name{".........abc"} );
   BOOST_TEST( name{"122."} <= name{"123."} );
   BOOST_TEST( name{"123."} <= name{"123........."} );
   BOOST_TEST( name{".1"} <= name{".a.b.c.1.2.3."} );

   BOOST_TEST( name{"abb"} <= name{"abc.123"} );
   BOOST_TEST( name{"123.aaa"} <= name{"123.abc"} );

   BOOST_TEST( name{"12345abcdefga"} <= name{"12345abcdefgj"} );
   BOOST_TEST( name{"hijklmnopqrsh"} <= name{"hijklmnopqrsj"} );
   BOOST_TEST( name{"tuvwxyz.1234d"} <= name{"tuvwxyz.1234j"} );

   BOOST_TEST( name{"111111111111a"} <= name{"111111111111j"} );
   BOOST_TEST( name{"555555555555b"} <= name{"555555555555j"} );
   BOOST_TEST( name{"aaaaaaaaaaaaj"} <= name{"aaaaaaaaaaaaj"} );
   BOOST_TEST( name{"zzzzzzzzzzzzi"} <= name{"zzzzzzzzzzzzj"} );

      // ---------------------------------------------------------
   // friend constexpr bool operator < ( const name& a, const name& b )
   BOOST_TEST( name{"1"} > name{""} );
   BOOST_TEST( name{"5"} > name{"1"} );
   BOOST_TEST( name{"a"} > name{"5"} );
   BOOST_TEST( name{"z"} > name{"a"} );
   BOOST_TEST( name{"z1"} > name{"z"} );

   BOOST_TEST( name{"abd"} > name{"abc"} );
   BOOST_TEST( name{"124"} > name{"123"} );

   BOOST_TEST( name{".zzz"} > name{".abc"} );
   BOOST_TEST( name{".........z"} > name{".........abc"} );
   BOOST_TEST( name{"124."} > name{"123."} );
   BOOST_TEST( name{"124........."} > name{"123........."} );
   BOOST_TEST( name{".a.b.z.1.2.3."} > name{".a.b.c.1.2.3."} );

   BOOST_TEST( name{"abz.123"} > name{"abc.123"} );
   BOOST_TEST( name{"124.abc"} > name{"123.abc"} );

   BOOST_TEST( name{"13345abcdefgj"} > name{"12345abcdefgj"} );
   BOOST_TEST( name{"zijklmnopqrsj"} > name{"hijklmnopqrsj"} );
   BOOST_TEST( name{"tuvwxyz.1235j"} > name{"tuvwxyz.1234j"} );

   BOOST_TEST( name{"zzzzzzzzzzzzj"} > name{"111111111111j"} );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} > name{"555555555555j"} );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} > name{"aaaaaaaaaaaaj"} );

         // ---------------------------------------------------------
   // friend constexpr bool operator < ( const name& a, const name& b )
   BOOST_TEST( name{"1"} >= name{""} );
   BOOST_TEST( name{"5"} >= name{"1"} );
   BOOST_TEST( name{"a"} >= name{"5"} );
   BOOST_TEST( name{"z"} >= name{"a"} );
   BOOST_TEST( name{"z1"} >= name{"z"} );

   BOOST_TEST( name{"abd"} >= name{"abc"} );
   BOOST_TEST( name{"124"} >= name{"123"} );

   BOOST_TEST( name{".zzz"} >= name{".abc"} );
   BOOST_TEST( name{".........z"} >= name{".........abc"} );
   BOOST_TEST( name{"124."} >= name{"123."} );
   BOOST_TEST( name{"124........."} >= name{"123........."} );
   BOOST_TEST( name{".a.b.z.1.2.3."} >= name{".a.b.c.1.2.3."} );

   BOOST_TEST( name{"abz.123"} >= name{"abc.123"} );
   BOOST_TEST( name{"124.abc"} >= name{"123.abc"} );

   BOOST_TEST( name{"13345abcdefgj"} >= name{"12345abcdefgj"} );
   BOOST_TEST( name{"zijklmnopqrsj"} >= name{"hijklmnopqrsj"} );
   BOOST_TEST( name{"tuvwxyz.1235j"} >= name{"tuvwxyz.1234j"} );

   BOOST_TEST( name{"zzzzzzzzzzzzj"} >= name{"111111111111j"} );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} >= name{"555555555555j"} );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} >= name{"zzzzzzzzzzzzj"} );

   BOOST_TEST( name{"1"} == 576460752303423488ULL );
   BOOST_TEST( name{"5"} == 2882303761517117440ULL );
   BOOST_TEST( name{"a"} == 3458764513820540928ULL );
   BOOST_TEST( name{"z"} == 17870283321406128128ULL );

   BOOST_TEST( name{"abc"} == 3589368903014285312ULL );
   BOOST_TEST( name{"123"} == 614178399182651392ULL );

   BOOST_TEST( name{".abc"} == 112167778219196416ULL );
   BOOST_TEST( name{".........abc"} == 102016ULL );
   BOOST_TEST( name{"123."} == 614178399182651392ULL );
   BOOST_TEST( name{"123........."} == 614178399182651392ULL );
   BOOST_TEST( name{".a.b.c.1.2.3."} == 108209673814966320ULL );

   BOOST_TEST( name{"abc.123"} == 3589369488740450304ULL );
   BOOST_TEST( name{"123.abc"} == 614181822271586304ULL );

   BOOST_TEST( name{"12345abcdefgj"} == 614251623682315983ULL );
   BOOST_TEST( name{"hijklmnopqrsj"} == 7754926748989239183ULL );
   BOOST_TEST( name{"tuvwxyz.1234j"} == 14895601873741973071ULL );

   BOOST_TEST( name{"111111111111j"} == 595056260442243615ULL );
   BOOST_TEST( name{"555555555555j"} == 2975281302211218015ULL );
   BOOST_TEST( name{"aaaaaaaaaaaaj"} == 3570337562653461615ULL );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} == u64max );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(nmacro_test) {
try {
   
   // ------------------------------------
   // N() macro
   BOOST_TEST( name{} == N() );

   BOOST_TEST( name{"1"} == N(1) );
   BOOST_TEST( name{"5"} == N(5) );
   BOOST_TEST( name{"a"} == N(a) );
   BOOST_TEST( name{"z"} == N(z) );

   BOOST_TEST( name{"abc"} == N(abc) );
   BOOST_TEST( name{"123"} == N(123) );

   BOOST_TEST( name{".abc"} == N(.abc) );
   BOOST_TEST( name{".........abc"} == N(.........abc) );
   BOOST_TEST( name{"123."} == N(123.) );
   BOOST_TEST( name{"123........."} == N(123.........) );
   BOOST_TEST( name{".a.b.c.1.2.3."} == N(.a.b.c.1.2.3.) );

   BOOST_TEST( name{"abc.123"} == N(abc.123) );
   BOOST_TEST( name{"123.abc"} == N(123.abc) );

   BOOST_TEST( name{"12345abcdefgj"} == N(12345abcdefgj) );
   BOOST_TEST( name{"hijklmnopqrsj"} == N(hijklmnopqrsj) );
   BOOST_TEST( name{"tuvwxyz.1234j"} == N(tuvwxyz.1234j) );

   BOOST_TEST( name{"111111111111j"} == N(111111111111j) );
   BOOST_TEST( name{"555555555555j"} == N(555555555555j) );
   BOOST_TEST( name{"aaaaaaaaaaaaj"} == N(aaaaaaaaaaaaj) );
   BOOST_TEST( name{"zzzzzzzzzzzzj"} == N(zzzzzzzzzzzzj) );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()