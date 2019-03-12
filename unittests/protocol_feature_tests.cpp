/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/abi_serializer.hpp>
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
   tester c(setup_policy::none, db_read_mode::SPECULATIVE);

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



BOOST_AUTO_TEST_SUITE_END()
