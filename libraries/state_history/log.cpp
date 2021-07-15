#include <eosio/state_history/compression.hpp>
#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/state_history/serialization.hpp>
#include <eosio/state_history/trace_converter.hpp>
#include <fc/log/logger_config.hpp> //set_thread_name

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
    : name(name), logger(config.logger ? *config.logger : fc::logger::get(DEFAULT_LOGGER)) {
   catalog.open(config.log_dir, config.retained_dir, config.archive_dir, name);
   catalog.max_retained_files = config.max_retained_files;
   this->stride               = config.stride;
   open_log(config.log_dir / (std::string(name) + ".log"));
   open_index(config.log_dir / (std::string(name) + ".index"));

   thr = std::thread([this] {
      fc::set_os_thread_name( this->name );
      try {
         this->ctx.run();
      }
      catch(...) {
         fc_elog(logger,"catched exception from ${name} write thread", ("name", this->name));
         eptr = std::current_exception();
         write_thread_has_exception = true;
      }
      fc_ilog(logger,"${name} thread ended", ("name", this->name));
   });
   
}

void state_history_log::stop() {
   if (thr.joinable()) {
      work_guard.reset();
      thr.join();
      cached.clear();
   }
}

// called from constructor, write_entry() or get_entry_header() indirectly
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


// called from read_entry(), modify_entry() and get_block_id()
std::optional<state_history_log_header>
state_history_log::get_entry_header(state_history_log::block_num_type block_num) {
   if (block_num < _begin_block || block_num >= _end_block)
      return {};
   return get_entry_header_i(block_num);
}

// only called from write_entry() indirectly or get_entry_header()
state_history_log_header state_history_log::get_entry_header_i(block_num_type block_num) {
   state_history_log_header header;
   read_log.seek(get_pos(block_num));
   read_header(header);
   return header;
}

// only called from main thread
std::optional<chain::block_id_type> state_history_log::get_block_id(state_history_log::block_num_type block_num) {
   std::lock_guard lock(mx);
   auto result = catalog.id_for_block(block_num);
   if (!result) {
      auto header = get_entry_header(block_num);
      if (header)
         return header->block_id;
   }
   return result;
}

// only called from constructor indirectly
bool state_history_log::get_last_block(uint64_t size) {
   state_history_log_header header;
   uint64_t                 suffix;
   read_log.seek(size - sizeof(suffix));
   read_log.read((char*)&suffix, sizeof(suffix));
   if (suffix > size || suffix + state_history_log_header_serial_size > size) {
      fc_elog(logger,"corrupt ${name}.log (2)", ("name", name));
      return false;
   }
   read_log.seek(suffix);
   read_header(header, false);
   if (!is_ship(header.magic) || !is_ship_supported_version(header.magic) ||
       suffix + state_history_log_header_serial_size + header.payload_size + sizeof(suffix) != size) {
      fc_elog(logger,"corrupt ${name}.log (3)", ("name", name));
      return false;
   }
   _end_block    = chain::block_header::num_from_id(header.block_id) + 1;
   last_block_id = header.block_id;
   if (_begin_block >= _end_block) {
      fc_elog(logger,"corrupt ${name}.log (4)", ("name", name));
      return false;
   }
   return true;
}

// only called from constructor indirectly
void state_history_log::recover_blocks(uint64_t size) {
   fc_ilog(logger,"recover ${name}.log", ("name", name));
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
         fc_dlog(logger,"${num_found} blocks found, log pos = ${pos}", ("num_found", num_found)("pos", pos));
      }
   }
   read_log.flush();
   boost::filesystem::resize_file(read_log.get_file_path(), pos);
   read_log.flush();
   EOS_ASSERT(get_last_block(pos), chain::state_history_exception, "recover ${name}.log failed", ("name", name));
}

