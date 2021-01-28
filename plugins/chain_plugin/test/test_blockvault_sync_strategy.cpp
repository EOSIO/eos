#define BOOST_TEST_MODULE blockvault_sync_strategy
#include <boost/test/unit_test.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain_plugin/blockvault_sync_strategy.hpp>
#include <eosio/testing/tester.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace eosio::chain_apis;
using namespace eosio::blockvault;

struct mock_genesis_t {};

struct mock_signed_block_t {
   block_id_type _id;
   uint32_t      _block_num;
   block_id_type calculate_id() { return _id; }
   uint32_t      block_num() { return _block_num; }
};

struct mock_chain_t {
   void startup(std::function<void()> shutdown, std::function<bool()> check_shutdown,
                std::shared_ptr<istream_snapshot_reader> reader) {
      _shutdown              = shutdown;
      _check_shutdown        = check_shutdown;
      _reader                = reader;
      _startup_reader_called = true;
   }

   mock_signed_block_t* _last_irreversible_block = nullptr;
   mock_signed_block_t* last_irreversible_block() { return _last_irreversible_block; }

   std::function<void()>                    _shutdown;
   std::function<bool()>                    _check_shutdown;
   std::shared_ptr<istream_snapshot_reader> _reader;

   bool _startup_reader_called;

   uint32_t _head_block_num  = 0;
   uint32_t head_block_num() {return _head_block_num;}
};

struct mock_blockvault_t : public block_vault_interface {
   eosio::chain::block_id_type _previous_block_id{};
   bool                        _previous_block_id_sent = false;

   virtual void async_propose_constructed_block(uint32_t lib,
                                                eosio::chain::signed_block_ptr block,
                                                std::function<void(bool)>      handler) override {}
   virtual void async_append_external_block(uint32_t lib, eosio::chain::signed_block_ptr block,
                                            std::function<void(bool)> handler) override {}

   virtual bool propose_snapshot(watermark_t watermark, const char* snapshot_filename) override { return true; }
   virtual void sync(const eosio::chain::block_id_type* previous_block_id, sync_callback& callback) override {
      if (nullptr != previous_block_id) {
         _previous_block_id      = *previous_block_id;
         _previous_block_id_sent = true;
      }
   }
};

struct mock_chain_plugin_t {
   mock_chain_plugin_t() {
      _accept_block_rc = true;
      chain            = std::make_unique<mock_chain_t>();
   }

   bool incoming_blockvault_sync_method(const chain::signed_block_ptr& block, bool check_connectivity) {
      _block = block;
      return _accept_block_rc;
   }

   bool                          _accept_block_rc;
   signed_block_ptr              _block;
   block_id_type                 _id;
   std::unique_ptr<mock_chain_t> chain;

   bool _startup_non_snapshot_called = false;

   void do_non_snapshot_startup(std::function<void()> shutdown, std::function<bool()> check_shutdown) {
      _startup_non_snapshot_called = true;
   }
};

BOOST_AUTO_TEST_SUITE(blockvault_sync_strategy_tests)

BOOST_FIXTURE_TEST_CASE(empty_previous_block_id_test, TESTER) {
   try {

      mock_chain_plugin_t plugin;
      mock_blockvault_t   bv;

      auto shutdown       = []() { return false; };
      auto check_shutdown = []() { return false; };

      blockvault_sync_strategy<mock_chain_plugin_t> uut(&bv, plugin, shutdown, check_shutdown);
      uut.do_sync();

      BOOST_TEST(!bv._previous_block_id_sent);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(nonempty_previous_block_id_test, TESTER) {
   try {

      mock_chain_plugin_t  plugin;
      mock_blockvault_t    bv;
      auto                 shutdown       = []() { return false; };
      auto                 check_shutdown = []() { return false; };
      std::string          bid_hex("deadbabe000000000000000000000000000000000000000000000000deadbeef");
      chain::block_id_type bid(bid_hex);
      mock_signed_block_t  lib;
      lib._block_num = 100;
      lib._id        = bid;

      plugin.chain->_last_irreversible_block = &lib;

      blockvault_sync_strategy<mock_chain_plugin_t> uut(&bv, plugin, shutdown, check_shutdown);
      uut.do_sync();

      BOOST_TEST(bv._previous_block_id == bid);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(on_block_no_snapshot, TESTER) {
   try {

      mock_chain_plugin_t plugin;
      mock_blockvault_t   bv;
      plugin.chain = std::make_unique<mock_chain_t>();

      auto                                          shutdown       = []() { return false; };
      auto                                          check_shutdown = []() { return false; };
      blockvault_sync_strategy<mock_chain_plugin_t> uut(&bv, plugin, shutdown, check_shutdown);
      auto                                          b = produce_empty_block();

      uut.on_block(b);
      BOOST_TEST(plugin.chain->_reader == nullptr);
      BOOST_TEST(plugin._startup_non_snapshot_called);
      BOOST_TEST(!plugin.chain->_startup_reader_called);
      BOOST_TEST(plugin._block->calculate_id() == b->calculate_id());
   }
   FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()
