/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/crypto.hpp>
#include <eosio/datastream.hpp>
#include <eosio/eosio.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/privileged.hpp>
#include <eosio/serialize.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>

#include <rem.system/rem.system.hpp>
#include <rem.token/rem.token.hpp>

#include <algorithm>
#include <cmath>

namespace eosiosystem {

   using eosio::const_mem_fun;
   using eosio::indexed_by;
   using eosio::singleton;
   using eosio::transaction;
   using namespace std::string_literals;


   void system_contract::regproducer( const name& producer, const eosio::public_key& producer_key, const std::string& url, uint16_t location ) {
      check( url.size() < 512, "url too long" );
      check( producer_key != eosio::public_key(), "public key should not be the default value" );
      require_auth( producer );

      // TODO implement as constexpr string
      static const auto stake_err = "user should stake at least "s + std::to_string( producer_stake_threshold ) + " REM to become a producer"s;
      check( does_satisfy_stake_requirement( producer ), stake_err );

      auto prod = _producers.find( producer.value );
      const auto ct = current_time_point();

      user_resources_table totals_tbl( _self, producer.value );
      const auto& tot = totals_tbl.get(producer.value, "producer must have resources");
      if ( prod != _producers.end() ) {
         if (!prod->active()) {
            _gstate.total_producer_stake += tot.own_stake_amount;
         }

         _producers.modify( prod, producer, [&]( producer_info& info ){
            info.producer_key = producer_key;
            info.is_active    = true;
            info.url          = url;
            info.location     = location;
            if ( info.last_claim_time == time_point() ) {
               info.last_claim_time = ct;
               info.last_expected_produced_blocks_update = ct;
            }
         });

         auto prod2 = _producers2.find( producer.value );
         if ( prod2 == _producers2.end() ) {
            _producers2.emplace( producer, [&]( producer_info2& info ){
               info.owner                     = producer;
               info.last_votepay_share_update = ct;
            });
         }
      } else {
         _producers.emplace( producer, [&]( producer_info& info ){
            info.owner           = producer;
            info.total_votes     = 0;
            info.producer_key    = producer_key;
            info.is_active       = true;
            info.url             = url;
            info.location        = location;
            info.last_claim_time = ct;
            info.last_expected_produced_blocks_update = ct;
         });
         _producers2.emplace( producer, [&]( producer_info2& info ){
            info.owner                     = producer;
            info.last_votepay_share_update = ct;
         });
         _gstate.total_producer_stake += tot.own_stake_amount;
      }

   }

