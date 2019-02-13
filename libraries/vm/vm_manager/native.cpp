//#include <ro_db.hpp>
#include "native.hpp"
#include <eosio.system/eosio.system.hpp>
using namespace eosiosystem;

bool is_jit_account(uint64_t account) {
    jit_account_table      _jit_accounts(N(eosio), N(eosio));
    if (_jit_accounts.find(account) != _jit_accounts.end()) {
        return true;
    }
    return false;
}
