#include <eosio/eosio.hpp>

using namespace eosio;

class [[eosio::contract]] action_results : public contract {
  public:
      using contract::contract;

      [[eosio::action]]
      int actionresret() {
         return 10;
      }

  private:
};
