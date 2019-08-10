/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.system/rem.system.hpp>
#include <rem.token/rem.token.hpp>

namespace eosiosystem {

   const int64_t  min_pervote_daily_pay = 100'0000;
   const int64_t  min_activated_stake   = 150'000'000'0000;
   const uint32_t blocks_per_year       = 52*7*24*2*3600;   // half seconds per year
   const uint32_t seconds_per_year      = 52*7*24*3600;
   const uint32_t blocks_per_day        = 2 * 24 * 3600;
   const uint32_t blocks_per_hour       = 2 * 3600;
   const int64_t  useconds_per_day      = 24 * 3600 * int64_t(1000000);
   const int64_t  useconds_per_year     = seconds_per_year*1000000ll;

   const static int producer_count = 21;
   const static int producer_repetitions = 12;
   const static int blocks_per_round = producer_count * producer_repetitions;


   int64_t system_contract::share_pervote_reward_between_producers(int64_t amount)
   {
      int64_t amount_remained = amount;
      int64_t total_reward_distributed = 0;
      for (const auto& p: _gstate.last_schedule) {
         const auto reward = std::min(int64_t(amount * p.second), amount_remained);
         amount_remained -= reward;
         total_reward_distributed += reward;
         const auto& prod = _producers.get(p.first.value);
         _producers.modify(prod, eosio::same_payer, [&](auto& p) {
            p.pending_pervote_reward += reward;
         });
         if (amount_remained == 0) {
            break;
         }
      }
      check(total_reward_distributed <= amount, "distributed reward above the given amount");
      return total_reward_distributed;
   }

