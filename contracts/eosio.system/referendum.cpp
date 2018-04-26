#include <eosiolib/contract.hpp>
#include <eosiolib/dispatcher.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {
   using eosio::action_meta;
   using eosio::singleton;
   using eosio::key256;
   using std::string;
   using std::vector;

   constexpr uint64_t proposal_fee = 1000000;
   constexpr time delay_period = 30*24*3600;

   struct proposal {
      uint64_t     id;
      account_name proposer;
      account_name contract;
      bytes        contract_code;
      uint128_t    yes_votes = 0;
      uint128_t    no_votes = 0;
      time         created;
      time         above_threshold_since = 0;

      uint64_t primary_key() const { return id; }
      uint64_t above_since() const { return above_threshold_since; }
   };

   typedef eosio::multi_index<N(props), proposal,
                              indexed_by<N(posdate), const_mem_fun<proposal, uint64_t, &proposal::above_since>  >
                              >  proposals_table;

   struct vote_record {
      account_name voter;
      eosio::asset stake_voted;
      int8_t      value;
      uint64_t primary_key() const { return voter; }
   };

   typedef eosio::multi_index< N(votes), vote_record> votes_table;

   void system_contract::propose( const account_name proposer,
                                  const account_name contract,
                                  const bytes&       contract_code )
   {
      require_auth( proposer );
      proposals_table proposals( _self, _self );
      auto it = proposals.find( proposer );
      eosio_assert( it == proposals.end(), "proposal already exists" );
      auto new_id = proposals.available_primary_key();
      proposals.emplace( proposer, [&]( proposal& p ) {
            p.id            = new_id;
            p.proposer      = proposer;
            p.contract      = contract;
            p.contract_code = contract_code;
            p.created       = now();
         }
      );

      eosio::asset fee( proposal_fee, system_token_symbol );
      INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {proposer, N(active)},
                                                    { proposer, N(eosio), fee, std::string("system contract upgrade proposal fee") } );
      eosio::print("created propsal ID: ", new_id, "\n");
   }

   void system_contract::cancel( const uint64_t proposal_id ) {
      proposals_table proposals( _self, _self );
      auto it = proposals.find( proposal_id );
      eosio_assert( it != proposals.end(), "proposal doesn't exist" );
      require_auth( it->proposer );
      proposals.erase( it );
   }

   void system_contract::voteprop( const account_name voter, const uint64_t proposal_id, int8_t value ) {
      require_auth( voter );
      eosio_assert( value == 1 || value == -1, "value should be 1 (vote yes) or -1 (vote no)" );
      proposals_table proposals( _self, _self );
      auto prop_it = proposals.find( proposal_id );
      eosio_assert( prop_it != proposals.end(), "proposal doesn't exist" );
      votes_table votes( _self, proposal_id );
      auto vote_it = votes.find( voter );
      int8_t prev_value = 0;
      asset prev_stake_voted(0, system_token_symbol);
      asset current_stake = get_voting_stake( voter );
      if ( vote_it != votes.end() ) {
         prev_value = vote_it->value;
         prev_stake_voted = vote_it->stake_voted;
         votes.modify( vote_it, 0, [&]( vote_record& v ) {
               v.value = value;
               v.stake_voted = current_stake;
            }
         );
      } else {
         votes.emplace( voter, [&]( vote_record& v ) {
               v.voter = voter;
               v.value = value;
               v.stake_voted = current_stake;
            }
         );
      }
      const eosio::asset token_supply = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name());
      proposals.modify( prop_it, 0, [&]( proposal& p) {
            if ( prev_value == 1) {
               p.yes_votes -= prev_stake_voted.amount;
            } else if ( prev_value == -1 ) {
               p.no_votes  -= prev_stake_voted.amount;
            }//don't do anythin if prev_value == 0

            if ( vote_it->value == 1 ) {
               p.yes_votes += current_stake.amount;
            } else {
               p.no_votes  += current_stake.amount;
            }
            auto turnout = p.yes_votes + p.no_votes;
            if ( token_supply.amount < turnout*10 && 45*p.yes_votes < 55*p.no_votes ) {
               if ( p.above_threshold_since == 0) {
                  p.above_threshold_since = now();
               } // else keep previous value
            } else {
               p.above_threshold_since = 0;
            }
         }
      );
   }

   void system_contract::unvoteprop( const account_name voter, const uint64_t proposal_id ) {
      require_auth( voter );
      proposals_table proposals( _self, _self );
      votes_table votes( _self, proposal_id );
      auto vote_it = votes.find( voter );
      eosio_assert( vote_it != votes.end(), "voter have not voted for this proposal" );
      auto prop_it = proposals.find( proposal_id );
      if ( prop_it != proposals.end() ) { //XXX is finished
         proposals.modify( prop_it, 0, [&]( proposal& p) {
               if ( vote_it->value == 1 ) {
                  p.yes_votes -= vote_it->stake_voted.amount;
               } else {
                  p.no_votes  -= vote_it->stake_voted.amount;
               }
            }
         );
      }
      votes.erase( vote_it );
   }

   void system_contract::upgrade( ) {
      proposals_table proposals( _self, _self );
      auto idx = proposals.template get_index<N(posdate)>();
      auto it = idx.upper_bound( 0 );
      auto cur_time = now();
      if ( it != idx.end() && it->above_threshold_since + delay_period <= cur_time ) {
         INLINE_ACTION_SENDER( system_contract, setcode )( N(eosio), { N(eosio), N(active) },
            { it->contract, 0, 0, it->contract_code } );

         eosio::asset fee( proposal_fee, system_token_symbol );
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio), N(active)},
            { N(eosio), it->proposer, fee, std::string("system contract upgrade proposal fee refund") } );
         proposals.erase( *it );
      }
   }

} /// namespace eosiosystem