// only called from constructor
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
      fc_ilog(logger,"${name}.log has blocks ${b}-${e}", ("name", name)("b", _begin_block)("e", _end_block - 1));
   } else {
      EOS_ASSERT(!size, chain::state_history_exception, "corrupt ${name}.log (5)", ("name", name));
      fc_ilog(logger,"${name}.log is empty", ("name", name));
   }
}

// only called from constructor
void state_history_log::open_index(bfs::path index_filename) {
   index.set_file_path(index_filename);
   index.open("a+b"); // std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app
   index.seek_end(0);
   if (index.tellp() == (static_cast<int>(_end_block) - _begin_block) * sizeof(uint64_t))
      return;
   fc_ilog(logger,"Regenerate ${name}.index", ("name", name));
   index.close();

   state_history_log_data(read_log.get_file_path()).construct_index(index_filename);
   index.open("a+b"); // std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app
   index.seek_end(0);
}

// only called from write_entry() indirectly or get_entry_header()
state_history_log::file_position_type state_history_log::get_pos(state_history_log::block_num_type block_num) {
   uint64_t pos;
   index.seek((block_num - _begin_block) * sizeof(pos));
   index.read((char*)&pos, sizeof(pos));
   return pos;
}

// only called from write_entry() indirectly
void state_history_log::truncate(state_history_log::block_num_type block_num) {
   write_log.flush();
   index.flush();
   auto first_block_num     = catalog.empty() ? _begin_block : catalog.first_block_num();
   auto new_begin_block_num = catalog.truncate(block_num, read_log.get_file_path());

   if (new_begin_block_num > 0) {
      // in this case, the original index/log file has been replaced from some files from the catalog, we need to
      // reopen read_log, write_log and index.
      index.close();
      index.open("rb");
      _begin_block = new_begin_block_num;
   }

   uint64_t num_removed;
   if (block_num <= _begin_block) {
      num_removed = _end_block - first_block_num;
      boost::filesystem::resize_file(read_log.get_file_path(), 0);
      boost::filesystem::resize_file(index.get_file_path(), 0);
      _begin_block = _end_block = block_num;
   } else {
      num_removed  = _end_block - block_num;
      uint64_t pos = get_pos(block_num);
      boost::filesystem::resize_file(read_log.get_file_path(), pos);
      boost::filesystem::resize_file(index.get_file_path(), (block_num - _begin_block) * sizeof(uint64_t));
      _end_block = block_num;
   }

   read_log.close();
   read_log.open("rb");
   write_log.close();
   write_log.open("rb+");
   index.close();
   index.open("a+b");

   fc_ilog(logger,"fork or replay: removed ${n} blocks from ${name}.log", ("n", num_removed)("name", name));
}

// only called from write_entry()
std::pair<state_history_log::block_num_type, state_history_log::file_position_type>
state_history_log::write_entry_header(const state_history_log_header& header, const chain::block_id_type& prev_id) {
   block_num_type block_num = chain::block_header::num_from_id(header.block_id);
   fc_dlog(logger,"write_entry_header name=${name} block_num=${block_num}",("name", name) ("block_num", block_num));

   EOS_ASSERT(_begin_block == _end_block || block_num <= _end_block, chain::state_history_exception,
              "missed a block in ${name}.log", ("name", name));

   if (_begin_block != _end_block && block_num > _begin_block) {
      if (block_num == _end_block) {
         EOS_ASSERT(prev_id == last_block_id, chain::state_history_exception, "missed a fork change in ${name}.log",
                    ("name", name));
      } else {
         state_history_log_header prev = get_entry_header_i(block_num - 1);
         EOS_ASSERT(prev_id == prev.block_id, chain::state_history_exception, "missed a fork change in ${name}.log",
                    ("name", name));
      }
   }

   if (block_num < _end_block) {
      truncate(block_num);
   }
   write_log.seek_end(0);
   file_position_type pos = write_log.tellp();
   write_header(header);
   return std::make_pair(block_num, pos);
}

// only called from write_entry()
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

