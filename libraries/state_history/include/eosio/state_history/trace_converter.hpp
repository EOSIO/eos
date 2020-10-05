#pragma once

#include <boost/convert.hpp>
#include <eosio/state_history/compression.hpp>
#include <eosio/state_history/serialization.hpp>
#include <eosio/state_history/types.hpp>
#include <fc/io/datastream.hpp>

namespace eosio {
namespace state_history {
namespace trace_converter {

using eosio::chain::overloaded;
using eosio::chain::packed_transaction;
using eosio::chain::state_history_exception;
using eosio::chain::transaction_id_type;
using prunable_data_type = packed_transaction::prunable_data_type;

struct ondisk_prunable_full_legacy {
   std::vector<signature_type> signatures;
   std::vector<bytes>          context_free_segments;
   eosio::chain::digest_type   digest;
   eosio::chain::digest_type   prunable_digest() const { return digest; }
};

using ondisk_prunable_data_t = std::variant<ondisk_prunable_full_legacy, prunable_data_type::none,
                                            prunable_data_type::partial, prunable_data_type::full>;


inline prunable_data_type to_prunable_data_type(ondisk_prunable_data_t&& data) {
   return std::visit(
       overloaded{[](ondisk_prunable_full_legacy&& v) {
                     prunable_data_type::full y{std::move(v.signatures), std::move(v.context_free_segments)};
                     return prunable_data_type{y};
                  },
                  [](auto&& v) { return prunable_data_type{std::move(v)}; }}, std::move(data));
}

template <typename Lambda>
void for_each_packed_transaction(const eosio::state_history::augmented_transaction_trace& obj, const Lambda& lambda) {
   auto& trace = *obj.trace;
   if (trace.failed_dtrx_trace) {
      for_each_packed_transaction(
          eosio::state_history::augmented_transaction_trace{trace.failed_dtrx_trace, obj.packed_trx}, lambda);
   }
   bool include_packed_trx = obj.packed_trx && !trace.failed_dtrx_trace;
   if (include_packed_trx) {
      lambda(*obj.packed_trx);
   }
}

/// used to traverse every packed_transaction inside traces before the pruned_data has been serialized
template <typename Lambda>
void for_each_packed_transaction(const std::vector<eosio::state_history::augmented_transaction_trace>& traces,
                                 const Lambda&                                                         lambda) {
   for (const auto& v : traces) {
      for_each_packed_transaction(v, lambda);
   }
}

template <typename OSTREAM, typename T>
void pack(OSTREAM&& strm, const T& obj, compression_type compression) {
   switch (compression) {
   case compression_type::zlib:
      zlib_pack(strm, obj);
      break;
   case compression_type::none:
      fc::raw::pack(strm, obj);
      break;
   default:
      EOS_ASSERT(false, state_history_exception, "unsupported compression type");
   }
}

template <typename ISTREAM, typename T>
void unpack(ISTREAM&& strm, T& obj, compression_type compression) {
   switch (compression) {
   case compression_type::zlib:
      zlib_unpack(strm, obj);
      break;
   case compression_type::none:
      fc::raw::unpack(strm, obj);
      break;
   default:
      EOS_ASSERT(false, state_history_exception, "unsupported compression type");
   }
}

template <typename OSTREAM>
size_t pack(OSTREAM&& strm, const prunable_data_type& obj, compression_type compression) {
   auto start_pos = strm.tellp();
   fc::raw::pack(strm, static_cast<uint8_t>(obj.prunable_data.index()));

   std::visit(overloaded{[&strm](const prunable_data_type::none& data) { fc::raw::pack(strm, data); },
                                      [&strm, compression](const prunable_data_type::full_legacy& data) {
                                         fc::raw::pack(strm, data.signatures);
                                         pack(strm, data.context_free_segments, compression);
                                         fc::raw::pack(strm, data.prunable_digest());
                                      },
                                      [&strm, compression](const auto& data) {
                                         fc::raw::pack(strm, data.signatures);
                                         pack(strm, data.context_free_segments, compression);
                                      }}, obj.prunable_data);
   auto consumed = strm.tellp() - start_pos;
   return std::max(sizeof(uint8_t) + sizeof(chain::digest_type), consumed);
}

template <typename ISTREAM>
void unpack(ISTREAM&& strm, ondisk_prunable_data_t& prunable, compression_type compression) {
   uint8_t tag;
   fc::raw::unpack(strm, tag);
   // set_variant_by_index(prunable, tag);
   fc::from_index(prunable, tag);
   std::visit(overloaded{[&strm](prunable_data_type::none& data) { fc::raw::unpack(strm, data); },
                         [&strm, compression](ondisk_prunable_full_legacy& data) {
                            fc::raw::unpack(strm, data.signatures);
                            unpack(strm, data.context_free_segments, compression);
                            fc::raw::unpack(strm, data.digest);
                         },
                         [&strm, compression](auto& data) {
                            fc::raw::unpack(strm, data.signatures);
                            unpack(strm, data.context_free_segments, compression);
                         }}, prunable);
}

/// used to traverse each trace along with its associated unpacked prunable_data
template <typename ISTREAM, typename Visitor>
void visit_deserialized_trace(ISTREAM& strm, transaction_trace& trace, compression_type compression,
                              Visitor&& visitor) {
   auto& trace_v0 = std::get<transaction_trace_v0>(trace);
   if (trace_v0.failed_dtrx_trace.size()) {
      // failed_dtrx_trace have at most one element because it is encoded as an optional
      visit_deserialized_trace(strm, trace_v0.failed_dtrx_trace[0].recurse, compression,
                               std::forward<Visitor>(visitor));
   }
   if (trace_v0.partial) {
      ondisk_prunable_data_t prunable;
      unpack(strm, prunable, compression);
      visitor(trace_v0, std::move(prunable));
   }
}

template <typename OSTREAM>
struct trace_pruner {
   bytes&   buffer; // the buffer contains the entire prunable section, which is the storage used by read_strm
   uint64_t last_read_pos = 0;
   std::vector<transaction_id_type>& ids; /// the transaction ids to be pruned
   fc::datastream<const char*>       read_strm;
   OSTREAM&                          write_strm;
   uint64_t change_position = 0; /// when it's nonzero, represents the offset to data where the content has been changed
   compression_type compression;

