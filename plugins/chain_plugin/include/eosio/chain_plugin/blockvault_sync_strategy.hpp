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
            virtual void sync(std::optional<uint32_t> previous_block_id, sync_callback& callback)                          = 0;
        };

        template<typename BP>
        struct blockvault_sync_strategy : sync_callback {
            blockvault_sync_strategy(block_vault_interface& service_provider, BP& blockchain_provider) :
                _service_provider(service_provider), _blockchain_provider(blockchain_provider) {

            }

            blockvault_sync_strategy(block_vault_interface& service_provider, BP& blockchain_provider, uint32_t block_id) :
                    _service_provider(service_provider), _blockchain_provider(blockchain_provider), _previous_block_id(block_id) {

            }

            void do_sync() {
                _service_provider.sync(_previous_block_id, *this);
            }

            void on_snapshot(watermark_t watermark, std::string_view snapshot_filename) override {

            }

            void on_block(watermark_t watermark, std::string_view block) override {

            }

        private:
            std::optional<uint32_t> _previous_block_id;
            block_vault_interface& _service_provider;
            BP& _blockchain_provider;
        };

    } // namespace blockvault
} // namespace eosio


