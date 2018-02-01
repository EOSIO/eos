#pragma once
#include <eosiolib/generic_currency.hpp>

namespace eosio {

/* TODO: make native currency match generic currency, requires changes in the native code
struct native_token_def {
   static const uint64_t code = N(eosio);
   static const uint64_t symbol = N(eos);
};
using native_currency = generic_currency<native_token_def>;
 */

struct native_currency {
   struct transfer : public action_meta<N(eosio),N(transfer)> {
      account_name from;
      account_name to;
      uint64_t     quantity;
      string       memo;

      template<typename DataStream>
      friend DataStream& operator << ( DataStream& ds, const transfer& t ){
         return ds << t.from << t.to << t.quantity << t.memo;
      }

      template<typename DataStream>
      friend DataStream& operator >> ( DataStream& ds,  transfer& t ){
         return ds >> t.from >> t.to >> t.quantity >> t.memo;

      }

   };
};

} /// namespace eosio

