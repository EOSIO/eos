#include "eosio.system.hpp"
#include <eosiolib/dispatcher.hpp>

#include "delegate_bandwidth.cpp"
#include "producer_pay.cpp"
#include "voting.cpp"

system_contract::system_contract( account_name s )
:native(s),
 _voters(_self,_self),
 _producers(_self,_self),
 _global(_self,_self)
{
   _gstate = _global.exists() ? _global.get() : get_default_parameters();
}

system_contract::~system_contract() {
   _global.set( _gstate, _self );
   eosio_exit(0);
}


EOSIO_ABI( eosiosystem::system_contract,
     (setparams)
     // delegate_bandwith.cpp
     (delegatebw)(undelegatebw)(refund)
     (buyram)(sellram)
     (regproxy)
     // voting.cpp
     (unregproxy)(regproducer)(unregprod)(voteproducer)
     // producer_pay.cpp
     (claimrewards)
     // native.hpp
     //XXX
     (onblock)
     (newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(postrecovery)(passrecovery)(vetorecovery)(onerror)(canceldelay)
     // defined in eosio.system.hpp
     (nonce)
)
