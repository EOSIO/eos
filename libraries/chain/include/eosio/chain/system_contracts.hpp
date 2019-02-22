#pragma once

#include <eosio/chain/types.hpp>
#include <vector>
#include <map>

namespace eosio { namespace chain {

using contract_hashes_map = std::map<name, std::vector<digest_type>>;

// Note: 1st values of each vector ignored if "allow initial set" enabled
static const contract_hashes_map allowed_code_hashes = {
    {config::system_account_name, {
        digest_type{"11cbab7dc161da95072f197e323ec425c6415cdd6e2b2afc37cffe59e241b5bc"}, // precompiled cyber.bios code
    }},
    {config::msig_account_name, {
        digest_type{"4eaf92464fa36fca59d4ea96872b4afa14750272c952dbf15c36c94a339de65d"},
    }},
    {config::token_account_name, {
        digest_type{"50bb85bcd813379ae4794a75aeb1d0817fb3d1133f6305d80d50165a2fc403d9"},
    }},
    {config::domain_account_name, {
        digest_type{"59c7781bc8d2dfa33f81d00341c63db1e142fc5426834167a5dd3fc63dddec6c"},
    }},
};

// Note: 1st values of each vector ignored if "allow initial set" enabled and there is no system abi set on contract
static const contract_hashes_map allowed_abi_hashes = {
    {config::system_account_name, {
        digest_type{"de12d0cf86da8ac9efd35a22c4800f5631b58cb738c7ad1ec926f525b02196d2"}, // precompiled cyber.bios abi
    }},
    {config::msig_account_name, {
        digest_type{"e7c807b2358f17e9b812b178434c8ea8ebdeebbef3b4307e81f8342d837748b4"},
    }},
    {config::token_account_name, {
        digest_type{"441466ff1d6f23c156d9e205307a1cc6eda07aaf82a4d8c07071e6b582120f1d"},
    }},
    {config::domain_account_name, {
        digest_type{"4ee692ac5c827771f9bb2280b7436056760850736e20a35b2fd3671148c3767a"},
    }},
};


}} // eosio::chain
