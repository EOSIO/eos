/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/BlockchainConfiguration.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/immutable_chain_parameters.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace eos { namespace native_contract {
using std::string;
using std::vector;
using chain::PublicKey;
using chain::Asset;
using chain::Time;
using chain::BlockchainConfiguration;

struct genesis_state_type {
   struct initial_account_type {
      initial_account_type(const string& name = string(),
                           uint64_t staking_bal = 0,
                           uint64_t liquid_bal = 0,
                           const PublicKey& owner_key = PublicKey(),
                           const PublicKey& active_key = PublicKey())
         : name(name), staking_balance(staking_bal), liquid_balance(liquid_bal),
           owner_key(owner_key),
           active_key(active_key == PublicKey()? owner_key : active_key)
      {}
      string          name;
      Asset           staking_balance;
      Asset           liquid_balance;
      PublicKey owner_key;
      PublicKey active_key;
   };
   struct initial_producer_type {
      initial_producer_type(const string& name = string(),
                            const PublicKey& signing_key = PublicKey())
         : owner_name(name), block_signing_key(signing_key)
      {}
      /// Must correspond to one of the initial accounts
      string owner_name;
      PublicKey block_signing_key;
   };

   Time                                     initial_timestamp;
   BlockchainConfiguration                  initial_configuration = {
      config::DefaultMaxBlockSize,
      config::DefaultTargetBlockSize,
      config::DefaultMaxStorageSize,
      config::DefaultElectedPay,
      config::DefaultRunnerUpPay,
      config::DefaultMinEosBalance,
      config::DefaultMaxTrxLifetime,
      config::DefaultAuthDepthLimit,
      config::DefaultMaxTrxRuntime,
      config::DefaultInlineDepthLimit,
      config::DefaultMaxInlineMsgSize,
      config::DefaultMaxGenTrxSize
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

} } // namespace eos::native_contract

FC_REFLECT(eos::native_contract::genesis_state_type::initial_account_type,
           (name)(staking_balance)(liquid_balance)(owner_key)(active_key))

FC_REFLECT(eos::native_contract::genesis_state_type::initial_producer_type, (owner_name)(block_signing_key))

FC_REFLECT(eos::native_contract::genesis_state_type,
           (initial_timestamp)(initial_configuration)(initial_accounts)
           (initial_producers)(initial_chain_id))
