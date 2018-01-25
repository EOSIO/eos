/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/types.hpp>
#include <eoslib/token.hpp>
#include <eoslib/print.hpp>
#include <eoslib/db.hpp>
#include <eoslib/action.hpp>
#include <eosio.system/eosio.system.hpp>

namespace dice {

   using eos_tokens = eosiosystem::native_tokens;

   //@abi action
   struct offer_bet {
      eos_tokens    amount;
      account_name  player;
      checksum      commitment;
   };

   //@abi action
   struct cancel_offer {
      checksum  commitment;
   };

   //@abi action
   struct reveal {
      checksum  commitment;
      checksum  source;
   };

   //@abi action
   struct claim_expired {
      uint64_t gameid = 0;
   };

   struct offer_primary_key {
      account_name        owner;
      eos_tokens          bet;
   };

   /**
    *  This struct is used with i128xi128 index which allows us to lookup and
    *  iterate over offers by either {owner,bet,commitment} or {commitment,owner,bet} 
    *
    *  Only the first 128 bits of commitment ID are used in the index, but there will
    *  be a requirement that these first 128 bits be unique and any collisions will
    *  be rejected.
    *
    *  If there is no existing offer of equal bet size then gameid is set as 0, if it is 
    *  matched then a new game record is created and the offer is modified so that bet
    *  goes to 0 (and therefore moves to end of queue and not matched with other bets) and
    *  the gameid of both offers are updated.
    */
   
   //@abi table i128i128
   struct PACKED( offer ) {
      offer_primary_key primary;
      checksum          commitment;
      uint64_t          gameid = 0;
   };

   struct player {
      checksum     commitment;
      checksum     reveal;
   };

   //@abi table
   struct PACKED( game ) {
      uint64_t   gameid;
      eos_tokens bet;
      time       deadline;
      player     player1;
      player     player2;
   };

   //@abi table i64 global
   struct PACKED( global_dice ) {
      uint64_t nextgameid = 0;
   };

   //@abi table
   struct PACKED( account ) {
      account( account_name o = account_name() ):owner(o){}

      account_name        owner;
      eos_tokens          eos_balance;
      uint32_t            open_offers = 0;
      uint32_t            open_games = 0;

      bool is_empty()const { return ! ( bool(eos_balance) | open_offers | open_games ); }
   };

   using accounts = eosio::table<N(dice),N(dice),N(account),account,uint64_t>;
   using global_dices = eosio::table<N(dice),N(dice),N(global),global_dice,uint64_t>;
   using offers = eosio::table<N(dice),N(dice),N(offer),offer,offer_primary_key,uint128_t>;
   using games = eosio::table<N(dice),N(dice),N(game),game,uint64_t>;

   inline account get_account( account_name owner ) {
      account owned_account(owner);
      accounts::get( owned_account );
      return owned_account;
   }
};