   void system_contract::onblock( ignore<block_header> ) {
      using namespace eosio;

      require_auth(_self);

      block_timestamp timestamp;
      name producer;
      uint16_t confirmed;
      checksum256 previous;
      checksum256 transaction_mroot;
      checksum256 action_mroot;
      uint32_t schedule_version = 0;
      _ds >> timestamp >> producer >> confirmed >> previous >> transaction_mroot >> action_mroot >> schedule_version;

      // _gstate2.last_block_num is not used anywhere in the system contract code anymore.
      // Although this field is deprecated, we will continue updating it for now until the last_block_num field
      // is eventually completely removed, at which point this line can be removed.
      _gstate2.last_block_num = timestamp;

      /** until activated stake crosses this threshold no new rewards are paid */
      if( _gstate.total_activated_stake < min_activated_stake )
         return;

      //end of round: count all unpaid blocks produced within this round
      if (timestamp.slot >= _gstate.current_round_start_time.slot + blocks_per_round) {
         const auto rounds_passed = (timestamp.slot - _gstate.current_round_start_time.slot) / blocks_per_round;
         _gstate.current_round_start_time = block_timestamp(_gstate.current_round_start_time.slot + (rounds_passed * blocks_per_round));
         for (const auto p: _gstate.last_schedule) {
            const auto& prod = _producers.get(p.first.value);
            _producers.modify(prod, same_payer, [&](auto& p) {
               p.unpaid_blocks += p.current_round_unpaid_blocks;
               p.current_round_unpaid_blocks = 0;
            });
         }
      }

      if (schedule_version > _gstate.last_schedule_version) {
         for (size_t producer_index = 0; producer_index < _gstate.last_schedule.size(); producer_index++) {
            const auto producer_name = _gstate.last_schedule[producer_index].first;
            const auto& prod = _producers.get(producer_name.value);
            //blocks from full rounds
            const auto full_rounds_passed = (timestamp.slot - prod.last_expected_produced_blocks_update.slot) / blocks_per_round;
            uint32_t expected_produced_blocks = full_rounds_passed * producer_repetitions;
            if ((timestamp.slot - prod.last_expected_produced_blocks_update.slot) % blocks_per_round != 0) {
               //if last round is incomplete, calculate number of blocks produced in this round by prod
               const auto current_round_start_position = _gstate.current_round_start_time.slot % blocks_per_round;
               const auto producer_first_block_position = producer_repetitions * producer_index;
               const uint32_t current_round_blocks_before_producer_start_producing = current_round_start_position <= producer_first_block_position ?
                                                                                     producer_first_block_position - current_round_start_position :
                                                                                     blocks_per_round - (current_round_start_position - producer_first_block_position);

               const auto total_current_round_blocks = timestamp.slot - _gstate.current_round_start_time.slot;
               if (current_round_blocks_before_producer_start_producing < total_current_round_blocks) {
                  expected_produced_blocks += std::min(total_current_round_blocks - current_round_blocks_before_producer_start_producing, uint32_t(producer_repetitions));
               } else if (blocks_per_round - current_round_blocks_before_producer_start_producing < producer_repetitions) {
                  expected_produced_blocks += std::min(producer_repetitions - (blocks_per_round - current_round_blocks_before_producer_start_producing), total_current_round_blocks);
               }
            }
            _producers.modify(prod, same_payer, [&](auto& p) {
               p.expected_produced_blocks += expected_produced_blocks;
               p.last_expected_produced_blocks_update = timestamp;
               p.unpaid_blocks += p.current_round_unpaid_blocks;
               p.current_round_unpaid_blocks = 0;
            });
         }

         _gstate.current_round_start_time = timestamp;
         _gstate.last_schedule_version = schedule_version;
         std::vector<name> active_producers = eosio::get_active_producers();
         _gstate.total_active_producer_vote_weight = 0.0;
         for (const auto prod_name: active_producers) {
            const auto& prod = _producers.get(prod_name.value);
            _gstate.total_active_producer_vote_weight += prod.total_votes;
         }

         if (active_producers.size() != _gstate.last_schedule.size()) {
            _gstate.last_schedule.resize(active_producers.size());
         }
         for (size_t i = 0; i < active_producers.size(); i++) {
            const auto& prod_name = active_producers[i];
            const auto& prod = _producers.get(prod_name.value);
            _producers.modify(prod, same_payer, [&](auto& p) {
               p.last_expected_produced_blocks_update = timestamp;
            });
            _gstate.last_schedule[i] = std::make_pair(prod_name, prod.total_votes / _gstate.total_active_producer_vote_weight);
         }
      }

      if( _gstate.last_pervote_bucket_fill == time_point() )  /// start the presses
         _gstate.last_pervote_bucket_fill = current_time_point();


      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find( producer.value );
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;

         const auto& voter = _voters.get( producer.value );
         // TODO fix coupling in voter-producer entities
         if ( voter.vote_is_reasserted() ) {
            _producers.modify( prod, same_payer, [&](auto& p ) {
                  p.current_round_unpaid_blocks++;
            });
         }
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_elected_producers( timestamp );

         if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
            name_bid_table bids(_self, _self.value);
            auto idx = bids.get_index<"highbid"_n>();
            auto highest = idx.lower_bound( std::numeric_limits<uint64_t>::max()/2 );
            if( highest != idx.end() &&
                highest->high_bid > 0 &&
                (current_time_point() - highest->last_bid_time) > microseconds(useconds_per_day) &&
                _gstate.thresh_activated_stake_time > time_point() &&
                (current_time_point() - _gstate.thresh_activated_stake_time) > microseconds(14 * useconds_per_day)
            ) {
               _gstate.last_name_close = timestamp;
               channel_namebid_to_rex( highest->high_bid );
               idx.modify( highest, same_payer, [&]( auto& b ){
                  b.high_bid = -b.high_bid;
               });
            }
         }
      }
   }

   using namespace eosio;
   void system_contract::claimrewards( const name& owner ) {
      require_auth( owner );

      const auto& prod = _producers.get( owner.value );
      check( prod.active(), "producer does not have an active key" );

      check( _gstate.total_activated_stake >= min_activated_stake,
                    "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );

      const auto ct = current_time_point();

      check( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

      const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
      const auto usecs_since_last_fill = (ct - _gstate.last_pervote_bucket_fill).count();

      if( usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > time_point() ) {
         auto new_tokens = static_cast<int64_t>( (_gstate4.continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) );

         auto to_producers     = new_tokens / _gstate4.inflation_pay_factor;
         auto to_savings       = new_tokens - to_producers;
         auto to_per_block_pay = to_producers / _gstate4.votepay_factor;
         auto to_per_vote_pay  = to_producers - to_per_block_pay;

         if( new_tokens > 0 ) {
            {
               token::issue_action issue_act{ token_account, { {_self, active_permission} } };
               issue_act.send( _self, asset(new_tokens, core_symbol()), "issue tokens for producer pay and savings" );
            }
            {
               token::transfer_action transfer_act{ token_account, { {_self, active_permission} } };
               if( to_savings > 0 ) {
                  transfer_act.send( _self, saving_account, asset(to_savings, core_symbol()), "unallocated inflation" );
               }
               if( to_per_block_pay > 0 ) {
                  transfer_act.send( _self, bpay_account, asset(to_per_block_pay, core_symbol()), "fund per-block bucket" );
               }
               if( to_per_vote_pay > 0 ) {
                  transfer_act.send( _self, vpay_account, asset(to_per_vote_pay, core_symbol()), "fund per-vote bucket" );
               }
            }
         }

         _gstate.pervote_bucket          += to_per_vote_pay;
         _gstate.perblock_bucket         += to_per_block_pay;
         _gstate.last_pervote_bucket_fill = ct;
      }

      if (_gstate.perstake_bucket > 1'0000) { //do not distribute small amounts
         int64_t total_producer_per_stake_pay = 0;
         for (auto it = _producers.begin(); it != _producers.end(); it++) {
            user_resources_table totals_tbl( _self, it->owner.value );
            const auto& tot = totals_tbl.get(it->owner.value, "producer must have resources");
            const auto producer_per_stake_pay = int64_t(
               _gstate.perstake_bucket * (double(tot.own_stake_amount) / double(_gstate.total_producer_stake)));
            _producers.modify(it, same_payer, [&](auto &p) {
               p.pending_perstake_reward += producer_per_stake_pay;
            });
            total_producer_per_stake_pay += producer_per_stake_pay;
         }
         _gstate.perstake_bucket -= total_producer_per_stake_pay;
         check(_gstate.perstake_bucket >= 0, "perstake_bucket cannot be negative");
      }
      const auto producer_per_stake_pay = prod.pending_perstake_reward;

      int64_t producer_per_vote_pay = prod.pending_pervote_reward;
      auto expected_produced_blocks = prod.expected_produced_blocks;
      if (std::find_if(std::begin(_gstate.last_schedule), std::end(_gstate.last_schedule),
            [&owner](const auto& prod){ return prod.first.value == owner.value; }) != std::end(_gstate.last_schedule)) {
         const auto full_rounds_passed = (_gstate.current_round_start_time.slot - prod.last_expected_produced_blocks_update.slot) / blocks_per_round;
         expected_produced_blocks += full_rounds_passed * producer_repetitions;
      }
      if (prod.unpaid_blocks != expected_produced_blocks && expected_produced_blocks > 0) {
         producer_per_vote_pay = (prod.pending_pervote_reward * prod.unpaid_blocks) / expected_produced_blocks;
      }
      const auto punishment = prod.pending_pervote_reward - producer_per_vote_pay;

      _gstate.pervote_bucket      -= producer_per_vote_pay;
      _gstate.total_unpaid_blocks -= prod.unpaid_blocks;

      _producers.modify( prod, same_payer, [&](auto& p) {
         p.last_claim_time                      = ct;
         p.last_expected_produced_blocks_update = _gstate.current_round_start_time;
         p.unpaid_blocks                        = 0;
         p.expected_produced_blocks             = 0;
         p.pending_perstake_reward              = 0;
         p.pending_pervote_reward               = 0;
      });

      if ( producer_per_stake_pay > 0 ) {
         token::transfer_action transfer_act{ token_account, { {spay_account, active_permission}, {owner, active_permission} } };
         transfer_act.send( spay_account, owner, asset(producer_per_stake_pay, core_symbol()), "producer stake pay" );
      }
      if ( producer_per_vote_pay > 0 ) {
         token::transfer_action transfer_act{ token_account, { {vpay_account, active_permission}, {owner, active_permission} } };
         transfer_act.send( vpay_account, owner, asset(producer_per_vote_pay, core_symbol()), "producer vote pay" );
      }
      if ( punishment > 0 ) {
         token::transfer_action transfer_act{ token_account, { {vpay_account, active_permission} } };
         transfer_act.send( vpay_account, saving_account, asset(punishment, core_symbol()), "punishment transfer" );
      }
   }

   void system_contract::torewards( const name& payer, const asset& amount ) {
      require_auth( payer );
      const auto to_per_stake_pay = amount.amount * 0.7; //TODO: move to constants section
      const auto to_per_vote_pay  = share_pervote_reward_between_producers(amount.amount * 0.2); //TODO: move to constants section
      const auto to_rem           = amount.amount - (to_per_stake_pay + to_per_vote_pay);
      if( amount.amount > 0 ) {
        token::transfer_action transfer_act{ token_account, { {_self, active_permission} } };
        if( to_rem > 0 ) {
           transfer_act.send( _self, saving_account, asset(to_rem, core_symbol()), "Remme Savings" );
        }
        if( to_per_stake_pay > 0 ) {
           transfer_act.send( _self, spay_account, asset(to_per_stake_pay, core_symbol()), "fund per-stake bucket" );
        }
        if( to_per_vote_pay > 0 ) {
           transfer_act.send( _self, vpay_account, asset(to_per_vote_pay, core_symbol()), "fund per-vote bucket" );
        }
      }

      _gstate.pervote_bucket          += to_per_vote_pay;
      _gstate.perstake_bucket         += to_per_stake_pay;
   }

} //namespace eosiosystem
