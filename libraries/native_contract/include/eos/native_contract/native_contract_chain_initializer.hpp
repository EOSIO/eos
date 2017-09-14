#pragma once

#include <eos/native_contract/genesis_state.hpp>

#include <eos/chain/chain_controller.hpp>

namespace eos { namespace native_contract {

class native_contract_chain_initializer : public chain::chain_initializer_interface {
   genesis_state_type genesis;
public:
   native_contract_chain_initializer(const genesis_state_type& genesis) : genesis(genesis) {}
   virtual ~native_contract_chain_initializer() {}

   virtual types::Time get_chain_start_time() override;
   virtual chain::BlockchainConfiguration get_chain_start_configuration() override;
   virtual std::array<types::AccountName, config::BlocksPerRound> get_chain_start_producers() override;

   virtual void register_types(chain::chain_controller& chain, chainbase::database& db) override;
   virtual std::vector<chain::Message> prepare_database(chain::chain_controller& chain,
                                                        chainbase::database& db) override;

   static types::Abi eos_contract_abi();
};

} } // namespace eos::native_contract

