#include <eosio/eosio.hpp>
#include <eosio/privileged.hpp>

using namespace eosio;

class [[eosio::contract]] chgpayerhk : public contract {
   public:
      using contract::contract;

      [[eosio::action]] 
      void changepayer( void );

      [[eosio::action]] 
      void setpayerhf( void );
};
