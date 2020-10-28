#pragma once
#include <stdint.h>
#include <string_view>

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

} // namespace blockvault
} // namespace eosio
