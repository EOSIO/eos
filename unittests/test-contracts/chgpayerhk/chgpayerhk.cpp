#include "chgpayerhk.hpp"

[[eosio::action]]
void chgpayerhk::changepayer() {
   bool result = register_transaction_hook(eosio::transaction_hook::preexecution, "chgpayerhk"_n, "setpayerhf"_n);

   print(result);
}

[[eosio::action]]
void chgpayerhk::setpayerhf() {
   bool result = set_transaction_resource_payer("respayer"_n, 1024, 1000);

   print(result);
}