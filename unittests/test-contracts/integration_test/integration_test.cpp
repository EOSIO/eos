/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

using namespace eosio;

CONTRACT integration_test : public contract {
   using contract::contract;

   TABLE payload {
      uint64_t              key;
      std::vector<uint64_t> data;

      uint64_t primary_key()const { return key; }

      EOSLIB_SERIALIZE( payload, (key)(data) )
   };
   typedef eosio::multi_index<"payloads"_n, payload> payloads;

public:
   ACTION store( name from, name to, uint64_t num ) {
      require_auth(from);
        
      eosio_assert( is_account( to ), "to account does not exist" );
      eosio_assert( num < std::numeric_limits<size_t>::max(), "num to large" );
        
      payloads data ( _self, from.value );
      uint64_t key = 0;
      const uint64_t num_keys = 5;
        
      while ( data.find( key ) != data.end() ) {
         key += num_keys;
      }
      for (size_t i = 0; i < num_keys; ++i) {
         data.emplace(from, [&]( auto& g ) {
                                 g.key = key + i;
                                 g.data = std::vector<uint64_t>( static_cast<size_t>(num), 5); });
      }
   }
};

EOSIO_DISPATCH( integration_test, (store) )
