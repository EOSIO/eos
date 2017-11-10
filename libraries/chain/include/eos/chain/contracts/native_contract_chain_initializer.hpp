/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/contract/genesis_state.hpp>

#include <eos/chain/chain_controller.hpp>

namespace eosio { namespace native_contract {

class native_contract_chain_initializer : public chain::chain_initializer_interface {
   genesis_state_type genesis;
public:
   native_contract_chain_initializer(const genesis_state_type& genesis) : genesis(genesis) {}
   virtual ~native_contract_chain_initializer() {}

   virtual types::Time get_chain_start_time() override;
   virtual chain::blockchain_configuration get_chain_start_configuration() override;
   virtual std::array<types::account_name, config::BlocksPerRound> get_chain_start_producers() override;

   virtual void register_types(chain::chain_controller& chain, chainbase::database& db) override;
   virtual std::vector<chain::Message> prepare_database(chain::chain_controller& chain,
                                                        chainbase::database& db) override;

   static types::abi eos_contract_abi();
};

} } // namespace eosio::chain::contracts

