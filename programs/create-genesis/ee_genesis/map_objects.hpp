#pragma once

#include "golos_dump_container.hpp"
#include "golos_operations.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <chainbase/chainbase.hpp>

namespace cyberway { namespace genesis { namespace ee {

using namespace boost::multi_index;
using namespace cyberway::golos::ee;

enum object_type
{
    null_object_type = 0,
    comment_header_object_type,
    vote_header_object_type,
    reblog_header_object_type,
    follow_header_object_type,
    account_metadata_object_type,
};

struct comment_header : public chainbase::object<comment_header_object_type, comment_header> {
    template<typename Constructor, typename Allocator>
    comment_header(Constructor &&c, chainbase::allocator<Allocator> a) {
        c(*this);
    }

    id_type id;
    uint64_t hash;
    uint64_t parent_hash;
    uint64_t offset;
    operation_number create_op;
    operation_number last_delete_op;
    fc::time_point_sec created;
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
    int64_t rshares;
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

struct follow_header : public chainbase::object<follow_header_object_type, follow_header> {
    template<typename Constructor, typename Allocator>
    follow_header(Constructor &&c, chainbase::allocator<Allocator> a) {
        c(*this);
    }

    id_type id;
    account_name_type follower;
    account_name_type following;
    bool ignores;
};

struct account_metadata : public chainbase::object<account_metadata_object_type, account_metadata> {
    template<typename Constructor, typename Allocator>
    account_metadata(Constructor &&c, chainbase::allocator<Allocator> a) {
        c(*this);
    }

    id_type id;
    account_name_type account;
    uint64_t offset;
};

struct by_id;
struct by_hash;
struct by_parent_hash;

using comment_header_index = chainbase::shared_multi_index_container<
    comment_header,
    indexed_by<
        ordered_unique<
            tag<by_id>,
            member<comment_header, comment_header::id_type, &comment_header::id>>,
        ordered_unique<
            tag<by_hash>,
            member<comment_header, uint64_t, &comment_header::hash>>,
        ordered_non_unique<
            tag<by_parent_hash>,
            member<comment_header, uint64_t, &comment_header::parent_hash>>>
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

struct by_pair;

using follow_header_index = chainbase::shared_multi_index_container<
    follow_header,
    indexed_by<
        ordered_unique<
            tag<by_id>,
            member<follow_header, follow_header::id_type, &follow_header::id>>,
        ordered_unique<
            tag<by_pair>,
            composite_key<
                follow_header,
                member<follow_header, account_name_type, &follow_header::follower>,
                member<follow_header, account_name_type, &follow_header::following>>,
            composite_key_compare<
                std::less<account_name_type>,
                std::less<account_name_type>>>>
>;

struct by_account;

using account_metadata_index = chainbase::shared_multi_index_container<
    account_metadata,
    indexed_by<
        ordered_unique<
            tag<by_id>,
            member<account_metadata, account_metadata::id_type, &account_metadata::id>>,
        ordered_unique<
            tag<by_account>,
            member<account_metadata, account_name_type, &account_metadata::account>>>
>;

} } } // cyberway::genesis::ee

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::ee::comment_header, cyberway::genesis::ee::comment_header_index)

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::ee::vote_header, cyberway::genesis::ee::vote_header_index)

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::ee::reblog_header, cyberway::genesis::ee::reblog_header_index)

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::ee::follow_header, cyberway::genesis::ee::follow_header_index)

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::ee::account_metadata, cyberway::genesis::ee::account_metadata_index)
