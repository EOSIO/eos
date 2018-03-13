#pragma once

#include <eosiolib/asset.hpp>

namespace eosio {

   typedef double real_type;

   struct margin_state {
      extended_asset total_lendable;
      extended_asset total_lent;
      real_type      least_collateralized = std::numeric_limits<double>::max();

      /**
       * Total shares allocated to those who have lent, when someone unlends they get
       * total_lendable * user_interest_shares / interest_shares and total_lendable is reduced.
       *
       * When interest is paid, it shows up in total_lendable
       */
      real_type      interest_shares = 0;

      real_type lend( int64_t new_lendable ) {
         if( total_lendable.amount > 0 ) {
            real_type new_shares =  (interest_shares * new_lendable) / total_lendable.amount;
            interest_shares += new_shares;
            total_lendable.amount += new_lendable;
         } else {
            interest_shares += new_lendable;
            total_lendable.amount  += new_lendable;
         }
         return new_lendable;
      }

      extended_asset unlend( double ishares ) {
         extended_asset result = total_lent;
         print( "unlend: ", ishares, " existing interest_shares:  ", interest_shares, "\n" ); 
         result.amount  = int64_t( (ishares * total_lendable.amount) / interest_shares );

         total_lendable.amount -= result.amount;
         interest_shares -= ishares;

         eosio_assert( interest_shares >= 0, "underflow" );
         eosio_assert( total_lendable.amount >= 0, "underflow" );

         return result;
      }

      EOSLIB_SERIALIZE( margin_state, (total_lendable)(total_lent)(least_collateralized)(interest_shares) )
   };

   /**
    *  Uses Bancor math to create a 50/50 relay between two asset types. The state of the
    *  bancor exchange is entirely contained within this struct. There are no external
    *  side effects associated with using this API.
    */
   struct exchange_state {
      account_name      manager;
      extended_asset    supply;
      uint32_t          fee = 0;

      struct connector {
         extended_asset balance;
         uint32_t       weight = 500;

         margin_state   peer_margin; /// peer_connector collateral lending balance

         EOSLIB_SERIALIZE( connector, (balance)(weight)(peer_margin) )
      };

      connector base;
      connector quote;

      uint64_t primary_key()const { return supply.symbol.name(); }

      extended_asset convert_to_exchange( connector& c, extended_asset in ); 
      extended_asset convert_from_exchange( connector& c, extended_asset in );
      extended_asset convert( extended_asset from, extended_symbol to );

      bool requires_margin_call( const exchange_state::connector& con )const;
      bool requires_margin_call()const;

      EOSLIB_SERIALIZE( exchange_state, (manager)(supply)(fee)(base)(quote) )
   };

   typedef eosio::multi_index<N(markets), exchange_state> markets;

} /// namespace eosio
