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

      struct eosio_parameters : eosio::blockchain_parameters {
         uint32_t inflation_rate = 0; // inflation coefficient * 10000 (i.e. inflation in percent * 100)
         uint32_t storage_reserve_ratio = 1; // ratio * 1000
         uint64_t total_storage_bytes_reserved = 0;
         system_token_type total_storage_stake;
         eosio_parameters() { bzero(this, sizeof(*this)); }

         EOSLIB_SERIALIZE_DERIVED( eosio_parameters, eosio::blockchain_parameters, (inflation_rate)(storage_reserve_ratio)
                                   (storage_reserve_ratio)(total_storage_bytes_reserved)(total_storage_stake) )
      };

      typedef eosio::singleton<SystemAccount, N(inflation), SystemAccount, eosio_parameters> eosio_parameters_singleton;
   };
}