   trace_pruner(bytes& entry_payload, std::vector<transaction_id_type>& ids, OSTREAM& ostrm)
       : buffer(entry_payload)
       , ids(ids)
       , read_strm(entry_payload.data(), entry_payload.size())
       , write_strm(ostrm) {
      fc::raw::unpack(read_strm, compression);
   }

   void operator()(transaction_trace_v0& trace, ondisk_prunable_data_t&& incoming_prunable) {
      auto itr = std::find(ids.begin(), ids.end(), trace.id);
      if (itr != ids.end()) {
         // the incoming trace matches the one of ids to be pruned
         if (change_position == 0)
            change_position = write_strm.tellp();
         auto digest = std::visit([](const auto& v) { return v.prunable_digest(); }, incoming_prunable);
         uint8_t which = fc::get_index<ondisk_prunable_data_t, prunable_data_type::none>();
         fc::raw::pack(write_strm, std::make_pair(which, digest));
         ids.erase(itr);
      } else if (change_position == 0) {
         // no change so far, skip the original serialized prunable_data bytes
         write_strm.skip(read_strm.tellp() - last_read_pos);
      } else {
         // change detected,  shift the original serialized prunable_data bytes to new location
         write_strm.write(buffer.data() + last_read_pos, read_strm.tellp() - last_read_pos);
      }
      last_read_pos = read_strm.tellp();
   }
};

template <typename OSTREAM>
void pack(OSTREAM&& strm, const chainbase::database& db, bool trace_debug_mode,
          const std::vector<augmented_transaction_trace>& traces, compression_type compression) {

   // In version 1 of SHiP traces log disk format, it log entry consists of 3 parts.
   //  1. a zlib compressed unprunable section contains the serialization of the vector of traces excluding
   //     the prunable_data data (i.e. signatures and context free data)
   //  2. an uint8_t tag indicating the compression mechanism for the context free data inside the prunable section.
   //  3. a prunable section contains the serialization of the vector of ondisk_prunable_data_t.
   zlib_pack(strm, make_history_context_wrapper(db, trace_receipt_context{.debug_mode = trace_debug_mode}, traces));
   fc::raw::pack(strm, static_cast<uint8_t>(compression));
   const auto pos               = strm.tellp();
   size_t     size_with_padding = 0;
   for_each_packed_transaction(traces, [&strm, &size_with_padding, compression](const chain::packed_transaction& pt) {
      size_with_padding += trace_converter::pack(strm, pt.get_prunable_data(), compression);
   });
   strm.seekp(pos + size_with_padding);
}

template <typename ISTREAM>
void unpack(ISTREAM&& strm, std::vector<transaction_trace>& traces) {
   zlib_unpack(strm, traces);
   uint8_t compression;
   fc::raw::unpack(strm, compression);
   for (auto& trace : traces) {
      visit_deserialized_trace(strm, trace, static_cast<compression_type>(compression),
                               [](transaction_trace_v0& trace, ondisk_prunable_data_t&& prunable_data) {
                                  auto& ptrx         = std::get<partial_transaction_v1>(*trace.partial);
                                  ptrx.prunable_data = to_prunable_data_type(std::move(prunable_data));
                               });
   }
}

template <typename IOSTREAM>
void prune_traces(IOSTREAM&& strm, uint32_t entry_len, std::vector<transaction_id_type>& ids) {
   std::vector<transaction_trace> traces;
   size_t                         unprunable_section_pos = strm.tellp();
   zlib_unpack(strm, traces);
   size_t            prunable_section_pos = strm.tellp();
   std::vector<char> buffer(unprunable_section_pos + entry_len - prunable_section_pos);
   strm.read(buffer.data(), buffer.size());
   // restore the stream position to the start of prunable section so it can used for writing
   strm.seekp(prunable_section_pos);

   auto pruner = trace_pruner<IOSTREAM>{buffer, ids, strm};
   for (auto& trace : traces) {
      visit_deserialized_trace(pruner.read_strm, trace, pruner.compression, pruner);
   }
}

} // namespace trace_converter
} // namespace state_history
} // namespace eosio

FC_REFLECT(eosio::state_history::trace_converter::ondisk_prunable_full_legacy,
           (signatures)(context_free_segments)(digest))
