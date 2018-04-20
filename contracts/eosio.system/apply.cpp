#include "eosio.system.hpp"

#include <eosiolib/dispatcher.hpp>

EOSIO_ABI( eosiosystem::system_contract,
           // delegate_bandwith.cpp
           (delegatebw)(undelegatebw)(refund)
           (regproxy)
           // voting.cpp
           (unregproxy)(regproducer)(unregprod)(voteproducer)(onblock)
           // producer_pay.cpp
           (claimrewards)
           // native.hpp
           //XXX
           //(newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(postrecovery)(passrecovery)(vetorecovery)(onerror)(canceldelay)
           // defined in eosio.system.hpp
           (nonce)
)
