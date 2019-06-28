/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/chain/whitelisted_intrinsics.hpp>
#include <chainbase/chainbase.hpp>
#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   /**
    * @class protocol_state_object
    * @brief Maintains global state information about consensus protocol rules
    * @ingroup object
    * @ingroup implementation
    */
   class protocol_state_object : public chainbase::object<protocol_state_object_type, protocol_state_object>
   {
      OBJECT_CTOR(protocol_state_object, (activated_protocol_features)(preactivated_protocol_features)(whitelisted_intrinsics))

   public:
      struct activated_protocol_feature {
         digest_type feature_digest;
         uint32_t    activation_block_num = 0;

         activated_protocol_feature() = default;

         activated_protocol_feature( const digest_type& feature_digest, uint32_t activation_block_num )
         :feature_digest( feature_digest )
         ,activation_block_num( activation_block_num )
         {}

         bool operator==(const activated_protocol_feature& rhs) const {
            return feature_digest == rhs.feature_digest && activation_block_num == rhs.activation_block_num;
         }
      };

   public:
      id_type                                    id;
      shared_vector<activated_protocol_feature>  activated_protocol_features;
      shared_vector<digest_type>                 preactivated_protocol_features;
      whitelisted_intrinsics_type                whitelisted_intrinsics;
      uint32_t                                   num_supported_key_types = 0;
   };

   using protocol_state_multi_index = chainbase::shared_multi_index_container<
      protocol_state_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(protocol_state_object, protocol_state_object::id_type, id)
         >
      >
   >;

   struct snapshot_protocol_state_object {
      vector<protocol_state_object::activated_protocol_feature> activated_protocol_features;
      vector<digest_type>                                       preactivated_protocol_features;
      std::set<std::string>                                     whitelisted_intrinsics;
      uint32_t                                                  num_supported_key_types = 0;
   };

   namespace detail {
      template<>
      struct snapshot_row_traits<protocol_state_object> {
         using value_type = protocol_state_object;
         using snapshot_type = snapshot_protocol_state_object;

         static snapshot_protocol_state_object to_snapshot_row( const protocol_state_object& value,
                                                                const chainbase::database& db );

         static void from_snapshot_row( snapshot_protocol_state_object&& row,
                                        protocol_state_object& value,
                                        chainbase::database& db );
      };
   }

}}

CHAINBASE_SET_INDEX_TYPE(eosio::chain::protocol_state_object, eosio::chain::protocol_state_multi_index)

FC_REFLECT(eosio::chain::protocol_state_object::activated_protocol_feature,
            (feature_digest)(activation_block_num)
          )

FC_REFLECT(eosio::chain::protocol_state_object,
            (activated_protocol_features)(preactivated_protocol_features)(whitelisted_intrinsics)(num_supported_key_types)
          )

FC_REFLECT(eosio::chain::snapshot_protocol_state_object,
            (activated_protocol_features)(preactivated_protocol_features)(whitelisted_intrinsics)(num_supported_key_types)
          )
