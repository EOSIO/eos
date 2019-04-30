/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/authority_checker.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/io/json.hpp>
#include <fc/log/logger_config.hpp>
#include <appbase/execution_priority_queue.hpp>

#include <boost/test/unit_test.hpp>
#include "fork_test_utilities.hpp"

#include <eosio/chain/snapshot.hpp>

#define TESTER tester

using namespace eosio::chain;
using namespace eosio::testing;

class signal_tester : public base_tester {
public:
   void init(const controller::config &config) {
      cfg = config;
      control.reset( new controller(cfg, make_protocol_feature_set()) );
      control->add_indices();
   }
   
   void startup() {
      control->startup( []() { return false; }, nullptr);
      chain_transactions.clear();
      control->accepted_block.connect([this]( const block_state_ptr& block_state ){
        FC_ASSERT( block_state->block );
          for( const auto& receipt : block_state->block->transactions ) {
              if( receipt.trx.contains<packed_transaction>() ) {
                  auto &pt = receipt.trx.get<packed_transaction>();
                  chain_transactions[pt.get_transaction().id()] = receipt;
              } else {
                  auto& id = receipt.trx.get<transaction_id_type>();
                  chain_transactions[id] = receipt;
              }
          }
      });
   }

   signal_tester(controller::config config, int ordinal) {
      FC_ASSERT(config.blocks_dir.filename().generic_string() != "."
         && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config");

      controller::config copied_config = config;
      copied_config.blocks_dir =
              config.blocks_dir.parent_path() / std::to_string(ordinal).append(config.blocks_dir.filename().generic_string());
      copied_config.state_dir =
              config.state_dir.parent_path() / std::to_string(ordinal).append(config.state_dir.filename().generic_string());

      init(copied_config);
   }

   signal_tester(controller::config config, int ordinal, int copy_block_log_from_ordinal) {
      FC_ASSERT(config.blocks_dir.filename().generic_string() != "."
         && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config");

      controller::config copied_config = config;
      copied_config.blocks_dir =
              config.blocks_dir.parent_path() / std::to_string(ordinal).append(config.blocks_dir.filename().generic_string());
      copied_config.state_dir =
              config.state_dir.parent_path() / std::to_string(ordinal).append(config.state_dir.filename().generic_string());

      // create a copy of the desired block log and reversible
      auto block_log_path = config.blocks_dir.parent_path() / std::to_string(copy_block_log_from_ordinal).append(config.blocks_dir.filename().generic_string());
      fc::create_directories(copied_config.blocks_dir);
      fc::copy(block_log_path / "blocks.log", copied_config.blocks_dir / "blocks.log");
      fc::copy(block_log_path / config::reversible_blocks_dir_name, copied_config.blocks_dir / config::reversible_blocks_dir_name );

      init(copied_config);
   }

   signed_block_ptr produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
      return _produce_block(skip_time, false);
   }

   signed_block_ptr produce_empty_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
      control->abort_block();
      return _produce_block(skip_time, true);
   }

   signed_block_ptr finish_block()override {
      return _finish_block();
   }

   bool validate() { return true; }
};


BOOST_AUTO_TEST_SUITE(signal_tests)

BOOST_AUTO_TEST_CASE(no_signal)
{
    TESTER test;
    test.produce_block();
}

