#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/blockvault_client_plugin/blockvault.hpp>

namespace eosio {
    namespace blockvault {

        template<typename BP>
        struct blockvault_sync_strategy : public sync_callback {
            blockvault_sync_strategy(block_vault_interface* blockvault, BP& blockchain_provider, std::function<void()> shutdown,
                                     std::function<bool()> check_shutdown) :
                _got_snapshot(false), _blockvault(blockvault), _blockchain_provider(blockchain_provider), _shutdown(shutdown), _check_shutdown(check_shutdown) {
            }

            void do_sync() {
                auto head_block = _blockchain_provider.chain->fork_db().head();
                if (nullptr != head_block) {
                    _blockvault->sync(&head_block->id, *this);
                } else {
                    _blockvault->sync(nullptr, *this);
                }
            }

            void on_snapshot(const char* snapshot_filename) override final {
                auto infile = std::ifstream(snapshot_filename, (std::ios::in | std::ios::binary));
                auto reader = std::make_shared<chain::istream_snapshot_reader>(infile);
                _blockchain_provider.chain->startup(_shutdown, _check_shutdown, reader);
                infile.close();

                _got_snapshot = true;
            }

            void on_block(eosio::chain::signed_block_ptr block) override final {
                if (!_got_snapshot) {
                    _blockchain_provider.chain->startup(_shutdown, _check_shutdown);
                }

                _blockchain_provider.incoming_block_sync_method(block, block->calculate_id());
            }

        private:
            block_vault_interface* _blockvault;
            BP& _blockchain_provider;
            std::function<void()> _shutdown;
            std::function<bool()> _check_shutdown;
            bool _got_snapshot;
        };

    } // namespace blockvault
} // namespace eosio


