/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "eosio.system.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/transaction.hpp>

#include <eosio.token/eosio.token.hpp>


#include <cmath>
#include <map>

namespace eosiosystem {
   using eosio::asset;
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using eosio::print;
   using eosio::permission_level;
   using std::map;
   using std::pair;

   static constexpr time refund_delay = 3*24*3600;
   static constexpr time refund_expiration_time = 3600;

   struct user_resources {
      account_name  owner;
      asset         net_weight;
      asset         cpu_weight;
      uint64_t      storage_bytes = 0;

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(storage_bytes) )
   };


   /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct delegated_bandwidth {
      account_name  from;
      account_name  to;
      asset         net_weight;
      asset         cpu_weight;

      uint64_t  primary_key()const { return to; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight) )

   };

   struct refund_request {
      account_name  owner;
      time          request_time;
      eosio::asset  amount;

      uint64_t  primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(amount) )
   };

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this 
    *  facilitates simpler API for per-user queries
    */
   typedef eosio::multi_index< N(userres), user_resources>      user_resources_table;
   typedef eosio::multi_index< N(delband), delegated_bandwidth> del_bandwidth_table;
   typedef eosio::multi_index< N(refunds), refund_request>      refunds_table;



   /**
    *  When buying ram the payer irreversiblly transfers quant to system contract and only
    *  the receiver may reclaim the tokens via the sellram action. The receiver pays for the
    *  storage of all database records associated with this action.
    *
    *  RAM is a scarce resource whose supply is defined by global properties max_storage_size. RAM is
    *  priced using the bancor algorithm such that price-per-byte with a constant reserve ratio of 100:1. 
    */
   void system_contract::buyram( account_name payer, account_name receiver, asset quant ) 
   {
      require_auth( payer );
      eosio_assert( quant.amount > 0, "must purchase a positive amount" );

      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {payer,N(active)},
                                                    { payer, N(eosio), quant, std::string("buy ram") } );

      const double system_token_supply   = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name()).amount;
      const double unstaked_token_supply = system_token_supply - _gstate.total_storage_stake.amount;

      const double E = quant.amount;
      const double R = unstaked_token_supply - E;
      const double C = _gstate.free_ram();   //free_ram;
      const double F = .10; /// 10% reserve ratio pricing, assumes only 10% of tokens will ever want to stake for ram
      const double ONE(1.0);

      double T = C * (std::pow( ONE + E/R, F ) - ONE);
      T *= .99; /// 1% fee on every conversion
      int64_t bytes_out = static_cast<int64_t>(T);

      eosio_assert( bytes_out > 0, "must reserve a positive amount" );

      _gstate.total_storage_bytes_reserved += uint64_t(bytes_out);
      _gstate.total_storage_stake.amount   += quant.amount;

      user_resources_table  userres( _self, receiver );
      auto res_itr = userres.find( receiver );
      if( res_itr ==  userres.end() ) {
         res_itr = userres.emplace( receiver, [&]( auto& res ) {
               res.owner = receiver;
               res.storage_bytes = uint64_t(bytes_out);
            });
      } else {
         userres.modify( res_itr, receiver, [&]( auto& res ) {
               res.storage_bytes += uint64_t(bytes_out);
            });
      }
      set_resource_limits( res_itr->owner, res_itr->storage_bytes, uint64_t(res_itr->net_weight.amount), uint64_t(res_itr->cpu_weight.amount) );
   }


   /**
    *  While buying ram uses the current market price according to the bancor-algorithm, selling ram only
    *  refunds the purchase price to the account. In this way there is no profit to be made through buying
    *  and selling ram.
    */
   void system_contract::sellram( account_name account, uint64_t bytes ) {
      user_resources_table  userres( _self, account );
      auto res_itr = userres.find( account );
      eosio_assert( res_itr != userres.end(), "no resource row" );
      eosio_assert( res_itr->storage_bytes >= bytes, "insufficient quota" );

      const double system_token_supply   = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name()).amount;
      const double unstaked_token_supply = system_token_supply - _gstate.total_storage_stake.amount;

      const double R = unstaked_token_supply;
      const double C = _gstate.free_ram() + bytes;
      const double F = .10; 
      const double T = bytes;
      const double ONE(1.0);

      double E = -R * (ONE - std::pow( ONE + T/C, F ) );

      E *= .99; /// 1% fee on every conversion, 
                /// let the system contract profit on speculation while preventing abuse caused by rounding errors
      
      int64_t tokens_out = int64_t(E);
      eosio_assert( tokens_out > 0, "must free at least one token" );

      _gstate.total_storage_bytes_reserved -= bytes;
      _gstate.total_storage_stake.amount   -= tokens_out;

      userres.modify( res_itr, account, [&]( auto& res ) {
          res.storage_bytes -= bytes;
      });
      set_resource_limits( res_itr->owner, res_itr->storage_bytes, uint64_t(res_itr->net_weight.amount), uint64_t(res_itr->cpu_weight.amount) );

      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                    { N(eosio), account, asset(tokens_out), std::string("sell ram") } );
   }

   void system_contract::delegatebw( account_name from, account_name receiver,
                                     asset stake_net_quantity, 
                                     asset stake_cpu_quantity )
                                    
   {
      require_auth( from );

      eosio_assert( stake_cpu_quantity.amount >= 0, "must stake a positive amount" );
      eosio_assert( stake_net_quantity.amount >= 0, "must stake a positive amount" );

      asset total_stake = stake_cpu_quantity + stake_net_quantity;
      eosio_assert( total_stake.amount > 0, "must stake a positive amount" );

      del_bandwidth_table     del_tbl( _self, from );
      auto itr = del_tbl.find( receiver );
      if( itr == del_tbl.end() ) {
         del_tbl.emplace( from, [&]( auto& dbo ){
               dbo.from          = from;
               dbo.to            = receiver;
               dbo.net_weight    = stake_net_quantity;
               dbo.cpu_weight    = stake_cpu_quantity;
            });
      }
      else {
         del_tbl.modify( itr, from, [&]( auto& dbo ){
               dbo.net_weight    += stake_net_quantity;
               dbo.cpu_weight    += stake_cpu_quantity;
            });
      }

      user_resources_table   totals_tbl( _self, receiver );
      auto tot_itr = totals_tbl.find( receiver );
      if( tot_itr ==  totals_tbl.end() ) {
         tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
               tot.owner = receiver;
               tot.net_weight    = stake_net_quantity;
               tot.cpu_weight    = stake_cpu_quantity;
            });
      } else {
         totals_tbl.modify( tot_itr, from == receiver ? from : 0, [&]( auto& tot ) {
               tot.net_weight    += stake_net_quantity;
               tot.cpu_weight    += stake_cpu_quantity;
            });
      }

      set_resource_limits( tot_itr->owner, tot_itr->storage_bytes, uint64_t(tot_itr->net_weight.amount), uint64_t(tot_itr->cpu_weight.amount) );

      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {from,N(active)},
                                                    { from, N(eosio), total_stake, std::string("stake bandwidth") } );

      adjust_voting_power( from, (stake_net_quantity.amount + stake_cpu_quantity.amount) );
   } // delegatebw

   void system_contract::undelegatebw( account_name from, account_name receiver,
                                       asset unstake_net_quantity, asset unstake_cpu_quantity )
   {
      eosio_assert( unstake_cpu_quantity.amount >= 0, "must unstake a positive amount" );
      eosio_assert( unstake_net_quantity.amount >= 0, "must unstake a positive amount" );

      require_auth( from );

      //eosio_assert( is_account( receiver ), "can only delegate resources to an existing account" );

      del_bandwidth_table     del_tbl( _self, from );
      const auto& dbw = del_tbl.get( receiver );
      eosio_assert( dbw.net_weight >= unstake_net_quantity, "insufficient staked net bandwidth" );
      eosio_assert( dbw.cpu_weight >= unstake_cpu_quantity, "insufficient staked cpu bandwidth" );

      eosio::asset total_refund = unstake_cpu_quantity + unstake_net_quantity;

      eosio_assert( total_refund.amount > 0, "must unstake a positive amount" );

      del_tbl.modify( dbw, from, [&]( auto& dbo ){
            dbo.net_weight -= unstake_net_quantity;
            dbo.cpu_weight -= unstake_cpu_quantity;
         });

      user_resources_table totals_tbl( _self, receiver );

      const auto& totals = totals_tbl.get( receiver );
      totals_tbl.modify( totals, 0, [&]( auto& tot ) {
            tot.net_weight -= unstake_net_quantity;
            tot.cpu_weight -= unstake_cpu_quantity;
         });

      set_resource_limits( totals.owner, totals.storage_bytes, uint64_t(totals.net_weight.amount), uint64_t(totals.cpu_weight.amount) );

      refunds_table refunds_tbl( _self, from );
      //create refund request
      auto req = refunds_tbl.find( from );
      if ( req != refunds_tbl.end() ) {
         refunds_tbl.modify( req, 0, [&]( refund_request& r ) {
               r.amount += unstake_net_quantity + unstake_cpu_quantity;
               r.request_time = now();
            });
      } else {
         refunds_tbl.emplace( from, [&]( refund_request& r ) {
               r.owner = from;
               r.amount = unstake_net_quantity + unstake_cpu_quantity;
               r.request_time = now();
            });
      }
      //create or replace deferred transaction
      //refund act;
      //act.owner = from;
      eosio::transaction out;
      out.actions.emplace_back( permission_level{ from, N(active) }, _self, N(refund), from );
      out.delay_sec = refund_delay;
      out.send( from, receiver );

      adjust_voting_power( from, -(unstake_net_quantity.amount + unstake_cpu_quantity.amount) );

   } // undelegatebw


   void system_contract::refund( const account_name owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( _self, owner );
      auto req = refunds_tbl.find( owner );
      eosio_assert( req != refunds_tbl.end(), "refund request not found" );
      eosio_assert( req->request_time + refund_delay <= now(), "refund is not available yet" );
      // Until now() becomes NOW, the fact that now() is the timestamp of the previous block could in theory
      // allow people to get their tokens earlier than the 3 day delay if the unstake happened immediately after many
      // consecutive missed blocks.

      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                    { N(eosio), req->owner, req->amount, std::string("unstake") } );

      refunds_tbl.erase( req );
   }


   void system_contract::setparams( uint64_t max_storage_size, uint32_t storage_reserve_ratio ) {
         require_auth( _self );

         eosio_assert( storage_reserve_ratio > 0, "invalid reserve ratio" );

         global_state_singleton gs( _self, _self );
         auto parameters = gs.exists() ? gs.get() : get_default_parameters();

         eosio_assert( max_storage_size > parameters.total_storage_bytes_reserved, "attempt to set max below reserved" );

         parameters.max_storage_size = max_storage_size;
         parameters.storage_reserve_ratio = storage_reserve_ratio;
         gs.set( parameters, _self );
   }

} //namespace eosiosystem
