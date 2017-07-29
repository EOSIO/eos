
namespace dice {

   struct OfferBet {
      eos::Tokens   amount;
      AccountName   player;
      uint256       commitment;
   };

   struct CancelOffer {
      uint256  commitment;
   };

   struct Reveal {
      uint256  commitment;
      uint256  source;
   };

   struct ClaimExpired {
      uint64_t gameid = 0;
   };

   using EosTokens = eos::Tokens;

   struct OfferPrimaryKey {
      EosTokens          bet;
      AccountName        owner;
   };

   /**
    *  This struct is used with i128xi128 index which allows us to lookup and
    *  iterate over offers by either {bet,owner,commitment} or {commitment,bet,owner} 
    *
    *  Only the first 128 bits of commitment ID are used in the index, but there will
    *  be a requirement that these first 128 bits be unique and any collisions will
    *  be rejected.
    *
    *  If there is no existing offer of equal bet size then gameid is set as 0, if it is 
    *  matched then a new Game record is created and the Offer is modified so that bet 
    *  goes to 0 (and therefore moves to end of queue and not matched with other bets) and
    *  the gameid of both offers are updated.
    */
   struct PACKED( Offer ) {
      OfferPrimaryKey    primary;
      uint256            commitment;
      uint32_t           gameid = 0;
   };

   struct Player {
      uint256     commitment;
      uint256     reveal;
   };

   /**
    *
    */
   struct PACKED( Game ) {
      uint32_t  gameid;
      EosTokens bet;
      Time      deadline;
      Player    player1;
      Player    player2;
   };

   struct Packed( GlobalDice ) {
      uint32_t nextgameid = 0;
   };


   struct PACKED( Account ) {
      Account( AccountName o = AccountName() ):owner(o){}

      AccountName        owner;
      EosTokens          eos_balance;
      uint32_t           open_offers = 0;

      bool isEmpty()const { return ! ( bool(eos_balance) |  open_offers); }
   };

   using Accounts = Table<N(dice),N(dice),N(account),Account,uint64_t>;
   using GlobalDice = Table<N(dice),N(dice),N(global),GlobalDice,uint64_t>;
   using Offers = Table<N(dice),N(dice),N(global),GlobalDice,OfferPrimaryKey,uint128_t>;

   inline Account getAccount( AccountName owner ) {
      Account account(owner);
      Accounts::get( account );
      return account;
   }

};
