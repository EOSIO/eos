#pragma once

#include "tester.hpp"
#include <boost/signals2/signal.hpp>
#include <boost/signals2/shared_connection_block.hpp>

namespace eosio { namespace testing {
   using namespace boost::signals2;

   /**
    * @brief The tester_network class provides a simplistic virtual P2P network connecting testing_blockchains together.
    *
    * A tester may be connected to zero or more tester_networks at any given time. When a new
    * tester joins the network, it will be synced with all other blockchains already in the network (blocks
    * known only to the new chain will be pushed to the prior network members and vice versa, ignoring blocks not on the
    * main fork). After this, whenever any blockchain in the network gets a new block, that block will be pushed to all
    * other blockchains in the network as well.
    */
   class tester_network {
   public:
      /**
       * @brief Add a new blockchain to the network
       * @param new_blockchain The blockchain to add
       */
      void connect_blockchain(tester &new_blockchain);

      /**
       * @brief Remove a blockchain from the network
       * @param leaving_blockchain The blockchain to remove
       */
      void disconnect_blockchain(tester &leaving_blockchain);

      /**
       * @brief Disconnect all blockchains from the network
       */
      void disconnect_all();

      /**
       * @brief Send a block to all blockchains in this network
       * @param block The block to send
       */
      void propagate_block(const signed_block &block, const tester &skip_db);

   protected:
      std::map<tester *, scoped_connection> blockchains;
   };

} } /// eosio::testing