#include "eosio.system.hpp"
#include <eosiolib/dispatcher.hpp>

#include "delegate_bandwidth.cpp"
#include "producer_pay.cpp"
#include "voting.cpp"
#include "exchange_state.cpp"


namespace eosiosystem {

   system_contract::system_contract( account_name s )
   :native(s),
    _voters(_self,_self),
    _producers(_self,_self),
    _global(_self,_self),
    _rammarket(_self,_self)
   {
      //print( "construct system\n" );
      _gstate = _global.exists() ? _global.get() : get_default_parameters();

      auto itr = _rammarket.find(S(4,RAMEOS));

      if( itr == _rammarket.end() ) {
         auto system_token_supply   = eosio::token(N(eosio.token)).get_supply(eosio::symbol_type(system_token_symbol).name()).amount;
         itr = _rammarket.emplace( _self, [&]( auto& m ) {
            m.supply.amount = 100000000000000ll;
            m.supply.symbol = S(4,RAMEOS);
            m.base.balance.amount = int64_t(_gstate.free_ram());
            m.base.balance.symbol = S(0,RAM);
            m.quote.balance.amount = system_token_supply / 1000;
            m.quote.balance.symbol = S(4,EOS);
         });
      } else {
         //print( "ram market already created" );
      }
   }

   eosio_global_state system_contract::get_default_parameters() {
      eosio_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
   }


   system_contract::~system_contract() {
      //print( "destruct system\n" );
      _global.set( _gstate, _self );
      //eosio_exit(0);
   }

   void system_contract::setram( uint64_t max_ram_size ) {
      require_auth( _self );

      eosio_assert( max_ram_size < 1024ll*1024*1024*1024*1024, "ram size is unrealistic" );
      eosio_assert( max_ram_size > _gstate.total_ram_bytes_reserved, "attempt to set max below reserved" );

      auto delta = int64_t(max_ram_size) - int64_t(_gstate.max_ram_size);
      auto itr = _rammarket.find(S(4,RAMEOS));

      /**
       *  Increase or decrease the amount of ram for sale based upon the change in max
       *  ram size.
       */
      _rammarket.modify( itr, 0, [&]( auto& m ) {
         m.base.balance.amount += delta;
      });

      _gstate.max_ram_size = max_ram_size;
      _global.set( _gstate, _self );
   }

} /// eosio.system
 

EOSIO_ABI( eosiosystem::system_contract,
     (setram)
     // delegate_bandwith.cpp
     (delegatebw)(undelegatebw)(refund)
     (buyram)(buyrambytes)(sellram)
     // voting.cpp
     (regproxy)(regproducer)(unregprod)(voteproducer)
     // producer_pay.cpp
     (claimrewards)
     // native.hpp
     //XXX
     (onblock)
     (newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(postrecovery)(passrecovery)(vetorecovery)(onerror)(canceldelay)
)
