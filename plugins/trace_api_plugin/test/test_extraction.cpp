#define BOOST_TEST_MODULE trace_data_extraction
#include <boost/test/included/unit_test.hpp>

#include <fc/io/json.hpp>

#include <eosio/chain/trace.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/asset.hpp>

#include <eosio/trace_api_plugin/data_log.hpp>
#include <eosio/trace_api_plugin/metadata_log.hpp>

using namespace eosio;
using namespace eosio::trace_api_plugin;

namespace {
   fc::sha256 operator"" _h(const char* input, std::size_t) {
      return fc::sha256(input);
   }

   chain::name operator"" _n(const char* input, std::size_t) {
      return chain::name(input);
   }

   chain::asset operator"" _t(const char* input, std::size_t) {
      return chain::asset::from_string(input);
   }


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

   chain::bytes make_transfer_data( chain::name from, chain::name to, chain::asset quantity, std::string&& memo) {
      fc::datastream<size_t> ps;
      fc::raw::pack(ps, from, to, quantity, memo);
      chain::bytes result(ps.tellp());

      if( result.size() ) {
         fc::datastream<char*>  ds( result.data(), size_t(result.size()) );
         fc::raw::pack(ds, from, to, quantity, memo);
      }
      return result;
   }
}
namespace eosio::trace_api_plugin {
   // TODO: promote these to the main files?
   // I prefer not to have these operators but they are convenient for BOOST TEST integration
   //

   bool operator==(const authorization_trace_v0& lhs, const authorization_trace_v0& rhs) {
      return
         lhs.account == rhs.account &&
         lhs.permission == rhs.permission;
   }

   bool operator==(const action_trace_v0& lhs, const action_trace_v0& rhs) {
      return
         lhs.global_sequence == rhs.global_sequence &&
         lhs.receiver == rhs.receiver &&
         lhs.account == rhs.account &&
         lhs.action == rhs.action &&
         lhs.authorization == rhs.authorization &&
         lhs.data == rhs.data;
   }

   bool operator==(const transaction_trace_v0& lhs,  const transaction_trace_v0& rhs) {
      return
         lhs.id == rhs.id &&
         lhs.status == rhs.status &&
         lhs.actions == rhs.actions;
   }

   bool operator==(const block_trace_v0 &lhs, const block_trace_v0 &rhs) {
      return
         lhs.id == rhs.id &&
         lhs.number == rhs.number &&
         lhs.previous_id == rhs.previous_id &&
         lhs.timestamp == rhs.timestamp &&
         lhs.producer == rhs.producer &&
         lhs.transactions == rhs.transactions;
   }

   std::ostream& operator<<(std::ostream &os, const block_trace_v0 &bt) {
      os << fc::json::to_string( bt, fc::time_point::maximum() );
      return os;
   }

   bool operator==(const block_entry_v0& lhs, const block_entry_v0& rhs) {
      return
         lhs.id == rhs.id &&
         lhs.number == rhs.number &&
         lhs.offset == rhs.offset;
   }

   std::ostream& operator<<(std::ostream &os, const block_entry_v0 &be) {
      os << fc::json::to_string(be, fc::time_point::maximum());
      return os;
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
      uint64_t append_data_log( data_log_entry&& entry ) {
         fixture.data_log.emplace_back(std::move(entry));
         return fixture.data_log.size() - 1;
      }

      /**
       * Append an entry to the metadata log
       *
       * @param entry
       * @return
       */
      uint64_t append_metadata_log( metadata_log_entry&& entry ) {
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
      // route to extraction system
      // extraction_impl.on_applied_transaction(trace, strx);
   }

   void signal_accepted_block( const chain::block_state_ptr& bsp ) {
      // route to extraction system
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
      BOOST_REQUIRE_EQUAL(block_entry_for_height.at(1), expected_entry);
      BOOST_REQUIRE(data_log.at(expected_entry.offset).contains<block_trace_v0>());
      BOOST_REQUIRE_EQUAL(data_log.at(expected_entry.offset).get<block_trace_v0>(), expected_trace);
   }

BOOST_AUTO_TEST_SUITE_END()
