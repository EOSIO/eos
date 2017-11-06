/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <chainbase/chainbase.hpp>

namespace eosio {
using chain::block_id_type;
using chain::transaction_id_type;
using namespace boost::multi_index;

class transaction_history_object : public chainbase::object<chain::transaction_history_object_type, transaction_history_object> {
   OBJECT_CTOR(transaction_history_object)

   id_type             id;
   block_id_type       block_id;
   transaction_id_type transaction_id;
};

struct by_id;
struct by_trx_id;
using transaction_history_multi_index = chainbase::shared_multi_index_container<
   transaction_history_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(transaction_history_object, transaction_history_object::id_type, id)>,
      hashed_unique<tag<by_trx_id>, BOOST_MULTI_INDEX_MEMBER(transaction_history_object, transaction_id_type, transaction_id), std::hash<transaction_id_type>>
   >
>;

typedef chainbase::generic_index<transaction_history_multi_index> transaction_history_index;

}

CHAINBASE_SET_INDEX_TYPE( eosio::transaction_history_object, eosio::transaction_history_multi_index )

FC_REFLECT( eosio::transaction_history_object, (block_id)(transaction_id) )

