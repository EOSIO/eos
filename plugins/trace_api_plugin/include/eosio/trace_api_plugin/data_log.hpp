#pragma once
#include <fc/variant.hpp>
#include <eosio/trace_api_plugin/trace.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/protocol_feature_activation.hpp>

namespace eosio { namespace trace_api_plugin {

   using data_log_entry = fc::static_variant<
      block_trace_v0
   >;

   /**
    * append an entry to the end of the slice
    *
    * @param entry : the entry to append
    * @param slice : the slice to append entry to
    * @return the offset in the slice where that entry is written
    */
   template<typename DataEntry, typename Slice>
   uint64_t append_data_log(const DataEntry &entry, Slice &slice) {
      auto data = fc::raw::pack(entry);
      const auto offset = slice.tellp();
      slice.write(data.data(), data.size());
      slice.flush();
      slice.sync();
      return offset;
   }

   /**
    * extract an entry from the data log
    *
    * @param slice : the slice to extract entry from
    * @return the extracted data log
    */
   template<typename DataEntry, typename Slice>
   DataEntry extract_data_log( Slice& slice ) {
      DataEntry entry;
      auto ds = slice.create_datastream();
      fc::raw::unpack(ds, entry);
      return entry;
   }
}}
