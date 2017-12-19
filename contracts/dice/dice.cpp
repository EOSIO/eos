/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

namespace dice {

void apply_offer( const offer_bet& offer ) {
   assert( offer.amount > 0, "insufficient bet" );
   assert( !hasOffer( offer.commitment ), "offer with this commitment already exist" );
   require_auth( offer.player );

   auto acnt = get_account( offer.player );
   acnt.balance -= offer.amount;
   acnt.open_offers++;
   save( acnt );

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
}

void apply_cancel( const cancel_offer& offer ) {
   /**
    *  Lookup offer in offer's table and assert that gameid == 0
    *  Lookup account and increment balance by bet 
    *  Deelte offer from offers table
    */
}

/**
 *  No authority required on this message so long as it is properly revealed
 */
void apply_reveal( const reveal& offer ) {
   assert_sha256( &offer.source, sizeof(offer.source), &offer.commitment );

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
}

/**
 *  No authority required by this so long as
 */
void apply_claim( const claim_expired& claim ) {
   /// lookup game by id, assert now() > deadline and deadline != 0
   /// pay the player that revealed 
   /// delete game and open offers 
}


}

