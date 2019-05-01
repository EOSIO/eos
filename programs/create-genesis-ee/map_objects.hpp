#pragma once

#include "golos_dump_container.hpp"
#include "golos_types.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <chainbase/chainbase.hpp>

namespace cyberway { namespace genesis {

using namespace boost::multi_index;
using namespace cyberway::golos;

enum object_type
{
    null_object_type = 0,
    comment_header_object_type,
    vote_header_object_type,
    reblog_header_object_type
};

struct comment_header : public chainbase::object<comment_header_object_type, comment_header> {
    template<typename Constructor, typename Allocator>
    comment_header(Constructor &&c, chainbase::allocator<Allocator> a) {
        c(*this);
    }

    id_type id;
    uint64_t hash;
    uint64_t offset;
    operation_number create_op;
    operation_number last_delete_op;
    int64_t net_rshares;
    int64_t author_reward = 0;
    int64_t benefactor_reward = 0;
    int64_t curator_reward = 0;
};

struct vote_header : public chainbase::object<vote_header_object_type, vote_header> {
    template<typename Constructor, typename Allocator>
    vote_header(Constructor &&c, chainbase::allocator<Allocator> a) {
        c(*this);
    }

    id_type id;
    uint64_t hash;
    account_name_type voter;
    operation_number op_num;
    int16_t weight = 0;
    fc::time_point_sec timestamp;
};

struct reblog_header : public chainbase::object<reblog_header_object_type, reblog_header> {
    template<typename Constructor, typename Allocator>
    reblog_header(Constructor &&c, chainbase::allocator<Allocator> a) {
        c(*this);
    }

    id_type id;
    uint64_t hash;
    account_name_type account;
    operation_number op_num;
    uint64_t offset;
};

struct by_id;
struct by_hash;
struct by_created;

using comment_header_index = chainbase::shared_multi_index_container<
    comment_header,
    indexed_by<
        ordered_unique<
            tag<by_id>,
            member<comment_header, comment_header::id_type, &comment_header::id>>,
        ordered_unique<
            tag<by_hash>,
            member<comment_header, uint64_t, &comment_header::hash>>>
>;

struct by_hash_voter;

using vote_header_index = chainbase::shared_multi_index_container<
    vote_header,
    indexed_by<
        ordered_unique<
            tag<by_id>,
            member<vote_header, vote_header::id_type, &vote_header::id>>,
        ordered_unique<
            tag<by_hash_voter>,
            composite_key<
                vote_header,
                member<vote_header, uint64_t, &vote_header::hash>,
                member<vote_header, account_name_type, &vote_header::voter>>,
            composite_key_compare<
                std::less<uint64_t>,
                std::less<account_name_type>>>>
>;

struct by_hash_account;

using reblog_header_index = chainbase::shared_multi_index_container<
    reblog_header,
    indexed_by<
        ordered_unique<
            tag<by_id>,
            member<reblog_header, reblog_header::id_type, &reblog_header::id>>,
        ordered_unique<
            tag<by_hash_account>,
            composite_key<
                reblog_header,
                member<reblog_header, uint64_t, &reblog_header::hash>,
                member<reblog_header, account_name_type, &reblog_header::account>>,
            composite_key_compare<
                std::less<uint64_t>,
                std::less<account_name_type>>>>
>;

} } // cyberway::genesis

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::comment_header, cyberway::genesis::comment_header_index)

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::vote_header, cyberway::genesis::vote_header_index)

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::reblog_header, cyberway::genesis::reblog_header_index)
