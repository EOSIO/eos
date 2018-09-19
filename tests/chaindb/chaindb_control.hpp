#pragma once

#include "chaindb.h"

#include <eosio/chain/abi_serializer.hpp>

bool chaindb_set_abi(account_name_t code, eosio::chain::abi_def);