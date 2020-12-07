#include <eosio/state_history/compression.hpp>
#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/state_history/serialization.hpp>
#include <eosio/state_history/trace_converter.hpp>

namespace eosio {

uint64_t state_history_log_data::payload_size_at(uint64_t pos) const {
   EOS_ASSERT(file.size() >= pos + sizeof(state_history_log_header), chain::state_history_exception,
              "corrupt ${name}: invalid entry size at at position ${pos}", ("name", filename)("pos", pos));

   fc::datastream<const char*> ds(file.const_data() + pos, sizeof(state_history_log_header));
   state_history_log_header    header;
   fc::raw::unpack(ds, header);

   EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic), chain::state_history_exception,
              "corrupt ${name}: invalid header for entry at position ${pos}", ("name", filename)("pos", pos));

   EOS_ASSERT(file.size() >= pos + sizeof(state_history_log_header) + header.payload_size,
              chain::state_history_exception, "corrupt ${name}: invalid payload size for entry at position ${pos}",
              ("name", filename)("pos", pos));
   return header.payload_size;
}

void state_history_log_data::construct_index(const fc::path& index_file_name) const {
   fc::cfile index;
   index.set_file_path(index_file_name);
   index.open("w+b");

   uint64_t pos = 0;
   while (pos < file.size()) {
      uint64_t payload_size = payload_size_at(pos);
      index.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
      pos += (sizeof(state_history_log_header) + payload_size + sizeof(uint64_t));
   }
}

state_history_log::state_history_log(const char* const name, const state_history_config& config)
    : name(name) {
   catalog.open(config.log_dir, config.retained_dir, config.archive_dir, name);
   catalog.max_retained_files = config.max_retained_files;
   this->stride               = config.stride;
   open_log(config.log_dir / (std::string(name) + ".log"));
   open_index(config.log_dir / (std::string(name) + ".index"));
}

void state_history_log::read_header(state_history_log_header& header, bool assert_version) {
   char bytes[state_history_log_header_serial_size];
   read_log.read(bytes, sizeof(bytes));
   fc::datastream<const char*> ds(bytes, sizeof(bytes));
   fc::raw::unpack(ds, header);
   EOS_ASSERT(!ds.remaining(), chain::state_history_exception, "state_history_log_header_serial_size mismatch");
   version = get_ship_version(header.magic);
   if (assert_version)
      EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic), chain::state_history_exception,
                 "corrupt ${name}.log (0)", ("name", name));
}

void state_history_log::write_header(const state_history_log_header& header) { fc::raw::pack(write_log, header); }

// returns cfile positioned at payload
void state_history_log::get_entry_header(state_history_log::block_num_type block_num,
                                         state_history_log_header&         header) {
   EOS_ASSERT(block_num >= _begin_block && block_num < _end_block, chain::state_history_exception,
              "read non-existing block in ${name}.log", ("name", name));
   read_log.seek(get_pos(block_num));
   read_header(header);
}

std::optional<chain::block_id_type> state_history_log::get_block_id(state_history_log::block_num_type block_num) {
   auto result = catalog.id_for_block(block_num);
   if (!result && block_num >= _begin_block && block_num < _end_block){
      state_history_log_header header;
      get_entry_header(block_num, header);
      return header.block_id;
   }
   return result;
}

