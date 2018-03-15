/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/action.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/types.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/system.h>
#include <eosiolib/privileged.h>

using namespace eosio;

namespace testsystem {
   template<uint64_t Val>
   struct dispatchable {
      constexpr static uint64_t action_name = Val;
   };

   struct set_account_limits : dispatchable<N(setalimits)> {
      account_name      account;
      uint64_t          ram_bytes  = 0;
      uint64_t          net_weight = 0;
      uint64_t          cpu_weight = 0;

      static void process(const set_account_limits& act) {
         set_resource_limits(act.account, act.ram_bytes, act.net_weight, act.cpu_weight, 0);
      }

      EOSLIB_SERIALIZE( set_account_limits, (account)(ram_bytes)(net_weight)(cpu_weight) )
   };

   struct set_global_limits : dispatchable<N(setglimits)> {
      int64_t          cpu_usec_per_period;

      static void process(const set_global_limits& ) {
         // TODO: support this
      }

      EOSLIB_SERIALIZE( set_global_limits, (cpu_usec_per_period) )
   };

   struct producer_key {
      account_name          account;
      std::string           public_key;

      EOSLIB_SERIALIZE( producer_key, (account) ) //(public_key) )
   };

   struct set_producers : dispatchable<N(setprods)> {
      static void process(const set_producers&) {
         char buffer[action_data_size()];
         read_action_data( buffer, sizeof(buffer) );
         set_active_producers(buffer, sizeof(buffer));
      }
   };

   struct require_auth : dispatchable<N(reqauth)> {
      account_name from;

      static void process(const require_auth& r) {
         ::require_auth(r.from);
      }
   };

   template<typename ...Ts>
   struct dispatcher_impl;

   template<typename T>
   struct dispatcher_impl<T> {
      static bool dispatch(uint64_t action) {
         if (action == T::action_name) {
            T::process(current_action_data<T>());
            return true;
         }

         return false;
      }
   };

   template<typename T, typename ...Rem>
   struct dispatcher_impl<T, Rem...> {
      static bool dispatch(uint64_t action) {
         return dispatcher_impl<T>::dispatch(action) || dispatcher_impl<Rem...>::dispatch(action);
      }
   };

   using dispatcher = dispatcher_impl<set_account_limits, set_global_limits, set_producers, require_auth>;
};

extern "C" {

/// The apply method implements the dispatch of events to this contract
void apply( uint64_t code, uint64_t act ) {
   if (code == current_receiver()) {
      testsystem::dispatcher::dispatch(act);
   }
}

} // extern "C"
