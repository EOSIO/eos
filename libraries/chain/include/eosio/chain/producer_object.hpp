/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/chain_config.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {
class producer_object : public chainbase::object<producer_object_type, producer_object> {
   OBJECT_CTOR(producer_object)

   id_type            id;
   account_name       owner;
   uint64_t           last_aslot = 0;
   public_key_type    signing_key;
   int64_t            total_missed = 0;
   uint32_t           last_confirmed_block_num = 0;


   /// The blockchain configuration values this producer recommends
   chain_config       configuration;
};

struct by_key;
struct by_owner;
using producer_multi_index = chainbase::shared_multi_index_container<
   producer_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<producer_object, producer_object::id_type, &producer_object::id>>,
      ordered_unique<tag<by_owner>, member<producer_object, account_name, &producer_object::owner>>,
      ordered_unique<tag<by_key>,
         composite_key<producer_object,
            member<producer_object, public_key_type, &producer_object::signing_key>,
            member<producer_object, producer_object::id_type, &producer_object::id>
         >
      >
   >
>;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::producer_object, eosio::chain::producer_multi_index)

FC_REFLECT(eosio::chain::producer_object::id_type, (_id))
FC_REFLECT(eosio::chain::producer_object, (id)(owner)(last_aslot)(signing_key)(total_missed)(last_confirmed_block_num)
           (configuration))
