#define BOOST_TEST_MODULE trace_data_extraction
#include <boost/test/included/unit_test.hpp>

#include <eosio/chain/trace.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/block_state.hpp>

#include <eosio/trace_api_plugin/test_common.hpp>
#include <eosio/trace_api_plugin/chain_extraction.hpp>

using namespace eosio;
using namespace eosio::trace_api_plugin;
using namespace eosio::trace_api_plugin::test_common;

namespace {
   chain::transaction_trace_ptr make_transaction_trace( chain::transaction_id_type&& id, uint32_t block_number, uint32_t slot, chain::transaction_receipt_header::status_enum status, std::vector<chain::action_trace> && actions ) {
      return std::make_shared<chain::transaction_trace>(chain::transaction_trace{
         std::move(id),
         block_number,
         chain::block_timestamp_type(slot),
         {},
         chain::transaction_receipt_header{status},
         fc::microseconds(0),
         0,
         false,
         std::move(actions),
         {},
         {},
         {},
         {},
         {}
      });
   }

   chain::action_trace make_action_trace(uint64_t global_sequence, chain::name receiver, chain::name account, chain::name action, std::vector<chain::permission_level>&& authorizations, chain::bytes&& data ) {
      chain::action_trace result;
      // don't think we need any information other than receiver and global sequence
      result.receipt.emplace(chain::action_receipt{
         receiver,
         "0000000000000000000000000000000000000000000000000000000000000000"_h,
         global_sequence,
         0,
         {},
         0,
         0
      });
      result.receiver = receiver;
      result.act = chain::action( std::move(authorizations), account, action, std::move(data) );
      return result;
   }

   chain::block_state_ptr make_block_state( chain::block_id_type id, chain::block_id_type previous, uint32_t height, uint32_t slot, chain::name producer, std::vector<std::tuple<chain::transaction_id_type, chain::transaction_receipt_header::status_enum>> transactions ) {
      // TODO: it was going to be very complicated to produce a proper block_state_ptr, this can probably be changed
      // to some sort of intermediate form that a shim interface can extract from a block_state_ptr to make testing
      // and refactoring easier.
      return {};
   }
}

struct extraction_test_fixture {
   /**
    * MOCK implementation of the logfile input API
    */
   struct mock_logfile_provider {
      mock_logfile_provider(extraction_test_fixture& fixture)
      :fixture(fixture)
      {}

      /**
       * append an entry to the end of the data log
       *
       * @param entry : the entry to append
       * @return the offset in the log where that entry is written
       */
      uint64_t append_data_log( const data_log_entry& entry ) {
         fixture.data_log.emplace_back(entry);
         return fixture.data_log.size() - 1;
      }

      /**
       * Append an entry to the metadata log
       *
       * @param entry
       * @return
       */
      uint64_t append_metadata_log( const metadata_log_entry& entry ) {
         if (entry.contains<block_entry_v0>()) {
            const auto& b = entry.get<block_entry_v0>();
            fixture.block_entry_for_height[b.number] = b;
         } else if (entry.contains<lib_entry_v0>()) {
            const auto& lib = entry.get<lib_entry_v0>();
            fixture.max_lib = std::max(fixture.max_lib, lib.lib);
         } else {
            BOOST_FAIL("Unknown metadata log entry emitted");
         }
         return fixture.metadata_offset++;
      }

      extraction_test_fixture& fixture;
   };

   /**
    * TODO: initialize extraction implementation here with `mock_logfile_provider` as template param
    */
   extraction_test_fixture()
   // : extraction_impl(mock_logfile_provider(this))
   {

   }

   void signal_applied_transaction( const chain::transaction_trace_ptr& trace, const chain::signed_transaction& strx ) {
      // TODO: route to extraction system
      // extraction_impl.on_applied_transaction(trace, strx);
   }

   void signal_accepted_block( const chain::block_state_ptr& bsp ) {
      // TODO: route to extraction system
      // extraction_impl.on_accepted_block(bsp);
   }


   // fixture data and methods
   std::map<uint64_t, block_entry_v0> block_entry_for_height = {};
   uint64_t metadata_offset = 0;
   uint64_t max_lib = 0;
   std::vector<data_log_entry> data_log = {};

   // TODO: declare extraction implementation here with `mock_logfile_provider` as template param
   // extraction_impl_type<mock_logfile_provider> extraction_impl;
   
};

BOOST_AUTO_TEST_SUITE(block_extraction)
   BOOST_FIXTURE_TEST_CASE(basic_single_transaction_block, extraction_test_fixture)
   {
      // apply a basic transfer
      //
      signal_applied_transaction(
         make_transaction_trace(
            "0000000000000000000000000000000000000000000000000000000000000001"_h,
            1,
            1,
            chain::transaction_receipt_header::executed,
            {
                  make_action_trace(0, "eosio.token"_n, "eosio.token"_n, "transfer"_n, {{ "alice"_n, "active"_n }}, make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" ) ),
                  make_action_trace(1, "alice"_n, "eosio.token"_n, "transfer"_n, {{ "alice"_n, "active"_n }}, make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" ) ),
                  make_action_trace(2, "bob"_n, "eosio.token"_n, "transfer"_n, {{ "alice"_n, "active"_n }}, make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" ) )
            }
         ),
         {
               // I don't think we will need any data from here?
         }
      );
      
      // accept the block with one transaction
      //

      signal_accepted_block(
         make_block_state(
            "b000000000000000000000000000000000000000000000000000000000000001"_h,
            "0000000000000000000000000000000000000000000000000000000000000000"_h,
            1,
            1,
            "bp.one"_n,
            {
               { "0000000000000000000000000000000000000000000000000000000000000001"_h, chain::transaction_receipt_header::executed }
            }
         )
      );
      
      // Verify that the blockheight and LIB are correct
      //
      const uint64_t expected_lib = 0;
      const block_entry_v0 expected_entry {
        "b000000000000000000000000000000000000000000000000000000000000001"_h,
        1,
        0
      };

      const block_trace_v0 expected_trace {
         "b000000000000000000000000000000000000000000000000000000000000001"_h,
         1,
         "0000000000000000000000000000000000000000000000000000000000000000"_h,
         chain::block_timestamp_type(1),
         "bp.one"_n,
         {
            {
               "0000000000000000000000000000000000000000000000000000000000000001"_h,
               chain::transaction_receipt_header::executed,
               {
                  {
                     0,
                     "eosio.token"_n, "eosio.token"_n, "transfer"_n,
                     {{ "alice"_n, "active"_n }},
                     make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  },
                  {
                     1,
                     "alice"_n, "eosio.token"_n, "transfer"_n,
                     {{ "alice"_n, "active"_n }},
                     make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  },
                  {
                     2,
                     "bob"_n, "eosio.token"_n, "transfer"_n,
                     {{ "alice"_n, "active"_n }},
                     make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  }
               }
            }
         }
      };
      BOOST_REQUIRE_EQUAL(max_lib, 0);
      BOOST_REQUIRE(block_entry_for_height.count(1) > 0);
      BOOST_REQUIRE_EQUAL(block_entry_for_height.at(1), expected_entry);
      BOOST_REQUIRE(data_log.size() >= expected_entry.offset);
      BOOST_REQUIRE(data_log.at(expected_entry.offset).contains<block_trace_v0>());
      BOOST_REQUIRE_EQUAL(data_log.at(expected_entry.offset).get<block_trace_v0>(), expected_trace);
   }

BOOST_AUTO_TEST_SUITE_END()
