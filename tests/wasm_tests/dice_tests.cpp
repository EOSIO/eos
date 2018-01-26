#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <dice/dice.wast.hpp>
#include <dice/dice.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

struct offer_bet_t {
   int64_t       amount;
   account_name  player;
   checksum_type commitment;

   static account_name get_account() { return N(dice); }
   static action_name get_name() {return N(offerbet); }
};
FC_REFLECT(offer_bet_t, (amount)(player)(commitment));

struct cancel_offer_t {
   checksum_type commitment;

   static account_name get_account() { return N(dice); }
   static action_name get_name() {return N(canceloffer); } 
};
FC_REFLECT(cancel_offer_t, (commitment));

struct reveal_t {
   checksum_type commitment;
   checksum_type source;

   static account_name get_account() { return N(dice); }
   static action_name get_name() {return N(reveal); } 
};
FC_REFLECT(reveal_t, (commitment)(source));

struct __attribute((packed)) account_t {
   account_name owner;
   uint64_t     eos_balance;
   uint32_t     open_offers;
   uint32_t     open_games;
};
FC_REFLECT(account_t, (owner)(eos_balance)(open_offers)(open_games));

struct player_t {
   checksum_type commitment;
   checksum_type reveal;
};
FC_REFLECT(player_t, (commitment)(reveal));

struct __attribute((packed)) game_t {
   uint64_t             gameid;
   uint64_t             bet;
   fc::time_point_sec   deadline;
   player_t             player1;
   player_t             player2;
};
FC_REFLECT(game_t, (gameid)(bet)(deadline)(player1)(player2));

struct dice_tester : tester {

   void offer_bet(account_name account, int64_t amount, const checksum_type& commitment) {
      signed_transaction trx;
      action act( {{account, config::active_name}},
         offer_bet_t{amount, account, commitment} );
      trx.actions.push_back(act);
      set_tapos(trx);
      trx.sign(get_private_key( account, "active" ), chain_id_type());
      control->push_transaction(trx);
   }

   void cancel_offer(account_name account, const checksum_type& commitment) {
      signed_transaction trx;
      action act( {{account, config::active_name}},
         cancel_offer_t{commitment} );
      trx.actions.push_back(act);
      set_tapos(trx);
      trx.sign(get_private_key( account, "active" ), chain_id_type());
      control->push_transaction(trx);      
   }

   void deposit(account_name account, const char* amount) {
      transfer( account, N(dice), amount, "" );      
   }

   void reveal(account_name account, const checksum_type& commitment, const checksum_type& source ) {
      signed_transaction trx;
      action act( {{account, config::active_name}},
         reveal_t{commitment, source} );
      trx.actions.push_back(act);
      set_tapos(trx);
      trx.sign(get_private_key( account, "active" ), chain_id_type());
      control->push_transaction(trx);
   }

   bool dice_account(account_name account, account_t& acnt) {
      auto* maybe_tid = find_table(N(dice), N(dice), N(account));
      if(maybe_tid == nullptr) return false;

      auto* o = control->get_database().find<contracts::key_value_object, contracts::by_scope_primary>(boost::make_tuple(maybe_tid->id, account));
      if(o == nullptr) return false;

      auto* p = reinterpret_cast<char *>(&acnt);
      memcpy(p, &o->primary_key, sizeof(uint64_t));
      memcpy(p+sizeof(uint64_t), o->value.data(), sizeof(acnt)-sizeof(uint64_t));
      return true;
   }

   bool dice_game(uint64_t game_id, game_t& game) {
      auto* maybe_tid = find_table(N(dice), N(dice), N(game));
      if(maybe_tid == nullptr) return false;

      auto* o = control->get_database().find<contracts::key_value_object, contracts::by_scope_primary>(boost::make_tuple(maybe_tid->id, game_id));
      if(o == nullptr) return false;

      auto* p = reinterpret_cast<char *>(&game);
      memcpy(p, &o->primary_key, sizeof(uint64_t));
      memcpy(p+sizeof(uint64_t), o->value.data(), sizeof(game)-sizeof(uint64_t));
      return true;
   }

   uint32_t open_games(account_name account) {
      account_t acnt;
      if(!dice_account(account, acnt)) return 0;
      return acnt.open_games;
   }

   uint64_t game_bet(uint64_t game_id) {
      game_t game;
      if(!dice_game(game_id, game)) return 0;
      return game.bet;
   }

   uint32_t open_offers(account_name account) {
      account_t acnt;
      if(!dice_account(account, acnt)) return 0;
      return acnt.open_offers;
   }

