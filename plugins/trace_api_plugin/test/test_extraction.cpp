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
using eosio::chain::name;
using eosio::chain::digest_type;

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

   auto get_private_key( name keyname, std::string role = "owner" ) {
      auto secret = fc::sha256::hash( keyname.to_string() + role );
      return chain::private_key_type::regenerate<fc::ecc::private_key_shim>( secret );
   }

   auto get_public_key( name keyname, std::string role = "owner" ) {
      return get_private_key( keyname, role ).get_public_key();
   }

   auto create_test_block_state( std::vector<chain::transaction_metadata_ptr> trx_metas ) {
      chain::signed_block_ptr block = std::make_shared<chain::signed_block>();
      for( auto& trx_meta : trx_metas ) {
         block->transactions.emplace_back( *trx_meta->packed_trx());
      }

      block->producer = eosio::chain::config::system_account_name;

      auto priv_key = get_private_key( block->producer, "active" );
      auto pub_key = get_public_key( block->producer, "active" );

      auto prev = std::make_shared<chain::block_state>();
      auto header_bmroot = digest_type::hash( std::make_pair( block->digest(), prev->blockroot_merkle.get_root()));
      auto sig_digest = digest_type::hash( std::make_pair( header_bmroot, prev->pending_schedule.schedule_hash ));
      block->producer_signature = priv_key.sign( sig_digest );

      std::vector<chain::private_key_type> signing_keys;
      signing_keys.emplace_back( std::move( priv_key ));

      auto signer = [&]( digest_type d ) {
         std::vector<chain::signature_type> result;
         result.reserve( signing_keys.size());
         for( const auto& k: signing_keys )
            result.emplace_back( k.sign( d ));
         return result;
      };
      chain::pending_block_header_state pbhs;
      pbhs.producer = block->producer;
      chain::producer_authority_schedule schedule = {0, {chain::producer_authority{block->producer,
                                                                     chain::block_signing_authority_v0{1, {{pub_key, 1}}}}}};
      pbhs.active_schedule = schedule;
      pbhs.valid_block_signing_authority = chain::block_signing_authority_v0{1, {{pub_key, 1}}};
      auto bsp = std::make_shared<chain::block_state>(
            std::move( pbhs ),
            std::move( block ),
            std::move( trx_metas ),
            chain::protocol_feature_set(),
            []( chain::block_timestamp_type timestamp,
                const fc::flat_set<digest_type>& cur_features,
                const std::vector<digest_type>& new_features ) {},
            signer
      );

      return bsp;
   }

   chain::block_state_ptr make_block_state( chain::block_id_type id, chain::block_id_type previous, uint32_t height,
         uint32_t slot, chain::name producer,
         std::vector<std::tuple<chain::transaction_id_type, chain::transaction_receipt_header::status_enum>> transactions ) {
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
   struct mock_logfile_provider_type {
      mock_logfile_provider_type(extraction_test_fixture& fixture)
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
   : extraction_impl(mock_logfile_provider_type(*this), [](std::exception_ptr eptr) {})
   {
   }

   void signal_applied_transaction( const chain::transaction_trace_ptr& trace, const chain::signed_transaction& strx ) {
      extraction_impl.signal_applied_transaction(trace, strx);
   }

   void signal_accepted_block( const chain::block_state_ptr& bsp ) {
      extraction_impl.signal_accepted_block(bsp);
   }


   // fixture data and methods
   std::map<uint64_t, block_entry_v0> block_entry_for_height = {};
   uint64_t metadata_offset = 0;
   uint64_t max_lib = 0;
   std::vector<data_log_entry> data_log = {};

   chain_extraction_impl_type<mock_logfile_provider_type> extraction_impl;
   
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
