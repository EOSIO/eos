/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/dispatcher.hpp>

using namespace eosio;

class deferred_test : public eosio::contract {
   public:
      using contract::contract;

      struct deferfunc_args {
         uint64_t payload;
      };

      //@abi action
      void defercall( account_name payer, uint64_t sender_id, account_name contract, uint64_t payload ) {
         print( "defercall called on ", name{_self}, "\n" );
         require_auth( payer );

         print( "deferred send of deferfunc action to ", name{contract}, " by ", name{payer}, " with sender id ", sender_id );
         transaction trx;
         deferfunc_args a = {.payload = payload};
         trx.actions.emplace_back(permission_level{_self, N(active)}, contract, N(deferfunc), a);
         trx.send( (static_cast<uint128_t>(payer) << 64) | sender_id, payer);
      }

      //@abi action
      void deferfunc( uint64_t payload ) {
         print("deferfunc called on ", name{_self}, " with payload = ", payload, "\n");
         eosio_assert( payload != 13, "value 13 not allowed in payload" );
      }

   private:
};

void apply_onerror(uint64_t receiver, const onerror& error ) {
   print("onerror called on ", name{receiver}, "\n");
}

extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if( code == N(eosio) && action == N(onerror) ) {
         apply_onerror( receiver, onerror::from_current_action() );
      } else if( code == receiver ) {
         deferred_test thiscontract(receiver);
         if( action == N(defercall) ) {
            execute_action( &thiscontract, &deferred_test::defercall );
         } else if( action == N(deferfunc) ) {
            execute_action( &thiscontract, &deferred_test::deferfunc );
         }
      }
   }
}

//EOSIO_ABI( deferred_test, (defercall)(deferfunc) )
