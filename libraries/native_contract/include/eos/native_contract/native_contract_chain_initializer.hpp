#pragma once

#include <eos/native_contract/genesis_state.hpp>

#include <eos/chain/chain_controller.hpp>

namespace eos { namespace native_contract {

class native_contract_chain_initializer : public chain::chain_initializer_interface {
   genesis_state_type genesis;
public:
   native_contract_chain_initializer(const genesis_state_type& genesis) : genesis(genesis) {}
   virtual ~native_contract_chain_initializer() {}

   virtual std::vector<chain::Message> prepare_database(chain::chain_controller& chain, chainbase::database& db);
   virtual types::Time get_chain_start_time();
   virtual chain::BlockchainConfiguration get_chain_start_configuration();
   virtual std::array<types::AccountName, config::ProducerCount> get_chain_start_producers();
};

} } // namespace eos::native_contract

