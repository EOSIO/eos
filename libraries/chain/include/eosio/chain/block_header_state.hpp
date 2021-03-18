#pragma once
#include <eosio/chain/block_header.hpp>
#include <eosio/chain/incremental_merkle.hpp>
#include <eosio/chain/protocol_feature_manager.hpp>
#include <eosio/chain/chain_snapshot.hpp>
#include <eosio/chain/versioned_unpack_stream.hpp>
#include <future>

namespace eosio { namespace chain {

namespace legacy {

   /**
    * a fc::raw::unpack compatible version of the old block_state structure stored in
    * version 2 snapshots
    */
   struct snapshot_block_header_state_v2 {
      static constexpr uint32_t minimum_version = 0;
      static constexpr uint32_t maximum_version = 2;
      static_assert(chain_snapshot_header::minimum_compatible_version <= maximum_version, "snapshot_block_header_state_v2 is no longer needed");

      struct schedule_info {
         uint32_t                          schedule_lib_num = 0; /// last irr block num
         digest_type                       schedule_hash;
         producer_schedule_type            schedule;
      };

      /// from block_header_state_common
      uint32_t                             block_num = 0;
      uint32_t                             dpos_proposed_irreversible_blocknum = 0;
      uint32_t                             dpos_irreversible_blocknum = 0;
      producer_schedule_type               active_schedule;
      incremental_merkle                   blockroot_merkle;
      flat_map<account_name,uint32_t>      producer_to_last_produced;
      flat_map<account_name,uint32_t>      producer_to_last_implied_irb;
      public_key_type                      block_signing_key;
      vector<uint8_t>                      confirm_count;

      // from block_header_state
      block_id_type                        id;
      signed_block_header                  header;
      schedule_info                        pending_schedule;
      protocol_feature_activation_set_ptr  activated_protocol_features;
   };
}

using signer_callback_type = std::function<std::vector<signature_type>(const digest_type&)>;

struct block_header_state;

struct security_group_info_t {
   uint32_t                                 version = 0;
   boost::container::flat_set<account_name> participants;
};

namespace detail {
   struct block_header_state_common {
      uint32_t                          block_num = 0;
      uint32_t                          dpos_proposed_irreversible_blocknum = 0;
      uint32_t                          dpos_irreversible_blocknum = 0;
      producer_authority_schedule       active_schedule;
      incremental_merkle                blockroot_merkle;
      flat_map<account_name,uint32_t>   producer_to_last_produced;
      flat_map<account_name,uint32_t>   producer_to_last_implied_irb;
      block_signing_authority           valid_block_signing_authority;
      vector<uint8_t>                   confirm_count;
   };

   struct schedule_info {
      uint32_t                          schedule_lib_num = 0; /// last irr block num
      digest_type                       schedule_hash;
      producer_authority_schedule       schedule;
   };

   bool is_builtin_activated( const protocol_feature_activation_set_ptr& pfa,
                              const protocol_feature_set& pfs,
                              builtin_protocol_feature_t feature_codename );
}

struct pending_block_header_state : public detail::block_header_state_common {
   protocol_feature_activation_set_ptr  prev_activated_protocol_features;
   detail::schedule_info                prev_pending_schedule;
   bool                                 was_pending_promoted = false;
   block_id_type                        previous;
   account_name                         producer;
   block_timestamp_type                 timestamp;
   uint32_t                             active_schedule_version = 0;
   uint16_t                             confirmed = 1;
   security_group_info_t                security_group;

   signed_block_header make_block_header( const checksum256_type& transaction_mroot,
                                          const checksum256_type& action_mroot,
                                          const std::optional<producer_authority_schedule>& new_producers,
                                          vector<digest_type>&& new_protocol_feature_activations,
                                          const protocol_feature_set& pfs)const;

   block_header_state  finish_next( const signed_block_header& h,
                                    vector<signature_type>&& additional_signatures,
                                    const protocol_feature_set& pfs,
                                    const std::function<void( block_timestamp_type,
                                                              const flat_set<digest_type>&,
                                                              const vector<digest_type>& )>& validator,
                                    bool skip_validate_signee = false )&&;

   block_header_state  finish_next( signed_block_header& h,
                                    const protocol_feature_set& pfs,
                                    const std::function<void( block_timestamp_type,
                                                              const flat_set<digest_type>&,
                                                              const vector<digest_type>& )>& validator,
                                    const signer_callback_type& signer )&&;

protected:
   block_header_state  _finish_next( const signed_block_header& h,
                                     const protocol_feature_set& pfs,
                                     const std::function<void( block_timestamp_type,
                                                               const flat_set<digest_type>&,
                                                               const vector<digest_type>& )>& validator )&&;
};



/**
 *  @struct block_header_state
 *  @brief defines the minimum state necessary to validate transaction headers
 */
struct block_header_state : public detail::block_header_state_common {

   /// this version is coming from chain_snapshot_header.version
   static constexpr uint32_t minimum_snapshot_version_with_state_extension = 6; 

   block_id_type                        id;
   signed_block_header                  header;
   detail::schedule_info                pending_schedule;
   protocol_feature_activation_set_ptr  activated_protocol_features;
   vector<signature_type>               additional_signatures;

