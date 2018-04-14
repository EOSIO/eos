#include <eosio/chain/controller.hpp>


using namespace eosio::chain;

int main( int argc, char** argv ) {

   try { try {
      controller c( {} );
      controller c2( {.shared_memory_dir = "c2dir", .block_log_dir = "c2dir" } );

      for( uint32_t i = 0; i < 4; ++i ) {
          c.start_block();

          signed_transaction trx;
          trx.actions.resize(1);
          trx.actions[0].account = N(exchange);
          trx.actions[0].name    = N(transfer);
          trx.actions[0].authorization = { {N(dan),N(active) } };
          trx.expiration = c.head_block_time() + fc::seconds(3);
          trx.ref_block_num = c.head_block_num();
          c.push_transaction( std::make_shared<transaction_metadata>(trx) );


          c.finalize_block();
          c.sign_block( []( digest_type d ) {
                        auto priv = fc::variant("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3").as<private_key_type>();
                        return priv.sign(d);
                        });




          c.commit_block();
          idump((c.head_block_state()->header.id())(c.head_block_state()->block_num));

          ilog( "\n START CLONE  " );
          c2.push_block( c.head_block_state()->block );
          idump((c2.head_block_state()->header.id())(c2.head_block_state()->block_num));
          ilog( "\n END CLONE  \n" );
      }


   } FC_CAPTURE_AND_RETHROW() 
   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
   }
   return 0;
}
