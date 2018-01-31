#include <boost/test/unit_test.hpp>

#include <eosio/chain/transaction.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/raw.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace eosio::chain;
using namespace std;
namespace bio = boost::iostreams;

struct test_action {
   string value;

   static account_name get_account() {
      return N(test.account);
   }

   static action_name get_name() {
      return N(test.action);
   }
};

// 3x deflated bomb
// we only inflate once but this makes this very small to store in this test case as long as we decompress it 2x before use
static const char deflate_bomb[] = "789c012e00d1ff789cabb8f5f6a021230303c3a1055ffd7379e417308c8251300a46c1281805a360d8837d6c8c0f18180190e708b7ec451357";

/**
 * Utility predicate to check whether an EOS exception has the given message/code
 */
template<typename E>
struct exception_is {
   exception_is(string expected)
      :expected(expected)
   {}

   bool operator()( const fc::exception& ex ) {
      auto message = ex.get_log().at(0).get_message();
      return ex.code() == E::code_value && boost::algorithm::ends_with(message, expected);
   }

   string expected;
};

FC_REFLECT(test_action, (value));

BOOST_AUTO_TEST_SUITE(transaction_test)

   BOOST_AUTO_TEST_CASE(compress_transaction)
   {
      try {

         std::string expected = "78da63606060d8bf7ff5eab2198ace8c13962fe3909cb0f114835aa9248866044a3284784ef402d12bde1a19090a1425e6a5e4e72aa42496242a64a416a50200a9d114bb";

         transaction trx;
         trx.region = 0xBFBFU;
         trx.ref_block_num = 0xABABU;
         trx.ref_block_prefix = 0x43219876UL;
         trx.actions.emplace_back(vector<permission_level>{{N(decomp), config::active_name}},
                                  test_action {"random data here"});

         packed_transaction t;
         t.set_transaction(trx, packed_transaction::zlib);

         auto actual = fc::to_hex(t.data);
         BOOST_CHECK_EQUAL(expected, actual);
      } FC_LOG_AND_RETHROW();

   }

   BOOST_AUTO_TEST_CASE(decompress_transaction)
   {
      try {
         transaction expected;
         expected.region = 0xBFBFU;
         expected.ref_block_num = 0xABABU;
         expected.ref_block_prefix = 0x43219876UL;
         expected.actions.emplace_back(vector<permission_level>{{N(decomp), config::active_name}},
                                       test_action {"random data here"});

         char compressed_tx_raw[] = "78da63606060d8bf7ff5eab2198ace8c13962fe3909cb0f114835aa9248866044a3284784ef402d12bde1a19090a1425e6a5e4e72aa42496242a64a416a50200a9d114bb";

         packed_transaction t;
         t.data.resize((sizeof(compressed_tx_raw) - 1) / 2);
         fc::from_hex(compressed_tx_raw, t.data.data(), t.data.size());
         t.compression= packed_transaction::zlib;

         auto actual = t.get_transaction();
         BOOST_CHECK_EQUAL(expected.region, actual.region);
         BOOST_CHECK_EQUAL(expected.ref_block_num, actual.ref_block_num);
         BOOST_CHECK_EQUAL(expected.ref_block_prefix, actual.ref_block_prefix);
         BOOST_REQUIRE_EQUAL(expected.actions.size(), actual.actions.size());
         BOOST_CHECK_EQUAL((string)expected.actions[0].name, (string)actual.actions[0].name);
         BOOST_CHECK_EQUAL((string)expected.actions[0].account, (string)actual.actions[0].account);
         BOOST_REQUIRE_EQUAL(expected.actions[0].authorization.size(), actual.actions[0].authorization.size());
         BOOST_CHECK_EQUAL((string)expected.actions[0].authorization[0].actor, (string)actual.actions[0].authorization[0].actor);
         BOOST_CHECK_EQUAL((string)expected.actions[0].authorization[0].permission, (string)actual.actions[0].authorization[0].permission);
         BOOST_REQUIRE_EQUAL(fc::to_hex(expected.actions[0].data), fc::to_hex(actual.actions[0].data));

      } FC_LOG_AND_RETHROW();

   }

   BOOST_AUTO_TEST_CASE(decompress_bomb)
   {
      try {
         bytes bomb_bytes;
         bomb_bytes.resize((sizeof(deflate_bomb) - 1) / 2);
         fc::from_hex(deflate_bomb, bomb_bytes.data(), bomb_bytes.size());

         packed_transaction t;

         // double inflate the bomb into the tx data
         bio::filtering_ostream decomp;
         decomp.push(bio::zlib_decompressor());
         decomp.push(bio::zlib_decompressor());
         decomp.push(bio::back_inserter(t.data));
         bio::write(decomp, bomb_bytes.data(), bomb_bytes.size());
         bio::close(decomp);

         t.compression= packed_transaction::zlib;

         BOOST_REQUIRE_EXCEPTION(t.get_transaction(), fc::exception, exception_is<tx_decompression_error>("Exceeded maximum decompressed transaction size"));
      } FC_LOG_AND_RETHROW();

   }

BOOST_AUTO_TEST_SUITE_END()
