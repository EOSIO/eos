#include "../block_vault_impl.hpp"
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

struct null_compressor {
   std::string compress(const char* f) { return f;  }
   std::string decompress(const char* f) { return f;  }
};


class mock_backend : public blockvault::backend {

   eosio::controller& controller;
   bool propose_constructed_block_called = false;
   eosio::chain::block_ptr expected_block;


   bool propose_constructed_block(std::pair<uint32_t, uint32_t> watermark, uint32_t lib,
                                  const std::vector<char>& block_content, std::string_view block_id,
                                  std::string_view previous_block_id) {
      return expected_propose_constructed_block_handler(watermark, lib, block_content, block_id, previous_block_id);
   }

   bool append_external_block(uint32_t block_num, uint32_t lib, const std::vector<char>& block_content,
                              std::string_view block_id, std::string_view previous_block_id);
   bool propose_snapshot(std::pair<uint32_t, uint32_t> watermark, const char* snapshot_filename);
   void sync(std::string_view block_id, sync_callback& callback);
};

BOOST_AUTO_TEST_CASE() {

   eosio::testing::tester            chain;

   fc::logger                        logger;
   block_vault_impl<null_compressor> impl(logger, std::make_unique<mock_backend>());

   auto block = main.produce_block();

   impl.async_propose_contructed_block({chain.control->head_block(), chain.control->head_block_time()}, chain.control->last_irreversible_block_num(), block);

   impl.run();
}