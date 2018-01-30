#include <boost/test/unit_test.hpp>

#include <eosio/chain/transaction.hpp>
#include <eosio/chain/contracts/types.hpp>

#include <fc/io/raw.hpp>

using namespace eosio::chain;
using namespace std;

struct test_action {
   string value;

   static account_name get_account() {
      return N(test.account);
   }

   static action_name get_name() {
      return N(test.action);
   }
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

         signed_transaction t;
         t.set_transaction(trx, signed_transaction::zlib);

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

         signed_transaction t;
         t.data.resize((sizeof(compressed_tx_raw) - 1) / 2);
         fc::from_hex(compressed_tx_raw, t.data.data(), t.data.size());
         t.compression= signed_transaction::zlib;

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

BOOST_AUTO_TEST_SUITE_END()
