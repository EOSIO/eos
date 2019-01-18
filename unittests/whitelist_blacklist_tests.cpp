/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/testing/tester_network.hpp>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

using mvo = fc::mutable_variant_object;

template<class Tester = TESTER>
class whitelist_blacklist_tester {
   public:
      whitelist_blacklist_tester() {}

      static controller::config get_default_chain_configuration( const fc::path& p ) {
         controller::config cfg;
         cfg.blocks_dir = p / config::default_blocks_dir_name;
         cfg.state_dir  = p / config::default_state_dir_name;
         cfg.state_size = 1024*1024*8;
         cfg.state_guard_size = 0;
         cfg.reversible_cache_size = 1024*1024*8;
         cfg.reversible_guard_size = 0;
         cfg.contracts_console = true;

         cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
         cfg.genesis.initial_key = base_tester::get_public_key( config::system_account_name, "active" );

         for(int i = 0; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
            if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wavm"))
               cfg.wasm_runtime = chain::wasm_interface::vm_type::wavm;
            else if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wabt"))
               cfg.wasm_runtime = chain::wasm_interface::vm_type::wabt;
         }

         return cfg;
      }

      void init( bool bootstrap = true ) {
         FC_ASSERT( !chain, "chain is already up" );

         auto cfg = get_default_chain_configuration( tempdir.path() );
         cfg.sender_bypass_whiteblacklist = sender_bypass_whiteblacklist;
         cfg.actor_whitelist = actor_whitelist;
         cfg.actor_blacklist = actor_blacklist;
         cfg.contract_whitelist = contract_whitelist;
         cfg.contract_blacklist = contract_blacklist;
         cfg.action_blacklist = action_blacklist;

         chain.emplace(cfg);
         wdump((last_produced_block));
         chain->set_last_produced_block_map( last_produced_block );

         if( !bootstrap ) return;

         chain->create_accounts({N(eosio.token), N(alice), N(bob), N(charlie)});
         chain->set_code(N(eosio.token), contracts::eosio_token_wasm() );
         chain->set_abi(N(eosio.token), contracts::eosio_token_abi().data() );
         chain->push_action( N(eosio.token), N(create), N(eosio.token), mvo()
              ( "issuer", "eosio.token" )
              ( "maximum_supply", "1000000.00 TOK" )
         );
         chain->push_action( N(eosio.token), N(issue), N(eosio.token), mvo()
              ( "to", "eosio.token" )
              ( "quantity", "1000000.00 TOK" )
              ( "memo", "issue" )
         );
         chain->produce_blocks();
      }

      void shutdown() {
         FC_ASSERT( chain.valid(), "chain is not up" );
         last_produced_block = chain->get_last_produced_block_map();
         wdump((last_produced_block));
         chain.reset();
      }

      transaction_trace_ptr transfer( account_name from, account_name to, string quantity = "1.00 TOK" ) {
         return chain->push_action( N(eosio.token), N(transfer), from, mvo()
            ( "from", from )
            ( "to", to )
            ( "quantity", quantity )
            ( "memo", "" )
         );
      }

      fc::temp_directory                tempdir; // Must come before chain
      fc::optional<Tester>              chain;
      flat_set<account_name>            sender_bypass_whiteblacklist;
      flat_set<account_name>            actor_whitelist;
      flat_set<account_name>            actor_blacklist;
      flat_set<account_name>            contract_whitelist;
      flat_set<account_name>            contract_blacklist;
      flat_set< pair<account_name, action_name> >  action_blacklist;
      map<account_name, block_id_type>  last_produced_block;
};

struct transfer_args {
   account_name from;
   account_name to;
   asset        quantity;
   string       memo;
};

FC_REFLECT( transfer_args, (from)(to)(quantity)(memo) )


BOOST_AUTO_TEST_SUITE(whitelist_blacklist_tests)

