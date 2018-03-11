#include <eosiolib/currency.hpp>
#include <boost/container/flat_map.hpp>
#include <cmath>

namespace eosio {

   typedef double real_type;
   using boost::container::flat_map;

   struct exchange_state {
      account_name      manager;
      extended_asset    supply;
      uint32_t          fee = 0;

      struct connector {
         extended_asset balance;
         uint32_t       weight = 500;

         EOSLIB_SERIALIZE( connector, (balance)(weight) )
      };

      connector base;
      connector quote;

      extended_asset convert_to_exchange( connector& c, extended_asset in ) {
         /*
         print( "initial connector balance: ", c.balance, "\n" );
         print( "initial ex supply balance: ", supply, "\n" );
         */

         real_type R(supply.amount);
         real_type C(c.balance.amount+in.amount);
         real_type F(c.weight/1000.0);
         real_type T(in.amount);
         real_type ONE(1.0);

         /*
         print( "R: ", R, " \n" );
         print( "C: ", C, " \n" );
         print( "F: ", F, " \n" );
         print( "T: ", T, " \n" );
         */

         real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
         int64_t issued = int64_t(E);

         supply.amount += issued;
         c.balance.amount += in.amount;

         /*
         print( "final connector balance: ", c.balance, "\n" );
         print( "final ex supply balance: ", supply, "\n" );
         print( "----------END FROM CON --------------------\n" );
         */
         return extended_asset( issued, supply.get_extended_symbol() );
      }

      extended_asset convert_from_exchange( connector& c, extended_asset in ) {
         /*
         print( "initial connector balance: ", c.balance, "\n" );
         print( "initial ex supply balance: ", supply, "\n" );
         */
         eosio_assert( in.contract == supply.contract, "unexpected asset contract input" );
         eosio_assert( in.symbol== supply.symbol, "unexpected asset symbol input" );

         real_type R(supply.amount - in.amount);
         real_type C(c.balance.amount);
         real_type F(1000.0/c.weight);
         real_type E(in.amount);
         real_type ONE(1.0);

         /*
         print( "R: ", R, " \n" );
         print( "C: ", C, " \n" );
         print( "F: ", F, " \n" );
         print( "E: ", E, " \n" );
         */

         real_type T = C * (std::pow( ONE + E/R, F) - ONE);
         int64_t out = int64_t(T);

         supply.amount -= in.amount;
         c.balance.amount -= out;

         /*
         print( "final connector balance: ", c.balance, "\n" );
         print( "final ex supply balance: ", supply, "\n" );
         print( "---------- END FROM EX-----------------------\n" );
         */
         return extended_asset( out, c.balance.get_extended_symbol() );
      }

      uint64_t primary_key()const { return supply.symbol.name(); }

      EOSLIB_SERIALIZE( exchange_state, (manager)(supply)(fee)(base)(quote) )
   };
   

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
         account_name _this_contract;
         currency     _excurrencies;

      public:
         exchange( account_name self )
         :_this_contract(self),_excurrencies(self){}


         typedef eosio::multi_index<N(markets), exchange_state> markets;

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

         struct deposit {
            account_name    from;
            extended_asset  quantity;

            EOSLIB_SERIALIZE( deposit, (from)(quantity) )
         };

         void on( const deposit& d ) {
            currency::inline_transfer( d.from, _this_contract, d.quantity, "deposit" );
            adjust_balance( d.from, d.quantity, "deposit" );
         }

         struct withdraw {
            account_name    from;
            extended_asset  quantity;

            EOSLIB_SERIALIZE( withdraw, (from)(quantity) )
         };
         void on( const withdraw& w ) {
            require_auth( w.from );
            eosio_assert( w.quantity.amount >= 0, "cannot withdraw negative balance" );
            adjust_balance( w.from, -w.quantity );
            currency::inline_transfer( _this_contract, w.from, w.quantity, "withdraw" );
         }

         struct trade {
            account_name    seller;
            symbol_type     market;
            extended_asset  sell;
            extended_asset  min_receive;
            uint32_t        expire = 0;
            uint8_t         fill_or_kill = true;

            EOSLIB_SERIALIZE( trade, (seller)(market)(sell)(min_receive)(expire)(fill_or_kill) )
         };

         extended_asset convert( exchange_state& state, extended_asset from, extended_symbol to ) {
            auto sell_symbol  = from.get_extended_symbol();
            auto ex_symbol    = extended_symbol( state.supply.symbol, _this_contract );
            auto base_symbol  = state.base.balance.get_extended_symbol();
            auto quote_symbol = state.quote.balance.get_extended_symbol();

            if( sell_symbol != ex_symbol ) {
               if( sell_symbol == base_symbol ) {
                  from = state.convert_to_exchange( state.base, from );
               } else if( sell_symbol == quote_symbol ) {
                  from = state.convert_to_exchange( state.quote, from );
               } else { 
                  eosio_assert( false, "invalid sell" );
               }
            } else {
               if( to == base_symbol ) {
                  from = state.convert_from_exchange( state.base, from ); 
               } else if( to == quote_symbol ) {
                  from = state.convert_from_exchange( state.quote, from ); 
               } else {
                  eosio_assert( false, "invalid conversion" );
               }
            }

            if( to != from.get_extended_symbol() )
               return convert( state, from, to );

            return from;
         }

