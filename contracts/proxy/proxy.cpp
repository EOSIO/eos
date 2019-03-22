/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <proxy/proxy.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>

namespace proxy {
   using namespace eosio;

   namespace configs {

      bool get(config &out, const account_name &self) {
         auto it = chaindb_opt_find_by_pk(self, self, N(configs), out.key);
         auto pk = chaindb_current(self, it);
         if (pk != end_primary_key) {
            auto size = chaindb_datasize(self, it);
            eosio_assert(size == sizeof(config), "Wrong record size");
            chaindb_data(self, it, (void*)&out, sizeof(config));
            return true;
         } else {
            return false;
         }
      }

      void store(const config &in, const account_name &self) {
         auto it = chaindb_opt_find_by_pk(self, self, N(configs), in.key);
         auto pk = chaindb_current(self, it);
         if (pk != end_primary_key) {
            chaindb_update(self, self, N(configs), self, in.key, (void *)&in, sizeof(config));
         } else {
            chaindb_insert(self, self, N(configs), self, in.key, (void *)&in, sizeof(config));
         }
      }
   };

   template<typename T>
   void apply_transfer(uint64_t receiver, account_name /* code */, const T& transfer) {
      config code_config;
      const auto self = receiver;
      auto get_res = configs::get(code_config, self);
      eosio_assert(get_res, "Attempting to use unconfigured proxy");
      if (transfer.from == self) {
         eosio_assert(transfer.to == code_config.owner,  "proxy may only pay its owner" );
      } else {
         eosio_assert(transfer.to == self, "proxy is not involved in this transfer");
         T new_transfer = T(transfer);
         new_transfer.from = self;
         new_transfer.to = code_config.owner;

         auto id = code_config.next_id++;
         configs::store(code_config, self);

         transaction out;
         out.actions.emplace_back(permission_level{self, N(active)}, TOKEN_ACC, N(transfer), new_transfer);
         out.delay_sec = code_config.delay;
         out.send(id, self);
      }
   }

   void apply_setowner(uint64_t receiver, set_owner params) {
      const auto self = receiver;
      require_auth(params.owner);
      config code_config;
      configs::get(code_config, self);
      code_config.owner = params.owner;
      code_config.delay = params.delay;
      eosio::print("Setting owner to: ", name{params.owner}, " with delay: ", params.delay, "\n");
      configs::store(code_config, self);
   }

   template<size_t ...Args>
   void apply_onerror(uint64_t receiver, const onerror& error ) {
      eosio::print("starting onerror\n");
      const auto self = receiver;
      config code_config;
      eosio_assert(configs::get(code_config, self), "Attempting use of unconfigured proxy");

      auto id = code_config.next_id++;
      configs::store(code_config, self);

      eosio::print("Resending Transaction: ", error.sender_id, " as ", id, "\n");
      transaction dtrx = error.unpack_sent_trx();
      dtrx.delay_sec = code_config.delay;
      dtrx.send(id, self);
   }
}

using namespace proxy;
using namespace eosio;

extern "C" {

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if( code == SYSTEM_ACC && action == N(onerror) ) {
         apply_onerror( receiver, onerror::from_current_action() );
      } else if( code == TOKEN_ACC ) {
         if( action == N(transfer) ) {
            apply_transfer(receiver, code, unpack_action_data<eosio::token::transfer_args>());
         }
      } else if( code == receiver ) {
         if( action == N(setowner) ) {
            apply_setowner(receiver, unpack_action_data<set_owner>());
         }
      }
   }
}