bool state_history_log::get_last_block(uint64_t size) {
   state_history_log_header header;
   uint64_t                 suffix;
   read_log.seek(size - sizeof(suffix));
   read_log.read((char*)&suffix, sizeof(suffix));
   if (suffix > size || suffix + state_history_log_header_serial_size > size) {
      elog("corrupt ${name}.log (2)", ("name", name));
      return false;
   }
   read_log.seek(suffix);
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
      read_log.seek(pos);
      read_header(header, false);
      uint64_t suffix;
      if (!is_ship(header.magic) || !is_ship_supported_version(header.magic) || header.payload_size > size ||
          pos + state_history_log_header_serial_size + header.payload_size + sizeof(suffix) > size) {
         EOS_ASSERT(!is_ship(header.magic) || is_ship_supported_version(header.magic), chain::state_history_exception,
                    "${name}.log has an unsupported version", ("name", name));
         break;
      }
      read_log.seek(pos + state_history_log_header_serial_size + header.payload_size);
      read_log.read((char*)&suffix, sizeof(suffix));
      if (suffix != pos)
         break;
      pos = pos + state_history_log_header_serial_size + header.payload_size + sizeof(suffix);
      if (!(++num_found % 10000)) {
         printf("%10u blocks found, log pos=%12llu\r", (unsigned)num_found, (unsigned long long)pos);
         fflush(stdout);
      }
   }
   read_log.flush();
   boost::filesystem::resize_file(read_log.get_file_path(), pos);
   read_log.flush();
   EOS_ASSERT(get_last_block(pos), chain::state_history_exception, "recover ${name}.log failed", ("name", name));
}

