#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <infinite/infinite.wast.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(limits_tests)

BOOST_AUTO_TEST_CASE(infinite_loop) {
#if 0
   try {
      // set up runtime limits
      tester t({fc::milliseconds(1), fc::milliseconds(1)});
      t.produce_block();

      t.create_account(N(infinite));
      t.set_code(N(infinite), infinite_wast);

      {
         action act;
         act.account = N(infinite);
         act.name = N(op);
         act.authorization = vector<permission_level>{{N(infinite), config::active_name}};

         signed_transaction trx;
         trx.actions.emplace_back(std::move(act));

         t.set_tapos(trx);
         trx.sign(t.get_private_key(N(infinite), "active"), chain_id_type());

         BOOST_REQUIRE_THROW(t.push_transaction(trx), checktime_exceeded);
      }
   } FC_LOG_AND_RETHROW();
#endif
}

BOOST_AUTO_TEST_SUITE_END()
