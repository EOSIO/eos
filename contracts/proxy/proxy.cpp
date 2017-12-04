/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <proxy/proxy.hpp>
#include <currency/currency.hpp>

namespace proxy {
   using namespace eosio;

   template<typename T>
   void apply_transfer(account_name code, const T& transfer) {
      const auto self = current_receiver();
      config code_config;
      assert(configs::get(code_config, self), "Attempting to use unconfigured proxy");
      if (transfer.from == self) {
         assert(transfer.to == code_config.owner,  "proxy may only pay its owner" );
      } else {
         assert(transfer.to == self, "proxy is not involved in this transfer");
         T new_transfer = T(transfer);
         new_transfer.from = self;
         new_transfer.to = code_config.owner;

         auto out_msg = message(code, N(transfer), new_transfer, self, N(code));
         transaction out;
         out.add_message(out_msg);
         out.add_scope(self);
         out.add_scope(code_config.owner);
         out.send();
      }
   }

   void apply_setowner(set_owner params) {
      const auto self = current_receiver();
      config code_config;
      configs::get(code_config, self);
      code_config.owner = params.owner;
      configs::store(code_config, self);
   }
}

using namespace proxy;
using namespace eosio;

extern "C" {
    void init()  {
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(currency) ) {
          if( action == N(transfer) ) {
             apply_transfer(code, current_action<currency::transfer>());
          }
       } else if ( code == N(eos) ) {
          if( action == N(transfer) ) {
             apply_transfer(code, current_action<eosio::transfer>());
          }
       } else if (code == N(proxy) ) {
          if ( action == N(setowner)) {
             apply_setowner(current_action<set_owner>());
          }
       }
    }
}
