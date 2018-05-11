#pragma once
#include <eosiolib/privileged.hpp>

#include <vector>

namespace eosio {
   /**
    *  Defines both the order, account name, and signing keys of the active set of producers.
    */
   struct producer_schedule {
      uint32_t                     version;   ///< sequentially incrementing version number
      std::vector<producer_key>    producers;
   };
} /// namespace eosio
