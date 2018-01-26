#pragma once
#include <eoslib/singleton.hpp>
#include <eoslib/table.hpp>
#include <eoslib/token.hpp>
#include <eoslib/asset.hpp>
#include <eoslib/dispatcher.hpp>

namespace eosio {

   template<typename Token>
   class generic_currency {
      public:
          typedef Token token_type;
          static const uint64_t code                = token_type::code;
          static const uint64_t symbol              = token_type::symbol;
          static const uint64_t accounts_table_name = N(account);
          static const uint64_t stats_table_name    = N(stat);

          struct issue : action_meta<code,N(issue)> {
             typedef action_meta<code,N(issue)> meta;
             account_name to;
             asset        quantity;

             template<typename DataStream>
             friend DataStream& operator << ( DataStream& ds, const issue& t ){
                return ds << t.to << t.quantity;
             }
             template<typename DataStream>
             friend DataStream& operator >> ( DataStream& ds, issue& t ){
                return ds >> t.to >> t.quantity;
             }
          };

          struct transfer : action_meta<code,N(transfer)> {
             transfer(){}
             transfer( account_name f, account_name t, token_type q ):from(f),to(t),quantity(q){}

             account_name from;
             account_name to;
             asset        quantity;

             //EOSLIB_SERIALIZE( transfer, (from)(to)(quantity)(symbol) )

             template<typename DataStream>
             friend DataStream& operator << ( DataStream& ds, const transfer& t ){
                return ds << t.from << t.to << t.quantity;
             }
             template<typename DataStream>
             friend DataStream& operator >> ( DataStream& ds, transfer& t ){
                ds >> t.from >> t.to >> t.quantity;
                assert( t.quantity.symbol== token_type::symbol, "unexpected asset type" );
                return ds;
             }
          };

          struct transfer_memo : public transfer {
             transfer_memo(){}
             transfer_memo( account_name f, account_name t, token_type q, string m )
             :transfer( f, t, q ), memo( move(m) ){}

             string       memo;

             template<typename DataStream>
             friend DataStream& operator << ( DataStream& ds, const transfer_memo& t ){
                ds << static_cast<const transfer&>(t);
                return ds << t.memo;
             }
             template<typename DataStream>
             friend DataStream& operator >> ( DataStream& ds, transfer_memo& t ){
                ds >> static_cast<transfer&>(t);
                return ds >> t.memo;
             }
          };

          struct account {
             uint64_t   symbol = token_type::symbol;
             token_type balance;

             template<typename DataStream>
             friend DataStream& operator << ( DataStream& ds, const account& t ){
                return ds << t.symbol << t.balance;
             }
             template<typename DataStream>
             friend DataStream& operator >> ( DataStream& ds, account& t ){
                return ds >> t.symbol >> t.balance;
             }
          };

          struct currency_stats {
             uint64_t   symbol = token_type::symbol;
             token_type supply;

             template<typename DataStream>
             friend DataStream& operator << ( DataStream& ds, const currency_stats& t ){
                return ds << t.symbol << t.supply;
             }
             template<typename DataStream>
             friend DataStream& operator >> ( DataStream& ds, currency_stats& t ){
                return ds >> t.symbol >> t.supply;
             }
          };

          /**
           *  Each user stores their balance in the singleton table under the
           *  scope of their account name.
           */
          typedef table64<code, accounts_table_name, account>      accounts;
          typedef table64<code, stats_table_name, currency_stats>  stats;

          static token_type get_balance( account_name owner ) {
             return accounts::get_or_create( token_type::symbol, owner ).balance;
          }

          static void set_balance( account_name owner, token_type balance ) {
             accounts::set( account{token_type::symbol,balance}, owner );
          }

          static void on( const issue& act ) {
             require_auth( code );

             auto s = stats::get_or_create(token_type::symbol);
             s.supply += act.quantity;
             stats::set(s);

             set_balance( code, get_balance( code ) + act.quantity );

             inline_transfer( code, act.to, act.quantity ); 
          }


          static void on( const transfer& act ) {
             require_auth( act.from );
             require_recipient(act.to,act.from);

             set_balance( act.from, get_balance( act.from ) - act.quantity );
             set_balance( act.to, get_balance( act.to ) + act.quantity );
          }

          static void inline_transfer( account_name from, account_name to, token_type quantity, 
                                       string memo = string() )
          {
             action act( permission_level(code,N(active)), transfer_memo( from, to, asset(quantity), move(memo) ));
             act.send();
          }


         static void apply( account_name c, action_name act) {
            eosio::dispatch<generic_currency, transfer_memo, issue>(c,act);
         }
   };

} /// namespace eosio