// only called from write_entry() indirectly
void state_history_log::split_log() {
   index.close();
   read_log.close();
   write_log.close();

   catalog.add(_begin_block, _end_block - 1, read_log.get_file_path().parent_path(), name);

   _begin_block = _end_block;

   write_log.open("w+b");
   read_log.open("rb");
   index.open("w+b");
}

// always called from main thread
void state_history_log::store_entry(const chain::block_id_type& id, const chain::block_id_type& prev_id,
                                    std::vector<char>&& data) {
   
   if (write_thread_has_exception) {
      std::rethrow_exception(eptr);
   }
   block_num_type block_num = chain::block_header::num_from_id(id);
   auto shared_data = std::make_shared<std::vector<char>>(std::move(data));
   boost::asio::post(work_strand, [this, id, prev_id, shared_data ]() {
      write_entry(id, prev_id, shared_data);
   });
   cached[block_num] = shared_data;

   while (cached.size() > num_buffered_entries && cached.begin()->second.use_count() == 1) {
      // the log entry has been removed from write_queue, so we can safely erase it. 
      cached.erase(cached.begin());
   }

   fc_dlog(logger,"store_entry name=${name}, block_num=${block_num} cached.size = ${sz}, num_buffered_entries=${num_buffered_entries}, id=${id}",
         ("name", name)("block_num", block_num)("sz", cached.size())("num_buffered_entries", num_buffered_entries)("id", id));

}

// always called from internal thread
void state_history_log::write_entry(const chain::block_id_type& id, const chain::block_id_type& prev_id,
                                    const std::shared_ptr<std::vector<char>>& data) {
   state_history_log_header header{.magic = ship_magic(ship_current_version), .block_id = id};

   std::unique_lock<std::mutex> lock(mx);
   auto [block_num, start_pos] = write_entry_header(header, prev_id);
   try {
      lock.unlock();
      this->write_payload(write_log, *data);
      lock.lock();
      write_entry_position(header, start_pos, block_num);
      fc_dlog(logger, "entry block_num=${block_num} id=${id} written", ("block_num", block_num)("id", id));
   } catch (...) {
      write_log.close();
      boost::filesystem::resize_file(write_log.get_file_path(), start_pos);
      write_log.open("rb+");
      throw;
   }
}

// always called from main thread
std::pair<std::vector<char>, uint32_t> state_history_log::read_entry(block_num_type block_num) {
   std::vector<char> data;
   std::lock_guard lock(mx);
   auto [ds, version] = catalog.ro_stream_for_block(block_num);
   std::size_t size   = ds.remaining();
   if (size) {
      data.resize(size);
      ds.read(data.data(), size);
   } 
   else {
      auto header = get_entry_header(block_num);
      if (header) {
         data.resize(header->payload_size);
         read_log.read(data.data(), header->payload_size);
         version = get_ship_version(header->magic);
      }
   }
   
   return std::make_pair(std::move(data), version);
}

// Only used for prune_transaction, not thread safe
template <typename Lambda>
void state_history_log::modify_entry(block_num_type block_num, Lambda f) {
   auto [ds, version] = catalog.rw_stream_for_block(block_num);

   if (ds.remaining()) {
      f(ds, version, ds.remaining());
      return;
   }

   auto header = get_entry_header(block_num);
   if (header) {
      write_log.seek(read_log.tellp());
      f(write_log, get_ship_version(header->magic), header->payload_size);
      write_log.flush();
   }
}

state_history_traces_log::state_history_traces_log(const state_history_config& config)
    : state_history_log("trace_history", config) {
}

