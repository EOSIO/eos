#include <eosio/eosio.hpp>
#include <eosio/privileged.hpp>

using namespace eosio;

class [[eosio::contract]] trxhookreg : public contract {
   public:
      using contract::contract;

      [[eosio::action]] 
      void preexreg( void );
};