   int64_t balance_of(account_name account) {
      account_t acnt;
      if(!dice_account(account, acnt)) return 0;
      return (int64_t)acnt.eos_balance;
   }

   checksum_type commitment_for( const char* secret ) {
      return commitment_for(checksum_type(secret));
   }

   checksum_type commitment_for( const checksum_type& secret ) {
      return fc::sha256::hash( secret.data(), sizeof(secret) );
   }
};

BOOST_AUTO_TEST_SUITE(dice_tests)

BOOST_FIXTURE_TEST_CASE( dice_test, dice_tester ) try {

   create_accounts( {N(dice),N(alice),N(bob),N(carol)}, 
      asset::from_string("10000.0000 EOS") );
   produce_block();

   transfer( N(inita), N(dice),  "10000.0000 EOS", "" );
   transfer( N(inita), N(alice), "10000.0000 EOS", "" );
   transfer( N(inita), N(bob),   "10000.0000 EOS", "" );
   transfer( N(inita), N(carol), "10000.0000 EOS", "" );
   produce_block();

   set_code(N(dice), dice_wast);
   set_abi(N(dice), dice_abi);

   produce_block();

   // Alice deposits 1000 EOS
   deposit( N(alice), "1000.0000 EOS"); 
   produce_block();

   BOOST_REQUIRE_EQUAL( balance_of(N(alice)), asset::from_string("1000.0000 EOS").amount);
   BOOST_REQUIRE_EQUAL( open_games(N(alice)), 0);

   // Alice tries to bet 0 EOS (fail)
   // secret : 9b886346e1351d4144d0b8392a975612eb0f8b6de7eae1cc9bcc55eb52be343c
   BOOST_CHECK_THROW( offer_bet( N(alice), asset::from_string("0.0000 EOS").amount, 
      commitment_for("9b886346e1351d4144d0b8392a975612eb0f8b6de7eae1cc9bcc55eb52be343c")
   ), fc::exception);
   
   // Alice bets 10 EOS (success)
   // secret : 0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46
   offer_bet( N(alice), asset::from_string("10.0000 EOS").amount, 
      commitment_for("0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46")
   );
   produce_block();

   // Bob tries to bet using a secret that has the same 128bits as Alice (fail)
   // secret : 00000000000000000000000000000002c334abe6ce13219a4cf3b862abb03c46
   BOOST_CHECK_THROW( offer_bet( N(bob), asset::from_string("10.0000 EOS").amount,
      commitment_for("0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46")
   ), fc::exception);
   produce_block();

   // Alice tries to bet 1000 EOS (fail)
   // secret : a512f6b1b589a8906d574e9de74a529e504a5c53a760f0991a3e00256c027971
   BOOST_CHECK_THROW( offer_bet( N(alice), asset::from_string("10000.0000 EOS").amount, 
      commitment_for("a512f6b1b589a8906d574e9de74a529e504a5c53a760f0991a3e00256c027971")
   ), fc::exception);
   produce_block();

   // Bob tries to bet 90 EOS without deposit
   // secret : 4facfc98932dde46fdc4403125a16337f6879a842a7ff8b0dc8e1ecddd59f3c8
   BOOST_CHECK_THROW( offer_bet( N(bob), asset::from_string("90.0000 EOS").amount, 
      commitment_for("4facfc98932dde46fdc4403125a16337f6879a842a7ff8b0dc8e1ecddd59f3c8")
   ), fc::exception);
   produce_block();

   // Bob deposits 500 EOS
   deposit( N(bob), "500.0000 EOS");
   BOOST_REQUIRE_EQUAL( balance_of(N(bob)), asset::from_string("500.0000 EOS").amount);

   // Bob bets 11 EOS (success)
   // secret : eec3272712d974c474a3e7b4028b53081344a5f50008e9ccf918ba0725a8d784
   offer_bet( N(bob), asset::from_string("11.0000 EOS").amount, 
      commitment_for("eec3272712d974c474a3e7b4028b53081344a5f50008e9ccf918ba0725a8d784")
   );
   produce_block();

   // const auto& idx = get_index<key128x128_value_index, by_scope_primary>();
   // BOOST_CHECK_EQUAL(idx.size(), 2);

   // const auto& dice_account = control->get_database().get<account_object,by_name>( N(dice) );
   // abi_def abi;
   // BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(dice_account.abi, abi), true);
   // abi_serializer abi_ser(abi);
   // for(auto& o : idx) {
   //    bytes b;
   //    b.insert(b.end(),(char*)&o.primary_key,(char*)&o.primary_key+sizeof(o.primary_key));
   //    b.insert(b.end(),(char*)&o.secondary_key,(char*)&o.secondary_key+sizeof(o.secondary_key));
   //    b.insert(b.end(),(char*)o.value.data(),(char*)(o.value.data()+o.value.size()));
   //    auto v = abi_ser.binary_to_variant("offer", b);
   //    wdump((v));
   // }

   // Bob cancels (success)
   BOOST_REQUIRE_EQUAL( open_offers(N(bob)), 1);
   cancel_offer( N(bob), commitment_for("eec3272712d974c474a3e7b4028b53081344a5f50008e9ccf918ba0725a8d784") );
   BOOST_REQUIRE_EQUAL( open_offers(N(bob)), 0);

   // Carol deposits 300 EOS
   deposit( N(carol), "300.0000 EOS");

   // Carol bets 10 EOS (success)
   // secret : 3efb4bd5e19b780f4980c919330c0306f8157f93db1fc72c7cefec63e0e7f37a
   offer_bet( N(carol), asset::from_string("10.0000 EOS").amount, 
      commitment_for("3efb4bd5e19b780f4980c919330c0306f8157f93db1fc72c7cefec63e0e7f37a")
   );
   produce_block();

   BOOST_REQUIRE_EQUAL( open_games(N(alice)), 1);
   BOOST_REQUIRE_EQUAL( open_offers(N(alice)), 0);
   
   BOOST_REQUIRE_EQUAL( open_games(N(carol)), 1);
   BOOST_REQUIRE_EQUAL( open_offers(N(carol)), 0);

   BOOST_REQUIRE_EQUAL( game_bet(1), (uint64_t)asset::from_string("20.0000 EOS").amount);


   // Alice tries to cancel a nonexistent bet (fail)
   BOOST_CHECK_THROW( cancel_offer( N(alice), 
      commitment_for("00000000000000000000000000000000000000000000000000000000abb03c46")
   ), fc::exception);

   // Alice tries to cancel an in-game bet (fail)
   BOOST_CHECK_THROW( cancel_offer( N(alice), 
      commitment_for("0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46")
   ), fc::exception);

   // Alice reveals secret (success)
   reveal( N(alice), 
      commitment_for("0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46"),
      checksum_type("0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46")
   );
   produce_block();

   // Alice tries to reveal again (fail)
   BOOST_CHECK_THROW( reveal( N(alice), 
      commitment_for("0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46"),
      checksum_type("0ba044d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46")
   ), fc::exception);

   // Bob tries to reveal an invalid (secret,commitment) pair (fail)
   BOOST_CHECK_THROW( reveal( N(bob), 
      commitment_for("121344d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46"),
      checksum_type("141544d2833758ee2c8f24d8a3f70c82c334abe6ce13219a4cf3b862abb03c46")
   ), fc::exception);

   // Bob tries to reveal a valid (secret,commitment) pair that has no offer/game (fail)
   BOOST_CHECK_THROW( reveal( N(bob), 
      commitment_for("e48c6884bb97ac5f5951df6012ce79f63bb8549ad0111315ad9ecbaf4c9b1eb8"),
      checksum_type("e48c6884bb97ac5f5951df6012ce79f63bb8549ad0111315ad9ecbaf4c9b1eb8")
   ), fc::exception);

   // Bob reveals Carol's secret (success)
   reveal( N(bob), 
      commitment_for("3efb4bd5e19b780f4980c919330c0306f8157f93db1fc72c7cefec63e0e7f37a"),
      checksum_type("3efb4bd5e19b780f4980c919330c0306f8157f93db1fc72c7cefec63e0e7f37a")
   );

   BOOST_REQUIRE_EQUAL( open_games(N(alice)), 0);
   BOOST_REQUIRE_EQUAL( open_offers(N(alice)), 0);
   BOOST_REQUIRE_EQUAL( balance_of(N(alice)), asset::from_string("1010.0000 EOS").amount);
   
   BOOST_REQUIRE_EQUAL( open_games(N(carol)), 0);
   BOOST_REQUIRE_EQUAL( open_offers(N(carol)), 0);
   BOOST_REQUIRE_EQUAL( balance_of(N(carol)), asset::from_string("290.0000 EOS").amount);

} FC_LOG_AND_RETHROW() /// basic_test

BOOST_AUTO_TEST_SUITE_END()
