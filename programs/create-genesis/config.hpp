#pragma once
#include <eosio/chain/name.hpp>


namespace cyberway { namespace genesis {

constexpr auto GBG = SY(3,GBG);
constexpr auto GLS = SY(3,GOLOS);
constexpr auto GESTS = SY(6,GESTS);
constexpr auto VESTS = SY(6,GOLOS);                 // Golos dApp vesting
constexpr auto posting_auth_name = "posting";

constexpr auto withdraw_interval_seconds = 60*60*24*7;
constexpr auto withdraw_intervals = 13;

constexpr int fixp_fract_digits = 12;


}} // cyberway::genesis
