#include <eosiolib/currency.hpp>
#include <boost/container/flat_map.hpp>
#include <cmath>
#include <exchange/market_state.hpp>

namespace eosio {

   /**
    *  This contract enables users to create an exchange between any pair of
    *  standard currency types. A new exchange is created by funding it with
    *  an equal value of both sides of the order book and giving the issuer
    *  the initial shares in that orderbook.
    *
    *  To prevent exessive rounding errors, the initial deposit should include
    *  a sizeable quantity of both the base and quote currencies and the exchange
    *  shares should have a quantity 100x the quantity of the largest initial
    *  deposit.
    *
    *  Users must deposit funds into the exchange before they can trade on the
    *  exchange.
    *
    *  Each time an exchange is created a new currency for that exchanges market
    *  maker is also created. This currencies supply and symbol must be unique and
    *  it uses the currency contract's tables to manage it.
    */
   class exchange {
      private:
         account_name      _this_contract;
         currency          _excurrencies;
         exchange_accounts _accounts;

      public:
         exchange( account_name self )
         :_this_contract(self),
          _excurrencies(self),
          _accounts(self)
         {}

         /**
          *  Create a new exchange between two extended asset types,
          *  creator will receive the initial supply of a new token type
          */
         struct createx  {
            account_name    creator;
            asset           initial_supply;
            uint32_t        fee;
            extended_asset  base_deposit;
            extended_asset  quote_deposit;

            EOSLIB_SERIALIZE( createx, (creator)(initial_supply)(fee)(base_deposit)(quote_deposit) )
         };

         void createx( account_name    creator,
                       asset           initial_supply,
                       uint32_t        fee,
                       extended_asset  base_deposit,
                       extended_asset  quote_deposit
                     );

         struct deposit {
            account_name    from;
            extended_asset  quantity;

            EOSLIB_SERIALIZE( deposit, (from)(quantity) )
         };

         struct withdraw {
            account_name    from;
            extended_asset  quantity;

            EOSLIB_SERIALIZE( withdraw, (from)(quantity) )
         };

         struct lend {
            account_name     lender;
            symbol_type      market;
            extended_asset   quantity;

            EOSLIB_SERIALIZE( lend, (lender)(market)(quantity) )
         };

         struct unlend {
            account_name     lender;
            symbol_type      market;
            double           interest_shares;
            extended_symbol  interest_symbol;

            EOSLIB_SERIALIZE( unlend, (lender)(market)(interest_shares)(interest_symbol) )
         };

         struct covermargin {
            account_name     borrower;
            symbol_type      market;
            extended_asset   cover_amount;

            EOSLIB_SERIALIZE( covermargin, (borrower)(market)(cover_amount) )
         };

         struct upmargin {
            account_name     borrower;
            symbol_type      market;
            extended_asset   delta_borrow;
            extended_asset   delta_collateral;

            EOSLIB_SERIALIZE( upmargin, (borrower)(market)(delta_borrow)(delta_collateral) )
         };

         struct trade {
            account_name    seller;
            symbol_type     market;
            extended_asset  sell;
            extended_asset  min_receive;
            uint32_t        expire = 0;
            uint8_t         fill_or_kill = true;

            EOSLIB_SERIALIZE( trade, (seller)(market)(sell)(min_receive)(expire)(fill_or_kill) )
         };

         // TODO: Need ability to cancel trades in order book.

         void on( const deposit& d  );
         void on( const withdraw& w );
         void on( const lend& w );
         void on( const unlend& w );
         //void on( const createx& c );
         void on( const trade& t    );
         void on( const upmargin& b );
         void on( const covermargin& b );
         void on( const currency::transfer& t, account_name code );


         bool apply( account_name contract, account_name act );
   };
}
