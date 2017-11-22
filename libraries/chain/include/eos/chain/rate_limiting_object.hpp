/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/types.hpp>
#include <eos/chain/authority.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   /**
    *  @brief tracks message rates associated with an account
    *  @ingroup object
    *
    *  An account has rate limits imposed on it for sending transaction messages for
    *  being part of the message's authorities and for it's code being used
    *  computationally. This structure is used for tracking data associated with the
    *  rate calculation.
    */
   class rate_limiting_object : public chainbase::object<rate_limiting_object_type, rate_limiting_object> {
      OBJECT_CTOR(rate_limiting_object)
      id_type             id;
      account_name        name;
      fc::time_point_sec  per_auth_account_txn_msg_rate_last_update_sec;
      uint32_t            per_auth_account_txn_msg_rate = 0;
      fc::time_point_sec  per_code_account_txn_msg_rate_last_update_sec;
      uint32_t            per_code_account_txn_msg_rate = 0;
      int64_t             per_code_account_db_bytes = 0;
   };
   using rate_limiting_id_type = rate_limiting_object::id_type;

   struct by_name;
   using rate_limiting_index = chainbase::shared_multi_index_container<
      rate_limiting_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<rate_limiting_object, rate_limiting_object::id_type, &rate_limiting_object::id>>,
         ordered_unique<tag<by_name>, member<rate_limiting_object, account_name, &rate_limiting_object::name>>
      >
   >;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::rate_limiting_object, eosio::chain::rate_limiting_index)

FC_REFLECT(chainbase::oid<eosio::chain::rate_limiting_object>, (_id))

FC_REFLECT(eosio::chain::rate_limiting_object, (id)(name)(per_auth_account_txn_msg_rate_last_update_sec)(per_auth_account_txn_msg_rate)(per_code_account_txn_msg_rate_last_update_sec)(per_code_account_txn_msg_rate))
