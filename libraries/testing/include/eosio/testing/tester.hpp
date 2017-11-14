#pragma once
#include <eosio/chain/chain_controller.hpp>

namespace eosio { namespace testing {

   using namespace eosio::chain;


   /**
    *  @class tester
    *  @brief provides utility function to simplify the creation of unit tests
    */
   class tester {
      public:
         tester();

         void              close();
         void              open();

         signed_block      produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) );
         void              produce_blocks( uint32_t n = 1 );

         void              push_transaction( signed_transaction& trx );
         void              set_tapos( signed_transaction& trx );

         void              create_account( account_name name, asset initial_balance = asset(), account_name creator = N(inita) );
         void              create_account( account_name name, string balance = "0.0000 EOS", account_name creator = N(inita));

         public_key_type   get_public_key( name keyname, string role = "owner" );
         private_key_type  get_private_key( name keyname, string role = "owner" );

         unique_ptr<chain_controller> control;

      private:
         fc::temp_directory                  tempdir;
         chain_controller::controller_config cfg;
   };

} } /// eosio::testing
