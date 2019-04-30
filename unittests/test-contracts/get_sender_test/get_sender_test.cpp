/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "get_sender_test.hpp"
#include <eosio/transaction.hpp>

using namespace eosio;

void get_sender_test::assertsender( name expected_sender ) {
   check( get_sender() == expected_sender, "sender did not match" );
}

void get_sender_test::sendinline( name to, name expected_sender ) {
   assertsender_action a( to,  std::vector<eosio::permission_level>{} );
   a.send( expected_sender );
}

void get_sender_test::notify( name to, name expected_sender, bool send_inline ) {
   require_recipient( to );
}

void get_sender_test::on_notify( name to, name expected_sender, bool send_inline ) {
   if( send_inline ) {
      assertsender_action a( get_first_receiver(),  std::vector<eosio::permission_level>{} );
      a.send( expected_sender );
   } else {
      check( get_sender() == expected_sender, "sender did not match" );
   }
}
