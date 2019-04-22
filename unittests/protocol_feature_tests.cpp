/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#include "fork_test_utilities.hpp"

using namespace eosio::chain;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(protocol_feature_tests)

BOOST_AUTO_TEST_CASE( activate_preactivate_feature ) try {
   tester c( setup_policy::none );
   const auto& pfm = c.control->get_protocol_feature_manager();

   c.produce_block();

   // Cannot set latest bios contract since it requires intrinsics that have not yet been whitelisted.
   BOOST_CHECK_EXCEPTION( c.set_code( config::system_account_name, contracts::eosio_bios_wasm() ),
                          wasm_exception, fc_exception_message_is("env.is_feature_activated unresolveable")
   );

   // But the old bios contract can still be set.
   c.set_code( config::system_account_name, contracts::before_preactivate_eosio_bios_wasm() );
   c.set_abi( config::system_account_name, contracts::before_preactivate_eosio_bios_abi().data() );

   auto t = c.control->pending_block_time();
   c.control->abort_block();
   BOOST_REQUIRE_EXCEPTION( c.control->start_block( t, 0, {digest_type()} ), protocol_feature_exception,
                            fc_exception_message_is( "protocol feature with digest '0000000000000000000000000000000000000000000000000000000000000000' is unrecognized" )
   );

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::preactivate_feature );

   BOOST_REQUIRE( d );

   // Activate PREACTIVATE_FEATURE.
   c.schedule_protocol_features_wo_preactivation({ *d });
   c.produce_block();

   // Now the latest bios contract can be set.
   c.set_code( config::system_account_name, contracts::eosio_bios_wasm() );
   c.set_abi( config::system_account_name, contracts::eosio_bios_abi().data() );

   c.produce_block();

   BOOST_CHECK_EXCEPTION( c.push_action( config::system_account_name, N(reqactivated), config::system_account_name,
                                          mutable_variant_object()("feature_digest",  digest_type()) ),
                           eosio_assert_message_exception,
                           eosio_assert_message_is( "protocol feature is not activated" )
   );

   c.push_action( config::system_account_name, N(reqactivated), config::system_account_name, mutable_variant_object()
      ("feature_digest",  *d )
   );

   c.produce_block();

   // Ensure validator node accepts the blockchain

   tester c2(setup_policy::none, db_read_mode::SPECULATIVE);
   push_blocks( c, c2 );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( activate_and_restart ) try {
   tester c( setup_policy::none );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto pfs = pfm.get_protocol_feature_set(); // make copy of protocol feature set

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::preactivate_feature );
   BOOST_REQUIRE( d );

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::preactivate_feature ) );

   // Activate PREACTIVATE_FEATURE.
   c.schedule_protocol_features_wo_preactivation({ *d });
   c.produce_blocks(2);

   auto head_block_num = c.control->head_block_num();

   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::preactivate_feature ) );

   c.close();
   c.open( std::move( pfs ), nullptr );

   BOOST_CHECK_EQUAL( head_block_num, c.control->head_block_num() );

   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::preactivate_feature ) );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( double_preactivation ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );
   BOOST_REQUIRE( d );

   c.push_action( config::system_account_name, N(activate), config::system_account_name,
                  fc::mutable_variant_object()("feature_digest", *d), 10 );

   std::string expected_error_msg("protocol feature with digest '");
   {
      fc::variant v;
      to_variant( *d, v );
      expected_error_msg += v.get_string();
      expected_error_msg += "' is already pre-activated";
   }

   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(activate), config::system_account_name,
                                          fc::mutable_variant_object()("feature_digest", *d), 20 ),
                           protocol_feature_exception,
                           fc_exception_message_is( expected_error_msg )
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( double_activation ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );
   BOOST_REQUIRE( d );

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.preactivate_protocol_features( {*d} );

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.schedule_protocol_features_wo_preactivation( {*d} );

   BOOST_CHECK_EXCEPTION(  c.produce_block();,
                           block_validate_exception,
                           fc_exception_message_starts_with( "attempted duplicate activation within a single block:" )
   );

   c.protocol_features_to_be_activated_wo_preactivation.clear();

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.produce_block();

   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.produce_block();

   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( require_preactivation_test ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );
   BOOST_REQUIRE( d );

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.schedule_protocol_features_wo_preactivation( {*d} );
   BOOST_CHECK_EXCEPTION( c.produce_block(),
                          protocol_feature_exception,
                          fc_exception_message_starts_with( "attempted to activate protocol feature without prior required preactivation:" )
   );

   c.protocol_features_to_be_activated_wo_preactivation.clear();

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.preactivate_protocol_features( {*d} );
   c.finish_block();

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   BOOST_CHECK_EXCEPTION( c.control->start_block(
                              c.control->head_block_time() + fc::milliseconds(config::block_interval_ms),
                              0,
                              {}
                          ),
                          block_validate_exception,
                          fc_exception_message_is( "There are pre-activated protocol features that were not activated at the start of this block" )
   );

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.produce_block();

   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( only_link_to_existing_permission_test ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );
   BOOST_REQUIRE( d );

   c.create_accounts( {N(alice), N(bob), N(charlie)} );

   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(linkauth), N(bob), fc::mutable_variant_object()
                              ("account", "bob")
                              ("code", name(config::system_account_name))
                              ("type", "")
                              ("requirement", "test" )
                           ), permission_query_exception,
                           fc_exception_message_is( "Failed to retrieve permission: test" )
   );

   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(linkauth), N(charlie), fc::mutable_variant_object()
                              ("account", "charlie")
                              ("code", name(config::system_account_name))
                              ("type", "")
                              ("requirement", "test" )
                           ), permission_query_exception,
                           fc_exception_message_is( "Failed to retrieve permission: test" )
   );

   c.push_action( config::system_account_name, N(updateauth), N(alice), fc::mutable_variant_object()
      ("account", "alice")
      ("permission", "test")
      ("parent", "active")
      ("auth", authority(get_public_key("testapi", "test")))
   );

   c.produce_block();

   // Verify the incorrect behavior prior to ONLY_LINK_TO_EXISTING_PERMISSION activation.
   c.push_action( config::system_account_name, N(linkauth), N(bob), fc::mutable_variant_object()
      ("account", "bob")
      ("code", name(config::system_account_name))
      ("type", "")
      ("requirement", "test" )
   );

   c.preactivate_protocol_features( {*d} );
   c.produce_block();

   // Verify the correct behavior after ONLY_LINK_TO_EXISTING_PERMISSION activation.
   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(linkauth), N(charlie), fc::mutable_variant_object()
                              ("account", "charlie")
                              ("code", name(config::system_account_name))
                              ("type", "")
                              ("requirement", "test" )
                           ), permission_query_exception,
                           fc_exception_message_is( "Failed to retrieve permission: test" )
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( subjective_restrictions_test ) try {
   tester c( setup_policy::none );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto restart_with_new_pfs = [&c]( protocol_feature_set&& pfs ) {
      c.close();
      c.open(std::move(pfs), nullptr);
   };

   auto get_builtin_digest = [&pfm]( builtin_protocol_feature_t codename ) -> digest_type {
      auto res = pfm.get_builtin_digest( codename );
      BOOST_REQUIRE( res );
      return *res;
   };

   auto preactivate_feature_digest = get_builtin_digest( builtin_protocol_feature_t::preactivate_feature );
   auto only_link_to_existing_permission_digest = get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );

   auto invalid_act_time = fc::time_point::from_iso_string( "2200-01-01T00:00:00" );
   auto valid_act_time = fc::time_point{};

   // First, test subjective_restrictions on feature that can be activated WITHOUT preactivation (PREACTIVATE_FEATURE)

   c.schedule_protocol_features_wo_preactivation({ preactivate_feature_digest });
   // schedule PREACTIVATE_FEATURE activation (persists until next successful start_block)

   subjective_restriction_map custom_subjective_restrictions = {
      { builtin_protocol_feature_t::preactivate_feature, {invalid_act_time, false, true} }
   };
   restart_with_new_pfs( make_protocol_feature_set(custom_subjective_restrictions) );
   // When a block is produced, the protocol feature activation should fail and throw an error
   BOOST_CHECK_EXCEPTION(  c.produce_block(),
                           protocol_feature_exception,
                           fc_exception_message_starts_with(
                              std::string(c.control->head_block_time()) +
                              " is too early for the earliest allowed activation time of the protocol feature"
                           )
   );
   BOOST_CHECK_EQUAL( c.protocol_features_to_be_activated_wo_preactivation.size(), 1 );

   // Revert to the valid earliest allowed activation time, however with enabled == false
   custom_subjective_restrictions = {
      { builtin_protocol_feature_t::preactivate_feature, {valid_act_time, false, false} }
   };
   restart_with_new_pfs( make_protocol_feature_set(custom_subjective_restrictions) );
   // This should also fail, but with different exception
   BOOST_CHECK_EXCEPTION(  c.produce_block(),
                           protocol_feature_exception,
                           fc_exception_message_is(
                              std::string("protocol feature with digest '") +
                              std::string(preactivate_feature_digest) +
                              "' is disabled"
                           )
   );
   BOOST_CHECK_EQUAL( c.protocol_features_to_be_activated_wo_preactivation.size(), 1 );

   // Revert to the valid earliest allowed activation time, however with subjective_restrictions enabled == true
   custom_subjective_restrictions = {
      { builtin_protocol_feature_t::preactivate_feature, {valid_act_time, false, true} }
   };
   restart_with_new_pfs( make_protocol_feature_set(custom_subjective_restrictions) );
   // Now it should be fine, the feature should be activated after the block is produced
   BOOST_CHECK_NO_THROW( c.produce_block() );
   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::preactivate_feature ) );
   BOOST_CHECK_EQUAL( c.protocol_features_to_be_activated_wo_preactivation.size(), 0 );

   // Second, test subjective_restrictions on feature that need to be activated WITH preactivation (ONLY_LINK_TO_EXISTING_PERMISSION)

   c.set_bios_contract();
   c.produce_block();

   custom_subjective_restrictions = {
      { builtin_protocol_feature_t::only_link_to_existing_permission, {invalid_act_time, true, true} }
   };
   restart_with_new_pfs( make_protocol_feature_set(custom_subjective_restrictions) );
   // It should fail
   BOOST_CHECK_EXCEPTION(  c.preactivate_protocol_features({only_link_to_existing_permission_digest}),
                           subjective_block_production_exception,
                           fc_exception_message_starts_with(
                              std::string(c.control->head_block_time() + fc::milliseconds(config::block_interval_ms)) +
                              " is too early for the earliest allowed activation time of the protocol feature"
                           )
   );

   // Revert with valid time and subjective_restrictions enabled == false
   custom_subjective_restrictions = {
      { builtin_protocol_feature_t::only_link_to_existing_permission, {valid_act_time, true, false} }
   };
   restart_with_new_pfs( make_protocol_feature_set(custom_subjective_restrictions) );
   // It should fail but with different exception
   BOOST_CHECK_EXCEPTION(  c.preactivate_protocol_features({only_link_to_existing_permission_digest}),
                           subjective_block_production_exception,
                           fc_exception_message_is(
                              std::string("protocol feature with digest '") +
                              std::string(only_link_to_existing_permission_digest)+
                              "' is disabled"
                           )
   );

   // Revert with valid time and subjective_restrictions enabled == true
   custom_subjective_restrictions = {
      { builtin_protocol_feature_t::only_link_to_existing_permission, {valid_act_time, true, true} }
   };
   restart_with_new_pfs( make_protocol_feature_set(custom_subjective_restrictions) );
   // Should be fine now, and activated in the next block
   BOOST_CHECK_NO_THROW( c.preactivate_protocol_features({only_link_to_existing_permission_digest}) );
   c.produce_block();
   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( replace_deferred_test ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );

   c.create_accounts( {N(alice), N(bob), N(test)} );
   c.set_code( N(test), contracts::deferred_test_wasm() );
   c.set_abi( N(test), contracts::deferred_test_abi().data() );
   c.produce_block();

   auto alice_ram_usage0 = c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) );

   c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 42)
      ("contract", "test")
      ("payload", 100)
   );

   auto alice_ram_usage1 = c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) );

   // Verify subjective mitigation is in place
   BOOST_CHECK_EXCEPTION(
      c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
                        ("payer", "alice")
                        ("sender_id", 42)
                        ("contract", "test")
                        ("payload", 101 )
                   ),
      subjective_block_production_exception,
      fc_exception_message_is( "Replacing a deferred transaction is temporarily disabled." )
   );

   BOOST_CHECK_EQUAL( c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) ), alice_ram_usage1 );

   c.control->abort_block();

   c.close();
   auto cfg = c.get_config();
   cfg.disable_all_subjective_mitigations = true;
   c.init( cfg, nullptr );

   BOOST_CHECK_EQUAL( c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) ), alice_ram_usage0 );

   c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 42)
      ("contract", "test")
      ("payload", 100)
   );

   BOOST_CHECK_EQUAL( c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) ), alice_ram_usage1 );
   auto dtrxs = c.get_scheduled_transactions();
   BOOST_CHECK_EQUAL( dtrxs.size(), 1 );
   auto first_dtrx_id = dtrxs[0];

   // With the subjective mitigation disabled, replacing the deferred transaction is allowed.
   c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 42)
      ("contract", "test")
      ("payload", 101)
   );

   auto alice_ram_usage2 = c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) );
   BOOST_CHECK_EQUAL( alice_ram_usage2, alice_ram_usage1 + (alice_ram_usage1 - alice_ram_usage0) );

   dtrxs = c.get_scheduled_transactions();
   BOOST_CHECK_EQUAL( dtrxs.size(), 1 );
   BOOST_CHECK_EQUAL( first_dtrx_id, dtrxs[0] ); // Incorrectly kept as the old transaction ID.

   c.produce_block();

   auto alice_ram_usage3 = c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) );
   BOOST_CHECK_EQUAL( alice_ram_usage3, alice_ram_usage1 );

   dtrxs = c.get_scheduled_transactions();
   BOOST_CHECK_EQUAL( dtrxs.size(), 0 );

   c.produce_block();

   c.close();
   cfg.disable_all_subjective_mitigations = false;
   c.init( cfg, nullptr );

   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::replace_deferred );
   BOOST_REQUIRE( d );

   c.preactivate_protocol_features( {*d} );
   c.produce_block();

   BOOST_CHECK_EQUAL( c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) ), alice_ram_usage0 );

   c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 42)
      ("contract", "test")
      ("payload", 100)
   );

   BOOST_CHECK_EQUAL( c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) ), alice_ram_usage1 );

   dtrxs = c.get_scheduled_transactions();
   BOOST_CHECK_EQUAL( dtrxs.size(), 1 );
   auto first_dtrx_id2 = dtrxs[0];

   // With REPLACE_DEFERRED activated, replacing the deferred transaction is allowed and now should work properly.
   c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 42)
      ("contract", "test")
      ("payload", 101)
   );

   BOOST_CHECK_EQUAL( c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) ), alice_ram_usage1 );

   dtrxs = c.get_scheduled_transactions();
   BOOST_CHECK_EQUAL( dtrxs.size(), 1 );
   BOOST_CHECK( first_dtrx_id2 != dtrxs[0] );

   // Replace again with a deferred transaction identical to the first one
   c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 42)
      ("contract", "test")
      ("payload", 100),
      100 // Needed to make this input transaction unique
   );

   BOOST_CHECK_EQUAL( c.control->get_resource_limits_manager().get_account_ram_usage( N(alice) ), alice_ram_usage1 );

   dtrxs = c.get_scheduled_transactions();
   BOOST_CHECK_EQUAL( dtrxs.size(), 1 );
   BOOST_CHECK_EQUAL( first_dtrx_id2, dtrxs[0] );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( no_duplicate_deferred_id_test ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   tester c2( setup_policy::none );

   c.create_accounts( {N(alice), N(test)} );
   c.set_code( N(test), contracts::deferred_test_wasm() );
   c.set_abi( N(test), contracts::deferred_test_abi().data() );
   c.produce_block();

   push_blocks( c, c2 );

   c2.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 1)
      ("contract", "test")
      ("payload", 50)
   );

   c2.finish_block();

   BOOST_CHECK_EXCEPTION(
      c2.produce_block(),
      fc::exception,
      fc_exception_message_is( "no transaction extensions supported yet for deferred transactions" )
   );

   c2.produce_empty_block( fc::minutes(10) );

   transaction_trace_ptr trace0;
   auto h2 = c2.control->applied_transaction.connect( [&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
      auto& t = std::get<0>(x);
      if( t && t->receipt && t->receipt->status == transaction_receipt::expired) {
         trace0 = t;
      }
   } );

   c2.produce_block();

   h2.disconnect();

   BOOST_REQUIRE( trace0 );

   c.produce_block();

   const auto& index = c.control->db().get_index<generated_transaction_multi_index,by_trx_id>();

   transaction_trace_ptr trace1;
   auto h = c.control->applied_transaction.connect( [&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
      auto& t = std::get<0>(x);
      if( t && t->receipt && t->receipt->status == transaction_receipt::executed) {
         trace1 = t;
      }
   } );

   BOOST_REQUIRE_EQUAL(0, index.size());

   c.push_action( config::system_account_name, N(reqauth), N(alice), fc::mutable_variant_object()
      ("from", "alice"),
      5, 2
   );

   BOOST_REQUIRE_EQUAL(1, index.size());

   c.produce_block();

   BOOST_REQUIRE_EQUAL(1, index.size());

   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d1 = pfm.get_builtin_digest( builtin_protocol_feature_t::replace_deferred );
   BOOST_REQUIRE( d1 );
   auto d2 = pfm.get_builtin_digest( builtin_protocol_feature_t::no_duplicate_deferred_id );
   BOOST_REQUIRE( d2 );

   c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 1)
      ("contract", "test")
      ("payload", 42)
   );
   BOOST_REQUIRE_EQUAL(2, index.size());

   c.preactivate_protocol_features( {*d1, *d2} );
   c.produce_block();
   // The deferred transaction with payload 42 that was scheduled prior to the activation of the protocol features should now be retired.

   BOOST_REQUIRE( trace1 );
   BOOST_REQUIRE_EQUAL(1, index.size());

   trace1 = nullptr;

   // Retire the delayed eosio::reqauth transaction.
   c.produce_blocks(5);
   BOOST_REQUIRE( trace1 );
   BOOST_REQUIRE_EQUAL(0, index.size());

   h.disconnect();

   auto check_generation_context = []( auto&& data,
                                       const transaction_id_type& sender_trx_id,
                                       unsigned __int128 sender_id,
                                       account_name sender )
   {
      transaction trx;
      fc::datastream<const char*> ds1( data.data(), data.size() );
      fc::raw::unpack( ds1, trx );
      BOOST_REQUIRE_EQUAL( trx.transaction_extensions.size(), 1 );
      BOOST_REQUIRE_EQUAL( trx.transaction_extensions.back().first, 0 );

      fc::datastream<const char*> ds2( trx.transaction_extensions.back().second.data(),
                                       trx.transaction_extensions.back().second.size() );

      transaction_id_type actual_sender_trx_id;
      fc::raw::unpack( ds2, actual_sender_trx_id );
      BOOST_CHECK_EQUAL( actual_sender_trx_id, sender_trx_id );

      unsigned __int128 actual_sender_id;
      fc::raw::unpack( ds2, actual_sender_id );
      BOOST_CHECK( actual_sender_id == sender_id );

      uint64_t actual_sender;
      fc::raw::unpack( ds2, actual_sender );
      BOOST_CHECK_EQUAL( account_name(actual_sender), sender );
   };

   BOOST_CHECK_EXCEPTION(
      c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
                        ("payer", "alice")
                        ("sender_id", 1)
                        ("contract", "test")
                        ("payload", 77 )
                   ),
      ill_formed_deferred_transaction_generation_context,
      fc_exception_message_is( "deferred transaction generaction context contains mismatching sender" )
   );

   BOOST_REQUIRE_EQUAL(0, index.size());

   auto trace2 = c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 1)
      ("contract", "test")
      ("payload", 40)
   );

   BOOST_REQUIRE_EQUAL(1, index.size());

   check_generation_context( index.begin()->packed_trx,
                             trace2->id,
                             ((static_cast<unsigned __int128>(N(alice)) << 64) | 1),
                             N(test) );

   c.produce_block();

   BOOST_REQUIRE_EQUAL(0, index.size());

   auto trace3 = c.push_action( N(test), N(defercall), N(alice), fc::mutable_variant_object()
      ("payer", "alice")
      ("sender_id", 1)
      ("contract", "test")
      ("payload", 50)
   );

   BOOST_REQUIRE_EQUAL(1, index.size());

   check_generation_context( index.begin()->packed_trx,
                             trace3->id,
                             ((static_cast<unsigned __int128>(N(alice)) << 64) | 1),
                             N(test) );

   c.produce_block();

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( fix_linkauth_restriction ) { try {
   tester chain( setup_policy::preactivate_feature_and_new_bios );

   const auto& tester_account = N(tester);

   chain.produce_blocks();
   chain.create_account(N(currency));
   chain.create_account(tester_account);
   chain.produce_blocks();

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", name(tester_account).to_string())
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 5))
   );

   auto validate_disallow = [&] (const char *code, const char *type) {
      BOOST_REQUIRE_EXCEPTION(
         chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
               ("account", name(tester_account).to_string())
               ("code", code)
               ("type", type)
               ("requirement", "first")),
         action_validate_exception,
         fc_exception_message_is(std::string("Cannot link eosio::") + std::string(type) + std::string(" to a minimum permission"))
      );
   };

   validate_disallow("eosio", "linkauth");
   validate_disallow("eosio", "unlinkauth");
   validate_disallow("eosio", "deleteauth");
   validate_disallow("eosio", "updateauth");
   validate_disallow("eosio", "canceldelay");

   validate_disallow("currency", "linkauth");
   validate_disallow("currency", "unlinkauth");
   validate_disallow("currency", "deleteauth");
   validate_disallow("currency", "updateauth");
   validate_disallow("currency", "canceldelay");

   const auto& pfm = chain.control->get_protocol_feature_manager();
   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::fix_linkauth_restriction );
   BOOST_REQUIRE( d );

   chain.preactivate_protocol_features( {*d} );
   chain.produce_block();

   auto validate_allowed = [&] (const char *code, const char *type) {
     chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
            ("account", name(tester_account).to_string())
            ("code", code)
            ("type", type)
            ("requirement", "first"));
   };

   validate_disallow("eosio", "linkauth");
   validate_disallow("eosio", "unlinkauth");
   validate_disallow("eosio", "deleteauth");
   validate_disallow("eosio", "updateauth");
   validate_disallow("eosio", "canceldelay");

   validate_allowed("currency", "linkauth");
   validate_allowed("currency", "unlinkauth");
   validate_allowed("currency", "deleteauth");
   validate_allowed("currency", "updateauth");
   validate_allowed("currency", "canceldelay");

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( disallow_empty_producer_schedule_test ) { try {
   tester c( setup_policy::preactivate_feature_and_new_bios );

   const auto& pfm = c.control->get_protocol_feature_manager();
   const auto& d = pfm.get_builtin_digest( builtin_protocol_feature_t::disallow_empty_producer_schedule );
   BOOST_REQUIRE( d );

   // Before activation, it is allowed to set empty producer schedule
   c.set_producers( {} );

   // After activation, it should not be allowed
   c.preactivate_protocol_features( {*d} );
   c.produce_block();
   BOOST_REQUIRE_EXCEPTION( c.set_producers( {} ),
                            wasm_execution_error,
                            fc_exception_message_is( "Producer schedule cannot be empty" ) );

   // Setting non empty producer schedule should still be fine
   vector<name> producer_names = {N(alice),N(bob),N(carol)};
   c.create_accounts( producer_names );
   c.set_producers( producer_names );
   c.produce_blocks(2);
   const auto& schedule = c.get_producer_keys( producer_names );
   BOOST_CHECK( std::equal( schedule.begin(), schedule.end(), c.control->active_producers().producers.begin()) );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( restrict_action_to_self_test ) { try {
   tester c( setup_policy::preactivate_feature_and_new_bios );

   const auto& pfm = c.control->get_protocol_feature_manager();
   const auto& d = pfm.get_builtin_digest( builtin_protocol_feature_t::restrict_action_to_self );
   BOOST_REQUIRE( d );

   c.create_accounts( {N(testacc), N(acctonotify), N(alice)} );
   c.set_code( N(testacc), contracts::restrict_action_test_wasm() );
   c.set_abi( N(testacc), contracts::restrict_action_test_abi().data() );

   c.set_code( N(acctonotify), contracts::restrict_action_test_wasm() );
   c.set_abi( N(acctonotify), contracts::restrict_action_test_abi().data() );

   // Before the protocol feature is preactivated
   // - Sending inline action to self = no problem
   // - Sending deferred trx to self = throw subjective exception
   // - Sending inline action to self from notification = throw subjective exception
   // - Sending deferred trx to self from notification = throw subjective exception
   BOOST_CHECK_NO_THROW( c.push_action( N(testacc), N(sendinline), N(alice), mutable_variant_object()("authorizer", "alice")) );
   BOOST_REQUIRE_EXCEPTION( c.push_action( N(testacc), N(senddefer), N(alice),
                                           mutable_variant_object()("authorizer", "alice")("senderid", 0)),
                            subjective_block_production_exception,
                            fc_exception_message_starts_with( "Authorization failure with sent deferred transaction" ) );

   BOOST_REQUIRE_EXCEPTION( c.push_action( N(testacc), N(notifyinline), N(alice),
                                        mutable_variant_object()("acctonotify", "acctonotify")("authorizer", "alice")),
                            subjective_block_production_exception,
                            fc_exception_message_starts_with( "Authorization failure with inline action sent to self" ) );

   BOOST_REQUIRE_EXCEPTION( c.push_action( N(testacc), N(notifydefer), N(alice),
                                           mutable_variant_object()("acctonotify", "acctonotify")("authorizer", "alice")("senderid", 1)),
                            subjective_block_production_exception,
                            fc_exception_message_starts_with( "Authorization failure with sent deferred transaction" ) );

   c.preactivate_protocol_features( {*d} );
   c.produce_block();

   // After the protocol feature is preactivated, all the 4 cases will throw an objective unsatisfied_authorization exception
   BOOST_REQUIRE_EXCEPTION( c.push_action( N(testacc), N(sendinline), N(alice), mutable_variant_object()("authorizer", "alice") ),
                            unsatisfied_authorization,
                            fc_exception_message_starts_with( "transaction declares authority" ) );

   BOOST_REQUIRE_EXCEPTION( c.push_action( N(testacc), N(senddefer), N(alice),
                                           mutable_variant_object()("authorizer", "alice")("senderid", 3)),
                            unsatisfied_authorization,
                            fc_exception_message_starts_with( "transaction declares authority" ) );

   BOOST_REQUIRE_EXCEPTION( c.push_action( N(testacc), N(notifyinline), N(alice),
                                           mutable_variant_object()("acctonotify", "acctonotify")("authorizer", "alice") ),
                            unsatisfied_authorization,
                            fc_exception_message_starts_with( "transaction declares authority" ) );

   BOOST_REQUIRE_EXCEPTION( c.push_action( N(testacc), N(notifydefer), N(alice),
                                           mutable_variant_object()("acctonotify", "acctonotify")("authorizer", "alice")("senderid", 4)),
                            unsatisfied_authorization,
                            fc_exception_message_starts_with( "transaction declares authority" ) );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( only_bill_to_first_authorizer ) { try {
   tester chain( setup_policy::preactivate_feature_and_new_bios );

   const auto& tester_account = N(tester);
   const auto& tester_account2 = N(tester2);

   chain.produce_blocks();
   chain.create_account(tester_account);
   chain.create_account(tester_account2);

   chain.push_action(config::system_account_name, N(setalimits), config::system_account_name, fc::mutable_variant_object()
      ("account", name(tester_account).to_string())
      ("ram_bytes", 10000)
      ("net_weight", 1000)
      ("cpu_weight", 1000));

   chain.push_action(config::system_account_name, N(setalimits), config::system_account_name, fc::mutable_variant_object()
      ("account", name(tester_account2).to_string())
      ("ram_bytes", 10000)
      ("net_weight", 1000)
      ("cpu_weight", 1000));

   const resource_limits_manager& mgr = chain.control->get_resource_limits_manager();

   chain.produce_blocks();

   {
      action act;
      act.account = tester_account;
      act.name = N(null);
      act.authorization = vector<permission_level>{
         {tester_account, config::active_name},
         {tester_account2, config::active_name}
      };

      signed_transaction trx;
      trx.actions.emplace_back(std::move(act));
      chain.set_transaction_headers(trx);

      trx.sign(get_private_key(tester_account, "active"), chain.control->get_chain_id());
      trx.sign(get_private_key(tester_account2, "active"), chain.control->get_chain_id());


      auto tester_cpu_limit0  = mgr.get_account_cpu_limit_ex(tester_account);
      auto tester2_cpu_limit0 = mgr.get_account_cpu_limit_ex(tester_account2);
      auto tester_net_limit0  = mgr.get_account_net_limit_ex(tester_account);
      auto tester2_net_limit0 = mgr.get_account_net_limit_ex(tester_account2);

      chain.push_transaction(trx);

      auto tester_cpu_limit1  = mgr.get_account_cpu_limit_ex(tester_account);
      auto tester2_cpu_limit1 = mgr.get_account_cpu_limit_ex(tester_account2);
      auto tester_net_limit1  = mgr.get_account_net_limit_ex(tester_account);
      auto tester2_net_limit1 = mgr.get_account_net_limit_ex(tester_account2);

      BOOST_CHECK(tester_cpu_limit1.used > tester_cpu_limit0.used);
      BOOST_CHECK(tester2_cpu_limit1.used > tester2_cpu_limit0.used);
      BOOST_CHECK(tester_net_limit1.used > tester_net_limit0.used);
      BOOST_CHECK(tester2_net_limit1.used > tester2_net_limit0.used);

      BOOST_CHECK_EQUAL(tester_cpu_limit1.used - tester_cpu_limit0.used, tester2_cpu_limit1.used - tester2_cpu_limit0.used);
      BOOST_CHECK_EQUAL(tester_net_limit1.used - tester_net_limit0.used, tester2_net_limit1.used - tester2_net_limit0.used);
   }

   const auto& pfm = chain.control->get_protocol_feature_manager();
   const auto& d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_bill_first_authorizer );
   BOOST_REQUIRE( d );

   chain.preactivate_protocol_features( {*d} );
   chain.produce_blocks();

   {
      action act;
      act.account = tester_account;
      act.name = N(null2);
      act.authorization = vector<permission_level>{
         {tester_account, config::active_name},
         {tester_account2, config::active_name}
      };

      signed_transaction trx;
      trx.actions.emplace_back(std::move(act));
      chain.set_transaction_headers(trx);

      trx.sign(get_private_key(tester_account, "active"), chain.control->get_chain_id());
      trx.sign(get_private_key(tester_account2, "active"), chain.control->get_chain_id());

      auto tester_cpu_limit0  = mgr.get_account_cpu_limit_ex(tester_account);
      auto tester2_cpu_limit0 = mgr.get_account_cpu_limit_ex(tester_account2);
      auto tester_net_limit0  = mgr.get_account_net_limit_ex(tester_account);
      auto tester2_net_limit0 = mgr.get_account_net_limit_ex(tester_account2);

      chain.push_transaction(trx);

      auto tester_cpu_limit1  = mgr.get_account_cpu_limit_ex(tester_account);
      auto tester2_cpu_limit1 = mgr.get_account_cpu_limit_ex(tester_account2);
      auto tester_net_limit1  = mgr.get_account_net_limit_ex(tester_account);
      auto tester2_net_limit1 = mgr.get_account_net_limit_ex(tester_account2);

      BOOST_CHECK(tester_cpu_limit1.used > tester_cpu_limit0.used);
      BOOST_CHECK(tester2_cpu_limit1.used == tester2_cpu_limit0.used);
      BOOST_CHECK(tester_net_limit1.used > tester_net_limit0.used);
      BOOST_CHECK(tester2_net_limit1.used == tester2_net_limit0.used);
   }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( forward_setcode_test ) { try {
   tester c( setup_policy::preactivate_feature_only );

   const auto& tester1_account = N(tester1);
   const auto& tester2_account = N(tester2);
   c.create_accounts( {tester1_account, tester2_account} );

   // Deploy contract that rejects all actions dispatched to it with the following exceptions:
   //   * eosio::setcode to set code on the eosio is allowed (unless the rejectall account exists)
   //   * eosio::newaccount is allowed only if it creates the rejectall account.
   c.set_code( config::system_account_name, contracts::reject_all_wasm() );
   c.produce_block();

   // Before activation, deploying a contract should work since setcode won't be forwarded to the WASM on eosio.
   c.set_code( tester1_account, contracts::noop_wasm() );

   // Activate FORWARD_SETCODE protocol feature and then return contract on eosio back to what it was.
   const auto& pfm = c.control->get_protocol_feature_manager();
   const auto& d = pfm.get_builtin_digest( builtin_protocol_feature_t::forward_setcode );
   BOOST_REQUIRE( d );
   c.set_bios_contract();
   c.preactivate_protocol_features( {*d} );
   c.produce_block();
   c.set_code( config::system_account_name, contracts::reject_all_wasm() );
   c.produce_block();

   // After activation, deploying a contract causes setcode to be dispatched to the WASM on eosio,
   // and in this case the contract is configured to reject the setcode action.
   BOOST_REQUIRE_EXCEPTION( c.set_code( tester2_account, contracts::noop_wasm() ),
                            eosio_assert_message_exception,
                            eosio_assert_message_is( "rejecting all actions" ) );


   tester c2(setup_policy::none);
   push_blocks( c, c2 ); // make a backup of the chain to enable testing further conditions.

   c.set_bios_contract(); // To allow pushing further actions for setting up the other part of the test.
   c.create_account( N(rejectall) );
   c.produce_block();
   // The existence of the rejectall account will make the reject_all contract reject all actions with no exception.

   // It will now not be possible to deploy the reject_all contract to the eosio account,
   // because after it is set by the native function, it is called immediately after which will reject the transaction.
   BOOST_REQUIRE_EXCEPTION( c.set_code( config::system_account_name, contracts::reject_all_wasm() ),
                            eosio_assert_message_exception,
                            eosio_assert_message_is( "rejecting all actions" ) );


   // Going back to the backup chain, we can create the rejectall account while the reject_all contract is
   // already deployed on eosio.
   c2.create_account( N(rejectall) );
   c2.produce_block();
   // Now all actions dispatched to the eosio account should be rejected.

   // However, it should still be possible to set the bios contract because the WASM on eosio is called after the
   // native setcode function completes.
   c2.set_bios_contract();
   c2.produce_block();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( get_sender_test ) { try {
   tester c( setup_policy::preactivate_feature_and_new_bios );

   const auto& tester1_account = account_name("tester1");
   const auto& tester2_account = account_name("tester2");
   c.create_accounts( {tester1_account, tester2_account} );
   c.produce_block();

   BOOST_CHECK_EXCEPTION(  c.set_code( tester1_account, contracts::get_sender_test_wasm() ),
                           wasm_exception,
                           fc_exception_message_is( "env.get_sender unresolveable" ) );

   const auto& pfm = c.control->get_protocol_feature_manager();
   const auto& d = pfm.get_builtin_digest( builtin_protocol_feature_t::get_sender );
   BOOST_REQUIRE( d );

   c.preactivate_protocol_features( {*d} );
   c.produce_block();

   c.set_code( tester1_account, contracts::get_sender_test_wasm() );
   c.set_abi( tester1_account, contracts::get_sender_test_abi().data() );
   c.set_code( tester2_account, contracts::get_sender_test_wasm() );
   c.set_abi( tester2_account, contracts::get_sender_test_abi().data() );
   c.produce_block();

   BOOST_CHECK_EXCEPTION(  c.push_action( tester1_account, N(sendinline), tester1_account, mutable_variant_object()
                                             ("to", tester2_account.to_string())
                                             ("expected_sender", account_name{}) ),
                           eosio_assert_message_exception,
                           eosio_assert_message_is( "sender did not match" ) );

   c.push_action( tester1_account, N(sendinline), tester1_account, mutable_variant_object()
      ("to", tester2_account.to_string())
      ("expected_sender", tester1_account.to_string())
   );

   c.push_action( tester1_account, N(notify), tester1_account, mutable_variant_object()
      ("to", tester2_account.to_string())
      ("expected_sender", tester1_account.to_string())
      ("send_inline", false)
   );

   c.push_action( tester1_account, N(notify), tester1_account, mutable_variant_object()
      ("to", tester2_account.to_string())
      ("expected_sender", tester2_account.to_string())
      ("send_inline", true)
   );

   c.push_action( tester1_account, N(assertsender), tester1_account, mutable_variant_object()
      ("expected_sender", account_name{})
   );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( ram_restrictions_test ) { try {
   tester c( setup_policy::preactivate_feature_and_new_bios );

   const auto& tester1_account = account_name("tester1");
   const auto& tester2_account = account_name("tester2");
   const auto& alice_account = account_name("alice");
   const auto& bob_account = account_name("bob");
   c.create_accounts( {tester1_account, tester2_account, alice_account, bob_account} );
   c.produce_block();
   c.set_code( tester1_account, contracts::ram_restrictions_test_wasm() );
   c.set_abi( tester1_account, contracts::ram_restrictions_test_abi().data() );
   c.produce_block();
   c.set_code( tester2_account, contracts::ram_restrictions_test_wasm() );
   c.set_abi( tester2_account, contracts::ram_restrictions_test_abi().data() );
   c.produce_block();

   // Basic setup
   c.push_action( tester1_account, N(setdata), alice_account, mutable_variant_object()
      ("len1", 10)
      ("len2", 0)
      ("payer", alice_account)
   );

   // Cannot bill more RAM to another account that has not authorized the action.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester1_account, N(setdata), bob_account, mutable_variant_object()
         ("len1", 20)
         ("len2", 0)
         ("payer", alice_account)
      ),
      missing_auth_exception,
      fc_exception_message_starts_with( "missing authority" )
   );

   // Cannot migrate data from table1 to table2 paid by another account
   // in a RAM usage neutral way without the authority of that account.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester1_account, N(setdata), bob_account, mutable_variant_object()
         ("len1", 0)
         ("len2", 10)
         ("payer", alice_account)
      ),
      missing_auth_exception,
      fc_exception_message_starts_with( "missing authority" )
   );

   // Cannot bill more RAM to another account within a notification
   // even if the account authorized the original action.
   // This is due to the subjective mitigation in place.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester2_account, N(notifysetdat), alice_account, mutable_variant_object()
         ("acctonotify", tester1_account)
         ("len1", 20)
         ("len2", 0)
         ("payer", alice_account)
      ),
      subjective_block_production_exception,
      fc_exception_message_is( "Cannot charge RAM to other accounts during notify." )
   );

   // Cannot migrate data from table1 to table2 paid by another account
   // in a RAM usage neutral way within a notification.
   // This is due to the subjective mitigation in place.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester2_account, N(notifysetdat), alice_account, mutable_variant_object()
         ("acctonotify", tester1_account)
         ("len1", 0)
         ("len2", 10)
         ("payer", alice_account)
      ),
      subjective_block_production_exception,
      fc_exception_message_is( "Cannot charge RAM to other accounts during notify." )
   );

   // Cannot send deferred transaction paid by another account that has not authorized the action.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester1_account, N(senddefer), bob_account, mutable_variant_object()
         ("senderid", 123)
         ("payer", alice_account)
      ),
      missing_auth_exception,
      fc_exception_message_starts_with( "missing authority" )
   );

   // Cannot send deferred transaction paid by another account within a notification
   // even if the account authorized the original action.
   // This is due to the subjective mitigation in place.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester2_account, N(notifydefer), alice_account, mutable_variant_object()
         ("acctonotify", tester1_account)
         ("senderid", 123)
         ("payer", alice_account)
      ),
      subjective_block_production_exception,
      fc_exception_message_is( "Cannot charge RAM to other accounts during notify." )
   );

   // Can send deferred transaction paid by another account if it has authorized the action.
   c.push_action( tester1_account, N(senddefer), alice_account, mutable_variant_object()
      ("senderid", 123)
      ("payer", alice_account)
   );
   c.produce_block();

   // Can migrate data from table1 to table2 paid by another account
   // in a RAM usage neutral way with the authority of that account.
   c.push_action( tester1_account, N(setdata), alice_account, mutable_variant_object()
      ("len1", 0)
      ("len2", 10)
      ("payer", alice_account)
   );

   c.produce_block();

   // Disable the subjective mitigation
   c.close();
   auto cfg = c.get_config();
   cfg.disable_all_subjective_mitigations = true;
   c.init( cfg, nullptr );

   c.produce_block();

   // Without the subjective mitigation, it is now possible to bill more RAM to another account
   // within a notification if the account authorized the original action.
   // This is due to the subjective mitigation in place.
   c.push_action( tester2_account, N(notifysetdat), alice_account, mutable_variant_object()
      ("acctonotify", tester1_account)
      ("len1", 10)
      ("len2", 10)
      ("payer", alice_account)
   );

   // Reset back to the original state.
   c.push_action( tester1_account, N(setdata), alice_account, mutable_variant_object()
      ("len1", 10)
      ("len2", 0)
      ("payer", alice_account)
   );
   c.produce_block();

   // Re-enable the subjective mitigation
   c.close();
   cfg.disable_all_subjective_mitigations = false;
   c.init( cfg, nullptr );

   c.produce_block();

   // Still cannot bill more RAM to another account within a notification
   // even if the account authorized the original action.
   // This is due to the subjective mitigation in place.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester2_account, N(notifysetdat), alice_account, mutable_variant_object()
         ("acctonotify", tester1_account)
         ("len1", 10)
         ("len2", 10)
         ("payer", alice_account)
      ),
      subjective_block_production_exception,
      fc_exception_message_is( "Cannot charge RAM to other accounts during notify." )
   );

   const auto& pfm = c.control->get_protocol_feature_manager();
   const auto& d = pfm.get_builtin_digest( builtin_protocol_feature_t::ram_restrictions );
   BOOST_REQUIRE( d );

   // Activate RAM_RESTRICTIONS protocol feature (this would also disable the subjective mitigation).
   c.preactivate_protocol_features( {*d} );
   c.produce_block();

   // Cannot send deferred transaction paid by another account that has not authorized the action.
   // This still fails objectively, but now with another error message.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester1_account, N(senddefer), bob_account, mutable_variant_object()
         ("senderid", 123)
         ("payer", alice_account)
      ),
      action_validate_exception,
      fc_exception_message_starts_with( "cannot bill RAM usage of deferred transaction to another account that has not authorized the action" )
   );

   // Cannot send deferred transaction paid by another account within a notification
   // even if the account authorized the original action.
   // This now fails with an objective error.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester2_account, N(notifydefer), alice_account, mutable_variant_object()
         ("acctonotify", tester1_account)
         ("senderid", 123)
         ("payer", alice_account)
      ),
      action_validate_exception,
      fc_exception_message_is( "cannot bill RAM usage of deferred transactions to another account within notify context" )
   );

   // Cannot bill more RAM to another account within a notification
   // even if the account authorized the original action.
   // This now fails with an objective error.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester2_account, N(notifysetdat), alice_account, mutable_variant_object()
         ("acctonotify", tester1_account)
         ("len1", 20)
         ("len2", 0)
         ("payer", alice_account)
      ),
      unauthorized_ram_usage_increase,
      fc_exception_message_starts_with( "unprivileged contract cannot increase RAM usage of another account within a notify context" )
   );

   // Cannot bill more RAM to another account that has not authorized the action.
   // This still fails objectively, but now with another error message.
   BOOST_REQUIRE_EXCEPTION(
      c.push_action( tester1_account, N(setdata), bob_account, mutable_variant_object()
         ("len1", 20)
         ("len2", 0)
         ("payer", alice_account)
      ),
      unauthorized_ram_usage_increase,
      fc_exception_message_starts_with( "unprivileged contract cannot increase RAM usage of another account that has not authorized the action" )
   );

   // Still can send deferred transaction paid by another account if it has authorized the action.
   c.push_action( tester1_account, N(senddefer), alice_account, mutable_variant_object()
      ("senderid", 123)
      ("payer", alice_account)
   );
   c.produce_block();

   // Now can migrate data from table1 to table2 paid by another account
   // in a RAM usage neutral way without the authority of that account.
   c.push_action( tester1_account, N(setdata), bob_account, mutable_variant_object()
      ("len1", 0)
      ("len2", 10)
      ("payer", alice_account)
   );

   // Now can also migrate data from table2 to table1 paid by another account
   // in a RAM usage neutral way even within a notification .
   c.push_action( tester2_account, N(notifysetdat), bob_account, mutable_variant_object()
      ("acctonotify", "tester1")
      ("len1", 10)
      ("len2", 0)
      ("payer", "alice")
   );

   // Of course it should also be possible to migrate data from table1 to table2 paid by another account
   // in a way that reduces RAM usage as well, even within a notification.
   c.push_action( tester2_account, N(notifysetdat), bob_account, mutable_variant_object()
      ("acctonotify", "tester1")
      ("len1", 0)
      ("len2", 5)
      ("payer", "alice")
   );

   // It should also still be possible for the receiver to take over payment of the RAM
   // if it is necessary to increase RAM usage without the authorization of the original payer.
   // This should all be possible to do even within a notification.
   c.push_action( tester2_account, N(notifysetdat), bob_account, mutable_variant_object()
      ("acctonotify", "tester1")
      ("len1", 10)
      ("len2", 10)
      ("payer", "tester1")
   );

   c.produce_block();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
