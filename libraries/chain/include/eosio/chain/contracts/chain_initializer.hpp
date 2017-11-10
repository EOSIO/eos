/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/contracts/genesis_state.hpp>

#include <eosio/chain/chain_controller.hpp>

namespace eosio { namespace chain {  namespace contracts {

   class contract_chain_initializer : public chain::chain_initializer_interface {

      public:
         native_contract_chain_initializer(const genesis_state_type& genesis) : genesis(genesis) {}
         virtual ~native_contract_chain_initializer() {}

         virtual time_point              get_chain_start_time() override;
         virtual chain::chain_config     get_chain_start_configuration() override;
         virtual producer_scheduler_type get_chain_start_producers() override;

         virtual void register_types(chain::chain_controller& chain, chainbase::database& db) override;


         virtual std::vector<action> prepare_database(chain::chain_controller& chain,
                                                      chainbase::database& db) override;

         static abi eos_contract_abi();

      private:
         genesis_state_type genesis;
   };

} } } // namespace eosio::chain::contracts

