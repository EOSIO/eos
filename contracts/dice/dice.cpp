
namespace dice {

void apply_offer( const OfferBet& offer ) {
   assert( offer.amount > 0, "insufficient bet" );
   assert( !hasOffer( offer.commitment ), "offer with this commitment already exist" );
   requireAuth( offer.player );

   auto acnt = getAccount( offer.player );
   acnt.balance -= offer.amount;
   acnt.open_offers++;
   save( acnt );

   /**
    *  1. Lookup lowerbound on offer's by OfferPrimaryKey
    *     if lowerbound.primary.bet == offer.bet
    *          GlobalDice.nextgameid++  (load and save to DB)
    *          create new Game and set game.bet == offer.bet
    *          set player1 == lowerbound player
    *          set player2 == offer.player
    *          update lowerbound.primary.bet = 0 and lwoerbound.gameid = GlobalDice.nextgameid
    *          Create Offer entry in Offers table with bet = 0 and gameid == --^
    *     else      
    *          Create Offer entry in Offers table with bet = offer.bet and gameid = 0
    */
}

void apply_cancel( const CancelOffer& offer ) {
   /**
    *  Lookup Offer in Offer's table and assert that gameid == 0
    *  Lookup account and increment balance by bet 
    *  Deelte offer from offers table
    */
}

/**
 *  No authority required on this message so long as it is properly revealed
 */
void apply_reveal( const Reveal& offer ) {
   assert_sha256( &offer.source, sizeof(offer.source), &offer.commitment );

   /**
    *  assert offer already exists
    *  assert offer has a gameid > 0
    *
    *  Lookup Game by gameid
    *  assert Game.player[x].reveal is 0  hasn't already been revealed
    *  set Game.player[x].reveal to revealed
    *
    *  if( Game.player[!x].reveal != 0 ) {
    *     uint256_t result;
    *     sha256( &game.player1, sizeof(Player)*2, result);
    *     if( result.words[1] < result.words[0] ) {
    *        payPlayer1 by incrementing account by bet and decrement open games
    *     }
    *     else 
    *        payPlayer2 by incrmenting account by bet and decrement open games
    *     delete game, and both offers
    *  }
    *  else {
    *     set Game.deadline = now() + timeout (5 minutes)
    *  }
    *
    */
}

/**
 *  No authority required by this so long as
 */
void apply_claim( const ClaimExpired& claim ) {
   /// lookup Game by id, assert now() > deadline and deadline != 0
   /// pay the player that revealed 
   /// delete game and open offers 
}


}