BOOST_AUTO_TEST_CASE(signal_basic_replay)
{
   controller::config conf;

   {
      TESTER c;
      conf = c.get_config();
   }

   signal_tester test(conf, 1);
   test.startup();

   test.produce_block();

   {
      int irr_count = 0;
      uint32_t last_irr_blocknum = 0;

      int pre_acc_blk_count = 0;
      uint32_t last_pre_acc_num = 0;

      int acc_blk_hdr_count = 0;
      uint32_t last_acc_blk_hdr_num = 0;

      int acc_blk_count = 0;
      uint32_t last_acc_blk_num = 0;

      int acc_txn_count = 0;
      transaction_id_type last_acc_txn_id;

      int app_txn_count = 0;
      transaction_id_type last_app_txn_id;
      transaction_trace_ptr last_signal_trace;

      test.control->irreversible_block.connect([&](const block_state_ptr& bsp) {
         ++irr_count;
         if (last_irr_blocknum) {
            BOOST_CHECK_EQUAL(bsp->block_num, last_irr_blocknum + 1);
         }
         last_irr_blocknum = bsp->block_num;
      });

      test.control->pre_accepted_block.connect([&](const signed_block_ptr& sbp) { // <- never invoked
         ++pre_acc_blk_count;
         last_pre_acc_num = sbp->block_num();
      });

      test.control->accepted_block_header.connect([&](const block_state_ptr& sbp) {
         ++acc_blk_hdr_count;
         last_acc_blk_hdr_num = sbp->block_num;
         BOOST_CHECK_EQUAL(acc_blk_hdr_count, acc_blk_count + 1); // ensure ordering
      });

      test.control->accepted_block.connect([&](const block_state_ptr& sbp) {
         ++acc_blk_count;
         last_acc_blk_num = sbp->block_num;
      });

      test.control->accepted_transaction.connect([&](const transaction_metadata_ptr& ptr) {
         ++acc_txn_count;
         last_acc_txn_id = ptr->id;
      });

      test.control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> t) {
         ++app_txn_count;
         last_app_txn_id = std::get<0>(t)->id;
         last_signal_trace = std::get<0>(t);
      });

      auto trace = test.create_account(N(abc), N(eosio));
      BOOST_CHECK_EQUAL(trace, last_signal_trace);
      BOOST_CHECK_EQUAL(acc_txn_count, 1);
      BOOST_CHECK_EQUAL(app_txn_count, 1);

      test.produce_block();
      BOOST_CHECK_EQUAL(irr_count, 1);
      BOOST_CHECK_EQUAL(last_irr_blocknum, test.control->last_irreversible_block_num());
      BOOST_CHECK_EQUAL(pre_acc_blk_count, 0); // <-- no pre accepted signal if blocks are produced
      BOOST_CHECK_EQUAL(acc_blk_hdr_count, 1);
      BOOST_CHECK_EQUAL(last_acc_blk_hdr_num, test.control->head_block_num());
      BOOST_CHECK_EQUAL(acc_blk_count, 1);
      BOOST_CHECK_EQUAL(last_acc_blk_num, test.control->head_block_num());

      test.produce_block();
      BOOST_CHECK_EQUAL(irr_count, 2);
      BOOST_CHECK_EQUAL(last_irr_blocknum, test.control->last_irreversible_block_num());
      BOOST_CHECK_EQUAL(pre_acc_blk_count, 0); // <-- no pre accepted signal if blocks are produced
      BOOST_CHECK_EQUAL(acc_blk_hdr_count, 2);
      BOOST_CHECK_EQUAL(last_acc_blk_hdr_num, test.control->head_block_num());
      BOOST_CHECK_EQUAL(acc_blk_count, 2);
      BOOST_CHECK_EQUAL(last_acc_blk_num, test.control->head_block_num());
   }

   signal_tester replaychain(conf, 2, 1);

   // test replay
   {
      int irr_count = 0;
      uint32_t last_irr_blocknum = 0;

      int pre_acc_blk_count = 0;
      uint32_t last_pre_acc_num = 0;

      int acc_blk_hdr_count = 0;
      uint32_t last_acc_blk_hdr_num = 0;

      int acc_blk_count = 0;
      uint32_t last_acc_blk_num = 0;

      int acc_txn_count = 0;
      transaction_id_type last_acc_txn_id;

      int app_txn_count = 0;
      transaction_id_type last_app_txn_id;
      transaction_trace_ptr last_signal_trace;

      replaychain.control->irreversible_block.connect([&](const block_state_ptr& bsp) {
         ++irr_count;
         if (last_irr_blocknum) {
            BOOST_CHECK_EQUAL(bsp->block_num, last_irr_blocknum + 1);
         }
         last_irr_blocknum = bsp->block_num;
      });

      replaychain.control->pre_accepted_block.connect([&](const signed_block_ptr& sbp) {
         ++pre_acc_blk_count;
         last_pre_acc_num = sbp->block_num();
         BOOST_CHECK_EQUAL(pre_acc_blk_count, acc_blk_hdr_count + 1); // ensure ordering
      });

      replaychain.control->accepted_block_header.connect([&](const block_state_ptr& sbp) {
         ++acc_blk_hdr_count;
         last_acc_blk_hdr_num = sbp->block_num;
         BOOST_CHECK_EQUAL(acc_blk_hdr_count, acc_blk_count + 1); // ensure ordering
      });

      replaychain.control->accepted_block.connect([&](const block_state_ptr& sbp) {
         ++acc_blk_count;
         last_acc_blk_num = sbp->block_num;
      });

      replaychain.control->accepted_transaction.connect([&](const transaction_metadata_ptr& ptr) {
         ++acc_txn_count;
         last_acc_txn_id = ptr->id;
      });

      replaychain.control->applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> t) {
         ++app_txn_count;
         last_app_txn_id = std::get<0>(t)->id;
         last_signal_trace = std::get<0>(t);
      });

      replaychain.startup();

      BOOST_CHECK_EQUAL(acc_txn_count, 3);
      BOOST_CHECK_EQUAL(app_txn_count, 3);

      BOOST_CHECK_EQUAL(irr_count, 2);
      BOOST_CHECK_EQUAL(last_irr_blocknum, replaychain.control->last_irreversible_block_num());
      BOOST_CHECK_EQUAL(pre_acc_blk_count, 2); // replay has pre_acc signal
      BOOST_CHECK_EQUAL(last_pre_acc_num, replaychain.control->head_block_num());
      BOOST_CHECK_EQUAL(acc_blk_hdr_count, 2);
      BOOST_CHECK_EQUAL(last_acc_blk_hdr_num, replaychain.control->head_block_num());
      BOOST_CHECK_EQUAL(acc_blk_count, 2);
      BOOST_CHECK_EQUAL(last_acc_blk_num, replaychain.control->head_block_num());
   }
}