std::shared_ptr<std::vector<char>> state_history_traces_log::get_log_entry(block_num_type block_num) {

   auto itr = cached.find(block_num);
   if (itr != cached.end()) {
      std::vector<state_history::transaction_trace> traces;
      auto ds = fc::datastream<const char*>(itr->second->data(), itr->second->size());
      state_history::trace_converter::unpack(ds, traces);
      return std::make_shared<std::vector<char>>(fc::raw::pack(traces));
   }

   auto [data, version] = read_entry(block_num);
   if (data.size()) {
      try {
         auto ds = fc::datastream<const char*>(data.data(), data.size());
         if (version == 0) {
            return std::make_shared<std::vector<char>>(state_history::zlib_decompress(ds));
         }
         else {
            std::vector<state_history::transaction_trace> traces;
            state_history::trace_converter::unpack(ds, traces);
            return std::make_shared<std::vector<char>>(fc::raw::pack(traces));
         }
      } catch (fc::exception& ex) {
         fc::cfile output;
         char      filename[PATH_MAX];
         snprintf(filename, PATH_MAX, "invalid_trace_%u_v%u.bin", block_num, version);
         output.set_file_path(filename);
         output.open("w");
         output.write(data.data(), data.size());

         ex.append_log(FC_LOG_MESSAGE(error,
                                    "trace data for block ${block_num} has been written to ${filename} for debugging",
                                    ("block_num", block_num)("filename", filename)));

         throw ex;
      }
   }
   return std::make_shared<std::vector<char>>();
}

// Only used by eosio_blocklog, not thread safe
void state_history_traces_log::prune_transactions(state_history_log::block_num_type        block_num,
                                                  std::vector<chain::transaction_id_type>& ids) {
   modify_entry(block_num, [&ids](auto& ds, uint32_t version, std::size_t size) {
      EOS_ASSERT(version > 0, chain::state_history_exception,
              "The trace log version 0 does not support transaction pruning.");
      state_history::trace_converter::prune_traces(ds, size, ids);
   });
}

void state_history_traces_log::store(const chainbase::database& db, const chain::block_state_ptr& block_state) {

   auto trace = cache.prepare_traces(block_state);
   fc::datastream<std::vector<char>> raw_strm;
   state_history::trace_converter::pack(raw_strm, db, trace_debug_mode, trace, compression);
   this->store_entry(block_state->id, block_state->block->previous, std::move(raw_strm.storage()));
}

void state_history_traces_log::write_payload(state_history_log::cfile_stream& stream, const std::vector<char>& data) {
   stream.write(data.data(), data.size());
}

bool state_history_traces_log::exists(bfs::path state_history_dir) {
   return bfs::exists(state_history_dir / "trace_history.log") &&
          bfs::exists(state_history_dir / "trace_history.index");
}

state_history_chain_state_log::state_history_chain_state_log(const state_history_config& config)
    : state_history_log("chain_state_history", config) {}

std::shared_ptr<std::vector<char>> state_history_chain_state_log::get_log_entry(block_num_type block_num) {
   auto itr = cached.find(block_num);
   if (itr != cached.end()) {
      return itr->second;
   }
   auto [data, version] = read_entry(block_num);
   if (data.size()) {
      auto ds = fc::datastream<const char*>(data.data(), data.size());
      return std::make_shared<std::vector<char>>(state_history::zlib_decompress(ds));
   }
   return std::make_shared<std::vector<char>>();
}

void state_history_chain_state_log::store(const chain::combined_database& db,
                                          const chain::block_state_ptr&   block_state) {
   auto [begin, end] = begin_end_block_nums();
   bool fresh        = begin == end;
   if (fresh)
      fc_ilog(logger,"Placing initial state in block ${n}", ("n", block_state->block->block_num()));

   using namespace state_history;
   std::vector<table_delta> deltas = create_deltas(db, fresh);

   fc::datastream<std::vector<char>> raw_strm;
   fc::raw::pack(raw_strm, deltas);
   this->store_entry(block_state->id, block_state->block->previous, std::move(raw_strm.storage()));
}

void state_history_chain_state_log::write_payload(state_history_log::cfile_stream& stream, const std::vector<char>& data) {
   state_history::zlib_pack_raw(stream, data);
}

} // namespace eosio
