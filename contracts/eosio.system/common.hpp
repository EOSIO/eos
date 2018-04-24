/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   template<account_name SystemAccount>
   class common {
      public:
         static constexpr account_name system_account = SystemAccount;


         static constexpr uint32_t     blocks_per_producer = 12;
         static constexpr uint32_t     seconds_per_day = 24 * 3600;
         static constexpr uint32_t     days_per_4years = 1461;

         static eosio_global_state& get_default_parameters() {
            static eosio_global_state dp;
            get_blockchain_parameters(dp);
            return dp;
         }
   };

}
