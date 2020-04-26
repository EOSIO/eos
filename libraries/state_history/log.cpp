#include <eosio/state_history/compression.hpp>
#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/state_history/trace_converter.hpp>
#include <eosio/state_history/serialization.hpp>

namespace eosio {

state_history_log::state_history_log(const char* const name, std::string log_filename, std::string index_filename)
    : name(name)
    , log_filename(std::move(log_filename))
    , index_filename(std::move(index_filename)) {
   open_log();
   open_index();
}

void state_history_log::read_header(state_history_log_header& header, bool assert_version) {
   char bytes[state_history_log_header_serial_size];
   log.read(bytes, sizeof(bytes));
   fc::datastream<const char*> ds(bytes, sizeof(bytes));
   fc::raw::unpack(ds, header);
   EOS_ASSERT(!ds.remaining(), chain::state_history_exception, "state_history_log_header_serial_size mismatch");
   version = get_ship_version(header.magic);
   if (assert_version)
      EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic), chain::state_history_exception,
                 "corrupt ${name}.log (0)", ("name", name));
}

void state_history_log::write_header(const state_history_log_header& header) {
   char                  bytes[state_history_log_header_serial_size];
   fc::datastream<char*> ds(bytes, sizeof(bytes));
   fc::raw::pack(ds, header);
   EOS_ASSERT(!ds.remaining(), chain::state_history_exception, "state_history_log_header_serial_size mismatch");
   log.write(bytes, sizeof(bytes));
}

// returns cfile positioned at payload
fc::cfile& state_history_log::get_entry_header(state_history_log::block_num_type block_num,
                                               state_history_log_header&         header) {
   EOS_ASSERT(block_num >= _begin_block && block_num < _end_block, chain::state_history_exception,
              "read non-existing block in ${name}.log", ("name", name));
   log.seek(get_pos(block_num));
   read_header(header);
   return log;
}

chain::block_id_type state_history_log::get_block_id(state_history_log::block_num_type block_num) {
   state_history_log_header header;
   get_entry_header(block_num, header);
   return header.block_id;
}

bool state_history_log::get_last_block(uint64_t size) {
   state_history_log_header header;
   uint64_t                 suffix;
   log.seek(size - sizeof(suffix));
   log.read((char*)&suffix, sizeof(suffix));
   if (suffix > size || suffix + state_history_log_header_serial_size > size) {
      elog("corrupt ${name}.log (2)", ("name", name));
      return false;
   }
   log.seek(suffix);
   read_header(header, false);
   if (!is_ship(header.magic) || !is_ship_supported_version(header.magic) ||
       suffix + state_history_log_header_serial_size + header.payload_size + sizeof(suffix) != size) {
      elog("corrupt ${name}.log (3)", ("name", name));
      return false;
   }
   _end_block    = chain::block_header::num_from_id(header.block_id) + 1;
   last_block_id = header.block_id;
   if (_begin_block >= _end_block) {
      elog("corrupt ${name}.log (4)", ("name", name));
      return false;
   }
   return true;
}

void state_history_log::recover_blocks(uint64_t size) {
   ilog("recover ${name}.log", ("name", name));
   uint64_t pos       = 0;
   uint32_t num_found = 0;
   while (true) {
      state_history_log_header header;
      if (pos + state_history_log_header_serial_size > size)
         break;
      log.seek(pos);
      read_header(header, false);
      uint64_t suffix;
      if (!is_ship(header.magic) || !is_ship_supported_version(header.magic) || header.payload_size > size ||
          pos + state_history_log_header_serial_size + header.payload_size + sizeof(suffix) > size) {
         EOS_ASSERT(!is_ship(header.magic) || is_ship_supported_version(header.magic), chain::state_history_exception,
                    "${name}.log has an unsupported version", ("name", name));
         break;
      }
      log.seek(pos + state_history_log_header_serial_size + header.payload_size);
      log.read((char*)&suffix, sizeof(suffix));
      if (suffix != pos)
         break;
      pos = pos + state_history_log_header_serial_size + header.payload_size + sizeof(suffix);
      if (!(++num_found % 10000)) {
         printf("%10u blocks found, log pos=%12llu\r", (unsigned)num_found, (unsigned long long)pos);
         fflush(stdout);
      }
   }
   log.flush();
   boost::filesystem::resize_file(log_filename, pos);
   log.flush();
   EOS_ASSERT(get_last_block(pos), chain::state_history_exception, "recover ${name}.log failed", ("name", name));
}

