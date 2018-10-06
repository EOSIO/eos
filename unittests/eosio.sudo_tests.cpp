#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <eosio.msig/eosio.msig.wast.hpp>
#include <eosio.msig/eosio.msig.abi.hpp>

#include <eosio.sudo/eosio.sudo.wast.hpp>
#include <eosio.sudo/eosio.sudo.abi.hpp>

#include <test_api/test_api.wast.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

class eosio_sudo_tester : public tester {
public:

   eosio_sudo_tester() {
      create_accounts( { N(eosio.msig), N(prod1), N(prod2), N(prod3), N(prod4), N(prod5), N(alice), N(bob), N(carol) } );
      produce_block();


      base_tester::push_action(config::system_account_name, N(setpriv),
                                 config::system_account_name,  mutable_variant_object()
                                 ("account", "eosio.msig")
                                 ("is_priv", 1)
      );

      set_code( N(eosio.msig), eosio_msig_wast );
      set_abi( N(eosio.msig), eosio_msig_abi );

      produce_blocks();

      signed_transaction trx;
      set_transaction_headers(trx);
      authority auth( 1, {}, {{{config::system_account_name, config::active_name}, 1}} );
      trx.actions.emplace_back( vector<permission_level>{{config::system_account_name, config::active_name}},
                                newaccount{
                                   .creator  = config::system_account_name,
                                   .name     = N(eosio.sudo),
                                   .owner    = auth,
                                   .active   = auth,
                                });

      set_transaction_headers(trx);
      trx.sign( get_private_key( config::system_account_name, "active" ), control->get_chain_id()  );
      push_transaction( trx );

      base_tester::push_action(config::system_account_name, N(setpriv),
                                 config::system_account_name,  mutable_variant_object()
                                 ("account", "eosio.sudo")
                                 ("is_priv", 1)
      );

      auto system_private_key = get_private_key( config::system_account_name, "active" );
      set_code( N(eosio.sudo), eosio_sudo_wast, &system_private_key );
      set_abi( N(eosio.sudo), eosio_sudo_abi, &system_private_key );

      produce_blocks();

      set_authority( config::system_account_name, config::active_name,
                     authority( 1, {{get_public_key( config::system_account_name, "active" ), 1}},
                                   {{{config::producers_account_name, config::active_name}, 1}} ),
                     config::owner_name,
                     { { config::system_account_name, config::owner_name } },
                     { get_private_key( config::system_account_name, "active" ) }
                   );

      set_producers( {N(prod1), N(prod2), N(prod3), N(prod4), N(prod5)} );

      produce_blocks();

      const auto& accnt = control->db().get<account_object,by_name>( N(eosio.sudo) );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);

      while( control->pending_block_state()->header.producer.to_string() == "eosio" ) {
         produce_block();
      }
   }

   void propose( name proposer, name proposal_name, vector<permission_level> requested_permissions, const transaction& trx ) {
      push_action( N(eosio.msig), N(propose), proposer, mvo()
                     ("proposer",      proposer)
                     ("proposal_name", proposal_name)
                     ("requested",     requested_permissions)
                     ("trx",           trx)
      );
   }

   void approve( name proposer, name proposal_name, name approver ) {
      push_action( N(eosio.msig), N(approve), approver, mvo()
                     ("proposer",      proposer)
                     ("proposal_name", proposal_name)
                     ("level",         permission_level{approver, config::active_name} )
      );
   }

   void unapprove( name proposer, name proposal_name, name unapprover ) {
      push_action( N(eosio.msig), N(unapprove), unapprover, mvo()
                     ("proposer",      proposer)
                     ("proposal_name", proposal_name)
                     ("level",         permission_level{unapprover, config::active_name})
      );
   }

   transaction sudo_exec( account_name executer, const transaction& trx, uint32_t expiration = base_tester::DEFAULT_EXPIRATION_DELTA );

   transaction reqauth( account_name from, const vector<permission_level>& auths, uint32_t expiration = base_tester::DEFAULT_EXPIRATION_DELTA );

   abi_serializer abi_ser;
};

transaction eosio_sudo_tester::sudo_exec( account_name executer, const transaction& trx, uint32_t expiration ) {
   fc::variants v;
   v.push_back( fc::mutable_variant_object()
                  ("actor", executer)
                  ("permission", name{config::active_name})
              );
  v.push_back( fc::mutable_variant_object()
                 ("actor", "eosio.sudo")
                 ("permission", name{config::active_name})
             );
   auto act_obj = fc::mutable_variant_object()
                     ("account", "eosio.sudo")
                     ("name", "exec")
                     ("authorization", v)
                     ("data", fc::mutable_variant_object()("executer", executer)("trx", trx) );
   transaction trx2;
   set_transaction_headers(trx2, expiration);
   action act;
   abi_serializer::from_variant( act_obj, act, get_resolver(), abi_serializer_max_time );
   trx2.actions.push_back( std::move(act) );
   return trx2;
}

