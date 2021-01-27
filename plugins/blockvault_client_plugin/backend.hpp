#pragma once
#include <eosio/blockvault_client_plugin/blockvault.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <stdint.h>
#include <string_view>

namespace eosio {
namespace blockvault {

class backend {
 public:
   struct sync_callback {
      virtual void on_snapshot(const char* snapshot_filename) = 0;
      virtual void on_block(std::string_view block)           = 0;
   };

   virtual ~backend() {}

   virtual bool propose_constructed_block(std::pair<uint32_t, uint32_t> watermark, uint32_t lib,
                                          const std::vector<char>& block_content, std::string_view block_id,
                                          std::string_view previous_block_id)                            = 0;
   virtual bool append_external_block(uint32_t block_num, uint32_t lib, const std::vector<char>& block_content,
                                      std::string_view block_id, std::string_view previous_block_id)     = 0;
   virtual bool propose_snapshot(std::pair<uint32_t, uint32_t> watermark, const char* snapshot_filename) = 0;
   virtual void sync(std::string_view block_id, sync_callback& callback)                                 = 0;
};

} // namespace blockvault
} // namespace eosio