   /// this data is redundant with the data stored in header, but it acts as a cache that avoids
   /// duplication of work
   flat_multimap<uint16_t, block_header_extension> header_exts;

   struct state_extension_v0 {
      security_group_info_t security_group_info;
   };

   // For future extension, one should use
   //
   // struct state_extension_v1 : state_extension_v0 { new_field_t new_field };
   // using state_extension_t = std::variant<state_extension_v0, state_extension_v1> state_extension;

   using state_extension_t = std::variant<state_extension_v0>;
   state_extension_t state_extension;

   block_header_state() = default;

   explicit block_header_state(detail::block_header_state_common&& base)
       : detail::block_header_state_common(std::move(base)) {}

   explicit block_header_state( legacy::snapshot_block_header_state_v2&& snapshot );

   pending_block_header_state  next( block_timestamp_type when, uint16_t num_prev_blocks_to_confirm )const;

   block_header_state   next( const signed_block_header& h,
                              vector<signature_type>&& additional_signatures,
                              const protocol_feature_set& pfs,
                              const std::function<void( block_timestamp_type,
                                                        const flat_set<digest_type>&,
                                                        const vector<digest_type>& )>& validator,
                              bool skip_validate_signee = false )const;

   bool                 has_pending_producers()const { return pending_schedule.schedule.producers.size(); }
   uint32_t             calc_dpos_last_irreversible( account_name producer_of_next_block )const;

   producer_authority     get_scheduled_producer( block_timestamp_type t )const;
   const block_id_type&   prev()const { return header.previous; }
   digest_type            sig_digest()const;
   void                   sign( const signer_callback_type& signer );
   void                   verify_signee()const;

   const vector<digest_type>& get_new_protocol_feature_activations() const;

   void set_security_group_info(security_group_info_t&& new_info) {
      std::visit([&new_info](auto& v) {  v.security_group_info = std::move(new_info); }, state_extension);
   }

   const security_group_info_t& get_security_group_info() const {
      return std::visit([](const auto& v) -> const security_group_info_t& { return v.security_group_info; },
                        state_extension);
   }
};

using block_header_state_ptr = std::shared_ptr<block_header_state>;

} } /// namespace eosio::chain

FC_REFLECT( eosio::chain::detail::block_header_state_common,
            (block_num)
            (dpos_proposed_irreversible_blocknum)
            (dpos_irreversible_blocknum)
            (active_schedule)
            (blockroot_merkle)
            (producer_to_last_produced)
            (producer_to_last_implied_irb)
            (valid_block_signing_authority)
            (confirm_count)
)

FC_REFLECT( eosio::chain::detail::schedule_info,
            (schedule_lib_num)
            (schedule_hash)
            (schedule)
)

FC_REFLECT(eosio::chain::security_group_info_t, (version)(participants))

FC_REFLECT( eosio::chain::block_header_state::state_extension_v0,
            (security_group_info)
)

// @ignore header_exts
FC_REFLECT_DERIVED(  eosio::chain::block_header_state, (eosio::chain::detail::block_header_state_common),
                     (id)
                     (header)
                     (pending_schedule)
                     (activated_protocol_features)
                     (additional_signatures)
                     (state_extension)
)

FC_REFLECT( eosio::chain::legacy::snapshot_block_header_state_v2::schedule_info,
          ( schedule_lib_num )
          ( schedule_hash )
          ( schedule )
)


FC_REFLECT( eosio::chain::legacy::snapshot_block_header_state_v2,
          ( block_num )
          ( dpos_proposed_irreversible_blocknum )
          ( dpos_irreversible_blocknum )
          ( active_schedule )
          ( blockroot_merkle )
          ( producer_to_last_produced )
          ( producer_to_last_implied_irb )
          ( block_signing_key )
          ( confirm_count )
          ( id )
          ( header )
          ( pending_schedule )
          ( activated_protocol_features )
)

namespace fc {
namespace raw {
namespace detail {

// C++20 Concept 
//
// template <typename T> 
// concept VersionedStream = requires (T t) {
//    t.version;
// }
//

template <typename VersionedStream, typename Class>
struct unpack_block_header_state_derived_visitor : fc::reflector_init_visitor<Class> {

   unpack_block_header_state_derived_visitor(Class& _c, VersionedStream& _s)
       : fc::reflector_init_visitor<Class>(_c)
       , s(_s) {}

   template <typename T, typename C, T(C::*p)>
   inline void operator()(const char* name) const {
      try {
         if constexpr (std::is_same_v<eosio::chain::block_header_state::state_extension_t,
                                      std::decay_t<decltype(this->obj.*p)>>)
            if (s.version < eosio::chain::block_header_state::minimum_snapshot_version_with_state_extension)
               return;

         fc::raw::unpack(s, this->obj.*p);
      }
      FC_RETHROW_EXCEPTIONS(warn, "Error unpacking field ${field}", ("field", name))
   }

 private:
   VersionedStream& s;
};

template <typename VersionedStream>
struct unpack_object_visitor<VersionedStream, eosio::chain::block_header_state>
    : unpack_block_header_state_derived_visitor<VersionedStream, eosio::chain::block_header_state> {
   using Base = unpack_block_header_state_derived_visitor<VersionedStream, eosio::chain::block_header_state>;
   using Base::Base;
};

} // namespace detail
} // namespace raw
} // namespace fc
