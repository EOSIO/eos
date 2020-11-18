#pragma once

#include <eosio/blockvault_client_plugin/backend.hpp>
#include <pqxx/pqxx>

namespace eosio {
namespace blockvault {

class postgres_backend : public backend {
 public:
   pqxx::connection conn;

   postgres_backend(const std::string& options);

   bool propose_constructed_block(std::pair<uint32_t, uint32_t> watermark, uint32_t lib,
                                  const std::vector<char>& block_content, std::string_view block_id,
                                  std::string_view previous_block_id) override;
   bool append_external_block(uint32_t block_num, uint32_t lib, const std::vector<char>& block_content,
                              std::string_view block_id, std::string_view previous_block_id) override;
   bool propose_snapshot(std::pair<uint32_t, uint32_t> watermark, const char* snapshot_filename) override;
   void sync(std::string_view previous_block_id, eosio::blockvault::backend::sync_callback& callback) override;
};

} // namespace blockvault
} // namespace eosio
