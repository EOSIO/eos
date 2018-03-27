#pragma once
#include <eosiolib/public_key.hpp>

#include <vector>

namespace eosio {
   struct producer_key {
      account_name     producer_name;
      public_key       block_signing_key;
   };
   /**
    *  Defines both the order, account name, and signing keys of the active set of producers. 
    */
   struct producer_schedule {
      uint32_t                     version;   ///< sequentially incrementing version number
      std::vector<producer_key>    producers;
   };
} /// namespace eosio
