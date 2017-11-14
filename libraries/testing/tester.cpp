#include <eosio/testing/tester.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/contracts/types.hpp>

namespace eosio { namespace testing {


   tester::tester() {
      cfg.block_log_dir      = tempdir.path() / "blocklog";
      cfg.shared_memory_dir  = tempdir.path() / "shared";
      cfg.shared_memory_size = 1024*1024*8;

      cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
      cfg.genesis.initial_accounts.resize( config::producer_count );
      cfg.genesis.initial_producers.resize( config::producer_count );

      uint64_t init_name = N(inita);
      for( uint32_t i = 0; i < config::producer_count; ++i ) {
         auto pubkey = get_public_key(init_name);
         cfg.genesis.initial_accounts[i].name               = string(account_name(init_name));
         cfg.genesis.initial_accounts[i].owner_key          = get_public_key(init_name,"owner");
         cfg.genesis.initial_accounts[i].active_key         = get_public_key(init_name,"active");
         cfg.genesis.initial_accounts[i].liquid_balance     = asset::from_string( "1000000.0000 EOS" );
         cfg.genesis.initial_accounts[i].staking_balance    = asset::from_string( "1000000.0000 EOS" );
         cfg.genesis.initial_producers[i].owner_name        = string(account_name(init_name));
         cfg.genesis.initial_producers[i].block_signing_key = get_public_key( init_name, "producer" );

         init_name++;
      }
      open();
   }

   public_key_type  tester::get_public_key( name keyname, string role ) { 
      return get_private_key( keyname, role ).get_public_key(); 
   }

   private_key_type tester::get_private_key( name keyname, string role ) { 
      return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(string(keyname)+role));
   }

   void tester::close() {
      control.reset();
   }
   void tester::open() {
      control.reset( new chain_controller(cfg) );
   }

   signed_block tester::produce_block( fc::microseconds skip_time ) {
      auto head_time = control->head_block_time();
      auto next_time = head_time + skip_time;
      uint32_t slot  = control->get_slot_at_time( next_time );
      auto sch_pro   = control->get_scheduled_producer(slot);
      auto priv_key  = get_private_key( sch_pro, "producer" );

      return control->generate_block( next_time, sch_pro, priv_key );
   }


   void tester::produce_blocks( uint32_t n ) {
      for( uint32_t i = 0; i < n; ++i )
         produce_block();
   }

  void tester::set_tapos( signed_transaction& trx ) {
     trx.set_reference_block( control->head_block_id() );
  }


   void tester::create_account( account_name a, asset initial_balance, account_name creator ) {
      signed_transaction trx;
      set_tapos( trx );
      trx.write_scope = { creator, config::eosio_auth_scope };

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}}, contracts::newaccount{ 
                              .creator = creator,
                              .name = a,
                              .owner    = authority( get_public_key( a, "owner" ) ),
                              .active   = authority( get_public_key( a, "active" ) ),
                              .recovery = authority( get_public_key( a, "recovery" ) ),
                              .deposit  = initial_balance
                          });
      trx.sign( get_private_key( creator, "active" ), chain_id_type()  ); 

      push_transaction(trx);
   }

   void tester::push_transaction( signed_transaction& trx ) {
      set_tapos(trx);
      control->push_transaction( trx );
   }

   void tester::create_account( account_name a, string initial_balance, account_name creator ) {
      create_account( a, asset::from_string(initial_balance), creator );
   }
   

} }  /// eosio::test