BOOST_AUTO_TEST_CASE(irreversible_block_multi_producers)
{
   int irr_count = 0;
   uint32_t last_irr_blocknum = 0;
   TESTER test;
   test.produce_block();
   test.control->irreversible_block.connect([&](const block_state_ptr& bsp) {
      ++irr_count;
      if (last_irr_blocknum) {
         BOOST_CHECK_EQUAL(bsp->block_num, last_irr_blocknum + 1);
      }
      last_irr_blocknum = bsp->block_num;
   });
   test.produce_block();
   BOOST_CHECK_EQUAL(irr_count, 1);

   test.create_accounts( {N(dan),N(sam),N(pam),N(scott)} );
   test.set_producers( {N(dan),N(sam),N(pam),N(scott)} );
   test.produce_blocks(100);

   BOOST_CHECK_EQUAL(irr_count, 57);
   BOOST_CHECK_EQUAL(last_irr_blocknum, test.control->last_irreversible_block_num());
}

BOOST_AUTO_TEST_CASE(signal_fork)
{
   int irr_count = 0;
   uint32_t last_irr_blocknum = 0;

   int pre_acc_blk_count = 0;
   uint32_t last_pre_acc_num = 0;

   int acc_blk_hdr_count = 0;
   uint32_t last_acc_blk_hdr_num = 0;

   int acc_blk_count = 0;
   uint32_t last_acc_blk_num = 0;
   
   TESTER c;
   c.produce_block();
   c.control->irreversible_block.connect([&](const block_state_ptr& bsp) {
      ++irr_count;
      if (last_irr_blocknum) {
         BOOST_CHECK_EQUAL(bsp->block_num, last_irr_blocknum + 1);
      }
      last_irr_blocknum = bsp->block_num;
   });

   c.control->pre_accepted_block.connect([&](const signed_block_ptr& sbp) {
      ++pre_acc_blk_count;
      last_pre_acc_num = sbp->block_num();
   });

   c.control->accepted_block_header.connect([&](const block_state_ptr& sbp) {
      ++acc_blk_hdr_count;
      last_acc_blk_hdr_num = sbp->block_num;
      BOOST_CHECK_EQUAL(acc_blk_hdr_count, acc_blk_count + 1); // ensure ordering
   });

   c.control->accepted_block.connect([&](const block_state_ptr& sbp) {
      ++acc_blk_count;
      last_acc_blk_num = sbp->block_num;
   });

   c.produce_block();
   BOOST_CHECK_EQUAL(irr_count, 1);

   c.create_accounts( {N(dan),N(sam),N(pam),N(scott)} );
   c.set_producers( {N(dan),N(sam),N(pam),N(scott)} );
   c.produce_blocks(100);

   BOOST_CHECK_EQUAL(irr_count, 57);
   BOOST_CHECK_EQUAL(last_irr_blocknum, c.control->last_irreversible_block_num());

   BOOST_CHECK_EQUAL(pre_acc_blk_count, 0);
   //BOOST_CHECK_EQUAL(last_pre_acc_num, c.control->head_block_num());
   BOOST_CHECK_EQUAL(acc_blk_hdr_count, 101);
   BOOST_CHECK_EQUAL(last_acc_blk_hdr_num, c.control->head_block_num());
   BOOST_CHECK_EQUAL(acc_blk_count, 101);
   BOOST_CHECK_EQUAL(last_acc_blk_num, c.control->head_block_num());

   TESTER c2;
   push_blocks(c, c2);   

   BOOST_REQUIRE_EQUAL(c.control->head_block_num(), c2.control->head_block_num());

   uint32_t fork_num = c.control->head_block_num();

   auto nextproducer = [](TESTER &c, int skip_interval) ->account_name {
      auto head_time = c.control->head_block_time();
      auto next_time = head_time + fc::milliseconds(config::block_interval_ms * skip_interval);
      return c.control->head_block_state()->get_scheduled_producer(next_time).producer_name;
   };

   // fork c: 1 producers: dan
   // fork c2: 3 producer: sam, pam, scott
   int skip1 = 1, skip2 = 1;
   for (int i = 0; i < 200; ++i) {
      account_name next1 = nextproducer(c, skip1);
      if (next1 == N(dan)) {
         c.produce_block(fc::milliseconds(config::block_interval_ms * skip1)); skip1 = 1;
      }
      else ++skip1;
      account_name next2 = nextproducer(c2, skip2);
      if (next2 == N(sam) || next2 == N(pam) || next2 == N(scott)) {
         c2.produce_block(fc::milliseconds(config::block_interval_ms * skip2)); skip2 = 1;
      }
      else ++skip2;
   }
   BOOST_CHECK_EQUAL(last_irr_blocknum, c.control->last_irreversible_block_num());
   BOOST_REQUIRE_EQUAL(last_irr_blocknum <= fork_num, true);
   BOOST_CHECK_EQUAL(pre_acc_blk_count, 0);
   //BOOST_CHECK_EQUAL(last_pre_acc_num, c.control->head_block_num());
   BOOST_CHECK_EQUAL(acc_blk_hdr_count, 152);
   BOOST_CHECK_EQUAL(last_acc_blk_hdr_num, c.control->head_block_num());
   BOOST_CHECK_EQUAL(acc_blk_count, 152);
   BOOST_CHECK_EQUAL(last_acc_blk_num, c.control->head_block_num());

   // push fork from c2 => c
   size_t p = fork_num;
   size_t count = 0;
   while ( p < c2.control->head_block_num()) {
      signed_block_ptr fb = c2.control->fetch_block_by_number(++p);
      c.push_block(fb);
      BOOST_CHECK_EQUAL(pre_acc_blk_count, ++count);
      BOOST_CHECK_EQUAL(last_pre_acc_num, fb->block_num());
   }

   BOOST_CHECK_EQUAL(last_irr_blocknum, c.control->last_irreversible_block_num());
   BOOST_REQUIRE_EQUAL(last_irr_blocknum > fork_num, true);
}

BOOST_AUTO_TEST_SUITE_END()
