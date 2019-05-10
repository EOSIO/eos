#pragma once
#include <eosio/chain/name.hpp>


namespace cyberway { namespace genesis {

static constexpr uint64_t gls_issuer_account_name = N(gls.issuer);
static constexpr uint64_t gls_ctrl_account_name = N(gls.ctrl);
static constexpr uint64_t gls_vest_account_name = N(gls.vesting);
static constexpr uint64_t gls_post_account_name = N(gls.publish);
static constexpr uint64_t gls_social_account_name = N(gls.social);
static constexpr uint64_t gls_charge_account_name = N(gls.charge);
constexpr auto notify_account_name = gls_ctrl_account_name;

constexpr auto GBG = SY(3,GBG);
constexpr auto GLS = SY(3,GOLOS);
constexpr auto GESTS = SY(6,GESTS);
constexpr auto VESTS = SY(6,GOLOS);                 // Golos dApp vesting
constexpr auto posting_auth_name = "posting";
constexpr auto golos_domain_name = "golos";

constexpr auto owner_recovery_wait_seconds = 60*60*24*30;
constexpr auto withdraw_interval_seconds = 60*60*24*7;
constexpr auto withdraw_intervals = 13;

constexpr int64_t system_max_supply = 1'000'000'000ll * 10000;  // 4 digits precision
constexpr int64_t golos_max_supply = 1'000'000'000ll * 1000;    // 3 digits precision

constexpr int fixp_fract_digits = 12;


}} // cyberway::genesis
