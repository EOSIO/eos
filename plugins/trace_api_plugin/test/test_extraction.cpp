#define BOOST_TEST_MODULE trace_data_extraction
#include <boost/test/included/unit_test.hpp>

#include <eosio/chain/types.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/block_state.hpp>

#include <eosio/trace_api/test_common.hpp>
#include <eosio/trace_api/chain_extraction.hpp>

#include <fc/bitutil.hpp>

using namespace eosio;
using namespace eosio::trace_api;
using namespace eosio::trace_api::test_common;
using eosio::chain::name;
using eosio::chain::digest_type;

namespace {
   chain::transaction_trace_ptr make_transaction_trace( const chain::transaction_id_type& id, uint32_t block_number,
         uint32_t slot, chain::transaction_receipt_header::status_enum status, std::vector<chain::action_trace>&& actions ) {
      return std::make_shared<chain::transaction_trace>(chain::transaction_trace{
         id,
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

   chain::bytes make_onerror_data( const chain::onerror& one ) {
      fc::datastream<size_t> ps;
      fc::raw::pack(ps, one);
      chain::bytes result(ps.tellp());

      if( result.size() ) {
         fc::datastream<char*>  ds( result.data(), size_t(result.size()) );
         fc::raw::pack(ds, one);
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

   auto make_transfer_action( chain::name from, chain::name to, chain::asset quantity, std::string memo ) {
      return chain::action( std::vector<chain::permission_level> {{from, chain::config::active_name}},
                            "eosio.token"_n, "transfer"_n, make_transfer_data( from, to, quantity, std::move(memo) ) );
   }

   auto make_onerror_action( chain::name creator, chain::uint128_t sender_id ) {
      return chain::action( std::vector<chain::permission_level>{{creator, chain::config::active_name}},
                                chain::onerror{ sender_id, "test ", 4 });
   }

   auto make_packed_trx( std::vector<chain::action> actions ) {
      chain::signed_transaction trx;
      trx.actions = std::move( actions );
      return packed_transaction( trx );
   }

   chain::action_trace make_action_trace( uint64_t global_sequence, chain::action act, chain::name receiver ) {
      chain::action_trace result;
      // don't think we need any information other than receiver and global sequence
      result.receipt.emplace(chain::action_receipt{
         receiver,
         digest_type::hash(act),
         global_sequence,
         0,
         {},
         0,
         0
      });
      result.receiver = receiver;
      result.act = std::move(act);
      return result;
   }

   auto make_block_state( chain::block_id_type previous, uint32_t height, uint32_t slot, chain::name producer,
                          std::vector<chain::packed_transaction> trxs ) {
      chain::signed_block_ptr block = std::make_shared<chain::signed_block>();
      for( auto& trx : trxs ) {
         block->transactions.emplace_back( trx );
      }
      block->producer = producer;
      block->timestamp = chain::block_timestamp_type(slot);
      // make sure previous contains correct block # so block_header::block_num() returns correct value
      if( previous == chain::block_id_type() ) {
         previous._hash[0] &= 0xffffffff00000000;
         previous._hash[0] += fc::endian_reverse_u32(height - 1);
      }
      block->previous = previous;

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
      pbhs.timestamp = block->timestamp;
      chain::producer_authority_schedule schedule = {0, {chain::producer_authority{block->producer,
                                                                     chain::block_signing_authority_v0{1, {{pub_key, 1}}}}}};
      pbhs.active_schedule = schedule;
      pbhs.valid_block_signing_authority = chain::block_signing_authority_v0{1, {{pub_key, 1}}};
      auto bsp = std::make_shared<chain::block_state>(
            std::move( pbhs ),
            std::move( block ),
            eosio::chain::deque<chain::transaction_metadata_ptr>(),
            chain::protocol_feature_set(),
            []( chain::block_timestamp_type timestamp,
                const fc::flat_set<digest_type>& cur_features,
                const std::vector<digest_type>& new_features ) {},
            signer
      );
      bsp->block_num = height;

      return bsp;
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
       * append an entry to the data store
       *
       * @param entry : the entry to append
       */
      void append( const block_trace_v0& entry ) {
         fixture.data_log.emplace_back(entry);
      }

      void append_lib( uint32_t lib ) {
         fixture.max_lib = std::max(fixture.max_lib, lib);
      }

      extraction_test_fixture& fixture;
   };

   extraction_test_fixture()
   : extraction_impl(mock_logfile_provider_type(*this), exception_handler{} )
   {
   }

   void signal_applied_transaction( const chain::transaction_trace_ptr& trace, const chain::signed_transaction& strx ) {
      extraction_impl.signal_applied_transaction(trace, strx);
   }

   void signal_accepted_block( const chain::block_state_ptr& bsp ) {
      extraction_impl.signal_accepted_block(bsp);
   }

   // fixture data and methods
   uint32_t max_lib = 0;
   std::vector<data_log_entry> data_log = {};

   chain_extraction_impl_type<mock_logfile_provider_type> extraction_impl;
};


BOOST_AUTO_TEST_SUITE(block_extraction)

   BOOST_FIXTURE_TEST_CASE(basic_single_transaction_block, extraction_test_fixture)
   {
      auto act1 = make_transfer_action( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" );
      auto act2 = make_transfer_action( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" );
      auto act3 = make_transfer_action( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" );
      auto actt1 = make_action_trace( 0, act1, "eosio.token"_n );
      auto actt2 = make_action_trace( 1, act2, "alice"_n );
      auto actt3 = make_action_trace( 2, act3, "bob"_n );
      auto ptrx1 = make_packed_trx( { act1, act2, act3 } );

      // apply a basic transfer
      signal_applied_transaction(
            make_transaction_trace( ptrx1.id(), 1, 1, chain::transaction_receipt_header::executed,
                  { actt1, actt2, actt3 } ),
            ptrx1.get_signed_transaction() );
      
      // accept the block with one transaction
      auto bsp1 = make_block_state( chain::block_id_type(), 1, 1, "bp.one"_n,
            { chain::packed_transaction(ptrx1) } );
      signal_accepted_block( bsp1 );
      
      const uint32_t expected_lib = 0;
      const block_trace_v0 expected_trace { bsp1->id, 1, bsp1->prev(), chain::block_timestamp_type(1), "bp.one"_n,
         {
            {
               ptrx1.id(),
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
      BOOST_REQUIRE(data_log.size() == 1);
      BOOST_REQUIRE(data_log.at(0).contains<block_trace_v0>());
      BOOST_REQUIRE_EQUAL(data_log.at(0).get<block_trace_v0>(), expected_trace);
   }

   BOOST_FIXTURE_TEST_CASE(basic_multi_transaction_block, extraction_test_fixture) {
      auto act1 = make_transfer_action( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" );
      auto act2 = make_transfer_action( "bob"_n, "alice"_n, "0.0001 SYS"_t, "Memo!" );
      auto act3 = make_transfer_action( "fred"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" );
      auto actt1 = make_action_trace( 0, act1, "eosio.token"_n );
      auto actt2 = make_action_trace( 1, act2, "bob"_n );
      auto actt3 = make_action_trace( 2, act3, "fred"_n );
      auto ptrx1 = make_packed_trx( { act1 } );
      auto ptrx2 = make_packed_trx( { act2 } );
      auto ptrx3 = make_packed_trx( { act3 } );

      signal_applied_transaction(
            make_transaction_trace( ptrx1.id(), 1, 1, chain::transaction_receipt_header::executed,
                  { actt1 } ),
            ptrx1.get_signed_transaction() );
      signal_applied_transaction(
            make_transaction_trace( ptrx2.id(), 1, 1, chain::transaction_receipt_header::executed,
                  { actt2 } ),
            ptrx2.get_signed_transaction() );
      signal_applied_transaction(
            make_transaction_trace( ptrx3.id(), 1, 1, chain::transaction_receipt_header::executed,
                  { actt3 } ),
            ptrx3.get_signed_transaction() );

      // accept the block with three transaction
      auto bsp1 = make_block_state( chain::block_id_type(), 1, 1, "bp.one"_n,
            { chain::packed_transaction(ptrx1), chain::packed_transaction(ptrx2), chain::packed_transaction(ptrx3) } );
      signal_accepted_block( bsp1 );

      const uint32_t expected_lib = 0;
      const block_trace_v0 expected_trace { bsp1->id, 1, bsp1->prev(), chain::block_timestamp_type(1), "bp.one"_n,
         {
            {
               ptrx1.id(),
               {
                  {
                     0,
                     "eosio.token"_n, "eosio.token"_n, "transfer"_n,
                     {{ "alice"_n, "active"_n }},
                     make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  }
               }
            },
            {
               ptrx2.id(),
               {
                  {
                     1,
                     "bob"_n, "eosio.token"_n, "transfer"_n,
                     {{ "bob"_n, "active"_n }},
                     make_transfer_data( "bob"_n, "alice"_n, "0.0001 SYS"_t, "Memo!" )
                  }
               }
            },
            {
               ptrx3.id(),
               {
                  {
                     2,
                     "fred"_n, "eosio.token"_n, "transfer"_n,
                     {{ "fred"_n, "active"_n }},
                     make_transfer_data( "fred"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  }
               }
            }
         }
      };

      BOOST_REQUIRE_EQUAL(max_lib, 0);
      BOOST_REQUIRE(data_log.size() == 1);
      BOOST_REQUIRE(data_log.at(0).contains<block_trace_v0>());
      BOOST_REQUIRE_EQUAL(data_log.at(0).get<block_trace_v0>(), expected_trace);
   }

   BOOST_FIXTURE_TEST_CASE(onerror_transaction_block, extraction_test_fixture)
   {
      auto onerror_act = make_onerror_action( "alice"_n, 1 );
      auto actt1 = make_action_trace( 0, onerror_act, "eosio.token"_n );
      auto ptrx1 = make_packed_trx( { onerror_act } );

      auto act2 = make_transfer_action( "bob"_n, "alice"_n, "0.0001 SYS"_t, "Memo!" );
      auto actt2 = make_action_trace( 1, act2, "bob"_n );
      auto transfer_trx = make_packed_trx( { act2 } );

      auto onerror_trace = make_transaction_trace( ptrx1.id(), 1, 1, chain::transaction_receipt_header::executed,
                              { actt1 } );
      auto transfer_trace = make_transaction_trace( transfer_trx.id(), 1, 1, chain::transaction_receipt_header::soft_fail,
                                                   { actt2 } );
      onerror_trace->failed_dtrx_trace = transfer_trace;

      signal_applied_transaction( onerror_trace, transfer_trx.get_signed_transaction() );

      auto bsp1 = make_block_state( chain::block_id_type(), 1, 1, "bp.one"_n,
            { chain::packed_transaction(transfer_trx) } );
      signal_accepted_block( bsp1 );

      const uint32_t expected_lib = 0;
      const block_trace_v0 expected_trace { bsp1->id, 1, bsp1->prev(), chain::block_timestamp_type(1), "bp.one"_n,
         {
            {
               transfer_trx.id(), // transfer_trx.id() because that is the trx id known to the user
               {
                  {
                     0,
                     "eosio.token"_n, "eosio"_n, "onerror"_n,
                     {{ "alice"_n, "active"_n }},
                     make_onerror_data( chain::onerror{ 1, "test ", 4 } )
                  }
               }
            }
         }
      };

      BOOST_REQUIRE_EQUAL(max_lib, 0);
      BOOST_REQUIRE(data_log.size() == 1);
      BOOST_REQUIRE(data_log.at(0).contains<block_trace_v0>());
      BOOST_REQUIRE_EQUAL(data_log.at(0).get<block_trace_v0>(), expected_trace);
   }

BOOST_AUTO_TEST_SUITE_END()
