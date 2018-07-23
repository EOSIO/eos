#include <eosiolib/types.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/time.hpp>
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
    *  To prevent excessive rounding errors, the initial deposit should include
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

         void createx( account_name    creator,
                       asset           initial_supply,
                       double          fee,
                       double          interest_rate,
                       extended_asset  base_deposit,
                       extended_asset  quote_deposit
                     );

         void withdraw( account_name  from, extended_asset quantity );
         void lend( account_name lender, symbol_type market, extended_asset quantity );

         void unlend(
            account_name     lender,
            symbol_type      market,
            double           interest_shares,
            extended_symbol  interest_symbol
         );

         struct covermargin {
            account_name     borrower;
            symbol_type      market;
            extended_asset   cover_amount;
         };

         struct upmargin {
            account_name     borrower;
            symbol_type      market;
            extended_asset   delta_borrow;
            extended_asset   delta_collateral;
         };

         /**
          *  This will sell through the bancor market maker and give the current price.
          *
          *  - check to see if there are any limit/stoploss that need to execute first, then
          *    execute this.
          */
         void marketorder( account_name seller, symbol_type market, extended_asset sell, extended_symbol receive );

         /**
          *  When market price falls to amount_to_sell / trigger_base => buy
          *
          *  Transfers amount_to_sell from seller's balance with exchange to this order, on
          *  fill, transfers proceeds to seller's account.  If canceled amount_to_sell is returned to
          *  users account.
          *
          *  @param order_id - unique per seller, (seller,order_id) ident this order for cancel
          *
          *  - create the order 
          *  - check to see if any orders need to be processed
          */
         void limitorder( account_name seller, uint16_t order_id, symbol_type market, 
                           extended_asset amount_to_sell,
                           extended_asset trigger_base, ///< ammount_to_sell / trigger_base => trigger price
                           block_timestamp_type  expiration ///< time at which this order is no longer valid 
                         );
         /**
          * When market price falls to amount_to_sell / trigger_base => sell amount to sell
          */
         void stoploss( account_name seller, uint16_t order_id, symbol_type market, 
                           extended_asset amount_to_sell,
                           extended_asset trigger_base, ///< ammount_to_sell / trigger_base => trigger price
                           block_timestamp_type  expiration ///< time at which this order is no longer valid 
                       );

         void continuation( symbol_type market_symbol, int max_ops );

         void on( const upmargin& b );
         void on( const covermargin& b );
         void on( const currency::transfer& t, account_name code );

         void apply( account_name contract, account_name act );
   };
} // namespace eosio
