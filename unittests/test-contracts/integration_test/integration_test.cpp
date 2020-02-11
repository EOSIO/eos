#include "integration_test.hpp"

using namespace eosio;

void integration_test::store( name from, name to, uint64_t num ) {
   require_auth( from );

   check( is_account( to ), "to account does not exist" );
   check( num < std::numeric_limits<size_t>::max(), "num to large" );

   payloads_table data( get_self(), from.value );
   uint64_t key = 0;
   const uint64_t num_keys = 5;

   while( data.find( key ) != data.end() ) {
      key += num_keys;
   }

   for( uint64_t i = 0; i < num_keys; ++i ) {
      data.emplace( from, [&]( auto& g ) {
         g.key = key + i;
         g.data = std::vector<uint64_t>( static_cast<size_t>(num), 5 );
      } );
   }
}
