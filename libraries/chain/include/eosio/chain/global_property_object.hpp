#pragma once
#include <fc/uint128.hpp>
#include <fc/array.hpp>

#include <eosio/chain/types.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/chain_snapshot.hpp>
#include <eosio/chain/kv_config.hpp>
#include <eosio/chain/wasm_config.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/incremental_merkle.hpp>
#include <eosio/chain/snapshot.hpp>
#include <chainbase/chainbase.hpp>
#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   /**
    * a fc::raw::unpack compatible version of the old global_property_object structure stored in
    * version 2 snapshots and before
    */
   namespace legacy {
      struct snapshot_global_property_object_v2 {
         static constexpr uint32_t minimum_version = 0;
         static constexpr uint32_t maximum_version = 2;
         static_assert(chain_snapshot_header::minimum_compatible_version <= maximum_version, "snapshot_global_property_object_v2 is no longer needed");

         std::optional<block_num_type>    proposed_schedule_block_num;
         producer_schedule_type           proposed_schedule;
         chain_config_v0                  configuration;
      };
      struct snapshot_global_property_object_v3 {
         static constexpr uint32_t minimum_version = 3;
         static constexpr uint32_t maximum_version = 3;
         static_assert(chain_snapshot_header::minimum_compatible_version <= maximum_version, "snapshot_global_property_object_v3 is no longer needed");

         std::optional<block_num_type>       proposed_schedule_block_num;
         producer_authority_schedule         proposed_schedule;
         chain_config_v0                     configuration;
         chain_id_type                       chain_id;
      };
      struct snapshot_global_property_object_v4 {
         static constexpr uint32_t minimum_version = 4;
         static constexpr uint32_t maximum_version = 4;
         static_assert(chain_snapshot_header::minimum_compatible_version <= maximum_version, "snapshot_global_property_object_v4 is no longer needed");

         std::optional<block_num_type>       proposed_schedule_block_num;
         producer_authority_schedule         proposed_schedule;
         chain_config_v0                     configuration;
         chain_id_type                       chain_id;
         kv_database_config                  kv_configuration;
         wasm_config                         wasm_configuration;
      };
   }

   /**
    * @class global_property_object
    * @brief Maintains global state information about block producer schedules and chain configuration parameters
    * @ingroup object
    * @ingroup implementation
    */
   class global_property_object : public chainbase::object<global_property_object_type, global_property_object>
   {
      OBJECT_CTOR(global_property_object, (proposed_schedule)(proposed_security_group_participants))

   public:
      id_type                             id;
      std::optional<block_num_type>       proposed_schedule_block_num;
      shared_producer_authority_schedule  proposed_schedule;
      chain_config                        configuration;
      chain_id_type                       chain_id;
      kv_database_config                  kv_configuration;
      wasm_config                         wasm_configuration;
      block_num_type                      proposed_security_group_block_num = 0;
      // members that are containers need to be shared_* containers, since this object is stored in a multi-index container
      shared_set<account_name>            proposed_security_group_participants;

      void initalize_from(const legacy::snapshot_global_property_object_v2& legacy, const chain_id_type& chain_id_val,
                          const kv_database_config& kv_config_val, const wasm_config& wasm_config_val) {
         proposed_schedule_block_num = legacy.proposed_schedule_block_num;
         proposed_schedule = producer_authority_schedule(legacy.proposed_schedule).to_shared(proposed_schedule.producers.get_allocator());
         configuration = legacy.configuration;
         chain_id = chain_id_val;
         kv_configuration = kv_config_val;
         wasm_configuration = wasm_config_val;
      }

      void initalize_from( const legacy::snapshot_global_property_object_v3& legacy, const kv_database_config& kv_config_val, const wasm_config& wasm_config_val ) {
         proposed_schedule_block_num = legacy.proposed_schedule_block_num;
         proposed_schedule = legacy.proposed_schedule.to_shared(proposed_schedule.producers.get_allocator());
         configuration = legacy.configuration;
         chain_id = legacy.chain_id;
         kv_configuration = kv_config_val;
         wasm_configuration = wasm_config_val;
      }

      void initalize_from( const legacy::snapshot_global_property_object_v4& legacy ) {
         proposed_schedule_block_num = legacy.proposed_schedule_block_num;
         proposed_schedule = legacy.proposed_schedule.to_shared(proposed_schedule.producers.get_allocator());
         configuration = legacy.configuration;
         chain_id = legacy.chain_id;
         kv_configuration = legacy.kv_configuration;
         wasm_configuration = legacy.wasm_configuration;
      }

      template<typename Iter>
      void set_proposed_security_group_participants(Iter begin, Iter end) {
         proposed_security_group_participants = {begin, end,
                                                 proposed_security_group_participants.key_comp(),
                                                 proposed_security_group_participants.get_allocator()};
      }
   };


   using global_property_multi_index = chainbase::shared_multi_index_container<
      global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)
         >
      >
   >;

   struct snapshot_global_property_object {
      std::optional<block_num_type>       proposed_schedule_block_num;
      producer_authority_schedule         proposed_schedule;
      chain_config                        configuration;
      chain_id_type                       chain_id;
      kv_database_config                  kv_configuration;
      wasm_config                         wasm_configuration;

      static constexpr uint32_t minimum_version_with_extension = 6;

      struct extension_v0 {
         // libstdc++ requires the following two constructors to work. 
         extension_v0(){};
         extension_v0(block_num_type num, flat_set<account_name> participants)
             : proposed_security_group_block_num(num)
             , proposed_security_group_participants(std::move(participants)) {}

         block_num_type                      proposed_security_group_block_num = 0;
         flat_set<account_name>              proposed_security_group_participants;
      };

      // for future extensions, please use the following pattern:
      // 
      // struct extension_v1 : extension_v0 { new_field_t new_field; };
      // using extension_t = std::variant<extension_v0, extension_v1>;
      //  
      // In addition, get_gpo_extension(), set_gpo_extension() and 
      // eosio::ship_protocol::global_property from ship_protocol.hpp 
      // in abieos has to be changed accordingly. 

      using extension_t = std::variant<extension_v0>;
      extension_t extension;
   };

   inline snapshot_global_property_object::extension_t get_gpo_extension(const global_property_object& gpo) {
      return snapshot_global_property_object::extension_v0{
          gpo.proposed_security_group_block_num,
          {gpo.proposed_security_group_participants.begin(), gpo.proposed_security_group_participants.end()}};
   }

   inline void set_gpo_extension(global_property_object&                             gpo,
                                 const snapshot_global_property_object::extension_t& extension) {
      std::visit(
          [&gpo](auto& ext) {
             gpo.proposed_security_group_block_num    = ext.proposed_security_group_block_num;
             gpo.set_proposed_security_group_participants(ext.proposed_security_group_participants.begin(),
                                                          ext.proposed_security_group_participants.end());
          },
          extension);
   }

   namespace detail {
      template<>
      struct snapshot_row_traits<global_property_object> {
         using value_type = global_property_object;
         using snapshot_type = snapshot_global_property_object;

         static snapshot_global_property_object to_snapshot_row(const global_property_object& value,
                                                                const chainbase::database&) {
            return {value.proposed_schedule_block_num,
                    producer_authority_schedule::from_shared(value.proposed_schedule),
                    value.configuration,
                    value.chain_id,
                    value.kv_configuration,
                    value.wasm_configuration,
                    get_gpo_extension(value)};
         }

         static void from_snapshot_row(snapshot_global_property_object&& row, global_property_object& value,
                                       chainbase::database&) {
            value.proposed_schedule_block_num = row.proposed_schedule_block_num;
            value.proposed_schedule =
                row.proposed_schedule.to_shared(value.proposed_schedule.producers.get_allocator());
            value.configuration      = row.configuration;
            value.chain_id           = row.chain_id;
            value.kv_configuration   = row.kv_configuration;
            value.wasm_configuration = row.wasm_configuration;
            set_gpo_extension(value, row.extension);
         }
      };
   }

   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information that frequently change
    * @ingroup object
    * @ingroup implementation
    */
   class dynamic_global_property_object : public chainbase::object<dynamic_global_property_object_type, dynamic_global_property_object>
   {
        OBJECT_CTOR(dynamic_global_property_object)

        id_type    id;
        uint64_t   global_action_sequence = 0;
   };

   using dynamic_global_property_multi_index = chainbase::shared_multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(dynamic_global_property_object, dynamic_global_property_object::id_type, id)
         >
      >
   >;

}}

