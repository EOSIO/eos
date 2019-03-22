#pragma once
#include <eosio/chain/abi_def.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <eosio/chain/authorization_manager.hpp>

namespace cyberway { namespace genesis {


static constexpr uint64_t gls_ctrl_account_name  = N(gls.ctrl);
static constexpr uint64_t gls_vest_account_name  = N(gls.vesting);
static constexpr uint64_t gls_post_account_name  = N(gls.publish);

void init_genesis_accounts(chaindb::chaindb_controller& chaindb);

}} // cyberway::genesis
