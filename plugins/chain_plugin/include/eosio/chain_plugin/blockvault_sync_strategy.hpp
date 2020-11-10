#pragma once
// #include <stdint.h>
// #include <string_view>
#include <eosio/chain_plugin/chain_plugin.hpp>
namespace eosio {
    namespace blockvault {
        using watermark_t = std::pair<uint32_t, eosio::chain::block_timestamp_type>;

        class sync_callback {
        public:
            virtual void on_snapshot(std::string_view snapshot_filename) = 0;
            virtual void on_block(std::string_view block)                = 0;
        };

        class block_vault_interface {
        public:
            virtual bool append_proposed_block(watermark_t watermark, uint32_t lib, std::string_view block_content) = 0;
            virtual bool append_external_block(uint32_t block_num, uint32_t lib, std::string_view block_content)    = 0;
            virtual bool propose_snapshot(watermark_t watermark, const char* snapshot_filename)                     = 0;
            virtual void sync(std::string_view block_id, sync_callback& callback)                          = 0;

            static block_vault_interface* get() {return nullptr;}
        };

    } // namespace blockvault
} // namespace eosio

namespace eosio {
    namespace blockvault {

        template<typename BP>
        struct blockvault_sync_strategy : public sync_callback {
            blockvault_sync_strategy(block_vault_interface* blockvault, BP& blockchain_provider, std::function<void()> shutdown,
                                     std::function<bool()> check_shutdown) :
                _blockvault(blockvault), _blockchain_provider(blockchain_provider), _shutdown(shutdown), _check_shutdown(check_shutdown) {

            }

            blockvault_sync_strategy(block_vault_interface* blockvault, BP& blockchain_provider, std::function<void()> shutdown,
                                     std::function<bool()> check_shutdown, chain::block_id_type block_id) :
                _blockvault(blockvault), _blockchain_provider(blockchain_provider), _shutdown(shutdown), _check_shutdown(check_shutdown),
                _previous_block_id(block_id) {
            }

            void do_sync() {
                std::string previous_block_id;
                if (_previous_block_id.has_value()) {
                    previous_block_id = _previous_block_id->str();
                }
                _blockvault->sync(previous_block_id, *this);
            }

            void on_snapshot(std::string_view snapshot_filename) override {
                auto infile = std::ifstream(snapshot_filename, (std::ios::in | std::ios::binary));
                auto reader = std::make_shared<chain::istream_snapshot_reader>(infile);
                _blockchain_provider.chain->startup(_shutdown, _check_shutdown, reader);
                infile.close();

                _got_snapshot = true;
            }

            void on_block(std::string_view block) override {
                if (!_got_snapshot) {
                    _blockchain_provider.chain->startup(_shutdown, _check_shutdown);
                }

                chain::signed_block_ptr b = std::make_shared<chain::signed_block>();
                fc::datastream<const char *> ds(block.cbegin(), block.size());
                fc::raw::unpack(ds, b);

                _blockchain_provider.incoming_block_sync_method(b, b->calculate_id());
            }

        private:
            std::optional<chain::block_id_type> _previous_block_id;
            block_vault_interface* _blockvault;
            BP& _blockchain_provider;
            std::function<void()> _shutdown;
            std::function<bool()> _check_shutdown;
            bool _got_snapshot;
        };

    } // namespace blockvault
} // namespace eosio


