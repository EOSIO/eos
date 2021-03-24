#include "set_trx_payer.hpp"

[[eosio::action]]
void set_trx_payer::settrxrespyr() {
   bool result = set_transaction_resource_payer("respayer"_n, 1024, 1000);

   print("%", result);
}