CHAINBASE_SET_INDEX_TYPE(eosio::chain::global_property_object, eosio::chain::global_property_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::dynamic_global_property_object,
                         eosio::chain::dynamic_global_property_multi_index)

FC_REFLECT(eosio::chain::global_property_object,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)(chain_id)(kv_configuration)(wasm_configuration)
            (proposed_security_group_block_num)(proposed_security_group_participants)
          )

FC_REFLECT(eosio::chain::legacy::snapshot_global_property_object_v2,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)
          )

FC_REFLECT(eosio::chain::legacy::snapshot_global_property_object_v3,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)(chain_id)
          )

FC_REFLECT(eosio::chain::legacy::snapshot_global_property_object_v4,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)(chain_id)(kv_configuration)(wasm_configuration)
          )

FC_REFLECT(eosio::chain::snapshot_global_property_object::extension_v0,
            (proposed_security_group_block_num)(proposed_security_group_participants)
          )

FC_REFLECT(eosio::chain::snapshot_global_property_object,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)(chain_id)(kv_configuration)(wasm_configuration)(extension)
          )

FC_REFLECT(eosio::chain::dynamic_global_property_object,
            (global_action_sequence)
          )

namespace fc {
namespace raw {
namespace detail {

template <typename VersionedStream>
struct unpack_object_visitor<VersionedStream, eosio::chain::snapshot_global_property_object>
    : fc::reflector_init_visitor<eosio::chain::snapshot_global_property_object> {
   unpack_object_visitor(eosio::chain::snapshot_global_property_object& _c, VersionedStream& _s)
       : fc::reflector_init_visitor<eosio::chain::snapshot_global_property_object>(_c)
       , s(_s) {}

   template <typename T, typename C, T(C::*p)>
   inline void operator()(const char* name) const {
      try {
         if constexpr (std::is_same_v<eosio::chain::snapshot_global_property_object::extension_t,
                                      std::decay_t<decltype(this->obj.*p)>>)
            if (s.version < eosio::chain::snapshot_global_property_object::minimum_version_with_extension)
               return;

         fc::raw::unpack(s, this->obj.*p);
      }
      FC_RETHROW_EXCEPTIONS(warn, "Error unpacking field ${field}", ("field", name))
   }

 private:
   VersionedStream& s;
};
} // namespace detail
} // namespace raw
} // namespace fc
