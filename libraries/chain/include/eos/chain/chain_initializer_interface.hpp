/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/blockchain_configuration.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/message.hpp>

namespace chainbase { class database; }

namespace eosio { namespace chain {
class chain_controller;

/**
 * @brief This class defines an interface for the @ref chain_controller to get the necessary data to launch a new chain,
 * as well as to give client code an opportunity to perform its own initializations on a brand new blockchain database.
 */
class chain_initializer_interface {
public:
   virtual ~chain_initializer_interface();

   /// Retrieve the timestamp to use as the blockchain start time
   virtual types::time get_chain_start_time() = 0;
   /// Retrieve the BlockchainConfiguration to use at blockchain start
   virtual blockchain_configuration get_chain_start_configuration() = 0;
   /// Retrieve the first round of block producers
   virtual std::array<account_name, config::blocks_per_round> get_chain_start_producers() = 0;

   /**
    * @brief Install necessary indices and message handlers that chain_controller doesn't know about
    *
    * This method is called every time the chain_controller is initialized, before any chain state is read or written,
    * regardless of whether the chain is new or not.
    *
    * This method may perform any necessary initializations on the chain and/or database, such as installing indices
    * and message handlers that should be defined before the first block is processed. This may be necessary in order
    * for the list of messages returned by @ref initialize_database to be processed successfully.
    */
   virtual void register_types(chain_controller& chain, chainbase::database& db) = 0;
   /**
    * @brief Prepare the database, creating objects and defining state which should exist before the first block
    * @param chain A reference to the @ref chain_controller
    * @param db A reference to the @ref chainbase::database
    * @return A list of @ref Message "Messages" to be applied before the first block
    *
    * This method is only called when starting a new blockchain. It is called at the end of chain initialization, after
    * setting the state used in core chain operations.
    *
    * This method creates the @ref account_object "account_objects" and @ref producer_object "producer_objects" for
    * at least the initial block producers returned by @ref get_chain_start_producers
    *
    * This method also provides an opportunity to create objects and setup the database to the state it should be in
    * prior to the first block, including registering any message types unknown to @ref chain_controller. This method
    * should only initialize state that the @ref chain_controller itself does not understand.
    *
    * The other methods on @ref chain_initializer_interface are called to retrieve the data necessary to initialize
    * chain state the controller does understand, including the initial round of block producers, the initial chain
    * time, and the initial @ref BlockchainConfiguration.
    */
   virtual vector<message> prepare_database(chain_controller& chain, chainbase::database& db) = 0;
};

} } // namespace eosio::chain
