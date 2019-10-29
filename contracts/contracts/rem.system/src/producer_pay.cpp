#include <rem.system/rem.system.hpp>
#include <rem.token/rem.token.hpp>

#include <cmath>

namespace eosiosystem {

   const static int producer_repetitions = 12;
   const static int blocks_per_round = system_contract::max_block_producers * producer_repetitions;

   using eosio::current_time_point;
   using eosio::microseconds;
   using eosio::token;

   int64_t system_contract::share_pervote_reward_between_producers(int64_t amount)
   {
      int64_t total_reward_distributed = 0;
      for (const auto& p: _gstate.last_schedule) {
         const auto reward = int64_t(amount * p.second);
         total_reward_distributed += reward;
         const auto& prod = _producers.get(p.first.value);
         _producers.modify(prod, eosio::same_payer, [&](auto& p) {
            p.pending_pervote_reward += reward;
         });
      }
      check(total_reward_distributed <= amount, "distributed reward above the given amount");
      return total_reward_distributed;
   }

   void system_contract::update_pervote_shares()
   {
      std::for_each(std::begin(_gstate.last_schedule), std::end(_gstate.last_schedule),
         [this](auto& p)
         {
            const auto& prod_name = p.first;
            const auto& prod = _producers.get(prod_name.value);
            const double share = prod.total_votes / _gstate.total_active_producer_vote_weight;
            // need to cut precision because sum of all shares can be greater that 1 due to floating point arithmetics
            p.second = std::floor(share * 100000.0) / 100000.0;
         });
   }

   int64_t system_contract::share_perstake_reward_between_guardians(int64_t amount)
   {
      using namespace eosio;
      int64_t total_reward_distributed = 0;

      const auto sorted_voters = _voters.get_index<"bystake"_n>();
      _gstate.total_guardians_stake = 0;
      for (auto it = sorted_voters.rbegin(); it != sorted_voters.rend() && it->staked >= _gremstate.guardian_stake_threshold; it++) {
         if ( vote_is_reasserted( it->last_reassertion_time ) ) {
            _gstate.total_guardians_stake += it->staked;
         }
      }

      for (auto it = sorted_voters.rbegin(); it != sorted_voters.rend() && it->staked >= _gremstate.guardian_stake_threshold; it++) {
         if ( vote_is_reasserted( it->last_reassertion_time ) ) {
            const int64_t pending_perstake_reward = amount * ( double(it->staked) / double(_gstate.total_guardians_stake) );

            _voters.modify( *it, same_payer, [&](auto &v) {
               v.pending_perstake_reward += pending_perstake_reward;
            });

            total_reward_distributed += pending_perstake_reward;
         }
      }

      check(total_reward_distributed <= amount, "distributed reward above the given amount");
      return total_reward_distributed;
   }

   void system_contract::onblock( ignore<block_header> ) {
      using namespace eosio;

      require_auth(get_self());

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
         if (active_producers.size() != _gstate.last_schedule.size()) {
            _gstate.last_schedule.resize(active_producers.size());
         }
         _gstate.total_active_producer_vote_weight = 0.0;
         for (size_t i = 0; i < active_producers.size(); i++) {
            const auto& prod_name = active_producers[i];
            const auto& prod = _producers.get(prod_name.value);
            _gstate.last_schedule[i] = std::make_pair(prod_name, 0.0);
            _gstate.total_active_producer_vote_weight += prod.total_votes;
            _producers.modify(prod, same_payer, [&](auto& p) {
               p.last_expected_produced_blocks_update = timestamp;
            });
         }
         update_pervote_shares();
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
         if ( vote_is_reasserted( voter.last_reassertion_time ) ) {
            _producers.modify( prod, same_payer, [&](auto& p ) {
                  p.current_round_unpaid_blocks++;
            });
         }
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_elected_producers( timestamp );

         if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
            name_bid_table bids(get_self(), get_self().value);
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

   void system_contract::claim_perstake( const name& guardian )
   {
      const auto& voter = _voters.get( guardian.value );

      const auto ct = current_time_point();
      check( ct - voter.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

      _gstate.perstake_bucket -= voter.pending_perstake_reward;

      if ( voter.pending_perstake_reward > 0 ) {
         token::transfer_action transfer_act{ token_account, { {spay_account, active_permission}, {guardian, active_permission} } };
         transfer_act.send( spay_account, guardian, asset(voter.pending_perstake_reward, core_symbol()), "guardian stake pay" );
      }

      _voters.modify( voter, same_payer, [&](auto& v) {
         v.last_claim_time         = ct;
         v.pending_perstake_reward = 0;
      });

   }

   void system_contract::claim_pervote( const name& producer )
   {
      const auto& prod = _producers.get( producer.value );
      check( prod.active(), "producer does not have an active key" );

      const auto ct = current_time_point();
      check( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

      int64_t producer_per_vote_pay = prod.pending_pervote_reward;
      auto expected_produced_blocks = prod.expected_produced_blocks;
      if (std::find_if(std::begin(_gstate.last_schedule), std::end(_gstate.last_schedule),
            [&producer](const auto& prod){ return prod.first.value == producer.value; }) != std::end(_gstate.last_schedule)) {
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
         p.pending_pervote_reward               = 0;
      });

      if ( producer_per_vote_pay > 0 ) {
         token::transfer_action transfer_act{ token_account, { {vpay_account, active_permission}, {producer, active_permission} } };
         transfer_act.send( vpay_account, producer, asset(producer_per_vote_pay, core_symbol()), "producer vote pay" );
      }
      if ( punishment > 0 ) {
         token::transfer_action transfer_act{ token_account, { {vpay_account, active_permission} } };
         transfer_act.send( vpay_account, saving_account, asset(punishment, core_symbol()), "punishment transfer" );
      }
   }

   void system_contract::claimrewards( const name& owner ) {
      require_auth( owner );
      check( _gstate.total_activated_stake >= min_activated_stake, "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );

      auto voter = _voters.find( owner.value );
      if( voter != _voters.end() ) {
         claim_perstake( owner );
      }

      auto prod = _producers.find( owner.value );
      if( prod != _producers.end() ) {
         claim_pervote( owner );
      }
   }

   void system_contract::torewards( const name& payer, const asset& amount ) {
      require_auth( payer );
      check( amount.is_valid(), "invalid amount" );
      check( amount.symbol == core_symbol(), "invalid symbol" );
      check( amount.amount > 0, "amount must be positive" );

      const auto to_per_stake_pay = share_perstake_reward_between_guardians( amount.amount * _gremstate.per_stake_share );
      const auto to_per_vote_pay  = share_pervote_reward_between_producers( amount.amount * _gremstate.per_vote_share );
      const auto to_rem           = amount.amount - (to_per_stake_pay + to_per_vote_pay);
      if( amount.amount > 0 ) {
        token::transfer_action transfer_act{ token_account, { {payer, active_permission} } };
        if( to_rem > 0 ) {
           transfer_act.send( payer, saving_account, asset(to_rem, amount.symbol), "Remme Savings" );
        }
        if( to_per_stake_pay > 0 ) {
           transfer_act.send( payer, spay_account, asset(to_per_stake_pay, amount.symbol), "fund per-stake bucket" );
        }
        if( to_per_vote_pay > 0 ) {
           transfer_act.send( payer, vpay_account, asset(to_per_vote_pay, amount.symbol), "fund per-vote bucket" );
        }
      }

      _gstate.pervote_bucket          += to_per_vote_pay;
      _gstate.perstake_bucket         += to_per_stake_pay;
   }

} //namespace eosiosystem
