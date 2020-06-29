#include <eosio/testing/tester.hpp>
#include <eosio/chain/global_property_object.hpp>

#include "eosio_system_tester.hpp"

BOOST_AUTO_TEST_SUITE(limit_tests)

BOOST_FIXTURE_TEST_CASE(limit_tests, eosio_system::eosio_system_tester) { try {
   create_account_with_resources( N(eosio.wrap), config::system_account_name, 30000 );
   produce_blocks();
#if 0
   base_tester::push_action( config::system_account_name, N(setpriv),
                             config::system_account_name, mvo()
                             ("account", "eosio.wrap")
                             ("is_priv", 1)
                            );
#endif
   auto auth = authority(get_public_key( config::system_account_name, "active" ));
   auth.accounts.push_back( permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1} );
   base_tester::push_action( config::system_account_name, N(updateauth), config::system_account_name, mvo()
                             ("account", config::system_account_name)
                             ("permission", config::active_name)
                             ("parent", config::owner_name)
                             ("auth", auth)
                           );
   set_code( N(eosio.wrap), contracts::eosio_wrap_wasm() );
   set_abi( N(eosio.wrap), contracts::eosio_wrap_abi().data() );
   produce_blocks();
   auto params = control->get_global_properties().configuration;
   params.max_inline_action_size = 250;
   base_tester::push_action(config::system_account_name, N(setparams),
                            config::system_account_name, mvo()
                            ("params", params));
   produce_blocks();
   const std::string memo(255, 'z');
   base_tester::push_action( N(eosio.token), N(transfer), N(eosio), mvo()
                             ("from", N(eosio))
                             ("to", N(alice1111111))
                             ("quantity", core_from_string("10.0000"))
                             ("memo", memo)
                           );
   signed_transaction trx;
   trx.actions.emplace_back( get_action( N(eosio.token), N(transfer),
                                         vector<permission_level>{{N(eosio), config::active_name}},
                                         mvo()
                                         ("from", N(eosio))
                                         ("to", N(alice1111111))
                                         ("quantity", core_from_string("10.0000"))
                                         ("memo", memo))
                           );
   set_transaction_headers( trx, DEFAULT_EXPIRATION_DELTA, 0 );
   trx.sign( get_private_key( N(eosio), "active" ), control->get_chain_id() );
   base_tester::push_action( N(eosio.wrap), N(exec),
                             { N(eosio.wrap), config::system_account_name },
                             mvo()
                             ("executer", config::system_account_name)
                             ("trx", trx)
                           );
   produce_blocks();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
