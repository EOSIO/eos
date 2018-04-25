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
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using eosio::print;
   using eosio::singleton;
   using eosio::transaction;


   static constexpr uint32_t blocks_per_year = 52*7*24*2*3600; // half seconds per year
   static constexpr uint32_t blocks_per_producer = 12;

   struct voter_info {
      account_name                owner = 0;
      account_name                proxy = 0;
      time                        last_update = 0;
      uint32_t                    is_proxy = 0;
      eosio::asset                staked;
      eosio::asset                unstaking;
      eosio::asset                unstake_per_week;
      uint128_t                   proxied_votes = 0;
      std::vector<account_name>   producers;
      uint32_t                    deferred_trx_id = 0;
      time                        last_unstake_time = 0; //uint32

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(last_update)(is_proxy)(staked)(unstaking)(unstake_per_week)(proxied_votes)(producers)(deferred_trx_id)(last_unstake_time) )
   };

   typedef eosio::multi_index< N(voters), voter_info>  voters_table;

   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of producer to register
    *
    */
   void system_contract::regproducer( const account_name producer, const bytes& packed_producer_key, const eosio_parameters& prefs ) {
      //eosio::print("produce_key: ", producer_key.size(), ", sizeof(public_key): ", sizeof(public_key), "\n");
      require_auth( producer );

      producers_table producers_tbl( _self, _self );
      auto prod = producers_tbl.find( producer );

      //check that we can unpack producer key
      public_key producer_key = eosio::unpack<public_key>( packed_producer_key );

      if ( prod != producers_tbl.end() ) {
         producers_tbl.modify( prod, producer, [&]( producer_info& info ){
               info.prefs = prefs;
               info.packed_key = eosio::pack<public_key>( producer_key );
            });
      } else {
         producers_tbl.emplace( producer, [&]( producer_info& info ){
               info.owner       = producer;
               info.total_votes = 0;
               info.prefs       = prefs;
               info.packed_key  = eosio::pack<public_key>( producer_key );
            });
      }
   }

   void system_contract::unregprod( const account_name producer ) {
      require_auth( producer );

      producers_table producers_tbl( _self, _self );
      auto prod = producers_tbl.find( producer );
      eosio_assert( prod != producers_tbl.end(), "producer not found" );

      producers_tbl.modify( prod, 0, [&]( producer_info& info ){
            info.packed_key.clear();
         });
   }

   void system_contract::increase_voting_power( account_name acnt, const eosio::asset& amount ) {
      voters_table voters_tbl( _self, _self );
      auto voter = voters_tbl.find( acnt );

      eosio_assert( 0 <= amount.amount, "negative asset" );

      if( voter == voters_tbl.end() ) {
         voter = voters_tbl.emplace( acnt, [&]( voter_info& a ) {
               a.owner = acnt;
               a.last_update = now();
               a.staked = amount;
            });
      } else {
         voters_tbl.modify( voter, 0, [&]( auto& av ) {
               av.last_update = now();
               av.staked += amount;
            });
      }

      const std::vector<account_name>* producers = nullptr;
      if ( voter->proxy ) {
         auto proxy = voters_tbl.find( voter->proxy );
         eosio_assert( proxy != voters_tbl.end(), "selected proxy not found" ); //data corruption
         voters_tbl.modify( proxy, 0, [&](voter_info& a) { a.proxied_votes += uint64_t(amount.amount); } );
         if ( proxy->is_proxy ) { //only if proxy is still active. if proxy has been unregistered, we update proxied_votes, but don't propagate to producers
            producers = &proxy->producers;
         }
      } else {
         producers = &voter->producers;
      }

      if ( producers ) {
         producers_table producers_tbl( _self, _self );
         for( auto p : *producers ) {
            auto prod = producers_tbl.find( p );
            eosio_assert( prod != producers_tbl.end(), "never existed producer" ); //data corruption
            producers_tbl.modify( prod, 0, [&]( auto& v ) {
                  v.total_votes += uint64_t(amount.amount);
               });
         }
      }
   }

   void system_contract::decrease_voting_power( account_name acnt, const eosio::asset& amount ) {
      require_auth( acnt );
      voters_table voters_tbl( _self, _self );
      auto voter = voters_tbl.find( acnt );
      eosio_assert( voter != voters_tbl.end(), "stake not found" );

      if ( 0 < amount.amount ) {
         eosio_assert( amount <= voter->staked, "cannot unstake more than total stake amount" );
         voters_tbl.modify( voter, 0, [&](voter_info& a) {
               a.staked -= amount;
               a.last_update = now();
            });

         const std::vector<account_name>* producers = nullptr;
         if ( voter->proxy ) {
            auto proxy = voters_tbl.find( voter->proxy );
            voters_tbl.modify( proxy, 0, [&](voter_info& a) { a.proxied_votes -= uint64_t(amount.amount); } );
            if ( proxy->is_proxy ) { //only if proxy is still active. if proxy has been unregistered, we update proxied_votes, but don't propagate to producers
               producers = &proxy->producers;
            }
         } else {
            producers = &voter->producers;
         }

         if ( producers ) {
            producers_table producers_tbl( _self, _self );
            for( auto p : *producers ) {
               auto prod = producers_tbl.find( p );
               eosio_assert( prod != producers_tbl.end(), "never existed producer" ); //data corruption
               producers_tbl.modify( prod, 0, [&]( auto& v ) {
                     v.total_votes -= uint64_t(amount.amount);
                  });
            }
         }
      } else {
         if (voter->deferred_trx_id) {
            //XXX cancel_deferred_transaction(voter->deferred_trx_id);
         }
         voters_tbl.modify( voter, 0, [&](voter_info& a) {
               a.staked += a.unstaking;
               a.unstaking.amount = 0;
               a.unstake_per_week.amount = 0;
               a.deferred_trx_id = 0;
               a.last_update = now();
            });
      }
   }

   eosio_global_state system_contract::get_default_parameters() {
      eosio_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
   }

   eosio::asset system_contract::payment_per_block(uint32_t percent_of_max_inflation_rate) {
      const eosio::asset token_supply = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name());
      const double annual_rate = double(max_inflation_rate * percent_of_max_inflation_rate) / double(10000);
      const double continuous_rate = std::log1p(annual_rate);
      int64_t payment = static_cast<int64_t>((continuous_rate * double(token_supply.amount)) / double(blocks_per_year));
      return eosio::asset(payment, system_token_symbol);
   }

   void system_contract::update_elected_producers(time cycle_time) {
      producers_table producers_tbl( _self, _self );
      auto idx = producers_tbl.template get_index<N(prototalvote)>();

      std::array<uint32_t, 21> base_per_transaction_net_usage;
      std::array<uint32_t, 21> base_per_transaction_cpu_usage;
      std::array<uint32_t, 21> base_per_action_cpu_usage;
      std::array<uint32_t, 21> base_setcode_cpu_usage;
      std::array<uint32_t, 21> per_signature_cpu_usage;
      std::array<uint32_t, 21> per_lock_net_usage;
      std::array<uint64_t, 21> context_free_discount_cpu_usage_num;
      std::array<uint64_t, 21> context_free_discount_cpu_usage_den;
      std::array<uint32_t, 21> max_transaction_cpu_usage;
      std::array<uint32_t, 21> max_transaction_net_usage;
      std::array<uint64_t, 21> max_block_cpu_usage;
      std::array<uint32_t, 21> target_block_cpu_usage_pct;
      std::array<uint64_t, 21> max_block_net_usage;
      std::array<uint32_t, 21> target_block_net_usage_pct;
      std::array<uint32_t, 21> max_transaction_lifetime;
      std::array<uint16_t, 21> max_authority_depth;
      std::array<uint32_t, 21> max_transaction_exec_time;
      std::array<uint16_t, 21> max_inline_depth;
      std::array<uint32_t, 21> max_inline_action_size;
      std::array<uint32_t, 21> max_generated_transaction_count;
      std::array<uint32_t, 21> max_transaction_delay;
      std::array<uint32_t, 21> percent_of_max_inflation_rate;
      std::array<uint32_t, 21> storage_reserve_ratio;

      eosio::producer_schedule schedule;
      schedule.producers.reserve(21);
      size_t n = 0;
      for ( auto it = idx.crbegin(); it != idx.crend() && n < 21 && 0 < it->total_votes; ++it ) {
         if ( it->active() ) {
            schedule.producers.emplace_back();
            schedule.producers.back().producer_name = it->owner;
            //eosio_assert( sizeof(schedule.producers.back().block_signing_key) == it->packed_key.size(), "size mismatch" );
            schedule.producers.back().block_signing_key = eosio::unpack<public_key>( it->packed_key );
            //std::copy( it->packed_key.begin(), it->packed_key.end(), schedule.producers.back().block_signing_key.data.data() );

            base_per_transaction_net_usage[n] = it->prefs.base_per_transaction_net_usage;
            base_per_transaction_cpu_usage[n] = it->prefs.base_per_transaction_cpu_usage;
            base_per_action_cpu_usage[n] = it->prefs.base_per_action_cpu_usage;
            base_setcode_cpu_usage[n] = it->prefs.base_setcode_cpu_usage;
            per_signature_cpu_usage[n] = it->prefs.per_signature_cpu_usage;
            per_lock_net_usage[n] = it->prefs.per_lock_net_usage;
            context_free_discount_cpu_usage_num[n] = it->prefs.context_free_discount_cpu_usage_num;
            context_free_discount_cpu_usage_den[n] = it->prefs.context_free_discount_cpu_usage_den;
            max_transaction_cpu_usage[n] = it->prefs.max_transaction_cpu_usage;
            max_transaction_net_usage[n] = it->prefs.max_transaction_net_usage;
            max_block_cpu_usage[n] = it->prefs.max_block_cpu_usage;
            target_block_cpu_usage_pct[n] = it->prefs.target_block_cpu_usage_pct;
            max_block_net_usage[n] = it->prefs.max_block_net_usage;
            target_block_net_usage_pct[n] = it->prefs.target_block_net_usage_pct;
            max_transaction_lifetime[n] = it->prefs.max_transaction_lifetime;
            max_authority_depth[n] = it->prefs.max_authority_depth;
            max_transaction_exec_time[n] = it->prefs.max_transaction_exec_time;
            max_inline_depth[n] = it->prefs.max_inline_depth;
            max_inline_action_size[n] = it->prefs.max_inline_action_size;
            max_generated_transaction_count[n] = it->prefs.max_generated_transaction_count;
            max_transaction_delay[n] = it->prefs.max_transaction_delay;

            storage_reserve_ratio[n] = it->prefs.storage_reserve_ratio;
            percent_of_max_inflation_rate[n] = it->prefs.percent_of_max_inflation_rate;
            ++n;
         }
      }
      if ( n == 0 ) { //no active producers with votes > 0
         return;
      }
      if ( 1 < n ) {
         std::sort( base_per_transaction_net_usage.begin(), base_per_transaction_net_usage.begin()+n );
         std::sort( base_per_transaction_cpu_usage.begin(), base_per_transaction_cpu_usage.begin()+n );
         std::sort( base_per_action_cpu_usage.begin(), base_per_action_cpu_usage.begin()+n );
         std::sort( base_setcode_cpu_usage.begin(), base_setcode_cpu_usage.begin()+n );
         std::sort( per_signature_cpu_usage.begin(), per_signature_cpu_usage.begin()+n );
         std::sort( per_lock_net_usage.begin(), per_lock_net_usage.begin()+n );
         std::sort( context_free_discount_cpu_usage_num.begin(), context_free_discount_cpu_usage_num.begin()+n );
         std::sort( context_free_discount_cpu_usage_den.begin(), context_free_discount_cpu_usage_den.begin()+n );
         std::sort( max_transaction_cpu_usage.begin(), max_transaction_cpu_usage.begin()+n );
         std::sort( max_transaction_net_usage.begin(), max_transaction_net_usage.begin()+n );
         std::sort( max_block_cpu_usage.begin(), max_block_cpu_usage.begin()+n );
         std::sort( target_block_cpu_usage_pct.begin(), target_block_cpu_usage_pct.begin()+n );
         std::sort( max_block_net_usage.begin(), max_block_net_usage.begin()+n );
         std::sort( target_block_net_usage_pct.begin(), target_block_net_usage_pct.begin()+n );
         std::sort( max_transaction_lifetime.begin(), max_transaction_lifetime.begin()+n );
         std::sort( max_transaction_exec_time.begin(), max_transaction_exec_time.begin()+n );
         std::sort( max_authority_depth.begin(), max_authority_depth.begin()+n );
         std::sort( max_inline_depth.begin(), max_inline_depth.begin()+n );
         std::sort( max_inline_action_size.begin(), max_inline_action_size.begin()+n );
         std::sort( max_generated_transaction_count.begin(), max_generated_transaction_count.begin()+n );
         std::sort( max_transaction_delay.begin(), max_transaction_delay.begin()+n );
         std::sort( storage_reserve_ratio.begin(), storage_reserve_ratio.begin()+n );
         std::sort( percent_of_max_inflation_rate.begin(), percent_of_max_inflation_rate.begin()+n );
      }

      // should use producer_schedule_type from libraries/chain/include/eosio/chain/producer_schedule.hpp
      bytes packed_schedule = pack(schedule);
      set_active_producers( packed_schedule.data(),  packed_schedule.size() );
      size_t median = n/2;

      global_state_singleton gs( _self, _self );
      auto parameters = gs.exists() ? gs.get() : get_default_parameters();

      parameters.base_per_transaction_net_usage = base_per_transaction_net_usage[median];
      parameters.base_per_transaction_cpu_usage = base_per_transaction_cpu_usage[median];
      parameters.base_per_action_cpu_usage = base_per_action_cpu_usage[median];
      parameters.base_setcode_cpu_usage = base_setcode_cpu_usage[median];
      parameters.per_signature_cpu_usage = per_signature_cpu_usage[median];
      parameters.per_lock_net_usage = per_lock_net_usage[median];
      parameters.context_free_discount_cpu_usage_num = context_free_discount_cpu_usage_num[median];
      parameters.context_free_discount_cpu_usage_den = context_free_discount_cpu_usage_den[median];
      parameters.max_transaction_cpu_usage = max_transaction_cpu_usage[median];
      parameters.max_transaction_net_usage = max_transaction_net_usage[median];
      parameters.max_block_cpu_usage = max_block_cpu_usage[median];
      parameters.target_block_cpu_usage_pct = target_block_cpu_usage_pct[median];
      parameters.max_block_net_usage = max_block_net_usage[median];
      parameters.target_block_net_usage_pct = target_block_net_usage_pct[median];
      parameters.max_transaction_lifetime = max_transaction_lifetime[median];
      parameters.max_transaction_exec_time = max_transaction_exec_time[median];
      parameters.max_authority_depth = max_authority_depth[median];
      parameters.max_inline_depth = max_inline_depth[median];
      parameters.max_inline_action_size = max_inline_action_size[median];
      parameters.max_generated_transaction_count = max_generated_transaction_count[median];
      parameters.max_transaction_delay = max_transaction_delay[median];
      parameters.storage_reserve_ratio = storage_reserve_ratio[median];
      parameters.percent_of_max_inflation_rate = percent_of_max_inflation_rate[median];

      // not voted on
      parameters.first_block_time_in_cycle = cycle_time;

      // derived parameters
      auto half_of_percentage = parameters.percent_of_max_inflation_rate / 2;
      auto other_half_of_percentage = parameters.percent_of_max_inflation_rate - half_of_percentage;
      parameters.payment_per_block = payment_per_block(half_of_percentage);
      parameters.payment_to_eos_bucket = payment_per_block(other_half_of_percentage);
      parameters.blocks_per_cycle = blocks_per_producer * schedule.producers.size();

      if ( parameters.max_storage_size < parameters.total_storage_bytes_reserved ) {
         parameters.max_storage_size = parameters.total_storage_bytes_reserved;
      }

      auto issue_quantity = parameters.blocks_per_cycle * (parameters.payment_per_block + parameters.payment_to_eos_bucket);
      INLINE_ACTION_SENDER(eosio::token, issue)( N(eosio.token), {{N(eosio),N(active)}},
                                                 {N(eosio), issue_quantity, std::string("producer pay")} );

      set_blockchain_parameters( parameters );
      gs.set( parameters, _self );
   }

   /**
    *  @pre producers must be sorted from lowest to highest
    *  @pre if proxy is set then no producers can be voted for
    *  @pre every listed producer or proxy must have been previously registered
    *  @pre voter must authorize this action
    *  @pre voter must have previously staked some EOS for voting
    */
   void system_contract::voteproducer( const account_name voter, const account_name proxy, const std::vector<account_name>& producers ) {
      require_auth( voter );

      //validate input
      if ( proxy ) {
         eosio_assert( producers.size() == 0, "cannot vote for producers and proxy at same time" );
         require_recipient( proxy );
      } else {
         eosio_assert( producers.size() <= 30, "attempt to vote for too many producers" );
         for( size_t i = 1; i < producers.size(); ++i ) {
            eosio_assert( producers[i-1] < producers[i], "producer votes must be unique and sorted" );
         }
      }

      voters_table voters_tbl( _self, _self );
      auto voter_it = voters_tbl.find( voter );

      eosio_assert( 0 <= voter_it->staked.amount, "negative stake" );
      eosio_assert( voter_it != voters_tbl.end() && ( 0 < voter_it->staked.amount || ( voter_it->is_proxy && 0 < voter_it->proxied_votes ) ), "no stake to vote" );
      if ( voter_it->is_proxy ) {
         eosio_assert( proxy == 0 , "account registered as a proxy is not allowed to use a proxy" );
      }

      //find old producers, update old proxy if needed
      const std::vector<account_name>* old_producers = nullptr;
      if( voter_it->proxy ) {
         if ( voter_it->proxy == proxy ) {
            return; // nothing changed
         }
         auto old_proxy = voters_tbl.find( voter_it->proxy );
         eosio_assert( old_proxy != voters_tbl.end(), "old proxy not found" ); //data corruption
         voters_tbl.modify( old_proxy, 0, [&](auto& a) { a.proxied_votes -= uint64_t(voter_it->staked.amount); } );
         if ( old_proxy->is_proxy ) { //if proxy stopped being a proxy, the votes were already taken back from producers by on( const unregister_proxy& )
            old_producers = &old_proxy->producers;
         }
      } else {
         old_producers = &voter_it->producers;
      }

      //find new producers, update new proxy if needed
      const std::vector<account_name>* new_producers = nullptr;
      if ( proxy ) {
         auto new_proxy = voters_tbl.find( proxy );
         eosio_assert( new_proxy != voters_tbl.end() && new_proxy->is_proxy, "proxy not found" );
         voters_tbl.modify( new_proxy, 0, [&](auto& a) { a.proxied_votes += uint64_t(voter_it->staked.amount); } );
         new_producers = &new_proxy->producers;
      } else {
         new_producers = &producers;
      }

      producers_table producers_tbl( _self, _self );
      uint128_t votes = uint64_t(voter_it->staked.amount);
      if ( voter_it->is_proxy ) {
         votes += voter_it->proxied_votes;
      }

      if ( old_producers ) { //old_producers == nullptr if proxy has stopped being a proxy and votes were taken back from the producers at that moment
         //revoke votes only from no longer elected
         std::vector<account_name> revoked( old_producers->size() );
         auto end_it = std::set_difference( old_producers->begin(), old_producers->end(), new_producers->begin(), new_producers->end(), revoked.begin() );
         for ( auto it = revoked.begin(); it != end_it; ++it ) {
            auto prod = producers_tbl.find( *it );
            eosio_assert( prod != producers_tbl.end(), "never existed producer" ); //data corruption
            producers_tbl.modify( prod, 0, [&]( auto& pi ) { pi.total_votes -= votes; } );
         }
      }

      //update newly elected
      std::vector<account_name> elected( new_producers->size() );
      auto end_it = elected.begin();
      if( old_producers ) {
         end_it = std::set_difference( new_producers->begin(), new_producers->end(), old_producers->begin(), old_producers->end(), elected.begin() );
      } else {
         end_it = std::copy( new_producers->begin(), new_producers->end(), elected.begin() );
      }
      for ( auto it = elected.begin(); it != end_it; ++it ) {
         auto prod = producers_tbl.find( *it );
         eosio_assert( prod != producers_tbl.end(), "producer is not registered" );
         if ( proxy == 0 ) { //direct voting, in case of proxy voting update total_votes even for inactive producers
            eosio_assert( prod->active(), "producer is not currently registered" );
         }
         producers_tbl.modify( prod, 0, [&]( auto& pi ) { pi.total_votes += votes; } );
      }

      // save new values to the account itself
      voters_tbl.modify( voter_it, 0, [&](voter_info& a) {
            a.proxy = proxy;
            a.last_update = now();
            a.producers = producers;
         });
   }

   void system_contract::regproxy( const account_name proxy ) {
      require_auth( proxy );

      voters_table voters_tbl( _self, _self );
      auto proxy_it = voters_tbl.find( proxy );
      if ( proxy_it != voters_tbl.end() ) {
         eosio_assert( proxy_it->is_proxy == 0, "account is already a proxy" );
         eosio_assert( proxy_it->proxy == 0, "account that uses a proxy is not allowed to become a proxy" );
         voters_tbl.modify( proxy_it, 0, [&](voter_info& a) {
               a.is_proxy = 1;
               a.last_update = now();
               //a.proxied_votes may be > 0, if the proxy has been unregistered, so we had to keep the value
            });
         if ( 0 < proxy_it->proxied_votes ) {
            producers_table producers_tbl( _self, _self );
            for ( auto p : proxy_it->producers ) {
               auto prod = producers_tbl.find( p );
               eosio_assert( prod != producers_tbl.end(), "never existed producer" ); //data corruption
               producers_tbl.modify( prod, 0, [&]( auto& pi ) { pi.total_votes += proxy_it->proxied_votes; });
            }
         }
      } else {
         voters_tbl.emplace( proxy, [&]( voter_info& a ) {
               a.owner = proxy;
               a.last_update = now();
               a.proxy = 0;
               a.is_proxy = 1;
               a.proxied_votes = 0;
               a.staked.amount = 0;
            });
      }
   }

   void system_contract::unregproxy( const account_name proxy ) {
      require_auth( proxy );

      voters_table voters_tbl( _self, _self );
      auto proxy_it = voters_tbl.find( proxy );
      eosio_assert( proxy_it != voters_tbl.end(), "proxy not found" );
      eosio_assert( proxy_it->is_proxy == 1, "account is not a proxy" );

      voters_tbl.modify( proxy_it, 0, [&](voter_info& a) {
            a.is_proxy = 0;
            a.last_update = now();
            //a.proxied_votes should be kept in order to be able to reenable this proxy in the future
         });

      if ( 0 < proxy_it->proxied_votes ) {
         producers_table producers_tbl( _self, _self );
         for ( auto p : proxy_it->producers ) {
            auto prod_it = producers_tbl.find( p );
            eosio_assert( prod_it != producers_tbl.end(), "never existed producer" ); //data corruption
            producers_tbl.modify( prod_it, 0, [&]( auto& pi ) { pi.total_votes -= proxy_it->proxied_votes; });
         }
      }
   }

}
