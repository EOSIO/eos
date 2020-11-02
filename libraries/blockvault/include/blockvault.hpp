#pragma once
#include <stdint.h>
#include <string_view>
#include <eosio/chain/block_timestamp.hpp>

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
};

} // namespace blockvault
} // namespace eosio
