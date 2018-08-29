#pragma once

#include <eosiolib/asset.hpp>

namespace eosio {

   typedef double real_type;

   struct margin_state {
      extended_asset total_lendable;
      extended_asset total_lent;
      extended_asset collected_interest;
      real_type      least_collateralized = std::numeric_limits<double>::max();

      /**
       * Total shares allocated to those who have lent, when someone unlends they get
       * total_lendable * user_interest_shares / interest_shares and total_lendable is reduced.
       *
       * When interest is paid, it shows up in total_lendable
       */
      real_type      interest_shares = 0;

      real_type lend( int64_t new_lendable ) {
         real_type new_shares = new_lendable;
         if( total_lendable.amount > 0 ) {
            new_shares =  (interest_shares * new_lendable) / total_lendable.amount;
            interest_shares += new_shares;
            total_lendable.amount += new_lendable;
         } else {
            interest_shares += new_lendable;
            total_lendable.amount  += new_lendable;
         }
         // Higher limits trigger a floating-point issue which causes unlend to refund too much
         eosio_assert( total_lendable.amount < (1ll << 53), "exceeded maximum lendable amount" );
         return new_shares;
      }

      extended_asset unlend( real_type ishares ) {
         extended_asset result = total_lent;
         print( "unlend: ", ishares, " existing interest_shares:  ", interest_shares, "\n" ); 
         result.amount  = int64_t( (ishares * total_lendable.amount) / interest_shares );

         total_lendable.amount -= result.amount;
         interest_shares -= ishares;

         // todo: call margins? zero interest_shares on underflow?
         eosio_assert( interest_shares >= 0, "underflow" );
         eosio_assert( total_lendable.amount >= 0, "underflow" );
         eosio_assert( total_lendable >= total_lent, "funds are in margins" );

         return result;
      }

      EOSLIB_SERIALIZE( margin_state, (total_lendable)(total_lent)(collected_interest)(least_collateralized)(interest_shares) )
   };

   /**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */
   struct exchange_state {
      account_name      manager;
      extended_asset    supply;
      double            fee = 0;
      double            interest_rate = 0;
      bool              need_continuation = false;

      struct market_continuation {
         bool              active = false;
         account_name      seller = 0;
         extended_asset    sell;
         extended_symbol   receive_symbol;

         EOSLIB_SERIALIZE( market_continuation, (active)(seller)(sell)(receive_symbol) )
      };

      market_continuation active_market_continuation;

      struct connector {
         extended_asset balance;
         uint32_t       weight = 500;

         margin_state   peer_margin; /// peer_connector collateral lending balance

         EOSLIB_SERIALIZE( connector, (balance)(weight)(peer_margin) )
      };

      connector base;
      connector quote;

      uint64_t primary_key()const { return supply.symbol.name(); }

      extended_asset convert_to_exchange( connector& c, extended_asset in, bool allow_zero, bool charge_fee ); 
      extended_asset convert_from_exchange( connector& c, extended_asset in, bool allow_zero, bool charge_fee );
      extended_asset convert( extended_asset from, extended_symbol to, bool allow_zero = false, bool charge_fee = true );

      bool requires_margin_call( const exchange_state::connector& con, const extended_symbol& collateral_symbol )const;
      bool requires_margin_call()const;

      EOSLIB_SERIALIZE( exchange_state, (manager)(supply)(fee)(interest_rate)(need_continuation)(active_market_continuation)(base)(quote) )
   };

   typedef eosio::multi_index<N(markets), exchange_state> markets;

} /// namespace eosio
