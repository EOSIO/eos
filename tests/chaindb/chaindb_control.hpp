#pragma once

#include "chaindb.h"

#include <eosio/chain/abi_serializer.hpp>

bool chaindb_set_abi(account_name_t code, eosio::chain::abi_def);
bool chaindb_close_abi(account_name_t code);
bool chaindb_close_code(account_name_t code);