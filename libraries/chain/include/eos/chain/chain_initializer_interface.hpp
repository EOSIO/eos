#pragma once

#include <eos/chain/BlockchainConfiguration.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/message.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {
class chain_controller;

/**
 * @brief This class defines an interface for the @ref chain_controller to get the necessary data to launch a new chain,
 * as well as to give client code an opportunity to perform its own initializations on a brand new blockchain database.
 */
class chain_initializer_interface {
public:
   virtual ~chain_initializer_interface();

   /// Retrieve the timestamp to use as the blockchain start time
   virtual types::Time get_chain_start_time() = 0;
   /// Retrieve the BlockchainConfiguration to use at blockchain start
   virtual BlockchainConfiguration get_chain_start_configuration() = 0;
   /// Retrieve the first round of block producers
   virtual std::array<AccountName, config::ProducerCount> get_chain_start_producers() = 0;

   /**
    * @brief Prepare the database, creating objects and defining state which should exist before the first block
    * @param chain A reference to the @ref chain_controller
    * @param db A reference to the @ref chainbase::database
    * @param A list of @ref Message "Messages" to be applied before the first block
    *
    * This method creates the @ref account_object "account_objects" and @ref producer_object "producer_objects" for
    * at least the initial block producers returned by @ref get_chain_start_producers
    *
    * This method also provides an opportunity to create objects and setup the database to the state it should be in
    * prior to the first block. This method should only initialize state that the @ref chain_controller itself does
    * not understand. The other methods on @ref chain_initializer are called to retrieve the data necessary to
    * initialize chain state the controller does understand, including the initial round of block producers and the
    * initial @ref BlockchainConfiguration.
    *
    * Finally, this method may perform any necessary initializations on the chain and/or database, such as
    * installing indexes and message handlers that should be defined before the first block is processed. This may
    * be necessary in order for the returned list of messages to be processed successfully.
    *
    * This method is called at the end of chain initialization, after setting the state used in core chain
    * operations.
    */
   virtual vector<Message> prepare_database(chain_controller& chain, chainbase::database& db) = 0;
};

} } // namespace eos::chain