         void on( const trade& t ) {
            auto marketid = t.market.name();
            eosio_assert( t.sell.get_extended_symbol() != t.min_receive.get_extended_symbol(), "invalid conversion" );

            require_auth( t.seller );

            markets market_table( _this_contract, marketid );
            auto market_state = market_table.find( marketid );
            eosio_assert( market_state != market_table.end(), "unknown market" );

            auto state = *market_state;

            auto output = convert( state, t.sell, t.min_receive.get_extended_symbol() );
            print( name(t.seller), "   ", t.sell, "  =>  ", output, "\n" );

            if( t.min_receive.amount != 0 ) {
               eosio_assert( t.min_receive.amount <= output.amount, "unable to fill" );
            }

            adjust_balance( t.seller, -t.sell, "sold" );
            adjust_balance( t.seller, output, "received" );

            if( market_state->supply.amount != state.supply.amount ) {
               auto delta = state.supply - market_state->supply;

               _excurrencies.issue_currency( { .to = _this_contract,
                                              .quantity = delta,
                                              .memo = string("") } );
            }

            market_table.modify( market_state, 0, [&]( auto& s ) {
               s = state;
            });
         }

         /**
          *  Each user has their own account with the exchange contract that keeps track
          *  of how much a user has on deposit for each extended asset type. The assumption
          *  is that storing a single flat map of all balances for a particular user will
          *  be more practical than breaking this down into a multi-index table sorted by
          *  the extended_symbol.  
          */
         struct exaccount {
            account_name                         owner;
            flat_map<extended_symbol, int64_t>   balances;

            uint64_t primary_key() const { return owner; }
            EOSLIB_SERIALIZE( exaccount, (owner)(balances) )
         };

         typedef eosio::multi_index<N(exaccounts), exaccount> exaccounts;


         /**
          *  Keep a cache of all accounts tables we access
          */
         flat_map<account_name, exaccounts> exaccounts_cache;

         void adjust_balance( account_name owner, extended_asset delta, const string& reason = string() ) {
            (void)reason;

            auto table = exaccounts_cache.find( owner );
            if( table == exaccounts_cache.end() ) {
               table = exaccounts_cache.emplace( owner, exaccounts(_this_contract, owner )  ).first;
            }
            auto useraccounts = table->second.find( owner );
            if( useraccounts == table->second.end() ) {
               table->second.emplace( owner, [&]( auto& exa ){
                 exa.owner = owner;
                 exa.balances[delta.get_extended_symbol()] = delta.amount;
                 eosio_assert( delta.amount >= 0, "overdrawn balance 1" );
               });
            } else {
               table->second.modify( useraccounts, 0, [&]( auto& exa ) {
                 const auto& b = exa.balances[delta.get_extended_symbol()] += delta.amount;
                 eosio_assert( b >= 0, "overdrawn balance 2" );
               });
            }
         }


         void on( const createx& c ) {
            require_auth( c.creator );
            eosio_assert( c.initial_supply.symbol.is_valid(), "invalid symbol name" );
            eosio_assert( c.initial_supply.amount > 0, "initial supply must be positive" );
            eosio_assert( c.base_deposit.get_extended_symbol() != c.quote_deposit.get_extended_symbol(), 
                          "must exchange between two different currencies" );

            print( "base: ", c.base_deposit.get_extended_symbol() );
            print( "quote: ",c.quote_deposit.get_extended_symbol() );

            auto exchange_symbol = c.initial_supply.symbol.name();
            print( "marketid: ", exchange_symbol, " \n " );

            markets exstates( _this_contract, exchange_symbol );
            auto existing = exstates.find( exchange_symbol );

            eosio_assert( existing == exstates.end(), "unknown market" );
            exstates.emplace( c.creator, [&]( auto& s ) {
                s.manager = c.creator;
                s.supply  = extended_asset(c.initial_supply, _this_contract);
                s.base.balance = c.base_deposit;
                s.quote.balance = c.quote_deposit;
            });

            _excurrencies.create_currency( { .issuer = _this_contract,
                                      .maximum_supply = asset( 0, c.initial_supply.symbol ),
                                      .issuer_can_freeze = false,
                                      .issuer_can_whitelist = false,
                                      .issuer_can_recall = false } );

            _excurrencies.issue_currency( { .to = _this_contract,
                                           .quantity = c.initial_supply,
                                           .memo = string("initial exchange tokens") } );

            adjust_balance( c.creator, extended_asset( c.initial_supply, _this_contract ), "new exchange issue" );
            adjust_balance( c.creator, -c.base_deposit, "new exchange deposit" );
            adjust_balance( c.creator, -c.quote_deposit, "new exchange deposit" );
         }

         bool apply( account_name contract, account_name act ) {

            if( contract != _this_contract ) 
               return false;

            switch( act ) {
               case N(createx):
                  on( unpack_action<createx>() );
                  return true;
               case N(trade):
                  on( unpack_action<trade>() );
                  return true;
               case N(deposit):
                  on( unpack_action<deposit>() );
                  return true;
               case N(withdraw):
                  on( unpack_action<withdraw>() );
                  return true;
               default:
                  return _excurrencies.apply( contract, act );
            }
         }

   };
}