   void system_contract::unregprod( const name& producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer.value, "producer not found" );
      if (prod.active()) {
         user_resources_table totals_tbl( _self, producer.value );
         const auto& tot = totals_tbl.get(producer.value, "producer must have resources");
         _gstate.total_producer_stake -= tot.own_stake_amount;
      }
      _producers.modify( prod, same_payer, [&]( producer_info& info ){
         info.deactivate();
      });
   }

   void system_contract::update_elected_producers( const block_timestamp& block_time ) {
      _gstate.last_producer_schedule_update = block_time;

      auto idx = _producers.get_index<"prototalvote"_n>();

      std::vector< std::pair<eosio::producer_key,uint16_t> > top_producers;
      top_producers.reserve(max_block_producers);

      // TODO rewrite in algorithm terms
      // TODO check if we can rely on producer_info::active()
      for ( auto it = idx.cbegin(); it != idx.cend() && top_producers.size() < 21 && 0 < it->total_votes && it->active(); ++it ) {
         top_producers.emplace_back( std::pair<eosio::producer_key,uint16_t>({{it->owner, it->producer_key}, it->location}) );
      }

      if ( top_producers.size() == 0 || top_producers.size() < _gstate.last_producer_schedule_size ) {
         return;
      }

      /// sort by producer name
      std::sort( top_producers.begin(), top_producers.end() );

      std::vector<eosio::producer_key> producers;

      producers.reserve(top_producers.size());
      for( const auto& item : top_producers )
         producers.push_back(item.first);

      if( set_proposed_producers( producers ) >= 0 ) {
         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( top_producers.size() );
      }
   }

   double stake2vote( int64_t staked, time_point vote_mature_time ) {
      check(vote_mature_time != time_point(), "vote should have mature time");

      const auto seconds_to_mature = fmax( (vote_mature_time - current_time_point()).to_seconds(), 0.0 );
      const auto rem_weight = 1.0 - seconds_to_mature / system_contract::vote_mature_period.to_seconds();
      const double weight = int64_t((current_time_point().sec_since_epoch() - (block_timestamp::block_timestamp_epoch / 1000)) / (seconds_per_day * 7)) / double(52);

      const auto vote_weight = double(staked) * weight * rem_weight;
      
      check( vote_weight >= 0.0, "vote weight cannot be negative" );
      return vote_weight;
   }

   void system_contract::voteproducer( const name& voter_name, const name& proxy, const std::vector<name>& producers ) {
      require_auth( voter_name );

      check( is_block_producer( voter_name ), "only block producers are allowed to vote" );

      vote_stake_updater( voter_name );
      update_votes( voter_name, proxy, producers, true );
      auto rex_itr = _rexbalance.find( voter_name.value );
      if( rex_itr != _rexbalance.end() && rex_itr->rex_balance.amount > 0 ) {
         check_voting_requirement( voter_name, "voter holding REX tokens must vote for at least 21 producers or for a proxy" );
      }
   }

   void system_contract::update_votes( const name& voter_name, const name& proxy, const std::vector<name>& producers, bool voting ) {
      //validate input
      if ( proxy ) {
         check( producers.size() == 0, "cannot vote for producers and proxy at same time" );
         check( voter_name != proxy, "cannot proxy to self" );
      } else {
         check( producers.size() <= 30, "attempt to vote for too many producers" );
         for( size_t i = 1; i < producers.size(); ++i ) {
            check( producers[i-1] < producers[i], "producer votes must be unique and sorted" );
         }
      }

      auto voter = _voters.find( voter_name.value );
      check( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object
      check( !proxy || !voter->is_proxy, "account registered as a proxy is not allowed to use a proxy" );


      /**
       * The first time someone votes we calculate and set last_vote_weight, since they cannot unstake until
       * after total_activated_stake hits threshold, we can use last_vote_weight to determine that this is
       * their first vote and should consider their stake activated.
       */
      if( voter->last_vote_weight <= 0.0 ) {
         _gstate.total_activated_stake += voter->staked;
         if( _gstate.total_activated_stake >= min_activated_stake && _gstate.thresh_activated_stake_time == time_point() ) {
            _gstate.thresh_activated_stake_time = current_time_point();
         }
      }

      auto new_vote_weight = stake2vote( voter->staked, voter->vote_mature_time );
      if( voter->is_proxy ) {
         new_vote_weight += voter->proxied_vote_weight;
      }

      std::map<name, pair<double, bool /*new*/> > producer_deltas;
      if ( voter->last_vote_weight > 0 ) {
         if( voter->proxy ) {
            auto old_proxy = _voters.find( voter->proxy.value );
            check( old_proxy != _voters.end(), "old proxy not found" ); //data corruption
            _voters.modify( old_proxy, same_payer, [&]( auto& vp ) {
                  vp.proxied_vote_weight -= voter->last_vote_weight;
               });
            propagate_weight_change( *old_proxy );
         } else {
            for( const auto& p : voter->producers ) {
               auto& d = producer_deltas[p];
               d.first -= voter->last_vote_weight;
               d.second = false;
            }
         }
      }

      if( proxy ) {
         auto new_proxy = _voters.find( proxy.value );
         check( new_proxy != _voters.end(), "invalid proxy specified" ); //if ( !voting ) { data corruption } else { wrong vote }
         check( !voting || new_proxy->is_proxy, "proxy not found" );
         if ( new_vote_weight >= 0 ) {
            _voters.modify( new_proxy, same_payer, [&]( auto& vp ) {
                  vp.proxied_vote_weight += new_vote_weight;
               });
            propagate_weight_change( *new_proxy );
         }
      } else {
         if( new_vote_weight >= 0 ) {
            for( const auto& p : producers ) {
               auto& d = producer_deltas[p];
               d.first += new_vote_weight;
               d.second = true;
            }
         }
      }

      for( const auto& pd : producer_deltas ) {
         auto pitr = _producers.find( pd.first.value );
         if( pitr != _producers.end() ) {
            if( voting && !pitr->active() && pd.second.second /* from new set */ ) {
               check( false, ( "producer " + pitr->owner.to_string() + " is not currently registered" ).data() );
            }
            _producers.modify( pitr, same_payer, [&]( auto& p ) {
               p.total_votes += pd.second.first;
               if ( p.total_votes < 0 ) { // floating point arithmetics can give small negative numbers
                  p.total_votes = 0;
               }
               _gstate.total_producer_vote_weight += pd.second.first;
            });
            const auto active_prod = std::find_if(std::begin(_gstate.last_schedule), std::end(_gstate.last_schedule),
               [target = pd.first](const auto& prod){ return prod.first.value == target.value; });
            if (active_prod != std::end(_gstate.last_schedule)) {
               _gstate.total_active_producer_vote_weight += pd.second.first;
               active_prod->second = pitr->total_votes / _gstate.total_active_producer_vote_weight;
            }
         } else {
            if( pd.second.second ) {
               check( false, ( "producer " + pd.first.to_string() + " is not registered" ).data() );
            }
         }
      }

      _voters.modify( voter, same_payer, [&]( auto& av ) {
         av.last_vote_weight = new_vote_weight;
         av.producers = producers;
         av.proxy     = proxy;
         av.last_reassertion_time = current_time_point();
      });
   }

   void system_contract::regproxy( const name& proxy, bool isproxy ) {
      require_auth( proxy );

      auto pitr = _voters.find( proxy.value );
      if ( pitr != _voters.end() ) {
         check( isproxy != pitr->is_proxy, "action has no effect" );
         check( !isproxy || !pitr->proxy, "account that uses a proxy is not allowed to become a proxy" );
         _voters.modify( pitr, same_payer, [&]( auto& p ) {
               p.is_proxy = isproxy;
            });
         propagate_weight_change( *pitr );
      } else {
         _voters.emplace( proxy, [&]( auto& p ) {
               p.owner  = proxy;
               p.is_proxy = isproxy;
            });
      }
   }

   void system_contract::propagate_weight_change( const voter_info& voter ) {
      check( !voter.proxy || !voter.is_proxy, "account registered as a proxy is not allowed to use a proxy" );
      double new_weight = stake2vote( voter.staked, voter.vote_mature_time );
      if ( voter.is_proxy ) {
         new_weight += voter.proxied_vote_weight;
      }

      /// don't propagate small changes (1 ~= epsilon)
      if ( fabs( new_weight - voter.last_vote_weight ) > 1 )  {
         if ( voter.proxy ) {
            auto& proxy = _voters.get( voter.proxy.value, "proxy not found" ); //data corruption
            _voters.modify( proxy, same_payer, [&]( auto& p ) {
                  p.proxied_vote_weight += new_weight - voter.last_vote_weight;
               }
            );
            propagate_weight_change( proxy );
         } else {
            auto delta = new_weight - voter.last_vote_weight;
            for ( auto acnt : voter.producers ) {
               auto& prod = _producers.get( acnt.value, "producer not found" ); //data corruption
               const double init_total_votes = prod.total_votes;
               _producers.modify( prod, same_payer, [&]( auto& p ) {
                  p.total_votes += delta;
                  _gstate.total_producer_vote_weight += delta;
               });
               const auto active_prod = std::find_if(std::begin(_gstate.last_schedule), std::end(_gstate.last_schedule),
                  [&](const auto& prod){ return prod.first.value == acnt.value; });
               if (active_prod != std::end(_gstate.last_schedule)) {
                  _gstate.total_active_producer_vote_weight += delta;
                  active_prod->second = prod.total_votes / _gstate.total_active_producer_vote_weight;
               }
            }
         }
      }

      _voters.modify( voter, same_payer, [&]( auto& v ) {
            v.last_vote_weight = new_weight;
         }
      );
   }

   bool system_contract::does_satisfy_stake_requirement( const name& producer ) const {
      const auto voter = _voters.find( producer.value );

      if ( voter == _voters.end() ) {
         return false;
      }

      return voter->staked >= producer_stake_threshold;
   }

   bool system_contract::is_block_producer( const name& producer ) const {
      return _producers.find( producer.value ) != _producers.end();
   }
   
   bool voter_info::vote_is_reasserted() const {
         return (current_time_point() - last_reassertion_time) < voter_info::reassertion_period;
   }

} /// namespace eosiosystem
