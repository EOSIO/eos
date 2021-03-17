#include "trxhookreg.hpp"

[[eosio::action]]
void trxhookreg::preexreg() {
   bool result = register_transaction_hook(eosio::transaction_hook::preexecution, "payloadless"_n, "doit"_n);

   print("%", result);
}