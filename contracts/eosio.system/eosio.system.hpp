/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio.system/native.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosio.system/exchange_state.hpp>

#include <string>

namespace eosiosystem {

   using eosio::asset;
   using eosio::indexed_by;
   using eosio::const_mem_fun;

   struct eosio_parameters : eosio::blockchain_parameters {
      uint64_t          max_storage_size = 64ll*1024 * 1024 * 1024;
      uint32_t          percent_of_max_inflation_rate = 0;
      uint32_t          storage_reserve_ratio = 100;      // ratio * 10000

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE_DERIVED( eosio_parameters, eosio::blockchain_parameters, (max_storage_size)(percent_of_max_inflation_rate)(storage_reserve_ratio) )
   };

   struct eosio_global_state : eosio_parameters {
      uint64_t free_ram()const { return max_storage_size - total_storage_bytes_reserved; }

      uint64_t             total_storage_bytes_reserved = 0;
      eosio::asset         total_storage_stake;
      eosio::asset         payment_per_block;
      eosio::asset         payment_to_eos_bucket;

      time                 first_block_time_in_cycle = 0;
      uint32_t             blocks_per_cycle = 0;
      time                 last_bucket_fill_time = 0;

      eosio::asset         eos_bucket;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE_DERIVED( eosio_global_state, eosio_parameters, (total_storage_bytes_reserved)(total_storage_stake)
                                (payment_per_block)(payment_to_eos_bucket)(first_block_time_in_cycle)(blocks_per_cycle)
                                (last_bucket_fill_time)(eos_bucket) )
   };

   struct producer_info {
      account_name          owner;
      double                total_votes = 0;
      eosio::public_key     producer_key; /// a packed public key object
      eosio::asset          per_block_payments;
      time                  last_rewards_claim = 0;
      time                  time_became_active = 0;
      time                  last_produced_block_time = 0;

      uint64_t    primary_key()const { return owner;                        }
      double      by_votes()const    { return -total_votes;                 }
      bool        active() const     { return producer_key != public_key(); }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(producer_key)
                        (per_block_payments)(last_rewards_claim)
                        (time_became_active)(last_produced_block_time) )
   };

   struct voter_info {
      account_name                owner = 0; /// the voter
      account_name                proxy = 0; /// the proxy set by the voter, if any
      std::vector<account_name>   producers; /// the producers approved by this voter if no proxy set
      uint64_t                    staked = 0;

      /**
       *  Every time a vote is cast we must first "undo" the last vote weight, before casting the
       *  new vote weight.  Vote weight is calculated as:
       *
       *  stated.amount * 2 ^ ( weeks_since_launch/weeks_per_year)
       */
      double                      last_vote_weight = 0; /// the vote weight cast the last time the vote was updated

      /**
       * Total vote weight delegated to this voter.
       */
      double                      proxied_vote_weight= 0; /// the total vote weight delegated to this voter as a proxy
      bool                        is_proxy = 0; /// whether the voter is a proxy for others


      uint32_t                    deferred_trx_id = 0; /// the ID of the 3-day delay deferred transaction
      time                        last_unstake_time = 0; /// the time when the deferred_trx_id was sent
      eosio::asset                unstaking; /// the total unstaking (pending 3 day delay)

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( voter_info, (owner)(proxy)(producers)(staked)(last_vote_weight)(proxied_vote_weight)(is_proxy)(deferred_trx_id)(last_unstake_time)(unstaking) )
   };

   typedef eosio::multi_index< N(voters), voter_info>  voters_table;


   typedef eosio::multi_index< N(producers), producer_info,
                               indexed_by<N(prototalvote), const_mem_fun<producer_info, double, &producer_info::by_votes>  >
                               >  producers_table;

   typedef eosio::singleton<N(global), eosio_global_state> global_state_singleton;

   static constexpr uint32_t     max_inflation_rate = 5;  // 5% annual inflation
   static constexpr uint32_t     seconds_per_day = 24 * 3600;
   static constexpr uint64_t     system_token_symbol = S(4,EOS);

   class system_contract : public native {
      private:
         voters_table           _voters;
         producers_table        _producers;
         global_state_singleton _global;

         eosio_global_state     _gstate;
         rammarket              _rammarket;

      public:
         system_contract( account_name s );
         [[noreturn]] ~system_contract();

         // Actions:
         void onblock( uint32_t timestamp_slot, account_name producer );
                      // const block_header& header ); /// only parse first 3 fields of block header

         // functions defined in delegate_bandwidth.cpp
         void delegatebw( account_name from, account_name receiver,
                          asset stake_net_quantity, asset stake_cpu_quantity );


         /**
          *  Decreases the total tokens delegated by from to receiver and/or
          *  frees the memory associated with the delegation if there is nothing
          *  left to delegate.
          *
          *  This will cause an immediate reduction in net/cpu bandwidth of the
          *  receiver. 
          *
          *  A transaction is scheduled to send the tokens back to 'from' after
          *  the staking period has passed. If existing transaction is scheduled, it
          *  will be canceled and a new transaction issued that has the combined
          *  undelegated amount.
          *
          *  The 'from' account loses voting power as a result of this call and
          *  all producer tallies are updated.
          */
         void undelegatebw( account_name from, account_name receiver,
                            asset unstake_net_quantity, asset unstake_cpu_quantity );


         /**
          * Increases receiver's ram quota based upon current price and quantity of
          * tokens provided. An inline transfer from receiver to system contract of
          * tokens will be executed.
          */
         void buyram( account_name buyer, account_name receiver, asset tokens );
         void buyrambytes( account_name buyer, account_name receiver, uint32_t bytes );

         /**
          *  Reduces quota my bytes and then performs an inline transfer of tokens
          *  to receiver based upon the average purchase price of the original quota.
          */
         void sellram( account_name receiver, uint32_t bytes );

         /**
          *  This action is called after the delegation-period to claim all pending
          *  unstaked tokens belonging to owner
          */
         void refund( account_name owner );

         // functions defined in voting.cpp

         void regproducer( const account_name producer, const public_key& producer_key, const std::string& url );

         void unregprod( const account_name producer );

         void setparams( uint64_t max_storage_size, uint32_t storage_reserve_ratio );

         void voteproducer( const account_name voter, const account_name proxy, const std::vector<account_name>& producers );

         void regproxy( const account_name proxy, bool isproxy );

         void nonce( const std::string& /*value*/ ) {}

         // functions defined in producer_pay.cpp
         void claimrewards( const account_name& owner );

      private:
         eosio::asset payment_per_block(uint32_t percent_of_max_inflation_rate);

         void update_elected_producers(time cycle_time);

         // Implementation details:

         //defined in voting.hpp
         static eosio_global_state get_default_parameters();

         // defined in voting.cpp
         void adjust_voting_power( account_name acnt, int64_t delta );
   };

} /// eosiosystem
