#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/abi_def.hpp>
#include <fc/reflect/reflect.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;

struct ee_genesis_header {
    char magic[12] = "CyberwayEEG";
    uint32_t version = 1;
    fc::sha256 hash;

    bool is_valid() {
        ee_genesis_header oth;
        return string(magic) == oth.magic && version == oth.version;
    }
};

struct ee_table_header {
    account_name code;
    table_name   name;
    type_name    abi_type;
    uint32_t     count;
};

enum class section_ee_type {
    messages
};

struct section_ee_header {
    section_ee_type type;

    section_ee_header(section_ee_type type_)
            : type(type_) {
    }
};

struct message_ee_object {
    account_name parent_author;
    string parent_permlink;
    account_name author;
    string permlink;
    string title;
    string body;
    flat_set<string> tags;
    int64_t net_rshares;
    asset author_reward;
    asset benefactor_reward;
    asset curator_reward;
};

}} // cyberway::genesis

FC_REFLECT(cyberway::genesis::ee_table_header, (code)(name)(abi_type)(count))
FC_REFLECT_ENUM(cyberway::genesis::section_ee_type, (messages))
FC_REFLECT(cyberway::genesis::section_ee_header, (type))

FC_REFLECT(cyberway::genesis::message_ee_object, (parent_author)(parent_permlink)(author)(permlink)(title)(body)(tags)
    (net_rshares)(author_reward)(benefactor_reward)(curator_reward))
