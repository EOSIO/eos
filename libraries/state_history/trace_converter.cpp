#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <eosio/state_history/compression.hpp>
#include <eosio/state_history/serialization.hpp>
#include <eosio/state_history/trace_converter.hpp>

extern const char* state_history_plugin_abi;

namespace bio = boost::iostreams;
namespace eosio {
namespace state_history {

using eosio::chain::packed_transaction;
using eosio::chain::state_history_exception;
using prunable_data_type = packed_transaction::prunable_data_type;

bool is_onblock(const transaction_trace_ptr& p) {
   if (p->action_traces.size() != 1)
      return false;
   auto& act = p->action_traces[0].act;
   if (act.account != eosio::chain::config::system_account_name || act.name != N(onblock) ||
       act.authorization.size() != 1)
      return false;
   auto& auth = act.authorization[0];
   return auth.actor == eosio::chain::config::system_account_name &&
          auth.permission == eosio::chain::config::active_name;
}

void trace_converter::add_transaction(const transaction_trace_ptr& trace, const packed_transaction_ptr& transaction) {
   if (trace->receipt) {
      if (is_onblock(trace))
         onblock_trace.emplace(trace, transaction);
      else if (trace->failed_dtrx_trace)
         cached_traces[trace->failed_dtrx_trace->id] = augmented_transaction_trace{trace, transaction};
      else
         cached_traces[trace->id] = augmented_transaction_trace{trace, transaction};
   }
}

namespace {

std::vector<augmented_transaction_trace> prepare_traces(trace_converter&       converter,
                                                        const block_state_ptr& block_state) {
   std::vector<augmented_transaction_trace> traces;
   if (converter.onblock_trace)
      traces.push_back(*converter.onblock_trace);
   for (auto& r : block_state->block->transactions) {
      transaction_id_type id;
      if (r.trx.contains<transaction_id_type>())
         id = r.trx.get<transaction_id_type>();
      else
         id = r.trx.get<packed_transaction>().id();
      auto it = converter.cached_traces.find(id);
      EOS_ASSERT(it != converter.cached_traces.end() && it->second.trace->receipt, state_history_exception,
                 "missing trace for transaction ${id}", ("id", id));
      traces.push_back(it->second);
   }
   converter.cached_traces.clear();
   converter.onblock_trace.reset();
   return traces;
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

template <typename Lambda>
void for_each_packed_transaction(const std::vector<eosio::state_history::augmented_transaction_trace>& traces,
                                 const Lambda&                                                         lambda) {
   for (const auto& v : traces) {
      for_each_packed_transaction(v, lambda);
   }
}

template <typename T>
void pack_fc_raw(bio::filtering_ostreambuf& strm, const T& value) {
   char                  buf[sizeof(value)];
   fc::datastream<char*> ds(buf, sizeof(value));
   fc::raw::pack(ds, value);
   bio::write(strm, buf, ds.tellp());
}

void pack(bio::filtering_ostreambuf& strm, fc::unsigned_int v) { pack_fc_raw(strm, v); }
void pack(bio::filtering_ostreambuf& strm, chain::signature_type v) { pack_fc_raw(strm, v); }
void pack(bio::filtering_ostreambuf& strm, const std::vector<char>& value) {
   pack(strm, fc::unsigned_int(value.size()));
   bio::write(strm, value.data(), value.size());
}

template <typename T>
void pack(bio::filtering_ostreambuf& strm, const std::vector<T>& value) {
   pack(strm, fc::unsigned_int(value.size()));
   for (const auto& e : value) {
      pack(strm, e);
   }
}

void push_compressor(bio::filtering_ostreambuf& strm, compression_type compression) {
   EOS_ASSERT(compression == compression_type::zlib, state_history_exception, "unspported compression format");
   strm.push(bio::zlib_compressor(bio::zlib::default_compression));
}
// each individual pruable data in a trasaction is packed as an optional prunable_data_type::full
// or an optional bytes to a compressed prunable_data_type::full
void pack_pruable(bytes& buffer, const std::vector<signature_type>* sigs, const std::vector<bytes>* cfds,
                  compression_type compression) {
   static const std::vector<signature_type> empty_sigs{};
   static const std::vector<bytes>          empty_cfd{};

   sigs = sigs ? sigs : &empty_sigs;
   cfds = cfds ? cfds : &empty_cfd;

   if (sigs->size() == 0 && cfds->size() == 0) {
      buffer.push_back(0);
   } else {
      buffer.push_back(1);
      size_t                    len_pos;
      bio::filtering_ostreambuf strm;
      if (compression != compression_type::none) {
         push_compressor(strm, compression);
         // we need to skip 4 bytes for the length of the compressed stream
         len_pos = buffer.size();
         buffer.resize(buffer.size() + 4);
      }
      strm.push(bio::back_inserter(buffer));

      pack(strm, *sigs);
      pack(strm, *cfds);
      bio::close(strm);

      if (compression != compression_type::none) {
         // now we need to write the length of the compressed data
         uint32_t len = buffer.size() - len_pos - sizeof(len);
         memcpy(&buffer[len_pos], &len, sizeof(len));
      }
   }
}

bytes decompress_buffer(const char* buffer, uint32_t len, compression_type compression) {
   bytes                     decompressed;
   bio::filtering_ostreambuf strm;
   EOS_ASSERT(compression == compression_type::zlib, state_history_exception, "unspported compression format");
   strm.push(bio::zlib_decompressor());
   strm.push(bio::back_inserter(decompressed));
   bio::write(strm, buffer, len);
   bio::close(strm);
   return decompressed;
}

void unpack_prunable(const char* read_buffer, fc::datastream<const char*>& ds, prunable_data_type& data,
                     compression_type compression) {
   bool has_data;
   fc::raw::unpack(ds, has_data);
   data.prunable_data = prunable_data_type::full{};
   auto& full         = data.prunable_data.get<prunable_data_type::full>();
   if (has_data) {
      if (compression != compression_type::none) {
         uint32_t len;
         fc::raw::unpack(ds, len);
         EOS_ASSERT(ds.remaining() >= len, fc::out_of_range_exception, "read datastream over by ${v}",
                    ("v", ds.remaining() - len));

         bytes decompressed = decompress_buffer(read_buffer + ds.tellp(), len, compression);
         ds.skip(len);

         fc::datastream<const char*> decompress_ds(decompressed.data(), decompressed.size());
         fc::raw::unpack(decompress_ds, full);
      } else {
         fc::raw::unpack(ds, full);
      }
   }
}

bytes pack_prunables(const std::vector<augmented_transaction_trace>& traces, compression_type compression) {

   std::vector<char> out;
   for_each_packed_transaction(traces, [&out, compression](const chain::packed_transaction& pt) {
      pack_pruable(out, pt.get_signatures(), pt.get_context_free_data(), compression);
   });
   return out;
}

std::pair<std::vector<transaction_trace>, compression_type>
traces_from_v1_entry_payload(const char* read_buffer, fc::datastream<const char*>& ds) {
   uint8_t compr;
   fc::raw::unpack(ds, compr);
   fc::unsigned_int len;
   fc::raw::unpack(ds, len);
   EOS_ASSERT(ds.remaining() >= static_cast<size_t>(len), fc::out_of_range_exception, "read datastream over by ${v}",
              ("v", ds.remaining() - len));

   auto unprunable_section = decompress_buffer(read_buffer + ds.tellp(), len, compression_type::zlib);
   ds.skip(len);

   std::vector<transaction_trace> traces;
   fc::datastream<const char*>    traces_ds(unprunable_section.data(), unprunable_section.size());
   fc::raw::unpack(traces_ds, traces);
   return std::make_pair(traces, static_cast<compression_type>(compr));
}

template <typename Visitor>
void do_visit_trace(const char* read_buffer, fc::datastream<const char*>& strm, transaction_trace& trace,
                    compression_type compression, Visitor&& visitor) {
   auto& trace_v0 = trace.get<transaction_trace_v0>();
   if (trace_v0.failed_dtrx_trace.size()) {
      // failed_dtrx_trace have at most one element because it is encoded as an optional
      do_visit_trace(read_buffer, strm, trace_v0.failed_dtrx_trace[0].recurse, compression, visitor);
   }
   if (trace_v0.partial) {
      prunable_data_type prunable;
      unpack_prunable(read_buffer, strm, prunable, compression);
      visitor(trace_v0, prunable);
   }
}
} // namespace

bytes trace_converter::pack(const chainbase::database& db, bool trace_debug_mode, const block_state_ptr& block_state,
                            uint32_t version, compression_type compression) {

   auto traces     = prepare_traces(*this, block_state);
   auto unprunable = zlib_compress_bytes(fc::raw::pack(make_history_context_wrapper(
       db, trace_receipt_context{.debug_mode = trace_debug_mode, .version = version}, traces)));

   if (version == 0) {
      return unprunable;
   } else {
      // In version 1 of ShiP traces log disk format, it log entry consists of 3 parts.
      //  1. a unit8_t compression tag to indicate the compression level of prunables
      //  2. an zlib compressed unprunable section contains the serialization of the vector of traces excluding
      //     the prunable_data data (i.e. signatures and context free data)
      //  3. a prunable section contains the serialization of the vector of optional (possibly compressed)
      //     prunable_data::full

      auto                   prunables = pack_prunables(traces, compression);
      fc::datastream<size_t> unprunable_size_strm;
      fc::raw::pack(unprunable_size_strm, unprunable);
      uint8_t compression_tag = static_cast<uint8_t>(compression);
      bytes   buffer(sizeof(compression_tag) + unprunable_size_strm.tellp() + prunables.size());

      fc::datastream<char*> strm(buffer.data(), buffer.size());
      fc::raw::pack(strm, compression_tag);
      fc::raw::pack(strm, unprunable);
      strm.write(prunables.data(), prunables.size());
      return buffer;
   }
}

bytes trace_converter::to_traces_bin_v0(const bytes& entry_payload, uint32_t version) {
   if (version == 0)
      return zlib_decompress(entry_payload);
   else {
      fc::datastream<const char*> strm(entry_payload.data(), entry_payload.size());

      auto [traces, compression] = traces_from_v1_entry_payload(entry_payload.data(), strm);
      for (auto& trace : traces) {
         do_visit_trace(entry_payload.data(), strm, trace, compression,
                        [](transaction_trace_v0& trace, prunable_data_type& prunable_data) {
                           auto& ptrx             = trace.partial->get<partial_transaction_v0>();
                           auto& prunable_full    = prunable_data.prunable_data.get<prunable_data_type::full>();
                           ptrx.signatures        = std::move(prunable_full.signatures);
                           ptrx.context_free_data = std::move(prunable_full.context_free_segments);
                        });
      }

      return fc::raw::pack(traces);
   }
}

uint64_t trace_converter::prune_traces(bytes& entry_payload, uint32_t version, std::vector<transaction_id_type>& ids) {
   EOS_ASSERT(version > 0, state_history_exception, "state history log version 0 does not support trace pruning");
   fc::datastream<const char*> read_strm(entry_payload.data(), entry_payload.size());

   auto [traces, compression] = traces_from_v1_entry_payload(entry_payload.data(), read_strm);
   uint64_t last_read_pos     = read_strm.tellp();
   uint64_t change_offset     = entry_payload.size();
   bool     modified          = false;

   fc::datastream<char*> write_strm(entry_payload.data() + last_read_pos, entry_payload.size() - last_read_pos);

   auto prune_trace = [&](transaction_trace_v0& trace, auto& incoming_prunable) {
      auto itr = std::find(ids.begin(), ids.end(), trace.id);
      if (itr != ids.end()) {
         // the incoming trace matches the one of ids to be pruned

         if (!modified) {
            change_offset = last_read_pos;
            modified      = true;
         }

         // rewrite the serialized prunable to indicate it's empty
         fc::raw::pack(write_strm, false);

         // delete the found id from the ids vector
         if (itr != ids.end() - 1) {
            *itr = ids.back();
         }
         ids.resize(ids.size() - 1);
      } else {
         // the incoming trace should not be pruned
         auto bytes_read = read_strm.tellp() - last_read_pos;
         if (modified) {
            // if we have rewritten some prunables, copy the serialized prunable from
            // old position to the new position
            write_strm.write(entry_payload.data() + last_read_pos, bytes_read);
         } else {
            // if we haven't rewritten any prunable, just skip the bytes that has been read
            write_strm.skip(bytes_read);
         }
      }
      last_read_pos = read_strm.tellp();
   };

   for (auto& trace : traces) {
      do_visit_trace(entry_payload.data(), read_strm, trace, compression, prune_trace);
   }
   return change_offset;
}

} // namespace state_history
} // namespace eosio
