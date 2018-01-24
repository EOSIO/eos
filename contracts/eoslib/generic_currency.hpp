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

