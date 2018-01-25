/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "dice.hpp"

#include <eoslib/crypto.h>
#include <eoslib/memory.h>

#define FIVE_MINUTES 5*60  //5 minutes

namespace dice {

using namespace eosio;

inline void save( const account& a ) {
   if( a.is_empty() ) {
      accounts::remove(a);
   }
   else {
      accounts::store(a);
   }
}

bool has_offer( const checksum& commitment ) {
   offer tmp;
   return offers::secondary_index::get(*reinterpret_cast<const uint128_t*>(&commitment), tmp);
}

bool is_equal(const checksum& a, const checksum& b) {
   return memcmp((void *)&a, (const void *)&b, sizeof(checksum)) == 0;
}

bool is_zero(const checksum& a) {
   return a.hash[0] == 0 && a.hash[1] == 0 && a.hash[2] == 0 && a.hash[3] == 0;
}

void pay_and_clean(const game& g, const offer& winner, const offer& loser) {
   auto winner_account = winner.primary.owner;
   auto loser_account  = loser.primary.owner;

   account acnt = get_account(winner_account);
   acnt.eos_balance.quantity += g.bet.quantity;
   acnt.open_games--;
   save(acnt);

   acnt = get_account(loser_account);
   acnt.open_games--;
   save(acnt);
   
   games::remove(g);
   offers::remove(winner);
   offers::remove(loser);
}

template<typename Lambda>
inline void modify_account( account_name a, Lambda&& modify ) {
   auto acnt = get_account( a );
   modify( acnt );
   save( acnt );
}

void apply_offer_bet( const offer_bet& new_offer_bet ) {
   assert( new_offer_bet.amount.quantity > 0, "insufficient bet" );
   assert( !has_offer( new_offer_bet.commitment ), "offer with this commitment already exist" );
   require_auth( new_offer_bet.player );

   auto acnt = get_account( new_offer_bet.player );
   acnt.eos_balance -= new_offer_bet.amount;
   acnt.open_offers++;

   /**
    *  1. Lookup lowerbound on offer's by offer_primary_key
    *     if lowerbound.primary.bet == offer.bet
    *          global_dice.nextgameid++  (load and save to DB)
    *          create new game and set game.bet == offer.bet
    *          set player1 == lowerbound player
    *          set player2 == offer.player
    *          update lowerbound.primary.bet = 0 and lwoerbound.gameid = global_dice.nextgameid
    *          Create offer entry in offers table with bet = 0 and gameid == --^
    *     else      
    *          Create offer entry in offers table with bet = offer.bet and gameid = 0
    */

   offer new_offer;
   new_offer.primary.bet   = new_offer_bet.amount;
   new_offer.primary.owner = new_offer_bet.player;
   new_offer.commitment    = new_offer_bet.commitment;
   new_offer.gameid        = 0;

   offer_primary_key pk;
   pk.owner = 0;
   pk.bet   = new_offer_bet.amount;

   offer result;
   bool res = offers::primary_index::lower_bound(pk, result);
   if( res && result.primary.bet == new_offer_bet.amount && result.primary.owner != new_offer_bet.player ) {

      // Increment global game counter
      global_dice gdice;
      global_dices::front(gdice);
      gdice.nextgameid++;
      global_dices::store(gdice);

      // Create a new game
      game new_game;
      new_game.deadline           = 0;
      new_game.gameid             = gdice.nextgameid;
      new_game.bet.quantity       = 2*new_offer_bet.amount.quantity;
      new_game.player1.commitment = new_offer_bet.commitment;
      memset(&new_game.player1.reveal, 0, sizeof(uint256));
      new_game.player2.commitment = result.commitment;
      memset(&new_game.player2.reveal, 0, sizeof(uint256));
      games::store(new_game);

      // Update player's offer
      new_offer.primary.bet.quantity = 0;
      new_offer.gameid = gdice.nextgameid;

      offers::remove(result);
      result.primary.bet.quantity = 0;
      result.gameid = gdice.nextgameid;
      offers::store(result);

      // Update player's accounts
      auto other = get_account(result.primary.owner);
      other.open_offers--;
      other.open_games++;
      save(other);

      acnt.open_offers--;
      acnt.open_games++;
   }

   save( acnt );
   offers::store(new_offer);
}

void apply_cancel_offer( const cancel_offer& offer ) {
   /**
    *  Lookup offer in offer's table and assert that gameid == 0
    *  Lookup account and increment balance by bet 
    *  Deelte offer from offers table
    */

   struct offer offer_to_cancel;
   auto offer_exists = offers::secondary_index::get(*reinterpret_cast<const uint128_t*>(&offer.commitment), offer_to_cancel);
   assert(offer_exists, "offer does not exists");
   assert(offer_to_cancel.gameid == 0, "unable to cancel offer");

   require_auth( offer_to_cancel.primary.owner );
   auto acnt = get_account(offer_to_cancel.primary.owner);
   acnt.open_offers--;
   acnt.eos_balance += offer_to_cancel.primary.bet;
   save(acnt);

   offers::remove(offer_to_cancel);
}

/**
 *  No authority required on this message so long as it is properly revealed
 */

void apply_reveal( const reveal& reveal_info ) {
   /**
    *  assert offer already exists
    *  assert offer has a gameid > 0
    *
    *  Lookup game by gameid
    *  assert game.player[x].reveal is 0  hasn't already been revealed
    *  set game.player[x].reveal to revealed
    *
    *  if( game.player[!x].reveal != 0 ) {
    *     uint256_t result;
    *     sha256( &game.player1, sizeof(player)*2, result);
    *     if( result.words[1] < result.words[0] ) {
    *        pay_player1 by incrementing account by bet and decrement open games
    *     }
    *     else 
    *        pay_player2 by incrmenting account by bet and decrement open games
    *     delete game, and both offers
    *  }
    *  else {
    *     set game.deadline = now() + timeout (5 minutes)
    *  }
    *
    */

   assert_sha256( (char *)&reveal_info.source, sizeof(reveal_info.source), (const checksum *)&reveal_info.commitment );

   struct offer offer[2];
   auto res = offers::secondary_index::get(*reinterpret_cast<const uint128_t*>(&reveal_info.commitment), offer[0]);

   assert(res, "offer not found");
   assert(offer[0].gameid > 0, "unable to reveal");

   struct game game;
   games::get(offer[0].gameid, game);

   struct player& player_0 = game.player1;
   struct player& player_1 = game.player2;
   
   if( !is_equal(game.player1.commitment, reveal_info.commitment) ) {
      auto tmp = player_0;
      player_0 = player_1;
      player_1 = tmp;
   }

   assert( is_zero(player_0.reveal) == true, "player already revealed");
   player_0.reveal = reveal_info.source;

   if( !is_zero(player_1.reveal) ) {
      
      offers::secondary_index::get(*reinterpret_cast<const uint128_t*>(&player_1.commitment), offer[1]);

      checksum result;
      sha256( (char *)&game.player1, sizeof(player)*2, &result);

      int winner = result.hash[1] < result.hash[0] ? 0 : 1;
      int loser  = winner^1;

      pay_and_clean(game, offer[winner], offer[loser]);

   } else {
      game.deadline = now() + FIVE_MINUTES;
      games::update(game);
   }

}

/**
 *  No authority required by this so long as
 */
void apply_claim_expired( const claim_expired& claim ) {
   /// lookup game by id, assert now() > deadline and deadline != 0
   /// pay the player that revealed 
   /// delete game and open offers 

   struct game game;
   auto res = games::get(claim.gameid, game);
   assert(res, "invalid game");

   assert(game.deadline != 0 && now() > game.deadline, "game not expired");

   struct offer offer[2];
   
   const auto& player1_commitment = game.player1.commitment;
   const auto& player2_commitment = game.player2.commitment;

   offers::secondary_index::get(*reinterpret_cast<const uint128_t*>(&player1_commitment), offer[0]);
   offers::secondary_index::get(*reinterpret_cast<const uint128_t*>(&player2_commitment), offer[1]);

   if( is_zero(game.player1.reveal) ) {
      auto tmp = offer[0];
      offer[0] = offer[1];
      offer[1] = tmp;
   }

   pay_and_clean(game, offer[0], offer[1]);
}

void apply_eos_transfer( const eosiosystem::transfer& transfer ) {
   if( transfer.to == N(dice) ) {
      modify_account( transfer.from, [&]( account& mod_account ){
         mod_account.eos_balance += transfer.quantity;
      });
   } else if( transfer.from == N(dice) ) {
      require_auth( transfer.to ); /// require the receiver of funds (account owner) to authorize this transfer

      modify_account( transfer.to, [&]( account& mod_account ){
         mod_account.eos_balance -= transfer.quantity;
      });
   } else {
      assert( false, "notified on transfer that is not relevant to this game" );
   }
}

}

using namespace dice;

extern "C" {

   void apply( uint64_t code, uint64_t action ) {
      if( code == N(dice) ) {
         switch( action ) {
            case N(offerbet):
               apply_offer_bet( current_action<dice::offer_bet>() );
               break;
            case N(canceloffer):
               apply_cancel_offer( current_action<dice::cancel_offer>() );
               break;
            case N(reveal):
               apply_reveal( current_action<dice::reveal>() );
               break;
            case N(claimexpired):
               apply_claim_expired( current_action<dice::claim_expired>() );
               break;
            default:
               assert( false, "unknown action" );
         }
      } 
      else if( code == N(eosio) ) {
         if( action == N(transfer) ) 
            apply_eos_transfer( current_action<eosiosystem::transfer>() );
      } 
      else {
         assert(false, "invalid action");
      }
   }
}