void state_history_log::open_log(bfs::path log_filename) {
   write_log.set_file_path(log_filename);
   read_log.set_file_path(log_filename);
   write_log.open("a+b"); // create file if not exists
   write_log.close();
   write_log.open("rb+"); // fseek doesn't work for append mode, need to open with update mode.
   write_log.seek_end(0);
   read_log.open("rb");
   uint64_t size = write_log.tellp();
   if (size >= state_history_log_header_serial_size) {
      state_history_log_header header;
      read_log.seek(0);
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

void state_history_log::open_index(bfs::path index_filename) {
   index.set_file_path(index_filename);
   index.open("a+b"); // std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app
   index.seek_end(0);
   if (index.tellp() == (static_cast<int>(_end_block) - _begin_block) * sizeof(uint64_t))
      return;
   ilog("Regenerate ${name}.index", ("name", name));
   index.close();

   state_history_log_data(read_log.get_file_path()).construct_index(index_filename);
   index.open("a+b"); // std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app
   index.seek_end(0);
}

state_history_log::file_position_type state_history_log::get_pos(state_history_log::block_num_type block_num) {
   uint64_t pos;
   index.seek((block_num - _begin_block) * sizeof(pos));
   index.read((char*)&pos, sizeof(pos));
   return pos;
}

void state_history_log::truncate(state_history_log::block_num_type block_num) {
   write_log.flush();
   index.flush();
   uint64_t num_removed = 0;
   if (block_num <= _begin_block) {
      num_removed = _end_block - _begin_block;
      write_log.seek(0);
      index.seek(0);
      boost::filesystem::resize_file(read_log.get_file_path(), 0);
      boost::filesystem::resize_file(index.get_file_path(), 0);
      _begin_block = _end_block = 0;
   } else {
      num_removed  = _end_block - block_num;
      uint64_t pos = get_pos(block_num);
      write_log.seek(0);
      index.seek(0);
      boost::filesystem::resize_file(index.get_file_path(), pos);
      boost::filesystem::resize_file(read_log.get_file_path(), (block_num - _begin_block) * sizeof(uint64_t));
      _end_block = block_num;
   }
   write_log.flush();
   index.flush();
   ilog("fork or replay: removed ${n} blocks from ${name}.log", ("n", num_removed)("name", name));
}

state_history_log::block_num_type state_history_log::write_entry_header(const state_history_log_header& header,
                                                                        const chain::block_id_type&     prev_id) {
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

   if (block_num < _end_block) {
      truncate(block_num);
   }
   write_log.seek_end(0);
   write_header(header);
   return block_num;
}

void state_history_log::write_entry_position(const state_history_log_header&       header,
                                             state_history_log::file_position_type pos,
                                             state_history_log::block_num_type     block_num) {
   uint64_t end               = write_log.tellp();
   uint64_t payload_start_pos = pos + state_history_log_header_serial_size;
   uint64_t payload_size      = end - payload_start_pos;
   write_log.write((char*)&pos, sizeof(pos));
   write_log.seek(payload_start_pos - sizeof(header.payload_size));
   write_log.write((char*)&payload_size, sizeof(header.payload_size));
   write_log.seek_end(0);

   index.seek_end(0);
   index.write((char*)&pos, sizeof(pos));
   if (_begin_block == _end_block)
      _begin_block = block_num;
   _end_block    = block_num + 1;
   last_block_id = header.block_id;

   write_log.flush();
   index.flush();

   if (block_num % stride == 0) {
      split_log();
   }
}

void state_history_log::split_log() {
   index.close();
   read_log.close();
   write_log.close();

   catalog.add(_begin_block, _end_block - 1, read_log.get_file_path().parent_path(), name);

   _begin_block = 0;
   _end_block   = 0;

   write_log.open("w+b");
   read_log.open("rb");
   index.open("w+b");
}

state_history_traces_log::state_history_traces_log(const state_history_config& config)
    : state_history_log("trace_history", config) {}

std::vector<state_history::transaction_trace> state_history_traces_log::get_traces(block_num_type block_num) {

   auto [ds, _] = catalog.ro_stream_for_block(block_num);
   if (ds.remaining()) {
      std::vector<state_history::transaction_trace> traces;
      state_history::trace_converter::unpack(ds, traces);
      return traces;
   }

   if (block_num < begin_block() || block_num >= end_block())
      return {};
   state_history_log_header header;
   get_entry_header(block_num, header);
   std::vector<state_history::transaction_trace> traces;
   state_history::trace_converter::unpack(read_log, traces);
   return traces;
}

void state_history_traces_log::prune_transactions(state_history_log::block_num_type        block_num,
                                                  std::vector<chain::transaction_id_type>& ids) {
   auto [ds, _] = catalog.rw_stream_for_block(block_num);
   if (ds.remaining()) {
      state_history::trace_converter::prune_traces(ds, ds.remaining(), ids);
      return;
   }

   if (block_num < begin_block() || block_num >= end_block())
      return;
   state_history_log_header header;
   get_entry_header(block_num, header);
   write_log.seek(read_log.tellp());
   state_history::trace_converter::prune_traces(write_log, header.payload_size, ids);
   write_log.flush();
}

void state_history_traces_log::store(const chainbase::database& db, const chain::block_state_ptr& block_state) {

   state_history_log_header header{.magic = ship_magic(ship_current_version), .block_id = block_state->id};
   auto                     trace = cache.prepare_traces(block_state);

   this->write_entry(header, block_state->block->previous, [&](auto& stream) {
      state_history::trace_converter::pack(stream, db, trace_debug_mode, trace, compression);
   });
}

bool state_history_traces_log::exists(bfs::path state_history_dir) {
   return bfs::exists(state_history_dir / "trace_history.log") &&
          bfs::exists(state_history_dir / "trace_history.index");
}

state_history_chain_state_log::state_history_chain_state_log(const state_history_config& config)
    : state_history_log("chain_state_history", config) {}

chain::bytes state_history_chain_state_log::get_log_entry(block_num_type block_num) {

   auto [ds, _] = catalog.ro_stream_for_block(block_num);
   if (ds.remaining()) {
      return state_history::zlib_decompress(ds);
   }

   if (block_num < begin_block() || block_num >= end_block())
      return {};
   state_history_log_header header;
   get_entry_header(block_num, header);
   return state_history::zlib_decompress(read_log);
}

void state_history_chain_state_log::store(const chain::combined_database& db, const chain::block_state_ptr& block_state) {
   bool fresh = this->begin_block() == this->end_block();
   if (fresh)
      ilog("Placing initial state in block ${n}", ("n", block_state->block->block_num()));

   using namespace state_history;
   std::vector<table_delta> deltas = create_deltas(db, fresh);
   state_history_log_header header{.magic = ship_magic(ship_current_version), .block_id = block_state->id};

   this->write_entry(header, block_state->block->previous, [&deltas](auto& stream) { zlib_pack(stream, deltas); });
}

} // namespace eosio
