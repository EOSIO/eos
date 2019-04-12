/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/database_utils.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   class code_object : public chainbase::object<code_object_type, code_object> {
      OBJECT_CTOR(code_object, (code))

      id_type      id;
      digest_type  code_id;
      shared_blob  code;
      uint64_t     code_ref_count;
      uint32_t     first_block_used;
   };

   struct by_code_id;
   using code_index = chainbase::shared_multi_index_container<
      code_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<code_object, code_object::id_type, &code_object::id>>,
         ordered_unique<tag<by_code_id>, member<code_object, digest_type, &code_object::code_id>>
      >
   >;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::code_object, eosio::chain::code_index)

FC_REFLECT(eosio::chain::code_object, (code_id)(code)(code_ref_count)(first_block_used))
