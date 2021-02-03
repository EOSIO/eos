#define BOOST_TEST_MODULE subjective_billing
#include <boost/test/included/unit_test.hpp>

#include <eosio/producer_plugin/subjective_billing.hpp>

#include <eosio/testing/tester.hpp>

namespace {

using namespace eosio;
using namespace eosio::chain;

BOOST_AUTO_TEST_SUITE( subjective_billing_test )

BOOST_AUTO_TEST_CASE( subjective_bill_test ) {

   fc::logger log;

   transaction_id_type id1 = sha256::hash( "1" );
   transaction_id_type id2 = sha256::hash( "2" );
   transaction_id_type id3 = sha256::hash( "3" );
   transaction_id_type id4 = sha256::hash( "4" );
   transaction_id_type id5 = sha256::hash( "5" );
   transaction_id_type id6 = sha256::hash( "6" );
   account_name a = "a"_n;
   account_name b = "b"_n;
   account_name c = "c"_n;

   {  // Failed transactions remain until expired in subjective billing.
      subjective_billing sub_bill;

      sub_bill.subjective_bill( id1, time_point::now(), a, fc::microseconds( 13 ), false );
      sub_bill.subjective_bill( id2, time_point::now(), a, fc::microseconds( 11 ), false );
      sub_bill.subjective_bill( id3, time_point::now(), b, fc::microseconds( 9 ), false );

      BOOST_CHECK_EQUAL( 13+11, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 9, sub_bill.get_subjective_bill(b, 0) );

      sub_bill.on_block({});
      sub_bill.abort_block(); // they all failed so nothing in aborted block

      BOOST_CHECK_EQUAL( 13+11, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 9, sub_bill.get_subjective_bill(b, 0) );

      // expires transactions but leaves them in the decay at full value
      sub_bill.remove_expired( log, time_point::now() + fc::microseconds(1), 0, fc::time_point::maximum() );

      BOOST_CHECK_EQUAL( 13+11, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 9, sub_bill.get_subjective_bill(b, 0) );
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(c, 0) );

      // ensure that the value decays away at the window
      constexpr uint32_t window_size = subjective_billing::expired_accumulator_average_window;
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(a, window_size) );
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(b, window_size) );
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(c, window_size) );
   }
   {  // db_read_mode HEAD mode, so transactions are immediately reverted
      subjective_billing sub_bill;

      sub_bill.subjective_bill( id1, time_point::now(), a, fc::microseconds( 23 ), false );
      sub_bill.subjective_bill( id2, time_point::now(), a, fc::microseconds( 19 ), false );
      sub_bill.subjective_bill( id3, time_point::now(), b, fc::microseconds( 7 ), false );

      BOOST_CHECK_EQUAL( 23+19, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 7, sub_bill.get_subjective_bill(b, 0) );

      sub_bill.on_block({}); // have not seen any of the transactions come back yet
      sub_bill.abort_block();

      BOOST_CHECK_EQUAL( 23+19, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 7, sub_bill.get_subjective_bill(b, 0) );

      sub_bill.on_block({});
      sub_bill.remove_subjective_billing( id1, 0 ); // simulate seeing id1 come back in block (this is what on_block would do)
      sub_bill.abort_block();

      BOOST_CHECK_EQUAL( 19, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 7, sub_bill.get_subjective_bill(b, 0) );
   }
   {  // db_read_mode SPECULATIVE mode, so transactions are in pending block until aborted
      subjective_billing sub_bill;

      sub_bill.subjective_bill( id1, time_point::now(), a, fc::microseconds( 23 ), true );
      sub_bill.subjective_bill( id2, time_point::now(), a, fc::microseconds( 19 ), false ); // trx outside of block
      sub_bill.subjective_bill( id3, time_point::now(), a, fc::microseconds( 55 ), true );
      sub_bill.subjective_bill( id4, time_point::now(), b, fc::microseconds( 3 ), true );
      sub_bill.subjective_bill( id5, time_point::now(), b, fc::microseconds( 7 ), true );
      sub_bill.subjective_bill( id6, time_point::now(), a, fc::microseconds( 11 ), false ); // trx outside of block

      BOOST_CHECK_EQUAL( 19+11, sub_bill.get_subjective_bill(a, 0) ); // should not include what is in the pending block
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(b, 0) );     // should not include what is in the pending block
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(c, 0) );

      sub_bill.on_block({});  // have not seen any of the transactions come back yet
      sub_bill.abort_block(); // aborts the pending block, so subjective billing needs to include the reverted trxs

      BOOST_CHECK_EQUAL( 23+19+55+11, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 3+7, sub_bill.get_subjective_bill(b, 0) );

      sub_bill.on_block({});
      sub_bill.remove_subjective_billing( id3, 0 ); // simulate seeing id3 come back in block (this is what on_block would do)
      sub_bill.remove_subjective_billing( id4, 0 ); // simulate seeing id4 come back in block (this is what on_block would do)
      sub_bill.abort_block();

      BOOST_CHECK_EQUAL( 23+19+11, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 7, sub_bill.get_subjective_bill(b, 0) );
   }
   { // failed handling logic, decay with repeated failures should be exponential, single failures should be linear
      subjective_billing sub_bill;
      constexpr uint32_t window_size = subjective_billing::expired_accumulator_average_window;

      sub_bill.subjective_bill_failure(a, fc::microseconds(1024), 0);
      sub_bill.subjective_bill_failure(b, fc::microseconds(1024), 0);
      BOOST_CHECK_EQUAL( 1024, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 1024, sub_bill.get_subjective_bill(b, 0) );

      sub_bill.subjective_bill_failure(a, fc::microseconds(1024), window_size >> 1);
      BOOST_CHECK_EQUAL( 512 + 1024, sub_bill.get_subjective_bill(a, window_size >> 1) );
      BOOST_CHECK_EQUAL( 512, sub_bill.get_subjective_bill(b, window_size >> 1) );

      sub_bill.subjective_bill_failure(a, fc::microseconds(1024), window_size);
      BOOST_CHECK_EQUAL( 256 + 512 + 1024, sub_bill.get_subjective_bill(a, window_size) );
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(b, window_size) );
   }

   { // expired handling logic, full billing until expiration then failed/decay logic
      subjective_billing sub_bill;
      constexpr uint32_t window_size = subjective_billing::expired_accumulator_average_window;
      const auto now = time_point::now();

      sub_bill.subjective_bill( id1, now, a, fc::microseconds( 1024 ), false );
      sub_bill.subjective_bill( id2, now + fc::microseconds(1), a, fc::microseconds( 1024 ), false );
      sub_bill.subjective_bill( id3, now, b, fc::microseconds( 1024 ), false );
      BOOST_CHECK_EQUAL( 1024 + 1024, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 1024, sub_bill.get_subjective_bill(b, 0) );

      sub_bill.remove_expired( log, now, 0, fc::time_point::maximum() );
      BOOST_CHECK_EQUAL( 1024 + 1024, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 1024, sub_bill.get_subjective_bill(b, 0) );

      BOOST_CHECK_EQUAL( 512 + 1024, sub_bill.get_subjective_bill(a, window_size >> 1) );
      BOOST_CHECK_EQUAL( 512, sub_bill.get_subjective_bill(b, window_size >> 1) );

      BOOST_CHECK_EQUAL( 1024, sub_bill.get_subjective_bill(a, window_size) );
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(b, window_size) );

      sub_bill.remove_expired( log, now + fc::microseconds(1), 0, fc::time_point::maximum() );
      BOOST_CHECK_EQUAL( 1024 + 1024, sub_bill.get_subjective_bill(a, 0) );
      BOOST_CHECK_EQUAL( 1024, sub_bill.get_subjective_bill(b, 0) );

      BOOST_CHECK_EQUAL( 512 + 512, sub_bill.get_subjective_bill(a, window_size >> 1) );
      BOOST_CHECK_EQUAL( 512, sub_bill.get_subjective_bill(b, window_size >> 1) );

      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(a, window_size) );
      BOOST_CHECK_EQUAL( 0, sub_bill.get_subjective_bill(b, window_size) );
   }

}

BOOST_AUTO_TEST_SUITE_END()

}
