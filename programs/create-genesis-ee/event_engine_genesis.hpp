#pragma once

#include <boost/filesystem.hpp>
#include <fc/crypto/sha256.hpp>
#include <cyberway/genesis/ee_genesis_serializer.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bfs = boost::filesystem;

class event_engine_genesis final {
public:
    event_engine_genesis(const event_engine_genesis&) = delete;
    event_engine_genesis() = default;

    void start(const bfs::path& ee_directory, const fc::sha256& hash);
    void finalize();
public:
    ee_genesis_serializer messages;
    ee_genesis_serializer transfers;
    ee_genesis_serializer pinblocks;
};

struct vote_info {
    OBJECT_CTOR(vote_info);

    name voter;
    int16_t weight;
    fc::time_point_sec time;
    int64_t rshares;
};

struct reblog_info {
    OBJECT_CTOR(reblog_info);

    name account;
    string title;
    string body;
    fc::time_point_sec time;
};

struct comment_info {
    OBJECT_CTOR(comment_info);
    
    name parent_author;
    string parent_permlink;
    name author;
    string permlink;
    string title;
    string body;
    fc::flat_set<string> tags;
    string language;
    int64_t net_rshares;
    asset author_reward;
    asset benefactor_reward;
    asset curator_reward;
    std::vector<vote_info> votes;
    std::vector<reblog_info> reblogs;
};

struct transfer_info {
    OBJECT_CTOR(transfer_info);

    name from;
    name to;
    asset quantity;
    string memo;
};

struct pin_info {
    OBJECT_CTOR(pin_info);

    name pinner;
    name pinning;
};

struct block_info {
    OBJECT_CTOR(block_info);

    name blocker;
    name blocking;
};

} } // cyberway::genesis

FC_REFLECT(cyberway::genesis::vote_info, (voter)(weight)(time)(rshares))
FC_REFLECT(cyberway::genesis::reblog_info, (account)(title)(body)(time))
FC_REFLECT(cyberway::genesis::comment_info, (parent_author)(parent_permlink)(author)(permlink)(title)(body)
        (tags)(language)(net_rshares)(author_reward)(benefactor_reward)(curator_reward)(votes)(reblogs))
FC_REFLECT(cyberway::genesis::transfer_info, (from)(to)(quantity)(memo))
FC_REFLECT(cyberway::genesis::pin_info, (pinner)(pinning))
FC_REFLECT(cyberway::genesis::block_info, (blocker)(blocking))
