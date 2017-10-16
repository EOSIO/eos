/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/types.hpp>
#include <eos/chain/BlockchainConfiguration.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {
class producer_object : public chainbase::object<producer_object_type, producer_object> {
   OBJECT_CTOR(producer_object)

   id_type          id;
   AccountName      owner;
   uint64_t         last_aslot = 0;
   public_key_type  signing_key;
   int64_t          total_missed = 0;
   uint32_t         last_confirmed_block_num = 0;

   /// The blockchain configuration values this producer recommends
   BlockchainConfiguration configuration;
};

struct by_key;
struct by_owner;
using producer_multi_index = chainbase::shared_multi_index_container<
   producer_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<producer_object, producer_object::id_type, &producer_object::id>>,
      ordered_unique<tag<by_owner>, member<producer_object, AccountName, &producer_object::owner>>,
      ordered_unique<tag<by_key>,
         composite_key<producer_object,
            member<producer_object, public_key_type, &producer_object::signing_key>,
            member<producer_object, producer_object::id_type, &producer_object::id>
         >
      >
   >
>;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::producer_object, eos::chain::producer_multi_index)

FC_REFLECT(eos::chain::producer_object::id_type, (_id))
FC_REFLECT(eos::chain::producer_object, (id)(owner)(last_aslot)(signing_key)(total_missed)(last_confirmed_block_num)
           (configuration))
