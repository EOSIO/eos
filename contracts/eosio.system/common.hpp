#pragma once
#include <eosiolib/eosio.hpp>

#include <eosiolib/generic_currency.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>

namespace eosiosystem {

   template<account_name SystemAccount>
   class common {
      public:
         static constexpr account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;
         typedef typename currency::token_type system_token_type;
         static constexpr uint64_t currency_symbol = currency::symbol;        // S(4,EOS)
         static constexpr uint8_t currency_decimals = currency_symbol & 0xFF; // 4
         static const uint32_t max_inflation_rate = 5;                        // 5% annual inflation
#warning "change to config parameter"
         static constexpr uint32_t producer_repititions = 12;
         static constexpr uint32_t blocks_per_cycle = 21 * producer_repititions;
         static const uint32_t mseconds_per_day = 24 * 3600 * 1000;
         static constexpr uint32_t days_per_4years = 1461;
         
         struct eosio_parameters : eosio::blockchain_parameters {
            uint32_t          percent_of_max_inflation_rate = 0; // inflation coefficient * 10000 (i.e. inflation in percent * 100)
            uint32_t          storage_reserve_ratio = 1000;      // ratio * 1000
            system_token_type payment_per_block = system_token_type();
            system_token_type payment_to_eos_bucket = system_token_type();
            time              first_block_time_in_cycle = 0;
            time              last_bucket_fill_time = 0;
            system_token_type eos_bucket = system_token_type();
               
            EOSLIB_SERIALIZE_DERIVED( eosio_parameters, eosio::blockchain_parameters, (percent_of_max_inflation_rate)(storage_reserve_ratio)
                                      (payment_per_block)(last_bucket_fill_time)(eos_bucket) )
         };

         struct eosio_global_state : eosio_parameters {
            uint64_t total_storage_bytes_reserved = 0;
            system_token_type total_storage_stake;

            EOSLIB_SERIALIZE_DERIVED( eosio_global_state, eosio_parameters, (total_storage_bytes_reserved)(total_storage_stake) )
         };

         typedef eosio::singleton<SystemAccount, N(inflation), SystemAccount, eosio_global_state> global_state_singleton;

         static eosio_global_state& get_default_parameters() {
            static eosio_global_state dp;
            get_blockchain_parameters(&dp);
            return dp;
         }
   };

   std::pair<uint64_t, uint64_t> int_logarithm_one_plus(uint32_t x) {
      static const uint64_t denom = 10000;
      uint64_t ret = 0;
      uint64_t x_power = x;
      const uint8_t n = 4;
      static constexpr uint64_t ten_power = denom * denom * denom * denom;
      for (uint64_t i = 0, p = ten_power; i < n; ++i) {
         uint64_t factor = x_power * p / (i+1);
         ret += factor;
         ret -= 2 * (i % 2) * factor;
         x_power *= x;
         p /= denom;
      }
      return std::make_pair(ret * denom/ten_power, denom);
   }

}
