/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/native_contract/genesis_state.hpp>

#include <eos/chain/chain_controller.hpp>

namespace eosio { namespace native_contract {

class native_contract_chain_initializer : public chain::chain_initializer_interface {
   genesis_state_type genesis;
public:
   native_contract_chain_initializer(const genesis_state_type& genesis) : genesis(genesis) {}
   virtual ~native_contract_chain_initializer() {}

   virtual types::time get_chain_start_time() override;
   virtual chain::blockchain_configuration get_chain_start_configuration() override;
   virtual std::array<types::account_name, config::blocks_per_round> get_chain_start_producers() override;

   virtual void register_types(chain::chain_controller& chain, chainbase::database& db) override;
   virtual std::vector<chain::message> prepare_database(chain::chain_controller& chain,
                                                        chainbase::database& db) override;

   static types::abi eos_contract_abi();
};

} } // namespace eosio::native_contract

