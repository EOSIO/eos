#pragma once
#include "posting_rules.hpp"
#include <eosio/chain/types.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <fc/reflect/reflect.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bfs = boost::filesystem;

struct genesis_info {

    struct file_hash {
        bfs::path path;
        fc::sha256 hash;
    };

    struct permission {
        permission_name name;
        fc::optional<permission_name> parent;   // defaults: "" for "owner" permission; "owner" for "active"; "active" for others; numeric id if adding permission to existing account
        std::string key;                // can use "INITIAL"; only empty "key" can co-exist with non-empty "keys" and vice-versa
        std::vector<string> keys;       // can use "INITIAL"
        std::vector<string> accounts;   // can use "name@permission"

        void init() {
            if (key.length() > 0) {
                keys.emplace_back(key);
                key = "";
            }
        }

        permission_name get_parent() const {
            if (parent) {
                return *parent;
            }
            return name == N(owner) ? N() : name == N(active) ? N(owner) : N(active);
        }

        std::vector<key_weight> key_weights(const public_key_type& initial_key) const {
            std::vector<key_weight> r;
            for (const auto& k: keys) {
                r.emplace_back(key_weight{k == "INITIAL" ? initial_key : public_key_type(k), 1});
            }
            return r;
        }
        std::vector<permission_level_weight> perm_levels(account_name code) const {
            std::vector<permission_level_weight> r;
            for (const auto& a: accounts) {
                std::vector<string> parts;
                split(parts, a, boost::algorithm::is_any_of("@"));
                auto acc = parts[0].size() == 0 ? code : account_name(parts[0]);
                auto perm = account_name(parts.size() == 1 ? "" : parts[1].c_str());
                r.emplace_back(permission_level_weight{permission_level{acc, perm}, 1});
            }
            return r;
        }
        authority make_authority(const public_key_type& initial_key, account_name code) const {
            return authority(1, key_weights(initial_key), perm_levels(code));
        }
    };

    struct account {
        account_name name;
        fc::optional<bool> update;
        std::vector<permission> permissions;
        fc::optional<file_hash> abi;
        fc::optional<file_hash> code;
    };

    bfs::path state_file;   // ? file_hash
    bfs::path genesis_json;
    std::vector<account> accounts;

    struct auth_link {
        using names_pair = std::pair<name,name>;
        string permission;              // account@permission
        std::vector<string> links;      // each link is "contract:action" or "contract:*" (simple "contract" also works)

        names_pair get_permission() const {
            std::vector<string> parts;
            split(parts, permission, boost::algorithm::is_any_of("@"));
            return names_pair{name(parts[0]), name(parts[1])};
        }

        std::vector<names_pair> get_links() const {
            std::vector<names_pair> r;
            for (const auto& l: links) {
                std::vector<string> parts;
                split(parts, l, boost::algorithm::is_any_of(":"));
                name action{parts.size() > 1 ? string_to_name(parts[1].c_str()) : 0};
                r.emplace_back(names_pair{name(parts[0]), action});
            }
            return r;
        }
    };
    std::vector<auth_link> auth_links;

    struct table {
        struct row {
            string scope;   // can be name/symbol/symbol_code
            name payer;
            uint64_t pk;
            variant data;

            uint64_t get_scope() const {
                name maybe_name{string_to_name(scope.c_str())};
                if (maybe_name.to_string() == scope) {
                    return name{scope}.value;
                } else if (scope.find(',') != string::npos) {
                    return symbol::from_string(scope).value();
                } else {
                    return symbol::from_string("0,"+scope).to_symbol_code().value;
                }
            }
        };
        account_name code;
        name table;
        string abi_type;
        std::vector<row> rows;
    };
    std::vector<table> tables;

    // parameters
    struct golos_config {
        std::string domain;

        struct golos_names {
            account_name issuer;
            account_name control;
            account_name vesting;
            account_name posting;
            account_name social;
            account_name charge;
            account_name memo;
        } names;

        struct recovery_params {
            uint32_t wait_days;
        } recovery;

        int64_t max_supply;
        int64_t sys_max_supply;

        struct posts_trx_params {
            uint16_t expiration_hours = 48;     // allows close-post transaction to live longer
            fc::optional<time_point_sec> initial_from;  // set to time of golos_state creation to shift transactions delay relatively (offset between initial_time and this value will be added to trxs)
        } posts_trx;
    } golos;

    struct stake_params {
        bool enabled = false;
        std::vector<uint8_t> max_proxies;
        int64_t payout_step_length;
        uint16_t payout_steps_num;
        int64_t min_own_staked_for_election = 0;
    };
    struct hardfork_info {
        uint32_t version;       // High byte is major version, next one is hardfork version, low 16bits are revision
        time_point_sec time;
    };

    struct parameters {
        stake_params stake;
        posting_rules posting_rules;
        fc::optional<hardfork_info> require_hardfork;
    } params;
};

}} // cyberway::genesis

FC_REFLECT(cyberway::genesis::genesis_info::file_hash, (path)(hash))
FC_REFLECT(cyberway::genesis::genesis_info::permission, (name)(parent)(key)(keys)(accounts))
FC_REFLECT(cyberway::genesis::genesis_info::account, (name)(update)(permissions)(abi)(code))
FC_REFLECT(cyberway::genesis::genesis_info::auth_link, (permission)(links))
FC_REFLECT(cyberway::genesis::genesis_info::table::row, (scope)(payer)(pk)(data))
FC_REFLECT(cyberway::genesis::genesis_info::table, (code)(table)(abi_type)(rows))
FC_REFLECT(cyberway::genesis::genesis_info::golos_config::golos_names,
    (issuer)(control)(vesting)(posting)(social)(charge)(memo))
FC_REFLECT(cyberway::genesis::genesis_info::golos_config::recovery_params, (wait_days))
FC_REFLECT(cyberway::genesis::genesis_info::golos_config::posts_trx_params, (expiration_hours)(initial_from))
FC_REFLECT(cyberway::genesis::genesis_info::golos_config, (domain)(names)(recovery)(max_supply)(sys_max_supply)(posts_trx))
FC_REFLECT(cyberway::genesis::genesis_info::stake_params,
    (enabled)(max_proxies)(payout_step_length)(payout_steps_num)(min_own_staked_for_election))
FC_REFLECT(cyberway::genesis::genesis_info::hardfork_info, (version)(time))
FC_REFLECT(cyberway::genesis::genesis_info::parameters, (stake)(posting_rules)(require_hardfork))
FC_REFLECT(cyberway::genesis::genesis_info, (state_file)(genesis_json)(accounts)(auth_links)(tables)(golos)(params))
