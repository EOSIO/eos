#pragma once

namespace eosio {

struct contract {
   contract( account_name n ):_self(n){}
   account_name _self;
};

} /// namespace eosio
