#include <currency/currency.hpp>

namespace exchange {

   using currency::CurrencyTokens;
   using EosTokens = eos::Tokens;

   struct OrderID {
      AccountName name    = 0;
      uint64_t    number  = 0;
   };

   typedef eos::price<EosTokens,CurrencyTokens>     Price;

   struct PACKED( Bid ) {
      OrderID            buyer;
      Price              price;
      eos::Tokens        quantity;
      Time               expiration;

      void print() {
         eos::print( "{ quantity: ", quantity, ", price: ", price, " }" );
      }
   };
   static_assert( sizeof(Bid) == 32+12, "unexpected padding" );

   struct PACKED( Ask ) {
      OrderID          seller;
      Price            price;
      CurrencyTokens   quantity;
      Time             expiration;

      void print() {
         eos::print( "{ quantity: ", quantity, ", price: ", price, " }" );
      }
   };
   static_assert( sizeof(Ask) == 32+12, "unexpected padding" );

   struct PACKED( Account ) {
      Account( AccountName o = AccountName() ):owner(o){}

      AccountName        owner;
      EosTokens          eos_balance;
      CurrencyTokens     currency_balance;
      uint32_t           open_orders = 0;

      bool isEmpty()const { return ! ( bool(eos_balance) | bool(currency_balance) | open_orders); }
   };

   using Accounts = Table<N(exchange),N(exchange),N(account),Account,uint64_t>;

   TABLE2(Bids,exchange,exchange,bids,Bid,BidsById,OrderID,BidsByPrice,Price); 
   TABLE2(Asks,exchange,exchange,asks,Ask,AsksById,OrderID,AsksByPrice,Price); 


   struct BuyOrder : public Bid  { uint8_t fill_or_kill = false; };
   struct SellOrder : public Ask { uint8_t fill_or_kill = false; };


   inline Account getAccount( AccountName owner ) {
      Account account(owner);
      Accounts::get( account );
      return account;
   }
}

