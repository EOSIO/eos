#pragma once
#include <eosiolib/public_key.hpp>

#include <vector>

namespace eosio {
   /**
   *  @defgroup producertype Producer Type
   *  @ingroup types
   *  @brief Defines producer type
   *
   *  @{
   */
   
   /**
    * Maps producer with its signing key, used for producer schedule
    * 
    * @brief Maps producer with its signing key
    */
   struct producer_key {

      /**
       * Name of the producer
       * 
       * @brief Name of the producer
       */
      account_name     producer_name;

      /**
       * Block signing key used by this producer
       * 
       * @brief Block signing key used by this producer
       */
      public_key       block_signing_key;
   };

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
