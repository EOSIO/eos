#pragma once
#include "posting_rules.hpp"
#include <eosio/chain/types.hpp>
#include <fc/reflect/reflect.hpp>
#include <boost/filesystem.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bfs = boost::filesystem;

struct genesis_info {

    struct file_hash {
        bfs::path path;
        fc::sha256 hash;
    };

    struct account {
        account_name name;
        std::string owner_key;      // use string here to allow "INITIAL" keyword
        std::string active_key;
        fc::optional<file_hash> abi;
        fc::optional<file_hash> code;
    };

    bfs::path state_file;   // ? file_hash
    bfs::path genesis_json;
    std::vector<account> accounts;

    // parameters
    struct stake_params {
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
FC_REFLECT(cyberway::genesis::genesis_info::account, (name)(owner_key)(active_key)(abi)(code))
FC_REFLECT(cyberway::genesis::genesis_info::stake_params,
    (max_proxies)(payout_step_length)(payout_steps_num)(min_own_staked_for_election))
FC_REFLECT(cyberway::genesis::genesis_info::hardfork_info, (version)(time))
FC_REFLECT(cyberway::genesis::genesis_info::parameters, (stake)(posting_rules)(require_hardfork))
FC_REFLECT(cyberway::genesis::genesis_info, (state_file)(genesis_json)(accounts)(params))
