/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <proxy/proxy.hpp>
#include <eosiolib/native_currency.hpp>
#include <eosiolib/transaction.hpp>

namespace proxy {
   using namespace eosio;

   template<typename T>
   void apply_transfer(account_name code, const T& transfer) {
      config code_config;
      const auto self = current_receiver();
      auto get_res = configs::get(code_config, self);
      assert(get_res, "Attempting to use unconfigured proxy");
      if (transfer.from == self) {
         assert(transfer.to == code_config.owner,  "proxy may only pay its owner" );
      } else {
         assert(transfer.to == self, "proxy is not involved in this transfer");
         T new_transfer = T(transfer);
         new_transfer.from = self;
         new_transfer.to = code_config.owner;

         auto id = code_config.next_id++;
         configs::store(code_config, self);

         transaction out;
         out.actions.emplace_back(vector<permission_level>{{self, N(active)}}, new_transfer);
         out.send(id, now() + code_config.delay);
      }
   }

   void apply_setowner(set_owner params) {
      const auto self = current_receiver();
      config code_config;
      configs::get(code_config, self);
      code_config.owner = params.owner;
      code_config.delay = params.delay;
      eosio::print("Setting owner to: ", name(params.owner), " with delay: ", params.delay, "\n");
      configs::store(code_config, self);
   }

   template<size_t ...Args>
   void apply_onerror( const deferred_transaction& failed_dtrx ) {
      const auto self = current_receiver();
      config code_config;
      assert(configs::get(code_config, self), "Attempting to use unconfigured proxy");

      auto id = code_config.next_id++;
      configs::store(code_config, self);

      eosio::print("Resending Transaction: ", failed_dtrx.sender_id, " as ", id, "\n");
      failed_dtrx.send(id, now() + code_config.delay);
   }
}

using namespace proxy;
using namespace eosio;

extern "C" {

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if ( code == N(eosio) ) {
          if( action == N(transfer) ) {
             apply_transfer(code, unpack_action<native_currency::transfer>());
          } else if ( action == N(onerror)) {
             apply_onerror(deferred_transaction::from_current_action());
          }
       } else if (code == current_receiver() ) {
          if ( action == N(setowner)) {
             apply_setowner(current_action<set_owner>());
          }
       }
    }
}
