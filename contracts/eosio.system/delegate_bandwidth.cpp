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

   struct total_resources {
      account_name  owner;
      asset         net_weight;
      asset         cpu_weight;
      asset         storage_stake;   
      uint64_t      storage_bytes = 0;

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( total_resources, (owner)(net_weight)(cpu_weight)(storage_stake)(storage_bytes) )
   };


   /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct delegated_bandwidth {
      account_name  from;
      account_name  to;
      asset         net_weight;
      asset         cpu_weight;
      asset         storage_stake;
      uint64_t      storage_bytes = 0;

      uint64_t  primary_key()const { return to; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight)(storage_stake)(storage_bytes) )

   };

   struct refund_request {
      account_name  owner;
      time          request_time;
      eosio::asset  amount;

      uint64_t  primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(amount) )
   };

   typedef eosio::multi_index< N(totalband), total_resources>   total_resources_table;
   typedef eosio::multi_index< N(delband), delegated_bandwidth> del_bandwidth_table;
   typedef eosio::multi_index< N(refunds), refund_request>      refunds_table;

   void system_contract::delegatebw( const account_name from, const account_name receiver,
                                     const asset& stake_net_quantity, const asset& stake_cpu_quantity,
                                     const asset& stake_storage_quantity )
   {
      require_auth( from );
      eosio_assert( stake_cpu_quantity.amount >= 0, "must stake a positive amount" );
      eosio_assert( stake_net_quantity.amount >= 0, "must stake a positive amount" );
      eosio_assert( stake_storage_quantity.amount >= 0, "must stake a positive amount" );

      if( stake_storage_quantity.amount > 0 ) {
         eosio_assert( from == receiver, "you may only stake storage to yourself" );
      }

      print( "adding stake...", stake_net_quantity, " ", stake_cpu_quantity, " ", stake_storage_quantity );
      asset total_stake = stake_cpu_quantity + stake_net_quantity + stake_storage_quantity;
      print( "\ntotal stake: ", total_stake );
      eosio_assert( total_stake.amount > 0, "must stake a positive amount" );


      //eosio_assert( is_account( receiver ), "can only delegate resources to an existing account" );
      int64_t storage_bytes = 0;
      if ( 0 < stake_storage_quantity.amount ) {
         global_state_singleton gs( _self, _self );
         auto parameters = gs.exists() ? gs.get() : get_default_parameters();
         const eosio::asset token_supply = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name());

         print( " token supply: ", token_supply, " \n" );
         print( " max storage size: ", parameters.max_storage_size, "\n");
         print( " total reserved: ", parameters.total_storage_bytes_reserved, "\n");

         //make sure that there is no posibility of overflow here
         int64_t storage_bytes_estimated = int64_t( parameters.max_storage_size - parameters.total_storage_bytes_reserved )
            * int64_t(parameters.storage_reserve_ratio) * stake_storage_quantity
            / ( token_supply - parameters.total_storage_stake ) / 1000 /* reserve ratio coefficient */;

         storage_bytes = ( int64_t(parameters.max_storage_size) - int64_t(parameters.total_storage_bytes_reserved) - storage_bytes_estimated )
            * int64_t(parameters.storage_reserve_ratio) * stake_storage_quantity
            / ( token_supply - stake_storage_quantity - parameters.total_storage_stake ) / 1000 /* reserve ratio coefficient */;

         eosio_assert( 0 < storage_bytes, "stake is too small to increase storage even by 1 byte" );

         parameters.total_storage_bytes_reserved += uint64_t(storage_bytes);
         print( "\ntotal storage stake: ", parameters.total_storage_stake, "\n" );
         parameters.total_storage_stake += stake_storage_quantity;
         gs.set( parameters, _self );
      }

      del_bandwidth_table     del_tbl( _self, from );
      auto itr = del_tbl.find( receiver );
      if( itr == del_tbl.end() ) {
         del_tbl.emplace( from, [&]( auto& dbo ){
               dbo.from          = from;
               dbo.to            = receiver;
               dbo.net_weight    = stake_net_quantity;
               dbo.cpu_weight    = stake_cpu_quantity;
               dbo.storage_stake = stake_storage_quantity;
               dbo.storage_bytes = uint64_t(storage_bytes);
            });
      }
      else {
         del_tbl.modify( itr, from, [&]( auto& dbo ){
               dbo.net_weight    += stake_net_quantity;
               dbo.cpu_weight    += stake_cpu_quantity;
               dbo.storage_stake += stake_storage_quantity;
               dbo.storage_bytes += uint64_t(storage_bytes);
            });
      }

      total_resources_table   totals_tbl( _self, receiver );
      auto tot_itr = totals_tbl.find( receiver );
      if( tot_itr ==  totals_tbl.end() ) {
         tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
               tot.owner = receiver;
               tot.net_weight    = stake_net_quantity;
               tot.cpu_weight    = stake_cpu_quantity;
               tot.storage_stake = stake_storage_quantity;
               tot.storage_bytes = uint64_t(storage_bytes);
            });
      } else {
         totals_tbl.modify( tot_itr, from == receiver ? from : 0, [&]( auto& tot ) {
               tot.net_weight    += stake_net_quantity;
               tot.cpu_weight    += stake_cpu_quantity;
               tot.storage_stake += stake_storage_quantity;
               tot.storage_bytes += uint64_t(storage_bytes);
            });
      }

      set_resource_limits( tot_itr->owner, tot_itr->storage_bytes, tot_itr->net_weight.amount, tot_itr->cpu_weight.amount );

      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {from,N(active)},
                                                    { from, N(eosio), total_stake, std::string("stake bandwidth") } );

      if ( asset(0) < stake_net_quantity + stake_cpu_quantity ) {
         increase_voting_power( from, stake_net_quantity + stake_cpu_quantity );
      }

   } // delegatebw

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

   void system_contract::undelegatebw( const account_name from, const account_name receiver,
                                       const asset unstake_net_quantity, const asset unstake_cpu_quantity,
                                       const uint64_t unstake_storage_bytes )
   {
      eosio_assert( unstake_cpu_quantity.amount >= 0, "must unstake a positive amount" );
      eosio_assert( unstake_net_quantity.amount >= 0, "must unstake a positive amount" );

      require_auth( from );

      //eosio_assert( is_account( receiver ), "can only delegate resources to an existing account" );

      del_bandwidth_table     del_tbl( _self, from );
      const auto& dbw = del_tbl.get( receiver );
      eosio_assert( dbw.net_weight >= unstake_net_quantity, "insufficient staked net bandwidth" );
      eosio_assert( dbw.cpu_weight >= unstake_cpu_quantity, "insufficient staked cpu bandwidth" );
      eosio_assert( dbw.storage_bytes >= unstake_storage_bytes, "insufficient staked storage" );

      eosio::asset storage_stake_decrease(0, system_token_symbol);
      if ( 0 < unstake_storage_bytes ) {
         storage_stake_decrease = 0 < dbw.storage_bytes ?
                                      dbw.storage_stake * int64_t(unstake_storage_bytes) / int64_t(dbw.storage_bytes)
                                      : eosio::asset(0, system_token_symbol);
         global_state_singleton gs( _self, _self );
         auto parameters = gs.get(); //it should exist if user staked for bandwith
         parameters.total_storage_bytes_reserved -= unstake_storage_bytes;
         parameters.total_storage_stake -= storage_stake_decrease;
         gs.set( parameters, _self );
      }

      eosio::asset total_refund = unstake_cpu_quantity + unstake_net_quantity + storage_stake_decrease;

      eosio_assert( total_refund.amount > 0, "must unstake a positive amount" );

      del_tbl.modify( dbw, from, [&]( auto& dbo ){
            dbo.net_weight -= unstake_net_quantity;
            dbo.cpu_weight -= unstake_cpu_quantity;
            dbo.storage_stake -= storage_stake_decrease;
            dbo.storage_bytes -= unstake_storage_bytes;
         });

      total_resources_table totals_tbl( _self, receiver );
      const auto& totals = totals_tbl.get( receiver );
      totals_tbl.modify( totals, 0, [&]( auto& tot ) {
            tot.net_weight -= unstake_net_quantity;
            tot.cpu_weight -= unstake_cpu_quantity;
            tot.storage_stake -= storage_stake_decrease;
            tot.storage_bytes -= unstake_storage_bytes;
         });

      set_resource_limits( totals.owner, totals.storage_bytes, totals.net_weight.amount, totals.cpu_weight.amount );

      refunds_table refunds_tbl( _self, from );
      //create refund request
      auto req = refunds_tbl.find( from );
      if ( req != refunds_tbl.end() ) {
         refunds_tbl.modify( req, 0, [&]( refund_request& r ) {
               r.amount += unstake_net_quantity + unstake_cpu_quantity + storage_stake_decrease;
               r.request_time = now();
            });
      } else {
         refunds_tbl.emplace( from, [&]( refund_request& r ) {
               r.owner = from;
               r.amount = unstake_net_quantity + unstake_cpu_quantity + storage_stake_decrease;
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

      if ( asset(0) < unstake_net_quantity + unstake_cpu_quantity ) {
         decrease_voting_power( from, unstake_net_quantity + unstake_cpu_quantity );
      }

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

} //namespace eosiosystem