void state_history_log::open_log() {
   log.set_file_path(log_filename);
   log.open("a+b"); // std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app
   log.seek_end(0);
   uint64_t size = log.tellp();
   if (size >= state_history_log_header_serial_size) {
      state_history_log_header header;
      log.seek(0);
      read_header(header, false);
      EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic) &&
                     state_history_log_header_serial_size + header.payload_size + sizeof(uint64_t) <= size,
                 chain::state_history_exception, "corrupt ${name}.log (1)", ("name", name));
      _begin_block  = chain::block_header::num_from_id(header.block_id);
      last_block_id = header.block_id;
      if (!get_last_block(size))
         recover_blocks(size);
      ilog("${name}.log has blocks ${b}-${e}", ("name", name)("b", _begin_block)("e", _end_block - 1));
   } else {
      EOS_ASSERT(!size, chain::state_history_exception, "corrupt ${name}.log (5)", ("name", name));
      ilog("${name}.log is empty", ("name", name));
   }
}

void state_history_log::open_index() {
   index.set_file_path(index_filename);
   index.open("a+b"); // std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app
   index.seek_end(0);
   if (index.tellp() == (static_cast<int>(_end_block) - _begin_block) * sizeof(uint64_t))
      return;
   ilog("Regenerate ${name}.index", ("name", name));
   index.close();
   index.open("w+b"); // std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc

   log.seek_end(0);
   uint64_t size      = log.tellp();
   uint64_t pos       = 0;
   uint32_t num_found = 0;
   while (pos < size) {
      state_history_log_header header;
      EOS_ASSERT(pos + state_history_log_header_serial_size <= size, chain::state_history_exception,
                 "corrupt ${name}.log (6)", ("name", name));
      log.seek(pos);
      read_header(header, false);
      uint64_t suffix_pos = pos + state_history_log_header_serial_size + header.payload_size;
      uint64_t suffix;
      EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic) &&
                     suffix_pos + sizeof(suffix) <= size,
                 chain::state_history_exception, "corrupt ${name}.log (7)", ("name", name));
      log.seek(suffix_pos);
      log.read((char*)&suffix, sizeof(suffix));
      // ilog("block ${b} at ${pos}-${end} suffix=${suffix} file_size=${fs}",
      //      ("b", header.block_num)("pos", pos)("end", suffix_pos + sizeof(suffix))("suffix", suffix)("fs", size));
      EOS_ASSERT(suffix == pos, chain::state_history_exception, "corrupt ${name}.log (8)", ("name", name));

      index.write((char*)&pos, sizeof(pos));
      pos = suffix_pos + sizeof(suffix);
      if (!(++num_found % 10000)) {
         printf("%10u blocks found, log pos=%12llu\r", (unsigned)num_found, (unsigned long long)pos);
         fflush(stdout);
      }
   }
}

state_history_log::file_position_type state_history_log::get_pos(state_history_log::block_num_type block_num) {
   uint64_t pos;
   index.seek((block_num - _begin_block) * sizeof(pos));
   index.read((char*)&pos, sizeof(pos));
   return pos;
}

void state_history_log::truncate(state_history_log::block_num_type block_num) {
   log.flush();
   index.flush();
   uint64_t num_removed = 0;
   if (block_num <= _begin_block) {
      num_removed = _end_block - _begin_block;
      log.seek(0);
      index.seek(0);
      boost::filesystem::resize_file(log_filename, 0);
      boost::filesystem::resize_file(index_filename, 0);
      _begin_block = _end_block = 0;
   } else {
      num_removed  = _end_block - block_num;
      uint64_t pos = get_pos(block_num);
      log.seek(0);
      index.seek(0);
      boost::filesystem::resize_file(log_filename, pos);
      boost::filesystem::resize_file(index_filename, (block_num - _begin_block) * sizeof(uint64_t));
      _end_block = block_num;
   }
   log.flush();
   index.flush();
   ilog("fork or replay: removed ${n} blocks from ${name}.log", ("n", num_removed)("name", name));
}

std::pair<state_history_log::file_position_type, state_history_log::block_num_type>
state_history_log::write_entry_header(const state_history_log_header& header, const chain::block_id_type& prev_id) {
   auto block_num = chain::block_header::num_from_id(header.block_id);
   EOS_ASSERT(_begin_block == _end_block || block_num <= _end_block, chain::state_history_exception,
              "missed a block in ${name}.log", ("name", name));

   if (_begin_block != _end_block && block_num > _begin_block) {
      if (block_num == _end_block) {
         EOS_ASSERT(prev_id == last_block_id, chain::state_history_exception, "missed a fork change in ${name}.log",
                    ("name", name));
      } else {
         state_history_log_header prev;
         get_entry_header(block_num - 1, prev);
         EOS_ASSERT(prev_id == prev.block_id, chain::state_history_exception, "missed a fork change in ${name}.log",
                    ("name", name));
      }
   }

   if (block_num < _end_block)
      truncate(block_num);
   log.seek_end(0);
   uint64_t pos = log.tellp();
   write_header(header);
   return std::make_pair(pos, block_num);
}

