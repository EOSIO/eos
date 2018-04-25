#include "eosio.system.hpp"
#include <eosiolib/dispatcher.hpp>

#include "delegate_bandwidth.cpp"
#include "producer_pay.cpp"
#include "voting.cpp"
#include "referendum.cpp"

EOSIO_ABI( eosiosystem::system_contract,
           // delegate_bandwith.cpp
           (delegatebw)(undelegatebw)(refund)
           (regproxy)
           // voting.cpp
           (unregproxy)(regproducer)(unregprod)(voteproducer)(onblock)
           // producer_pay.cpp
           (claimrewards)
           //referendum.cpp
           (propose)(cancel)(voteprop)(unvoteprop)
           // native.hpp
           (newaccount)(setcode)(setabi)(updateauth)(deleteauth)(linkauth)(unlinkauth)(postrecovery)(passrecovery)(vetorecovery)(onerror)(canceldelay)
           // defined in eosio.system.hpp
           (nonce)
)
