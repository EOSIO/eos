#pragma once
#include <eosiolib/privileged.hpp>

#include <vector>

namespace eosio {

   /**
    *  Defines both the order, account name, and signing keys of the active set of producers.
    *
    *  @brief Defines both the order, account name, and signing keys of the active set of producers.
    */
   struct producer_schedule {
      /**
       * Version number of the schedule. It is sequentially incrementing version number
       * 
       * @brief Version number of the schedule
       */
      uint32_t                     version;

      /**
       * List of producers for this schedule, including its signing key
       *
       * @brief List of producers for this schedule, including its signing key
       */
      std::vector<producer_key>    producers;
   };

   /// @} producertype
} /// namespace eosio
