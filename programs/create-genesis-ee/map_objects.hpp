#pragma once

#include "golos_dump_container.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <chainbase/chainbase.hpp>

namespace cyberway { namespace genesis {

using namespace boost::multi_index;

enum object_type
{
    null_object_type = 0,
    comment_header_object_type
};

struct comment_header : public chainbase::object<comment_header_object_type, comment_header> {
    template<typename Constructor, typename Allocator>
    comment_header(Constructor &&c, chainbase::allocator<Allocator> a) {
        c(*this);
    }

    id_type id;
    uint64_t hash;
    uint64_t offset;
    op_num create_op;
    op_num last_delete_op;
    int64_t net_rshares;
    int64_t author_reward;
    int64_t benefactor_reward;
    int64_t curator_reward;
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

} } // cyberway::genesis

CHAINBASE_SET_INDEX_TYPE(cyberway::genesis::comment_header, cyberway::genesis::comment_header_index)
