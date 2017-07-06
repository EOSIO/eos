#include <contracts/currency.hpp> /// defines transfer struct (abi)

struct CurrencyAccount {
   CurrencyAccount( uint64_t b = 0 ):balance(b){}

   uint64_t balance = 0;

   /** used as a hint to Db::store to tell it which table name within the current 
    *  scope to use.  Data is stored in tables under a structure like
    *
    *  scope/code/table/key->value
    *
    *  In this case the "singlton" table is designed for constant named keys that
    *  refer to a unique object type. User account balances are stored here:
    *  
    *  username/currency/singlton/account -> CurrencyAccount
    */ 
   static Name tableId() { return Name("singlton"); }
};

void init()  {
   ///        scope       key        value
   Db::store( "currency", "account", CurrencyAccount( 1000ll*1000ll*1000ll ) );
}


void apply_currency_transfer() {
   const auto& transfer  = currentMessage<Transfer>();
   /** will call apply_currency_transfer() method in code defined by transfer.to and transfer.from */
   requireNotice( transfer.to, transfer.from );
   requireAuth( transfer.from );

   static CurrencyAccount from_account;
   static CurrencyAccount to_account;
   Db::get( transfer.from, "account", from_account );
   Db::get( transfer.to, "account", to_account );

   assert( from_account.balance >= transfer.amount, "insufficient funds" );
   from_account.balance -= transfer.amount;
   to_account.balance   += transfer.amount;

   if( from_account.balance == 0 )
      Db::remove<CurrencyAccount>( transfer.from, "account" )
   else
      Db::store( transfer.from, "account", from_account ); 

   Db::store( transfer.to, "account", to_account ); 
}



