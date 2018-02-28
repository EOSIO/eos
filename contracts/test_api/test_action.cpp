/**
 * @file action_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/compiler_builtins.h>

#include "test_api.hpp"
void test_action::read_action_normal() {

   char buffer[100];
   uint32_t total = 0;

   eosio_assert( current_receiver() == N(testapi),  "current_receiver() == N(testapi)" );

   eosio_assert(action_data_size() == sizeof(dummy_action), "action_size() == sizeof(dummy_action)");

   total = read_action_data(buffer, 30);
   eosio_assert(total == sizeof(dummy_action) , "read_action(30)" );

   total = read_action_data(buffer, 100);
   eosio_assert(total == sizeof(dummy_action) , "read_action(100)" );

   total = read_action_data(buffer, 5);
   eosio_assert(total == 5 , "read_action(5)" );

   total = read_action_data(buffer, sizeof(dummy_action) );
   eosio_assert(total == sizeof(dummy_action), "read_action(sizeof(dummy_action))" );

   dummy_action *dummy13 = reinterpret_cast<dummy_action *>(buffer);
   
   eosio_assert(dummy13->a == DUMMY_ACTION_DEFAULT_A, "dummy13->a == DUMMY_ACTION_DEFAULT_A");
   eosio_assert(dummy13->b == DUMMY_ACTION_DEFAULT_B, "dummy13->b == DUMMY_ACTION_DEFAULT_B");
   eosio_assert(dummy13->c == DUMMY_ACTION_DEFAULT_C, "dummy13->c == DUMMY_ACTION_DEFAULT_C");
}

void test_action::test_dummy_action() {
   char buffer[100];
   uint32_t total = 0;

   // get_action
   total = get_action( 1, 0, buffer, 0 );
   total = get_action( 1, 0, buffer, total );
   eosio_assert( total > 0, "get_action failed" );
   eosio::action act = eosio::get_action( 1, 0 );
   eosio_assert( eosio::pack_size(act) == total, "pack_size does not match get_action size" );
   eosio_assert( act.account == N(testapi), "expected testapi account" );

   dummy_action dum13 = act.data_as<dummy_action>();

   eosio_assert( dum13.a == DUMMY_ACTION_DEFAULT_A, "dum13.a == DUMMY_ACTION_DEFAULT_A" );
   eosio_assert( dum13.b == DUMMY_ACTION_DEFAULT_B, "dum13.b == DUMMY_ACTION_DEFAULT_B" );
   eosio_assert( dum13.c == DUMMY_ACTION_DEFAULT_C, "dum13.c == DUMMY_ACTION_DEFAULT_C" );

}

void test_action::read_action_to_0() {
   read_action_data((void *)0, action_size());
}

void test_action::read_action_to_64k() {
   read_action_data( (void *)((1<<16)-2), action_size());
}

void test_action::require_notice() {
   if( current_receiver() == N(testapi) ) {
      eosio::require_recipient( N(acc1) );
      eosio::require_recipient( N(acc2) );
      eosio::require_recipient( N(acc1), N(acc2) );
      eosio_assert(false, "Should've failed");
   } else if ( current_receiver() == N(acc1) || current_receiver() == N(acc2) ) {
      return;
   }
   eosio_assert(false, "Should've failed");
}

void test_action::require_auth() {
   prints("require_auth");
   eosio::require_auth( N(acc3) );
   eosio::require_auth( N(acc4) );
}

void test_action::assert_false() {
   eosio_assert(false, "test_action::assert_false");
}

void test_action::assert_true() {
   eosio_assert(true, "test_action::assert_true");
}

void test_action::test_abort() {
   abort();
   eosio_assert( false, "should've aborted" );
}

void test_action::test_publication_time() {
   uint32_t pub_time = 0;
   read_action_data(&pub_time, sizeof(uint32_t));
   eosio_assert( pub_time == publication_time(), "pub_time == publication_time()" );
}

void test_action::test_current_receiver() {
   account_name cur_rec;
   read_action_data(&cur_rec, sizeof(account_name));
   
   eosio_assert( current_receiver() == cur_rec, "the current receiver does not match" );
}

void test_action::test_current_sender() {
   account_name cur_send;
   read_action_data(&cur_send, sizeof(account_name));
   eosio_assert( current_sender() == cur_send, "the current sender does not match" );
}

void test_action::now() {
   uint32_t tmp = 0;
   uint32_t total = read_action_data(&tmp, sizeof(uint32_t));
   eosio_assert( total == sizeof(uint32_t), "total == sizeof(uint32_t)");
   eosio_assert( tmp == ::now(), "tmp == now()" );
}
