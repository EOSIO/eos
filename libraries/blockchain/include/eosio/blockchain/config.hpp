#pragma once
#include <eosio/blockchain/types.hpp>

namespace eosio { namespace blockchain {

   namespace config {

      // static const char*    public_key_prefix = defined in public_key.hpp because of circular dependency
      static const uint64_t        block_interval_ms        = 500;
      static const uint32_t        producer_count           = 21;
      static const auto            initial_blockchain_time  = time_point() + fc::seconds( 60*60*24*365*30 ); /// TODO: set genesis time from config
      static const auto            initial_private_key      = fc::ecc::private_key::regenerate( fc::sha256::hash("initial_private_key") );
      static const public_key_type initial_public_key       = initial_private_key.get_public_key();

   }

} } /// eosio::blockchain
