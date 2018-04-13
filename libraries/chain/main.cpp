#include <eosio/chain/controller.hpp>


using namespace eosio::chain;

int main( int argc, char** argv ) {

   try { try {
      controller c( {} );
      controller c2( {.shared_memory_dir = "c2dir", .block_log_dir = "c2dir" } );

for( uint32_t i = 0; i < 4; ++i ) {
      c.start_block();
      c.finalize_block();
      c.sign_block( []( digest_type d ) {
                    auto priv = fc::variant("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3").as<private_key_type>();
                    return priv.sign(d);
                    });
      c.commit_block();

      c2.push_block( c.head_block_state()->block );
      idump((c.head_block_state()->header));
      idump((c2.head_block_state()->header));
}


   } FC_CAPTURE_AND_RETHROW() 
   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
   }
   return 0;
}
