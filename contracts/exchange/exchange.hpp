#include <currency/currency.hpp>

namespace exchange {

   struct OrderID {
      AccountName name    = 0;
      uint64_t    number  = 0;
   };

   typedef eos::price<eos::Tokens,currency::Tokens>     Price;

   struct Bid {
      OrderID            buyer;
      Price              price;
      eos::Tokens        quantity;
      Time               expiration;
   };

   struct Ask {
      OrderID            seller;
      Price              price;
      currency::Tokens   quantity;
      Time               expiration;
   };

   struct Account {
      Account( AccountName o = AccountName() ):owner(o){}

      AccountName        owner;
      eos::Tokens        eos_balance;
      currency::Tokens   currency_balance;
      uint32_t           open_orders = 0;

      bool isEmpty()const { return ! ( bool(eos_balance) | bool(currency_balance) | open_orders); }
   };

   Account getAccount( AccountName owner ) {
      Account account(owner);
      Db::get( N(exchange), N(exchange), N(account), owner, account );
      return account;
   }

   TABLE2(Bids,exchange,exchange,bids,Bid,BidsById,OrderID,BidsByPrice,Price); 
   TABLE2(Asks,exchange,exchange,bids,Ask,AsksById,OrderID,AsksByPrice,Price); 


   struct BuyOrder : public Bid  { uint8_t fill_or_kill = false; };
   struct SellOrder : public Ask { uint8_t fill_or_kill = false; };
}