#if 0
template<account_name RelayAccount, account_name FirstCurrency, account_name SecondCurrency>
class relay_contract {
   public:
      typedef generic_currency<RelayAccount>   relay_currency;
      typedef generic_currency<FirstCurrency>  first_currency;
      typedef generic_currency<SecondCurrency> second_currency;

      template<typename CurrencyType, uint32_t Weight=500000, uint32_t Base=1000000> 
      struct connector {
         typedef CurrencyType currency_type;

         relay_currency::token_type convert_to_relay( currency_type::token_type in, relay_state& state ) {
            currency_type::token_type balance = currency_type::get_balance( RelayAccount );

            /// balance already changed when transfer executed, get pre-transfer balance
            currency_type::token_type previous_balance = balance - in; 

            auto init_price = (previous_balance * Base) / (Weight * state.supply);
            auto init_out   = init_price * in;

            auto out_price  = (balance*Base) / (Weight * (state.supply+init_out) );
            auto final_out  = out_price * in;

            state.balance += final_out;
            state.supply  += final_out;

            return final_out;
         }


         currency_type::token_type convert_from_relay( relay_currency::token_type relay_in, relay_state& state ) {
            currency_type::token_type  to_balance             = CurrencyType::get_balance( RelayAccount );

            auto                      init_price = (to_balance * Base) / (Weight * state.supply);
            currency_type::token_type init_out   = init_price * in;

            state.supply  -= relay_in;
            state.balance -= relay_in;

            auto out_price  = ((to_balance-init_out) * Base) / ( Weight * (state.supply) )

            return out_price * relay_in;
         }

      };

      struct relay_state {
         relay_currency::token_type supply; /// total supply held by all users
         relay_currency::token_type balance; /// supply held by relay in its own balance
      };

      struct relay_args {
         account_name to_currency_type;
         uint64_t     min_return_currency;
      };


      /**
       * This is called when we receive RELAY tokens from user and wish to
       * convert to one of the connector currencies.
       */
      static void on_convert( const typename relay_currency::transfer& trans, 
                                    const relay_args& args, 
                                    relay_state& state ) {

         if( args.to_currency_type == first_currency ) {
            auto output = first_connector::convert_from_relay( trans.quantity, state );
            save_and_send( trans.from, state, output, args.min_return );
         }
         else if( args.to_currency_type == second_currency ) {
            auto output = second_connector::convert_from_relay( trans.quantity, state );
            save_and_send( trans.from, state, output, args.min_return );
         } 
         else {
            assert( false, "invalid to currency" );
         }
      }


      /**
       *  This is called when the relay receives one of the connector currencies and it 
       *  will send either relay tokens or a different connector currency in response.
       */
      template<typename ConnectorType>
      static void on_convert( const typename ConnectorType::currency_type::transfer& trans,
                                    const relay_args& args, 
                                    relay_state& state ) 
      {
         /// convert to relay
         auto relay_out = ConnectorType::convert_to_relay( trans.quantity, state );

         if( args.to_currency_type == relay_currency )
         {
            save_and_send( trans.from, state, relay_out, args.min_return );
         }
         else 
         {
            auto output = ConnectorType::convert_from_relay( relay_out, state ); 
            save_and_send( trans.from, state, output, args.min_return );
         }
      }


      /**
       *  This method factors out the boiler plate for parsing args and loading the
       *  initial state before dispatching to the proper on_convert case
       */
      template<typename CurrencyType>
      static void start_convert( const CurrencyType::transfer& trans ) {
         auto args = unpack<relay_args>( trans.memo );
         assert( args.to_currency_type != trans.quantity.token_type(), "cannot convert to self" );

         auto state = read_relay_state();
         on_convert( trans, args, state );
      }


      /**
       * RelayAccount first needs to call the currency handler to perform
       * user-to-user transfers of the relay token, then if a transfer is sending
       * the token back to the relay contract, it should convert like everything else.
       *
       *  This method should be called from apply( code, action ) for each of the
       *  transfer types that we support (for each currency)
       */
      static void on( const relay_currency::transfer& trans ) {
         relay_currency::on( trans );
         if( trans.to == RelayAccount ) {
            start_convert( trans );
         }
      }

      /**
       *  All other currencies simply call start_convert if to == RelayAccount
       */
      template<typename Currency>
      static void on( const Currency::transfer& trans ) {
         if( trans.to == RelayAccount ) {
            start_convert( trans );
         } else {
            assert( trans.from == RelayAccount, 
                    "received unexpected notification of transfer" );
         }
      }

      static void apply( account_name code, action_name action ) {
         if( !dispatch<Currency, transfer_memo, issue>( code, action ) )
            assert( false, "received unexpected action" );
      }
};

#endif