BOOST_AUTO_TEST_CASE( actor_whitelist ) { try {
   whitelist_blacklist_tester<> test;
   test.actor_whitelist = {config::system_account_name, N(eosio.token), N(alice)};
   test.init();

   test.transfer( N(eosio.token), N(alice), "1000.00 TOK" );

   test.transfer( N(alice), N(bob),  "100.00 TOK" );

   BOOST_CHECK_EXCEPTION( test.transfer( N(bob), N(alice) ),
                          actor_whitelist_exception,
                          fc_exception_message_is("authorizing actor(s) in transaction are not on the actor whitelist: [\"bob\"]")
                       );
   signed_transaction trx;
   trx.actions.emplace_back( vector<permission_level>{{N(alice),config::active_name}, {N(bob),config::active_name}},
                             N(eosio.token), N(transfer),
                             fc::raw::pack(transfer_args{
                               .from  = N(alice),
                               .to    = N(bob),
                               .quantity = asset::from_string("10.00 TOK"),
                               .memo = ""
                             })
                           );
   test.chain->set_transaction_headers(trx);
   trx.sign( test.chain->get_private_key( N(alice), "active" ), test.chain->control->get_chain_id() );
   trx.sign( test.chain->get_private_key( N(bob), "active" ), test.chain->control->get_chain_id() );
   BOOST_CHECK_EXCEPTION( test.chain->push_transaction( trx ),
                          actor_whitelist_exception,
                          fc_exception_message_starts_with("authorizing actor(s) in transaction are not on the actor whitelist: [\"bob\"]")
                        );
   test.chain->produce_blocks();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( actor_blacklist ) { try {
   whitelist_blacklist_tester<> test;
   test.actor_blacklist = {N(bob)};
   test.init();

   test.transfer( N(eosio.token), N(alice), "1000.00 TOK" );

   test.transfer( N(alice), N(bob),  "100.00 TOK" );

   BOOST_CHECK_EXCEPTION( test.transfer( N(bob), N(alice) ),
                          actor_blacklist_exception,
                          fc_exception_message_starts_with("authorizing actor(s) in transaction are on the actor blacklist: [\"bob\"]")
                        );

   signed_transaction trx;
   trx.actions.emplace_back( vector<permission_level>{{N(alice),config::active_name}, {N(bob),config::active_name}},
                             N(eosio.token), N(transfer),
                             fc::raw::pack(transfer_args{
                                .from  = N(alice),
                                .to    = N(bob),
                                .quantity = asset::from_string("10.00 TOK"),
                                .memo = ""
                             })
                           );
   test.chain->set_transaction_headers(trx);
   trx.sign( test.chain->get_private_key( N(alice), "active" ), test.chain->control->get_chain_id() );
   trx.sign( test.chain->get_private_key( N(bob), "active" ), test.chain->control->get_chain_id() );
   BOOST_CHECK_EXCEPTION( test.chain->push_transaction( trx ),
                          actor_blacklist_exception,
                          fc_exception_message_starts_with("authorizing actor(s) in transaction are on the actor blacklist: [\"bob\"]")
                        );
   test.chain->produce_blocks();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( contract_whitelist ) { try {
   whitelist_blacklist_tester<> test;
   test.contract_whitelist = {config::system_account_name, N(eosio.token), N(bob)};
   test.init();

   test.transfer( N(eosio.token), N(alice), "1000.00 TOK" );

   test.transfer( N(alice), N(eosio.token) );

   test.transfer( N(alice), N(bob) );
   test.transfer( N(alice), N(charlie), "100.00 TOK" );

   test.transfer( N(charlie), N(alice) );

   test.chain->produce_blocks();

   test.chain->set_code(N(bob), contracts::eosio_token_wasm() );
   test.chain->set_abi(N(bob), contracts::eosio_token_abi().data() );

   test.chain->produce_blocks();

   test.chain->set_code(N(charlie), contracts::eosio_token_wasm() );
   test.chain->set_abi(N(charlie), contracts::eosio_token_abi().data() );

   test.chain->produce_blocks();

   test.transfer( N(alice), N(bob) );

   BOOST_CHECK_EXCEPTION( test.transfer( N(alice), N(charlie) ),
                          contract_whitelist_exception,
                          fc_exception_message_is("account 'charlie' is not on the contract whitelist")
                        );


   test.chain->push_action( N(bob), N(create), N(bob), mvo()
      ( "issuer", "bob" )
      ( "maximum_supply", "1000000.00 CUR" )
   );

   BOOST_CHECK_EXCEPTION( test.chain->push_action( N(charlie), N(create), N(charlie), mvo()
                              ( "issuer", "charlie" )
                              ( "maximum_supply", "1000000.00 CUR" )
                          ),
                          contract_whitelist_exception,
                          fc_exception_message_starts_with("account 'charlie' is not on the contract whitelist")
                        );
   test.chain->produce_blocks();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( contract_blacklist ) { try {
   whitelist_blacklist_tester<> test;
   test.contract_blacklist = {N(charlie)};
   test.init();

   test.transfer( N(eosio.token), N(alice), "1000.00 TOK" );

   test.transfer( N(alice), N(eosio.token) );

   test.transfer( N(alice), N(bob) );
   test.transfer( N(alice), N(charlie), "100.00 TOK" );

   test.transfer( N(charlie), N(alice) );

   test.chain->produce_blocks();

   test.chain->set_code(N(bob), contracts::eosio_token_wasm() );
   test.chain->set_abi(N(bob), contracts::eosio_token_abi().data() );

   test.chain->produce_blocks();

   test.chain->set_code(N(charlie), contracts::eosio_token_wasm() );
   test.chain->set_abi(N(charlie), contracts::eosio_token_abi().data() );

   test.chain->produce_blocks();

   test.transfer( N(alice), N(bob) );

   BOOST_CHECK_EXCEPTION( test.transfer( N(alice), N(charlie) ),
                          contract_blacklist_exception,
                          fc_exception_message_is("account 'charlie' is on the contract blacklist")
                        );


   test.chain->push_action( N(bob), N(create), N(bob), mvo()
      ( "issuer", "bob" )
      ( "maximum_supply", "1000000.00 CUR" )
   );

   BOOST_CHECK_EXCEPTION( test.chain->push_action( N(charlie), N(create), N(charlie), mvo()
                              ( "issuer", "charlie" )
                              ( "maximum_supply", "1000000.00 CUR" )
                          ),
                          contract_blacklist_exception,
                          fc_exception_message_starts_with("account 'charlie' is on the contract blacklist")
                        );
   test.chain->produce_blocks();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( action_blacklist ) { try {
   whitelist_blacklist_tester<> test;
   test.contract_whitelist = {config::system_account_name, N(eosio.token), N(bob), N(charlie)};
   test.action_blacklist = {{N(charlie), N(create)}};
   test.init();

   test.transfer( N(eosio.token), N(alice), "1000.00 TOK" );

   test.chain->produce_blocks();

   test.chain->set_code(N(bob), contracts::eosio_token_wasm() );
   test.chain->set_abi(N(bob), contracts::eosio_token_abi().data() );

   test.chain->produce_blocks();

   test.chain->set_code(N(charlie), contracts::eosio_token_wasm() );
   test.chain->set_abi(N(charlie), contracts::eosio_token_abi().data() );

   test.chain->produce_blocks();

   test.transfer( N(alice), N(bob) );

   test.transfer( N(alice), N(charlie) ),

   test.chain->push_action( N(bob), N(create), N(bob), mvo()
      ( "issuer", "bob" )
      ( "maximum_supply", "1000000.00 CUR" )
   );

   BOOST_CHECK_EXCEPTION( test.chain->push_action( N(charlie), N(create), N(charlie), mvo()
                              ( "issuer", "charlie" )
                              ( "maximum_supply", "1000000.00 CUR" )
                          ),
                          action_blacklist_exception,
                          fc_exception_message_starts_with("action 'charlie::create' is on the action blacklist")
                        );
   test.chain->produce_blocks();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( blacklist_eosio ) { try {
   whitelist_blacklist_tester<tester> tester1;
   tester1.init();
   tester1.chain->produce_blocks();
   tester1.chain->set_code(config::system_account_name, contracts::eosio_token_wasm() );
   tester1.chain->produce_blocks();
   tester1.shutdown();
   tester1.contract_blacklist = {config::system_account_name};
   tester1.init(false);

   whitelist_blacklist_tester<tester> tester2;
   tester2.init(false);

   while( tester2.chain->control->head_block_num() < tester1.chain->control->head_block_num() ) {
      auto b = tester1.chain->control->fetch_block_by_number( tester2.chain->control->head_block_num()+1 );
      tester2.chain->push_block( b );
   }

   tester1.chain->produce_blocks(2);

   while( tester2.chain->control->head_block_num() < tester1.chain->control->head_block_num() ) {
      auto b = tester1.chain->control->fetch_block_by_number( tester2.chain->control->head_block_num()+1 );
      tester2.chain->push_block( b );
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( deferred_blacklist_failure ) { try {
   whitelist_blacklist_tester<tester> tester1;
   tester1.init();
   tester1.chain->produce_blocks();
   tester1.chain->set_code( N(bob), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(bob),  contracts::deferred_test_abi().data() );
   tester1.chain->set_code( N(charlie), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(charlie),  contracts::deferred_test_abi().data() );
   tester1.chain->produce_blocks();

   tester1.chain->push_action( N(bob), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 0 )
      ( "contract", "charlie" )
      ( "payload", 10 )
   );

   tester1.chain->produce_blocks(2);

   tester1.shutdown();

   tester1.contract_blacklist = {N(charlie)};
   tester1.init(false);

   whitelist_blacklist_tester<tester> tester2;
   tester2.init(false);

   while( tester2.chain->control->head_block_num() < tester1.chain->control->head_block_num() ) {
      auto b = tester1.chain->control->fetch_block_by_number( tester2.chain->control->head_block_num()+1 );
      tester2.chain->push_block( b );
   }

   tester1.chain->push_action( N(bob), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 1 )
      ( "contract", "charlie" )
      ( "payload", 10 )
   );

   BOOST_CHECK_EXCEPTION( tester1.chain->produce_blocks(), fc::exception,
                          fc_exception_message_is("account 'charlie' is on the contract blacklist")
                        );
   tester1.chain->produce_blocks(2, true); // Produce 2 empty blocks (other than onblock of course).

   while( tester2.chain->control->head_block_num() < tester1.chain->control->head_block_num() ) {
      auto b = tester1.chain->control->fetch_block_by_number( tester2.chain->control->head_block_num()+1 );
      tester2.chain->push_block( b );
   }
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE( blacklist_onerror ) { try {
   whitelist_blacklist_tester<TESTER> tester1;
   tester1.init();
   tester1.chain->produce_blocks();
   tester1.chain->set_code( N(bob), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(bob),  contracts::deferred_test_abi().data() );
   tester1.chain->set_code( N(charlie), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(charlie),  contracts::deferred_test_abi().data() );
   tester1.chain->produce_blocks();
   
   tester1.chain->push_action( N(bob), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 0 )
      ( "contract", "charlie" )
      ( "payload", 13 )
   );

   tester1.chain->produce_blocks();
   tester1.shutdown();

   tester1.action_blacklist = {{config::system_account_name, N(onerror)}};
   tester1.init(false);

   tester1.chain->push_action( N(bob), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 0 )
      ( "contract", "charlie" )
      ( "payload", 13 )
   );

   BOOST_CHECK_EXCEPTION( tester1.chain->produce_blocks(), fc::exception,
                          fc_exception_message_is("action 'eosio::onerror' is on the action blacklist")
                        );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( actor_blacklist_inline_deferred ) { try {
   whitelist_blacklist_tester<tester> tester1;
   tester1.init();
   tester1.chain->produce_blocks();
   tester1.chain->set_code( N(alice), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(alice),  contracts::deferred_test_abi().data() );
   tester1.chain->set_code( N(bob), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(bob),  contracts::deferred_test_abi().data() );
   tester1.chain->set_code( N(charlie), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(charlie),  contracts::deferred_test_abi().data() );
   tester1.chain->produce_blocks();

   auto auth = authority(eosio::testing::base_tester::get_public_key("alice", "active"));
   auth.accounts.push_back( permission_level_weight{{N(alice), config::eosio_code_name}, 1} );

   tester1.chain->push_action( N(eosio), N(updateauth), N(alice), mvo()
      ( "account", "alice" )
      ( "permission", "active" )
      ( "parent", "owner" )
      ( "auth", auth )
   );

   auth = authority(eosio::testing::base_tester::get_public_key("bob", "active"));
   auth.accounts.push_back( permission_level_weight{{N(alice), config::eosio_code_name}, 1} );
   auth.accounts.push_back( permission_level_weight{{N(bob), config::eosio_code_name}, 1} );

   tester1.chain->push_action( N(eosio), N(updateauth), N(bob), mvo()
      ( "account", "bob" )
      ( "permission", "active" )
      ( "parent", "owner" )
      ( "auth", auth )
   );

   auth = authority(eosio::testing::base_tester::get_public_key("charlie", "active"));
   auth.accounts.push_back( permission_level_weight{{N(charlie), config::eosio_code_name}, 1} );

   tester1.chain->push_action( N(eosio), N(updateauth), N(charlie), mvo()
      ( "account", "charlie" )
      ( "permission", "active" )
      ( "parent", "owner" )
      ( "auth", auth )
   );

   tester1.chain->produce_blocks(2);

   tester1.shutdown();

   tester1.actor_blacklist = {N(bob)};
   tester1.init(false);

   whitelist_blacklist_tester<tester> tester2;
   tester2.init(false);

   while( tester2.chain->control->head_block_num() < tester1.chain->control->head_block_num() ) {
      auto b = tester1.chain->control->fetch_block_by_number( tester2.chain->control->head_block_num()+1 );
      tester2.chain->push_block( b );
   }

   auto log_trxs = [&]( const transaction_trace_ptr& t) {
      if( !t || t->action_traces.size() == 0 ) return;

      const auto& act = t->action_traces[0].act;
      if( act.account == N(eosio) && act.name == N(onblock) ) return;

      if( t->receipt && t->receipt->status == transaction_receipt::executed ) {
         wlog( "${trx_type} ${id} executed (first action is ${code}::${action})",
              ("trx_type", t->scheduled ? "scheduled trx" : "trx")("id", t->id)("code", act.account)("action", act.name) );
      } else {
         wlog( "${trx_type} ${id} failed (first action is ${code}::${action})",
               ("trx_type", t->scheduled ? "scheduled trx" : "trx")("id", t->id)("code", act.account)("action", act.name) );
      }
   };

   auto c1 = tester1.chain->control->applied_transaction.connect( log_trxs );

   // Disallow inline actions authorized by actor in blacklist
   BOOST_CHECK_EXCEPTION( tester1.chain->push_action( N(alice), N(inlinecall), N(alice), mvo()
                                                         ( "contract", "alice" )
                                                         ( "authorizer", "bob" )
                                                         ( "payload", 10 ) ),
                           fc::exception,
                           fc_exception_message_is("authorizing actor(s) in transaction are on the actor blacklist: [\"bob\"]") );



   auto num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, num_deferred);

   // Schedule a deferred transaction authorized by charlie@active
   tester1.chain->push_action( N(charlie), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 0 )
      ( "contract", "charlie" )
      ( "payload", 10 )
   );

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, num_deferred);

   // Do not allow that deferred transaction to retire yet
   tester1.chain->finish_block();
   tester1.chain->produce_blocks(2, true); // Produce 2 empty blocks (other than onblock of course).

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, num_deferred);

   c1.disconnect();

   while( tester2.chain->control->head_block_num() < tester1.chain->control->head_block_num() ) {
      auto b = tester1.chain->control->fetch_block_by_number( tester2.chain->control->head_block_num()+1 );
      tester2.chain->push_block( b );
   }

   tester1.shutdown();

   tester1.actor_blacklist = {N(bob), N(charlie)};
   tester1.init(false);

   auto c2 = tester1.chain->control->applied_transaction.connect( log_trxs );

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, num_deferred);

   // With charlie now in the actor blacklist, retiring the previously scheduled deferred transaction should now not be possible.
   BOOST_CHECK_EXCEPTION( tester1.chain->produce_blocks(), fc::exception,
                          fc_exception_message_is("authorizing actor(s) in transaction are on the actor blacklist: [\"charlie\"]")
                        );


   // With charlie now in the actor blacklist, it is now not possible to schedule a deferred transaction authorized by charlie@active
   BOOST_CHECK_EXCEPTION( tester1.chain->push_action( N(charlie), N(defercall), N(alice), mvo()
                                                         ( "payer", "alice" )
                                                         ( "sender_id", 1 )
                                                         ( "contract", "charlie" )
                                                         ( "payload", 10 ) ),
                           fc::exception,
                           fc_exception_message_is("authorizing actor(s) in transaction are on the actor blacklist: [\"charlie\"]")
   );

   c2.disconnect();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( blacklist_sender_bypass ) { try {
   whitelist_blacklist_tester<tester> tester1;
   tester1.init();
   tester1.chain->produce_blocks();
   tester1.chain->set_code( N(alice), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(alice),  contracts::deferred_test_abi().data() );
   tester1.chain->set_code( N(bob), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(bob),  contracts::deferred_test_abi().data() );
   tester1.chain->set_code( N(charlie), contracts::deferred_test_wasm() );
   tester1.chain->set_abi( N(charlie),  contracts::deferred_test_abi().data() );
   tester1.chain->produce_blocks();

   auto auth = authority(eosio::testing::base_tester::get_public_key("alice", "active"));
   auth.accounts.push_back( permission_level_weight{{N(alice), config::eosio_code_name}, 1} );

   tester1.chain->push_action( N(eosio), N(updateauth), N(alice), mvo()
      ( "account", "alice" )
      ( "permission", "active" )
      ( "parent", "owner" )
      ( "auth", auth )
   );

   auth = authority(eosio::testing::base_tester::get_public_key("bob", "active"));
   auth.accounts.push_back( permission_level_weight{{N(bob), config::eosio_code_name}, 1} );

   tester1.chain->push_action( N(eosio), N(updateauth), N(bob), mvo()
      ( "account", "bob" )
      ( "permission", "active" )
      ( "parent", "owner" )
      ( "auth", auth )
   );

   auth = authority(eosio::testing::base_tester::get_public_key("charlie", "active"));
   auth.accounts.push_back( permission_level_weight{{N(charlie), config::eosio_code_name}, 1} );

   tester1.chain->push_action( N(eosio), N(updateauth), N(charlie), mvo()
      ( "account", "charlie" )
      ( "permission", "active" )
      ( "parent", "owner" )
      ( "auth", auth )
   );

   tester1.chain->produce_blocks(2);

   tester1.shutdown();

   tester1.sender_bypass_whiteblacklist = {N(charlie)};
   tester1.actor_blacklist = {N(bob), N(charlie)};
   tester1.init(false);

   BOOST_CHECK_EXCEPTION( tester1.chain->push_action( N(bob), N(deferfunc), N(bob), mvo()
                                                         ( "payload", 10 ) ),
                           fc::exception,
                           fc_exception_message_is("authorizing actor(s) in transaction are on the actor blacklist: [\"bob\"]")
   );


   BOOST_CHECK_EXCEPTION( tester1.chain->push_action( N(charlie), N(deferfunc), N(charlie), mvo()
                                                         ( "payload", 10 ) ),
                           fc::exception,
                           fc_exception_message_is("authorizing actor(s) in transaction are on the actor blacklist: [\"charlie\"]")
   );


   auto num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, num_deferred);

   BOOST_CHECK_EXCEPTION( tester1.chain->push_action( N(bob), N(defercall), N(alice), mvo()
                                                         ( "payer", "alice" )
                                                         ( "sender_id", 0 )
                                                         ( "contract", "bob" )
                                                         ( "payload", 10 ) ),
                           fc::exception,
                           fc_exception_message_is("authorizing actor(s) in transaction are on the actor blacklist: [\"bob\"]")
   );

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, num_deferred);

   // Schedule a deferred transaction authorized by charlie@active
   tester1.chain->push_action( N(charlie), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 0 )
      ( "contract", "charlie" )
      ( "payload", 10 )
   );

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, num_deferred);

   // Retire the deferred transaction successfully despite charlie being on the actor blacklist.
   // This is allowed due to the fact that the sender of the deferred transaction (also charlie) is in the sender bypass list.
   tester1.chain->produce_blocks();

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, num_deferred);

   // Schedule another deferred transaction authorized by charlie@active
   tester1.chain->push_action( N(charlie), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 1 )
      ( "contract", "bob" )
      ( "payload", 10 )
   );

   // Do not yet retire the deferred transaction
   tester1.chain->finish_block();

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, num_deferred);

   tester1.shutdown();

   tester1.sender_bypass_whiteblacklist = {N(charlie)};
   tester1.actor_blacklist = {N(bob), N(charlie)};
   tester1.contract_blacklist = {N(bob)}; // Add bob to the contract blacklist as well
   tester1.init(false);

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, num_deferred);

   // Now retire the deferred transaction successfully despite charlie being on both the actor blacklist and bob being on the contract blacklist
   // This is allowed due to the fact that the sender of the deferred transaction (also charlie) is in the sender bypass list.
   tester1.chain->produce_blocks();

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(0, num_deferred);

   tester1.chain->push_action( N(alice), N(defercall), N(alice), mvo()
      ( "payer", "alice" )
      ( "sender_id", 0 )
      ( "contract", "bob" )
      ( "payload", 10 )
   );

   num_deferred = tester1.chain->control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
   BOOST_REQUIRE_EQUAL(1, num_deferred);

   // Ensure that if there if the sender is not on the sender bypass list, then the contract blacklist is enforced.
   BOOST_CHECK_EXCEPTION( tester1.chain->produce_blocks(), fc::exception,
                          fc_exception_message_is("account 'bob' is on the contract blacklist") );

   whitelist_blacklist_tester<tester> tester2;
   tester2.init(false);

   while( tester2.chain->control->head_block_num() < tester1.chain->control->head_block_num() ) {
      auto b = tester1.chain->control->fetch_block_by_number( tester2.chain->control->head_block_num()+1 );
      tester2.chain->push_block( b );
   }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
