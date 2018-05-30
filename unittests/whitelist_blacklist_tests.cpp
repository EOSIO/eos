#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/testing/tester_network.hpp>

#include <fc/variant_object.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

using mvo = fc::mutable_variant_object;

class whitelist_blacklist_tester {
   public:
      whitelist_blacklist_tester() {}

      static controller::config get_default_chain_configuration( const fc::path& p ) {
         controller::config cfg;
         cfg.blocks_dir = p / config::default_blocks_dir_name;
         cfg.state_dir  = p / config::default_state_dir_name;
         cfg.state_size = 1024*1024*8;
         cfg.reversible_cache_size = 1024*1024*8;
         cfg.contracts_console = true;

         cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
         cfg.genesis.initial_key = base_tester::get_public_key( config::system_account_name, "active" );

         for(int i = 0; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
            if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--binaryen"))
               cfg.wasm_runtime = chain::wasm_interface::vm_type::binaryen;
            else if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wavm"))
               cfg.wasm_runtime = chain::wasm_interface::vm_type::wavm;
            else
               cfg.wasm_runtime = chain::wasm_interface::vm_type::binaryen;
         }

         return cfg;
      }

      void init() {
         auto cfg = get_default_chain_configuration( tempdir.path() );
         cfg.actor_whitelist = actor_whitelist;
         cfg.actor_blacklist = actor_blacklist;
         cfg.contract_whitelist = contract_whitelist;
         cfg.contract_blacklist = contract_blacklist;

         chain.emplace(cfg);
         chain->create_accounts({N(eosio.token), N(alice), N(bob), N(charlie)});
         chain->set_code(N(eosio.token), eosio_token_wast);
         chain->set_abi(N(eosio.token), eosio_token_abi);
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

      transaction_trace_ptr transfer( account_name from, account_name to, string quantity = "1.00 TOK" ) {
         return chain->push_action( N(eosio.token), N(transfer), from, mvo()
            ( "from", from )
            ( "to", to )
            ( "quantity", quantity )
            ( "memo", "" )
         );
      }

      fc::temp_directory       tempdir; // Must come before chain
      fc::optional<TESTER>     chain;
      flat_set<account_name>   actor_whitelist;
      flat_set<account_name>   actor_blacklist;
      flat_set<account_name>   contract_whitelist;
      flat_set<account_name>   contract_blacklist;
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
   whitelist_blacklist_tester test;
   test.actor_whitelist = {N(eosio), N(eosio.token), N(alice)};
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
   whitelist_blacklist_tester test;
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
   whitelist_blacklist_tester test;
   test.contract_whitelist = {N(eosio), N(eosio.token), N(bob)};
   test.init();

   test.transfer( N(eosio.token), N(alice), "1000.00 TOK" );

   test.transfer( N(alice), N(eosio.token) );

   test.transfer( N(alice), N(bob) );
   test.transfer( N(alice), N(charlie), "100.00 TOK" );

   test.transfer( N(charlie), N(alice) );

   test.chain->produce_blocks();

   test.chain->set_code(N(bob), eosio_token_wast);
   test.chain->set_abi(N(bob), eosio_token_abi);

   test.chain->produce_blocks();

   test.chain->set_code(N(charlie), eosio_token_wast);
   test.chain->set_abi(N(charlie), eosio_token_abi);

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
   whitelist_blacklist_tester test;
   test.contract_blacklist = {N(charlie)};
   test.init();

   test.transfer( N(eosio.token), N(alice), "1000.00 TOK" );

   test.transfer( N(alice), N(eosio.token) );

   test.transfer( N(alice), N(bob) );
   test.transfer( N(alice), N(charlie), "100.00 TOK" );

   test.transfer( N(charlie), N(alice) );

   test.chain->produce_blocks();

   test.chain->set_code(N(bob), eosio_token_wast);
   test.chain->set_abi(N(bob), eosio_token_abi);

   test.chain->produce_blocks();

   test.chain->set_code(N(charlie), eosio_token_wast);
   test.chain->set_abi(N(charlie), eosio_token_abi);

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

BOOST_AUTO_TEST_SUITE_END()