void state_history_log::write_entry_position(const state_history_log_header&       header,
                                             state_history_log::file_position_type pos,
                                             state_history_log::block_num_type     block_num) {
   uint64_t end = log.tellp();
   EOS_ASSERT(end == pos + state_history_log_header_serial_size + header.payload_size, chain::state_history_exception,
              "wrote payload with incorrect size to ${name}.log", ("name", name));
   log.write((char*)&pos, sizeof(pos));

   index.seek_end(0);
   index.write((char*)&pos, sizeof(pos));
   if (_begin_block == _end_block)
      _begin_block = block_num;
   _end_block    = block_num + 1;
   last_block_id = header.block_id;
}

fc::optional<std::pair<chain::bytes, state_history_log::version_type>>
state_history_log::get_entry(state_history_log::block_num_type block_num) {
   if (block_num < begin_block() || block_num >= end_block())
      return {};
   state_history_log_header header;
   auto&                    stream = get_entry_header(block_num, header);
   uint32_t                 s;
   stream.read((char*)&s, sizeof(s));
   chain::bytes data(s);
   if (s)
      stream.read(data.data(), s);
   return std::make_pair(std::move(data), get_ship_version(header.magic));
}

state_history_traces_log::state_history_traces_log(fc::path state_history_dir)
    : state_history_log("trace_history", (state_history_dir / "trace_history.log").string(),
                        (state_history_dir / "trace_history.index").string()) {}

fc::optional<chain::bytes> state_history_traces_log::get_log_entry(block_num_type block_num) {
   return this->get_entry(block_num, [](const chain::bytes& data, uint32_t version) {
      return state_history::trace_converter::to_traces_bin_v0(data, version);
   });
}

void state_history_traces_log::prune_transactions(state_history_log::block_num_type        block_num,
                                                  std::vector<chain::transaction_id_type>& ids) {
   auto entry_result = get_entry(block_num);
   EOS_ASSERT(entry_result, chain::state_history_exception, "nonexistant block num ${block_num}",
              ("block_num", block_num));

   auto pos = get_log().tellp();

   auto& [entry_payload, version]  = *entry_result;
   auto [start_offset, end_offset] = state_history::trace_converter::prune_traces(entry_payload, version, ids);

   if (end_offset > 0) {
      // the log object is in append mode, which cannot be used to update written content,
      // we need to open the file in the update mode

      fc::cfile update_log;
      update_log.set_file_path(get_log().get_file_path());
      update_log.open(fc::cfile::update_rw_mode);
      update_log.seek(pos - entry_payload.size() + start_offset);
      update_log.write(entry_payload.data() + start_offset, end_offset-start_offset);
   }
}

void state_history_traces_log::store(const chainbase::database& db, const chain::block_state_ptr& block_state) {

   auto traces_bin = trace_convert.pack(db, trace_debug_mode, block_state, ship_current_version);

   state_history_log_header header{.magic        = ship_magic(ship_current_version),
                                   .block_id     = block_state->id,
                                   .payload_size = sizeof(uint32_t) + traces_bin.size()};
   this->write_entry(header, block_state->block->previous, [&](auto& stream) {
      uint32_t s = (uint32_t)traces_bin.size();
      stream.write((char*)&s, sizeof(s));
      if (!traces_bin.empty())
         stream.write(traces_bin.data(), traces_bin.size());
   });
}

bool state_history_traces_log::exists(fc::path state_history_dir) {
   return fc::exists(state_history_dir / "trace_history.log") && fc::exists(state_history_dir / "trace_history.index");
}

state_history_chain_state_log::state_history_chain_state_log(fc::path state_history_dir)
    : state_history_log("chain_state_history", (state_history_dir / "chain_state_history.log").string(),
                        (state_history_dir / "chain_state_history.index").string()) {}

fc::optional<chain::bytes> state_history_chain_state_log::get_log_entry(block_num_type block_num) {
   return this->get_entry(block_num,
                          [](const chain::bytes& data, uint32_t) { return state_history::zlib_decompress(data); });
}

void state_history_chain_state_log::store(const chainbase::database& db, const chain::block_state_ptr& block_state) {
   bool fresh = this->begin_block() == this->end_block();
   if (fresh)
      ilog("Placing initial state in block ${n}", ("n", block_state->block->block_num()));

   using namespace state_history;

   std::vector<table_delta> deltas     = create_deltas(db, fresh);
   auto                     deltas_bin = zlib_compress_bytes(fc::raw::pack(deltas));
   EOS_ASSERT(deltas_bin.size() == (uint32_t)deltas_bin.size(), chain::state_history_exception, "deltas is too big");

   state_history_log_header header{.magic        = ship_magic(ship_current_version),
                                   .block_id     = block_state->id,
                                   .payload_size = sizeof(uint32_t) + deltas_bin.size()};

   this->write_entry(header, block_state->block->previous, [&](auto& stream) {
      uint32_t s = (uint32_t)deltas_bin.size();
      stream.write((char*)&s, sizeof(s));
      if (!deltas_bin.empty())
         stream.write(deltas_bin.data(), deltas_bin.size());
   });
}

} // namespace eosio
