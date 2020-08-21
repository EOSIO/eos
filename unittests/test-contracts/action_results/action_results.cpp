#include <eosio/eosio.hpp>
#include <vector>

using namespace eosio;
using namespace std;

class [[eosio::contract]] action_results : public contract {
  public:
      using contract::contract;

      [[eosio::action]]
      int actionresret() {
         return 10;
      }

      [[eosio::action]]
      vector<char> retoverlim() {
         return vector<char>(512, '0');
      }

  private:
};