transaction eosio_sudo_tester::reqauth( account_name from, const vector<permission_level>& auths, uint32_t expiration ) {
   fc::variants v;
   for ( auto& level : auths ) {
      v.push_back(fc::mutable_variant_object()
                  ("actor", level.actor)
                  ("permission", level.permission)
      );
   }
   auto act_obj = fc::mutable_variant_object()
                     ("account", name{config::system_account_name})
                     ("name", "reqauth")
                     ("authorization", v)
                     ("data", fc::mutable_variant_object() ("from", from) );
   transaction trx;
   set_transaction_headers(trx, expiration);
   action act;
   abi_serializer::from_variant( act_obj, act, get_resolver(), abi_serializer_max_time );
   trx.actions.push_back( std::move(act) );
   return trx;
}

BOOST_AUTO_TEST_SUITE(eosio_sudo_tests)

BOOST_FIXTURE_TEST_CASE( sudo_exec_direct, eosio_sudo_tester ) try {
   auto trx = reqauth( N(bob), {permission_level{N(bob), config::active_name}} );

   transaction_trace_ptr trace;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace = t; } } );

   {
      signed_transaction sudo_trx( sudo_exec( N(alice), trx ), {}, {} );
      /*
      set_transaction_headers( sudo_trx );
      sudo_trx.actions.emplace_back( get_action( N(eosio.sudo), N(exec),
                                                 {{N(alice), config::active_name}, {N(eosio.sudo), config::active_name}},
                                                 mvo()
                                                   ("executer", "alice")
                                                   ("trx", trx)
      ) );
      */
      sudo_trx.sign( get_private_key( N(alice), "active" ), control->get_chain_id() );
      for( const auto& actor : {"prod1", "prod2", "prod3", "prod4"} ) {
         sudo_trx.sign( get_private_key( actor, "active" ), control->get_chain_id() );
      }
      push_transaction( sudo_trx );
   }

   produce_block();

   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( "eosio", name{trace->action_traces[0].act.account} );
   BOOST_REQUIRE_EQUAL( "reqauth", name{trace->action_traces[0].act.name} );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( sudo_with_msig, eosio_sudo_tester ) try {
   auto trx = reqauth( N(bob), {permission_level{N(bob), config::active_name}} );
   auto sudo_trx = sudo_exec( N(alice), trx );

   propose( N(carol), N(first),
            { {N(alice), N(active)},
              {N(prod1), N(active)}, {N(prod2), N(active)}, {N(prod3), N(active)}, {N(prod4), N(active)}, {N(prod5), N(active)} },
            sudo_trx );

   approve( N(carol), N(first), N(alice) ); // alice must approve since she is the executer of the sudo::exec action

   // More than 2/3 of block producers approve
   approve( N(carol), N(first), N(prod1) );
   approve( N(carol), N(first), N(prod2) );
   approve( N(carol), N(first), N(prod3) );
   approve( N(carol), N(first), N(prod4) );

   vector<transaction_trace_ptr> traces;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) {
      if (t->scheduled) {
         traces.push_back( t );
      }
   } );

   // Now the proposal should be ready to execute
   push_action( N(eosio.msig), N(exec), N(alice), mvo()
                  ("proposer",      "carol")
                  ("proposal_name", "first")
                  ("executer",      "alice")
   );

   produce_block();

   BOOST_REQUIRE_EQUAL( 2, traces.size() );

   BOOST_REQUIRE_EQUAL( 1, traces[0]->action_traces.size() );
   BOOST_REQUIRE_EQUAL( "eosio.sudo", name{traces[0]->action_traces[0].act.account} );
   BOOST_REQUIRE_EQUAL( "exec", name{traces[0]->action_traces[0].act.name} );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, traces[0]->receipt->status );

   BOOST_REQUIRE_EQUAL( 1, traces[1]->action_traces.size() );
   BOOST_REQUIRE_EQUAL( "eosio", name{traces[1]->action_traces[0].act.account} );
   BOOST_REQUIRE_EQUAL( "reqauth", name{traces[1]->action_traces[0].act.name} );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, traces[1]->receipt->status );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( sudo_with_msig_unapprove, eosio_sudo_tester ) try {
   auto trx = reqauth( N(bob), {permission_level{N(bob), config::active_name}} );
   auto sudo_trx = sudo_exec( N(alice), trx );

   propose( N(carol), N(first),
            { {N(alice), N(active)},
              {N(prod1), N(active)}, {N(prod2), N(active)}, {N(prod3), N(active)}, {N(prod4), N(active)}, {N(prod5), N(active)} },
            sudo_trx );

   approve( N(carol), N(first), N(alice) ); // alice must approve since she is the executer of the sudo::exec action

   // 3 of the 4 needed producers approve
   approve( N(carol), N(first), N(prod1) );
   approve( N(carol), N(first), N(prod2) );
   approve( N(carol), N(first), N(prod3) );

   // first producer takes back approval
   unapprove( N(carol), N(first), N(prod1) );

   // fourth producer approves but the total number of approving producers is still 3 which is less than two-thirds of producers
   approve( N(carol), N(first), N(prod4) );

   produce_block();

   // The proposal should not have sufficient approvals to pass the authorization checks of eosio.sudo::exec.
   BOOST_REQUIRE_EXCEPTION( push_action( N(eosio.msig), N(exec), N(alice), mvo()
                                          ("proposer",      "carol")
                                          ("proposal_name", "first")
                                          ("executer",      "alice")
                                       ), eosio_assert_message_exception,
                                          eosio_assert_message_is("transaction authorization failed")
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( sudo_with_msig_producers_change, eosio_sudo_tester ) try {
   create_accounts( { N(newprod1) } );

   auto trx = reqauth( N(bob), {permission_level{N(bob), config::active_name}} );
   auto sudo_trx = sudo_exec( N(alice), trx, 36000 );

   propose( N(carol), N(first),
            { {N(alice), N(active)},
              {N(prod1), N(active)}, {N(prod2), N(active)}, {N(prod3), N(active)}, {N(prod4), N(active)}, {N(prod5), N(active)} },
            sudo_trx );

   approve( N(carol), N(first), N(alice) ); // alice must approve since she is the executer of the sudo::exec action

   // 2 of the 4 needed producers approve
   approve( N(carol), N(first), N(prod1) );
   approve( N(carol), N(first), N(prod2) );

   produce_block();

   set_producers( {N(prod1), N(prod2), N(prod3), N(prod4), N(prod5), N(newprod1)} ); // With 6 producers, the 2/3+1 threshold becomes 5

   while( control->pending_block_state()->active_schedule.producers.size() != 6 ) {
      produce_block();
   }

   // Now two more block producers approve which would have been sufficient under the old schedule but not the new one.
   approve( N(carol), N(first), N(prod3) );
   approve( N(carol), N(first), N(prod4) );

   produce_block();

   // The proposal has four of the five requested approvals but they are not sufficient to satisfy the authorization checks of eosio.sudo::exec.
   BOOST_REQUIRE_EXCEPTION( push_action( N(eosio.msig), N(exec), N(alice), mvo()
                                          ("proposer",      "carol")
                                          ("proposal_name", "first")
                                          ("executer",      "alice")
                                       ), eosio_assert_message_exception,
                                          eosio_assert_message_is("transaction authorization failed")
   );

   // Unfortunately the new producer cannot approve because they were not in the original requested approvals.
   BOOST_REQUIRE_EXCEPTION( approve( N(carol), N(first), N(newprod1) ),
                            eosio_assert_message_exception,
                            eosio_assert_message_is("approval is not on the list of requested approvals")
   );

   // But prod5 still can provide the fifth approval necessary to satisfy the 2/3+1 threshold of the new producer set
   approve( N(carol), N(first), N(prod5) );

   vector<transaction_trace_ptr> traces;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) {
      if (t->scheduled) {
         traces.push_back( t );
      }
   } );

   // Now the proposal should be ready to execute
   push_action( N(eosio.msig), N(exec), N(alice), mvo()
                  ("proposer",      "carol")
                  ("proposal_name", "first")
                  ("executer",      "alice")
   );

   produce_block();

   BOOST_REQUIRE_EQUAL( 2, traces.size() );

   BOOST_REQUIRE_EQUAL( 1, traces[0]->action_traces.size() );
   BOOST_REQUIRE_EQUAL( "eosio.sudo", name{traces[0]->action_traces[0].act.account} );
   BOOST_REQUIRE_EQUAL( "exec", name{traces[0]->action_traces[0].act.name} );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, traces[0]->receipt->status );

   BOOST_REQUIRE_EQUAL( 1, traces[1]->action_traces.size() );
   BOOST_REQUIRE_EQUAL( "eosio", name{traces[1]->action_traces[0].act.account} );
   BOOST_REQUIRE_EQUAL( "reqauth", name{traces[1]->action_traces[0].act.name} );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, traces[1]->receipt->status );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
