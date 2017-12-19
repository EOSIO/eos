/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/blockchain_configuration.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/immutable_chain_parameters.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace eosio { namespace native_contract {
using std::string;
using std::vector;
using chain::public_key;
using chain::asset;
using chain::time;
using chain::blockchain_configuration;

struct genesis_state_type {
   struct initial_account_type {
      initial_account_type(const string& name = string(),
                           uint64_t staking_bal = 0,
                           uint64_t liquid_bal = 0,
                           const public_key& owner_key = public_key(),
                           const public_key& active_key = public_key())
         : name(name), staking_balance(staking_bal), liquid_balance(liquid_bal),
           owner_key(owner_key),
           active_key(active_key == public_key()? owner_key : active_key)
      {}
      string          name;
      asset           staking_balance;
      asset           liquid_balance;
      public_key owner_key;
      public_key active_key;
   };
   struct initial_producer_type {
      initial_producer_type(const string& name = string(),
                            const public_key& signing_key = public_key())
         : owner_name(name), block_signing_key(signing_key)
      {}
      /// Must correspond to one of the initial accounts
      string owner_name;
      public_key block_signing_key;
   };

   time                                     initial_timestamp;
   blockchain_configuration                  initial_configuration = {
      config::default_max_block_size,
      config::default_target_block_size,
      config::default_max_storage_size,
      config::default_elected_pay,
      config::default_runner_up_pay,
      config::default_min_eos_balance,
      config::default_max_trx_lifetime,
      config::default_auth_depth_limit,
      config::default_max_trx_runtime,
      config::default_inline_depth_limit,
      config::default_max_inline_msg_size,
      config::default_max_gen_trx_size
   };
   vector<initial_account_type>             initial_accounts;
   vector<initial_producer_type>            initial_producers;

   /**
    * Temporary, will be moved elsewhere.
    */
   chain::chain_id_type initial_chain_id;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain::chain_id_type compute_chain_id() const;
};

} } // namespace eosio::native_contract

FC_REFLECT(eosio::native_contract::genesis_state_type::initial_account_type,
           (name)(staking_balance)(liquid_balance)(owner_key)(active_key))

FC_REFLECT(eosio::native_contract::genesis_state_type::initial_producer_type, (owner_name)(block_signing_key))

FC_REFLECT(eosio::native_contract::genesis_state_type,
           (initial_timestamp)(initial_configuration)(initial_accounts)
           (initial_producers)(initial_chain_id))
