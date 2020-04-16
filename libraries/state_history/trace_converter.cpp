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

void pack(fc::datastream<char*>& ds, const prunable_data_type&  data, compression_type compression) {
   if (compression == compression_type::none) {
      fc::raw::pack(ds, data);
   }
   else if (compression == compression_type::zlib) {
      bytes uncompressed = fc::raw::pack(data);
      fc::raw::pack(ds, zlib_compress_bytes(uncompressed));
   }
}

void unpack(fc::datastream<const char*>& ds, prunable_data_type& data, compression_type compression) {
   if (compression == compression_type::none) {
      fc::raw::unpack(ds, data);
   }
   else if (compression == compression_type::zlib) {
      bytes compressed;
      fc::raw::unpack(ds, compressed);
      bytes uncompressed = zlib_decompress(compressed);
      fc::datastream<const char*> uncompressed_ds(uncompressed.data(), uncompressed.size());
      fc::raw::unpack(uncompressed_ds, data);
   }
}


// each individual pruable data in a trasaction is packed as an optional prunable_data_type::full
// or an optional bytes to a compressed prunable_data_type::full
size_t pack_pruable(bytes& buffer, const packed_transaction& trx,
                  compression_type compression) {

   const auto& data = trx.get_prunable_data();

   int max_size = data.maximum_pruned_pack_size(compression);
   if (data.prunable_data.contains<prunable_data_type::full_legacy>()) {
      max_size += sizeof(uint8_t);
   }
   uint64_t start_buffer_position = buffer.size();
   buffer.resize(buffer.size() + max_size);
   fc::datastream<char*> ds(buffer.data() + start_buffer_position, max_size);
   pack(ds, data, compression);
   
   if (data.prunable_data.contains<prunable_data_type::full_legacy>()) {
      fc::raw::pack(ds, static_cast<uint8_t>(trx.get_compression()));
   }
   buffer.resize(start_buffer_position + ds.tellp());
   return max_size;
}


void unpack_prunable(const char* read_buffer, fc::datastream<const char*>& ds, prunable_data_type& data,
                     compression_type compression) {

   unpack(ds, data, compression);
   if (data.prunable_data.contains<prunable_data_type::full_legacy>()) {
      uint8_t local_comp;
      fc::raw::unpack(ds, local_comp);
      auto& full_legacy = data.prunable_data.get<prunable_data_type::full_legacy>();
      full_legacy.unpack_context_free_data(static_cast<chain::packed_transaction_v0::compression_type>(local_comp));
   }
}

bytes pack_prunables(const std::vector<augmented_transaction_trace>& traces, compression_type compression) {

   std::vector<char> out;
   out.reserve(1024);

   int size_with_paddings = 0;
   for_each_packed_transaction(
       traces, [&](const chain::packed_transaction& pt) { size_with_paddings += pack_pruable(out, pt, compression); });

   out.resize(size_with_paddings);
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
      //     prunable_data

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

struct restore_partial {
   partial_transaction_v0& ptrx;
   void operator()(prunable_data_type::full_legacy& data) const {
      ptrx.signatures        = std::move(data.signatures);
      ptrx.context_free_data = std::move(data.context_free_segments);
   }
   void operator()(prunable_data_type::none& data) const {}
   void operator()(prunable_data_type::signatures_only& data) const {
      ptrx.signatures        = std::move(data.signatures);
   }
   void operator()(prunable_data_type::partial& data) const  {
      ptrx.signatures        = std::move(data.signatures);
   }
   void operator()(prunable_data_type::full& data) const {
      ptrx.signatures        = std::move(data.signatures);
      ptrx.context_free_data = std::move(data.context_free_segments);
   }
};

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
                           prunable_data.prunable_data.visit(restore_partial{ptrx});
                        });
      }

      return fc::raw::pack(traces);
   }
}

bytes trace_converter::prune_traces(const bytes& entry_payload, uint32_t version, std::vector<transaction_id_type>& ids) {
   EOS_ASSERT(version > 0, state_history_exception, "state history log version 0 does not support trace pruning");
   fc::datastream<const char*> read_strm(entry_payload.data(), entry_payload.size());

   auto [traces, compression] = traces_from_v1_entry_payload(entry_payload.data(), read_strm);
   uint64_t last_read_pos     = read_strm.tellp();
   uint64_t change_offset     = entry_payload.size();
   bool     modified          = false;

   bytes write_buffer(entry_payload.size() - last_read_pos);
   fc::datastream<char*> write_strm(write_buffer.data(), write_buffer.size());

   auto prune_trace = [&, compression=compression](transaction_trace_v0& trace, auto& incoming_prunable) {
      auto itr = std::find(ids.begin(), ids.end(), trace.id);
      if (itr != ids.end()) {
         // the incoming trace matches the one of ids to be pruned
         state_history::pack(write_strm, incoming_prunable.prune_all(), compression);
         modified = true;

         // delete the found id from the ids vector
         if (itr != ids.end() - 1) {
            *itr = ids.back();
         }
         ids.resize(ids.size() - 1);
      } else {
         // the incoming trace should not be pruned
         auto bytes_read = read_strm.tellp() - last_read_pos; 
         write_strm.write(entry_payload.data() + last_read_pos, bytes_read);    
      }
      last_read_pos = read_strm.tellp();
   };

   for (auto& trace : traces) {
      do_visit_trace(entry_payload.data(), read_strm, trace, compression, prune_trace);
   }
   return modified? write_buffer : bytes{};
}

} // namespace state_history
} // namespace eosio
