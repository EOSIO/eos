#pragma once
#include <eosio/blockvault_client_plugin/blockvault.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {
namespace blockvault {

template <typename BP>
struct blockvault_sync_strategy : public sync_callback {
   blockvault_sync_strategy(block_vault_interface* blockvault, BP& blockchain_provider, std::function<void()> shutdown,
                            std::function<bool()> check_shutdown)
       : _blockvault(blockvault)
       , _blockchain_provider(blockchain_provider)
       , _shutdown(shutdown)
       , _check_shutdown(check_shutdown)
       , _startup_run(false)
       , _received_snapshot(false) {
      EOS_ASSERT(nullptr != blockvault, plugin_exception, "block_vault_interface cannot be null");
   }

   ~blockvault_sync_strategy() {
      if (_num_unlinkable_blocks)
         wlog("${num} out of ${total} blocks received are unlinkable",
              ("num", _num_unlinkable_blocks)("total", _num_blocks_received));
   }

   void run_startup() {
      _blockchain_provider.do_non_snapshot_startup(_shutdown, _check_shutdown);
      _startup_run = true;
   }

   void do_sync() {
      auto head_block = _blockchain_provider.chain->last_irreversible_block();
      if (nullptr != head_block) {
         block_id_type bid = head_block->calculate_id();
         ilog("Requesting blockvault sync from block id ${id}, block_num ${num}",
              ("id", (const string)bid)("num", head_block->block_num()));
         _blockvault->sync(&bid, *this);
      } else {
         ilog("Requesting complete blockvault sync.");
         _blockvault->sync(nullptr, *this);
      }

      if (!_startup_run) {
         ilog("Received no data from blockvault.");
         run_startup();
      }
   }

   void on_snapshot(const char* snapshot_filename) override final {
      ilog("Received snapshot from blockvault ${fn}", ("fn", snapshot_filename));
      EOS_ASSERT(!_received_snapshot, plugin_exception, "Received multiple snapshots from blockvault.", );
      _received_snapshot = true;

      if (_check_shutdown()) {
         _shutdown();
      }

      auto infile = std::ifstream(snapshot_filename, (std::ios::in | std::ios::binary));
      auto reader = std::make_shared<chain::istream_snapshot_reader>(infile);

      _blockchain_provider.chain->startup(_shutdown, _check_shutdown, reader);
      _startup_run = true;

      infile.close();
   }

   void on_block(eosio::chain::signed_block_ptr block) override final {
      if (_check_shutdown()) {
         _shutdown();
      }

      if (!_startup_run) {
         run_startup();
      }

      try {
         ++_num_blocks_received;
         _blockchain_provider.incoming_block_sync_method(block, block->calculate_id());
      } catch (unlinkable_block_exception& e) {
         if (block->block_num() == 2) {
            elog("Received unlinkable block 2. Please double check if --genesis-json and --genesis-timestamp are "
                 "correctly specified");
            throw e;
         }
         ++_num_unlinkable_blocks;
      }
   }

 private:
   block_vault_interface* _blockvault;
   BP&                    _blockchain_provider;
   std::function<void()>  _shutdown;
   std::function<bool()>  _check_shutdown;
   bool                   _startup_run;
   bool                   _received_snapshot;
   uint32_t               _num_unlinkable_blocks = 0;
   uint32_t               _num_blocks_received   = 0;
};

} // namespace blockvault
} // namespace eosio
