/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/types.hpp>
#include <eos/chain/authority.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {

   class rate_limiting_object : public chainbase::object<rate_limiting_object_type, rate_limiting_object> {
//      OBJECT_CTOR(rate_limiting_object)
      rate_limiting_object() = delete;
      public:
      template<typename Constructor, typename Allocator>
      rate_limiting_object(Constructor&& c, chainbase::allocator<Allocator>)
      { c(*this); }
      ~rate_limiting_object() {}
      id_type             id;
      AccountName         name;
      uint32_t            last_update_sec                   = 0;
      uint32_t            trans_msg_rate_per_account        = 0;
   };
   using rate_limiting_id_type = rate_limiting_object::id_type;

   struct by_name;
   using rate_limiting_index = chainbase::shared_multi_index_container<
      rate_limiting_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<rate_limiting_object, rate_limiting_object::id_type, &rate_limiting_object::id>>,
         ordered_unique<tag<by_name>, member<rate_limiting_object, AccountName, &rate_limiting_object::name>>
      >
   >;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::rate_limiting_object, eos::chain::rate_limiting_index)

FC_REFLECT(chainbase::oid<eos::chain::rate_limiting_object>, (_id))

FC_REFLECT(eos::chain::rate_limiting_object, (id)(name)(trans_msg_rate_per_account))
