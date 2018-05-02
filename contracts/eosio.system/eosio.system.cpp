#include "eosio.system.hpp"
#include <eosiolib/dispatcher.hpp>

#include "delegate_bandwidth.cpp"
#include "producer_pay.cpp"
#include "voting.cpp"


namespace eosiosystem {

   system_contract::system_contract( account_name s )
   :native(s),
    _voters(_self,_self),
    _producers(_self,_self),
    _global(_self,_self)
   {
      print( "construct system\n" );
      _gstate = _global.exists() ? _global.get() : get_default_parameters();
   }

   eosio_global_state system_contract::get_default_parameters() {
      eosio_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
   }


   system_contract::~system_contract() {
      print( "destruct system\n" );
      _global.set( _gstate, _self );
      eosio_exit(0);
   }

} /// eosio.system
 

EOSIO_ABI( eosiosystem::system_contract,
     (setparams)
     // delegate_bandwith.cpp
     (delegatebw)(undelegatebw)(refund)
     (buyram)(sellram)
     // voting.cpp
     (regproxy)(regproducer)(unregprod)(voteproducer)
     // producer_pay.cpp
     (claimrewards)
     // native.hpp
     //XXX
     (onblock)
     (newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(postrecovery)(passrecovery)(vetorecovery)(onerror)(canceldelay)
     // defined in eosio.system.hpp
     (nonce)
)
