/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/immutable_chain_parameters.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace eosio { namespace chain { namespace contracts {

struct genesis_state_type {
   struct initial_account_type {
      initial_account_type(const string& name = string(),
                           uint64_t staking_bal = 0,
                           uint64_t liquid_bal = 0,
                           const public_key_type& owner_key = public_key_type(),
                           const public_key_type& active_key = public_key_type())
         : name(name), staking_balance(staking_bal), liquid_balance(liquid_bal),
           owner_key(owner_key),
           active_key(active_key == public_key_type()? owner_key : active_key)
      {}
      string          name;
      asset           staking_balance;
      asset           liquid_balance;
      public_key_type owner_key;
      public_key_type active_key;
   };
   struct initial_producer_type {
      initial_producer_type(const string& name = string(),
                            const public_key_type& signing_key = public_key_type())
         : owner_name(name), block_signing_key(signing_key)
      {}
      /// Must correspond to one of the initial accounts
      string              owner_name;
      public_key_type     block_signing_key;
   };

   chain_config   initial_configuration = {
      .producer_pay                   = config::default_elected_pay,
      .target_block_size              = config::default_target_block_size,
      .max_block_size                 = config::default_max_block_size,
      .target_block_acts_per_scope    = config::default_target_block_acts_per_scope,
      .max_block_acts_per_scope       = config::default_max_block_acts_per_scope,
      .target_block_acts              = config::default_target_block_acts,
      .max_block_acts                 = config::default_max_block_acts,
      .real_threads                   = 0, // TODO: unused?
      .max_storage_size               = config::default_max_storage_size,
      .max_transaction_lifetime       = config::default_max_trx_lifetime,
      .max_authority_depth            = config::default_max_auth_depth,
      .max_transaction_exec_time      = 0, // TODO: unused?
      .max_inline_depth               = config::default_max_inline_depth,
      .max_inline_action_size         = config::default_max_inline_action_size,
      .max_generated_transaction_size = config::default_max_gen_trx_size
   };

   time_point                               initial_timestamp;
   vector<initial_account_type>             initial_accounts;
   vector<initial_producer_type>            initial_producers;

   /**
    * Temporary, will be moved elsewhere.
    */
   chain_id_type initial_chain_id;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;
};

} } } // namespace eosio::contracts

FC_REFLECT(eosio::chain::contracts::genesis_state_type::initial_account_type,
           (name)(staking_balance)(liquid_balance)(owner_key)(active_key))

FC_REFLECT(eosio::chain::contracts::genesis_state_type::initial_producer_type, (owner_name)(block_signing_key))

FC_REFLECT(eosio::chain::contracts::genesis_state_type,
           (initial_timestamp)(initial_configuration)(initial_accounts)
           (initial_producers)(initial_chain_id))
