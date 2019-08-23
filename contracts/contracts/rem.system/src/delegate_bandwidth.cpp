/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/datastream.hpp>
#include <eosio/eosio.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/privileged.hpp>
#include <eosio/serialize.hpp>
#include <eosio/transaction.hpp>

#include <rem.system/rem.system.hpp>
#include <rem.token/rem.token.hpp>

#include <cmath>
#include <map>

namespace eosiosystem {

   using eosio::asset;
   using eosio::const_mem_fun;
   using eosio::indexed_by;
   using eosio::permission_level;
   using eosio::time_point_sec;
   
   using std::map;
   using std::pair;

   static constexpr uint32_t refund_delay_sec = 3*24*3600;

   /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct [[eosio::table, eosio::contract("rem.system")]] delegated_bandwidth {
      name          from;
      name          to;
      asset         net_weight;
      asset         cpu_weight;

      bool is_empty()const { return net_weight.amount == 0 && cpu_weight.amount == 0; }
      uint64_t  primary_key()const { return to.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight))

   };

   struct [[eosio::table, eosio::contract("rem.system")]] refund_request {
      name            owner;
      time_point_sec  request_time;
      eosio::asset    resource_amount;

      bool is_empty()const { return resource_amount.amount == 0; }
      uint64_t  primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(resource_amount) )
   };

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this
    *  facilitates simpler API for per-user queries
    */
   typedef eosio::multi_index< "delband"_n, delegated_bandwidth > del_bandwidth_table;
   typedef eosio::multi_index< "refunds"_n, refund_request >      refunds_table;

   void validate_b1_vesting( int64_t stake ) {
      const int64_t base_time = 1527811200; /// 2018-06-01
      const int64_t max_claimable = 100'000'000'0000ll;
      const int64_t claimable = int64_t(max_claimable * double(current_time_point().sec_since_epoch() - base_time) / (10*seconds_per_year) );

      check( max_claimable - claimable <= stake, "b1 can only claim their tokens over 10 years" );
   }

   void system_contract::changebw( name from, const name& receiver,
                                   const asset& stake_delta, bool transfer )
   {
      require_auth( from );
      check( stake_delta.amount != 0, "should stake non-zero amount" );

      name source_stake_from = from;
      if ( transfer ) {
         from = receiver;
      }

      // update stake delegated from "from" to "receiver"
      {
         del_bandwidth_table     del_tbl( _self, from.value );
         auto itr = del_tbl.find( receiver.value );
         if( itr == del_tbl.end() ) {
            itr = del_tbl.emplace( from, [&]( auto& dbo ){
                  dbo.from          = from;
                  dbo.to            = receiver;
                  dbo.net_weight    = stake_delta;
                  dbo.cpu_weight    = stake_delta;
               });
         }
         else {
            del_tbl.modify( itr, same_payer, [&]( auto& dbo ){
                  dbo.net_weight    += stake_delta;
                  dbo.cpu_weight    += stake_delta;
               });
         }
         check( 0 <= itr->net_weight.amount, "insufficient staked net bandwidth" );
         check( 0 <= itr->cpu_weight.amount, "insufficient staked cpu bandwidth" );
         if ( itr->is_empty() ) {
            del_tbl.erase( itr );
         }
      } // itr can be invalid, should go out of scope

      // update totals of "receiver"
      {
         user_resources_table   totals_tbl( _self, receiver.value );
         auto tot_itr = totals_tbl.find( receiver.value );
         if( tot_itr ==  totals_tbl.end() ) {
            tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
                  tot.owner = receiver;
                  tot.net_weight    = stake_delta;
                  tot.cpu_weight    = stake_delta;
                  if (from == receiver) {
                     tot.own_stake_amount = stake_delta.amount;
                  }
               });
         } else {
            totals_tbl.modify( tot_itr, from == receiver ? from : same_payer, [&]( auto& tot ) {
                  tot.net_weight    += stake_delta;
                  tot.cpu_weight    += stake_delta;
                  if (from == receiver) {
                     tot.own_stake_amount += stake_delta.amount;
                  }
               });
         }
         check( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
         check( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );
         check( _gstate.min_account_stake <= tot_itr->own_stake_amount, "insufficient minimal account stake for " + receiver.to_string() );

         auto prod = _producers.find( receiver.value );
         if (prod != _producers.end() && prod->active() && from == receiver) {
            _gstate.total_producer_stake += stake_delta.amount;
         }

         {
            bool ram_managed = false;
            bool net_managed = false;
            bool cpu_managed = false;

            auto voter_itr = _voters.find( receiver.value );
            if( voter_itr != _voters.end() ) {
               ram_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::ram_managed );
               net_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::net_managed );
               cpu_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::cpu_managed );
            }
            if( !(net_managed && cpu_managed && ram_managed) ) {
               int64_t ram_bytes, net, cpu;
               get_resource_limits( receiver, ram_bytes, net, cpu );
               const auto system_token_max_supply = eosio::token::get_max_supply(token_account, system_contract::get_core_symbol().code() );
               const double bytes_per_token = (double)_gstate.max_ram_size / (double)system_token_max_supply.amount;
               const int64_t bytes_for_stake = bytes_per_token * tot_itr->own_stake_amount;
               //print("changebw from='", eosio::name{from} ,"' receiver='", eosio::name{receiver}, "' ram_bytes=", tot_itr->ram_bytes, " stake_delta=", stake_delta.amount, " transfer=", transfer, " bytes_per_token=", bytes_per_token / 10000, " system_token_max_supply=", system_token_max_supply.amount / 10000, " \n");
               set_resource_limits( receiver,
                                    ram_managed ? ram_bytes : bytes_for_stake,
                                    net_managed ? net : tot_itr->net_weight.amount,
                                    cpu_managed ? cpu : tot_itr->cpu_weight.amount );
            }
         }

      } // tot_itr can be invalid, should go out of scope

      // create refund or update from existing refund
      if ( stake_account != source_stake_from ) { //for eosio.stake both transfer and refund make no sense
         refunds_table refunds_tbl( _self, from.value );
         auto req = refunds_tbl.find( from.value );

         //create/update/delete refund
         auto temp_balance = stake_delta;
         bool need_deferred_trx = false;


         // net and cpu are same sign by assertions in delegatebw and undelegatebw
         // redundant assertion also at start of changebw to protect against misuse of changebw
         bool is_undelegating = stake_delta.amount < 0;
         bool is_delegating_to_self = (!transfer && from == receiver);

         if( is_delegating_to_self || is_undelegating ) {
            if ( req != refunds_tbl.end() ) { //need to update refund
               refunds_tbl.modify( req, same_payer, [&]( refund_request& r ) {
                  if ( temp_balance.amount < 0 ) {
                     r.request_time = current_time_point();
                  }
                  r.resource_amount -= temp_balance;
                  if ( r.resource_amount.amount < 0 ) {
                     temp_balance = -r.resource_amount;
                     r.resource_amount.amount = 0;
                  } else {
                     temp_balance.amount = 0;
                  }
               });

               check( 0 <= req->resource_amount.amount, "negative net refund amount" ); //should never happen

               if ( req->is_empty() ) {
                  refunds_tbl.erase( req );
                  need_deferred_trx = false;
               } else {
                  need_deferred_trx = true;
               }
            } else if ( temp_balance.amount < 0 ) { //need to create refund
               refunds_tbl.emplace( from, [&]( refund_request& r ) {
                  r.owner = from;
                  r.resource_amount = -temp_balance;
                  r.request_time = current_time_point();
               });
               temp_balance.amount = 0;
               need_deferred_trx = true;
            } // else stake increase requested with no existing row in refunds_tbl -> nothing to do with refunds_tbl
         } /// end if is_delegating_to_self || is_undelegating

         if ( need_deferred_trx ) {
            eosio::transaction out;
            out.actions.emplace_back( permission_level{from, active_permission},
                                      _self, "refund"_n,
                                      from
            );
            out.delay_sec = refund_delay_sec;
            cancel_deferred( from.value ); // TODO: Remove this line when replacing deferred trxs is fixed
            out.send( from.value, from, true );
         } else {
            cancel_deferred( from.value );
         }

         auto transfer_amount = temp_balance;
         if ( 0 < transfer_amount.amount ) {
            token::transfer_action transfer_act{ token_account, { {source_stake_from, active_permission} } };
            transfer_act.send( source_stake_from, stake_account, asset(transfer_amount), "stake bandwidth" );
         }
      }

      vote_stake_updater( from );
      update_voting_power( from, stake_delta );
   }

   void system_contract::update_voting_power( const name& voter, const asset& total_update )
   {
      auto voter_itr = _voters.find( voter.value );
      if( voter_itr == _voters.end() ) {
         voter_itr = _voters.emplace( voter, [&]( auto& v ) {
            v.owner            = voter;
            v.staked           = total_update.amount;
            v.stake_lock_time = current_time_point() + _gstate.stake_lock_period;
         });
      } else {
         _voters.modify( voter_itr, same_payer, [&]( auto& v ) {
            v.staked += total_update.amount;
            if ( total_update.amount >= 0 ) {
               const auto restake_rate = double(total_update.amount) / v.staked;
               const auto prevstake_rate = 1 - restake_rate;
               const auto time_to_stake_unlock = std::max( v.stake_lock_time - current_time_point(), microseconds{} );


                printf("==========================\n");
                printf("%lld\n",total_update.amount);
                printf("%lld\n",  v.staked);
                printf("%f\n",  restake_rate);
                printf("%f\n",  prevstake_rate);
               printf("%lld\n",current_time_point());
               printf("%lld\n", v.stake_lock_time);
                printf("%lld\n", time_to_stake_unlock.count());
                printf("%lld\n", microseconds{ static_cast< int64_t >( prevstake_rate * time_to_stake_unlock.count() ) });
               printf("==========================\n");


                printf("%lld\n",current_time_point());
                printf("%lld\n",current_time_point());
                printf("%lld\n",current_time_point());
                printf("%lld\n", _gstate.stake_lock_period.count() );
               v.stake_lock_time = current_time_point()
                     + microseconds{ static_cast< int64_t >( prevstake_rate * time_to_stake_unlock.count() ) }
                     + microseconds{ static_cast< int64_t >( restake_rate * _gstate.stake_lock_period.count() ) };

                printf("========A===============\n");
                printf("%lld\n", v.stake_lock_time);
                printf("========A=================\n");
            }
         });
      }

      check( 0 <= voter_itr->staked, "stake for voting cannot be negative" );

      if( voter == "b1"_n ) {
         validate_b1_vesting( voter_itr->staked );
      }

      if( voter_itr->producers.size() || voter_itr->proxy ) {
         update_votes( voter, voter_itr->proxy, voter_itr->producers, false );
      }
   }

   void system_contract::delegatebw( const name& from, const name& receiver,
                                     const asset& stake_quantity,
                                     bool transfer )
   {
      asset zero_asset( 0, core_symbol() );
      check( stake_quantity >= zero_asset, "must stake a positive amount" );
      check( !transfer || from != receiver, "cannot use transfer flag if delegating to self" );
      changebw( from, receiver, stake_quantity, transfer);
   } // delegatebw

   void system_contract::undelegatebw( const name& from, const name& receiver,
                                       const asset& unstake_quantity)
   {
      asset zero_asset( 0, core_symbol() );
      check( unstake_quantity >= zero_asset, "must unstake a positive amount" );
      check( _gstate.total_activated_stake >= min_activated_stake,
             "cannot undelegate bandwidth until the chain is activated (at least 15% of all tokens participate in voting)" );

      if(is_block_producer(from))
      {
          const auto &vot = _voters.get(from.value, "user has no resources");
          check(vot.locked_stake >= unstake_quantity.amount, "cannot undelegate more than was staked");
          check(vot.stake_lock_time <= current_time_point(), "cannot undelegate during stake lock period");
          
          _voters.modify(vot, from, [&](auto &v) {
              v.locked_stake -= unstake_quantity.amount;
              v.last_claim_time = current_time_point();
          });
      }

      changebw( from, receiver, -unstake_quantity, false);
   } // undelegatebw


   void system_contract::refund( const name& owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( _self, owner.value );
      auto req = refunds_tbl.find( owner.value );
      check( req != refunds_tbl.end(), "refund request not found" );
      check( req->request_time + seconds(refund_delay_sec) <= current_time_point(),
             "refund is not available yet" );
      token::transfer_action transfer_act{ token_account, { {stake_account, active_permission}, {req->owner, active_permission} } };
      transfer_act.send( stake_account, req->owner, req->resource_amount, "unstake" );
      refunds_tbl.erase( req );
   }


} //namespace eosiosystem
