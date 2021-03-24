#include <eosio/eosio.hpp>
#include <eosio/privileged.hpp>

using namespace eosio;

class [[eosio::contract]] set_trx_payer : public contract {
   public:
      using contract::contract;

      [[eosio::action]] 
      void settrxrespyr( void );
};
