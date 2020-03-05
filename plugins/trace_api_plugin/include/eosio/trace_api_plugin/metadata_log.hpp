#pragma once
#include <fc/variant.hpp>
#include <eosio/trace_api_plugin/trace.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/protocol_feature_activation.hpp>

namespace eosio { namespace trace_api_plugin {
   struct block_entry_v0 {
      chain::block_id_type   id;
      uint64_t               number;
      uint64_t               offset;
   };

   struct lib_entry_v0 {
      uint64_t               lib;
   };

   using metadata_log_entry = fc::static_variant<
      block_entry_v0,
      lib_entry_v0
   >;

   /**
    * extract an entry from the metadata log
    *
    * @param slice : the slice to extract entry from
    * @return the extracted metadata log
    */
   template<typename Slice>
   metadata_log_entry extract_metadata_log( uint64_t offset, Slice& slice ) {
      metadata_log_entry entry;
      auto ds = slice.create_datastream();
      fc::raw::unpack(ds, entry);
      return entry;
   }
}}

FC_REFLECT(eosio::trace_api_plugin::block_entry_v0, (id)(number)(offset));
FC_REFLECT(eosio::trace_api_plugin::lib_entry_v0, (lib));
