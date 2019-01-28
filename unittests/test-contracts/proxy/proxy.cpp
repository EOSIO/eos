/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/transaction.hpp>

#include "proxy.hpp"

using namespace eosio;

namespace proxy {
   namespace configs {
      bool get( config &out, const name &self ) {
         auto it = db_find_i64( self.value, self.value, name{"config"}.value, out.key.value );
         if (it != -1) {
            auto size = db_get_i64( it, (char*)&out, sizeof(config) );
            eosio_assert( size == sizeof(config), "Wrong record size" );
            return true;
         } else {
            return false;
         }
      }

      void store(const config &in, const name &self) {
         auto it = db_find_i64(self.value, self.value, name{"config"}.value, in.key.value);
         if (it != -1) {
            db_update_i64(it, self.value, (const char *)&in, sizeof(config));
         } else {
            db_store_i64(self.value, "config"_n.value, self.value, config::key.value, (const char *)&in, sizeof(config));
         }
      }
   } /// configs

   template<typename T>
   void apply_transfer( uint64_t receiver, name /* code */, const T& transfer ) {
      config code_config;
      const auto self = receiver;
      auto get_res = configs::get( code_config, name{self} );
      eosio_assert( get_res, "Attempting to use unconfigured proxy" );
      if ( transfer.from == name{self} ) {
         eosio_assert( transfer.to == name{code_config.owner},  "proxy may only pay its owner" );
      } else {
         eosio_assert( transfer.to == name{self}, "proxy is not involved in this transfer" );
         T new_transfer = T(transfer);
         new_transfer.from = name{self};
         new_transfer.to = name{code_config.owner};

         auto id = code_config.next_id++;
         configs::store( code_config, name{self} );

         transaction out;
         out.actions.emplace_back( permission_level{name{self}, "active"_n}, "eosio.token"_n, "transfer"_n, new_transfer );
         out.delay_sec = code_config.delay;
         out.send( id, name{self} );
      }
   }

void apply_setowner(uint64_t receiver, set_owner params) {
   const auto self = receiver;
   require_auth(params.owner);
   config code_config;
   configs::get(code_config, name{self});
   code_config.owner = params.owner.value;
   code_config.delay = params.delay;
   eosio::print("Setting owner to: ", name{params.owner}, " with delay: ", params.delay, "\n");
   configs::store(code_config, name{self});
}

template<size_t ...Args>
void apply_onerror(uint64_t receiver, const onerror& error ) {
   eosio::print("starting onerror\n");
   const auto self = receiver;
   config code_config;
   eosio_assert(configs::get(code_config, name{self}), "Attempting use of unconfigured proxy");

   auto id = code_config.next_id++;
   configs::store(code_config, name{self});

   eosio::print("Resending Transaction: ", error.sender_id, " as ", id, "\n");
   transaction dtrx = error.unpack_sent_trx();
   dtrx.delay_sec = code_config.delay;
   dtrx.send(id, name{self});
}   }; // namespace proxy

extern "C" {

   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if( code == "eosio"_n.value && action == "onerror"_n.value ) {
         proxy::apply_onerror( receiver, onerror::from_current_action() );
      } else if( code ==( "eosio.token"_n.value ) ) {
         if( action ==( "transfer"_n.value ) ) {
            apply_transfer( receiver, name{code}, unpack_action_data<proxy::transfer_args>() );
         }
      } else if( code == receiver ) {
         if( action == "setowner"_n.value ) {
            apply_setowner( receiver, unpack_action_data<proxy::set_owner>() );
         }
      }
   }
}
