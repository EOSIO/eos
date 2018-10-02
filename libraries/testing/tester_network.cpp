#include <eosio/testing/tester_network.hpp>

namespace eosio { namespace testing {

   void tester_network::connect_blockchain(base_tester &new_blockchain) {
      if (blockchains.count(&new_blockchain))
         return;

      // If the network isn't empty, sync the new blockchain with one of the old ones. The old ones are already in sync with
      // each other, so just grab one arbitrarily. The old blockchains are connected to the propagation signals, so when one
      // of them gets synced, it will propagate blocks to the others as well.
      if (!blockchains.empty()) {
         blockchains.begin()->first->sync_with(new_blockchain);
      }

      // The new blockchain is now in sync with any old ones; go ahead and connect the propagation signal.

/// TODO restore this
      
      /*
      blockchains[&new_blockchain] = new_blockchain.control->applied_block.connect(
              [this, &new_blockchain](const chain::block_trace& bt) {
                 propagate_block(bt.block, new_blockchain);
              });
              */
   }

   void tester_network::disconnect_blockchain(base_tester &leaving_blockchain) {
      blockchains.erase(&leaving_blockchain);
   }

   void tester_network::disconnect_all() {
      blockchains.clear();
   }

   void tester_network::propagate_block(const signed_block &block, const base_tester &skip_blockchain) {
    //  for (const auto &pair : blockchains) {
    //     if (pair.first == &skip_blockchain) continue;
    //     boost::signals2::shared_connection_block blocker(pair.second);
    //     pair.first->control->push_block(block, eosio::chain::validation_steps::created_block);
    //  }
   }

} } /// eosio::testing
