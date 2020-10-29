#pragma once
#include <stdint.h>
#include <string_view>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {
    namespace blockvault {
        using watermark_t = uint32_t;

        class sync_callback {
        public:
            virtual void on_snapshot(watermark_t watermark, std::string_view snapshot_filename) = 0;
            virtual void on_block(watermark_t watermark, std::string_view block)                = 0;
        };

        class block_vault_interface {
        public:
            virtual void append_proposed_block(watermark_t watermark, uint32_t lib, std::string_view block_content) = 0;
            virtual void append_external_block(watermark_t watermark, std::string_view block_content)               = 0;
            virtual void propose_snapshot(watermark_t watermark, const char* snapshot_filename)                     = 0;
            virtual void sync(std::string_view previous_block_id, sync_callback& callback)                          = 0;
        };

        template<typename BP>
        struct blockvault_sync_strategy : sync_callback {
            blockvault_sync_strategy(block_vault_interface& blockvault, BP& blockchain_provider) :
                _blockvault(blockvault), _blockchain_provider(blockchain_provider) {

            }

            blockvault_sync_strategy(block_vault_interface& blockvault, BP& blockchain_provider, chain::block_id_type block_id) :
                _blockvault(blockvault), _blockchain_provider(blockchain_provider), _previous_block_id(block_id) {
            }

            void do_sync() {
                std::string previous_block_id;
                if (_previous_block_id.has_value()) {
                    previous_block_id = _previous_block_id->str();
                }
                _blockvault.sync(previous_block_id, *this);
            }

            void on_snapshot(watermark_t watermark, std::string_view snapshot_filename) override {
                auto infile = std::ifstream(snapshot_filename, (std::ios::in | std::ios::binary));
                auto reader = std::make_shared<chain::istream_snapshot_reader>(infile);
                _blockchain_provider.startup_from_snapshot(reader);
                infile.close();
            }

            void on_block(watermark_t watermark, std::string_view block) override {
                chain::signed_block_ptr b = std::make_shared<chain::signed_block>();
                fc::datastream<const char *> ds(block.cbegin(), block.size());
                fc::raw::unpack(ds, b);

                _blockchain_provider.accept_block(b, b->calculate_id());
            }

        private:
            std::optional<chain::block_id_type> _previous_block_id;
            block_vault_interface& _blockvault;
            BP& _blockchain_provider;
        };

    } // namespace blockvault
} // namespace eosio


