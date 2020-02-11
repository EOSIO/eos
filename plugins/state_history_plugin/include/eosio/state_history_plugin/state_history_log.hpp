/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <boost/filesystem.hpp>
#include <fstream>
#include <stdint.h>

#include <eosio/chain/block_header.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <fc/log/logger.hpp>

namespace eosio {

/*
 *   *.log:
 *   +---------+----------------+-----------+------------------+-----+---------+----------------+
 *   | Entry i | Pos of Entry i | Entry i+1 | Pos of Entry i+1 | ... | Entry z | Pos of Entry z |
 *   +---------+----------------+-----------+------------------+-----+---------+----------------+
 *
 *   *.index:
 *   +----------------+------------------+-----+----------------+
 *   | Pos of Entry i | Pos of Entry i+1 | ... | Pos of Entry z |
 *   +----------------+------------------+-----+----------------+
 *
 * each entry:
 *    state_history_log_header
 *    payload
 */

inline uint64_t       ship_magic(uint32_t version) { return N(ship) | version; }
inline bool           is_ship(uint64_t magic) { return (magic & 0xffff'ffff'0000'0000) == N(ship); }
inline uint32_t       get_ship_version(uint64_t magic) { return magic; }
inline bool           is_ship_supported_version(uint64_t magic) { return get_ship_version(magic) == 0; }
static const uint32_t ship_current_version = 0;

struct state_history_log_header {
   uint64_t             magic        = ship_magic(ship_current_version);
   chain::block_id_type block_id     = {};
   uint64_t             payload_size = 0;
};
static const int state_history_log_header_serial_size = sizeof(state_history_log_header::magic) +
                                                        sizeof(state_history_log_header::block_id) +
                                                        sizeof(state_history_log_header::payload_size);

class state_history_log {
 private:
   const char* const    name = "";
   std::string          log_filename;
   std::string          index_filename;
   std::fstream         log;
   std::fstream         index;
   uint32_t             _begin_block = 0;
   uint32_t             _end_block   = 0;
   chain::block_id_type last_block_id;

 public:
   state_history_log(const char* const name, std::string log_filename, std::string index_filename)
       : name(name)
       , log_filename(std::move(log_filename))
       , index_filename(std::move(index_filename)) {
      open_log();
      open_index();
   }

   uint32_t begin_block() const { return _begin_block; }
   uint32_t end_block() const { return _end_block; }

   void read_header(state_history_log_header& header, bool assert_version = true) {
      char bytes[state_history_log_header_serial_size];
      log.read(bytes, sizeof(bytes));
      fc::datastream<const char*> ds(bytes, sizeof(bytes));
      fc::raw::unpack(ds, header);
      EOS_ASSERT(!ds.remaining(), chain::plugin_exception, "state_history_log_header_serial_size mismatch");
      if (assert_version)
         EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic), chain::plugin_exception,
                    "corrupt ${name}.log (0)", ("name", name));
   }

   void write_header(const state_history_log_header& header) {
      char                  bytes[state_history_log_header_serial_size];
      fc::datastream<char*> ds(bytes, sizeof(bytes));
      fc::raw::pack(ds, header);
      EOS_ASSERT(!ds.remaining(), chain::plugin_exception, "state_history_log_header_serial_size mismatch");
      log.write(bytes, sizeof(bytes));
   }

   template <typename F>
   void write_entry(const state_history_log_header& header, const chain::block_id_type& prev_id, F write_payload) {
      auto block_num = chain::block_header::num_from_id(header.block_id);
      EOS_ASSERT(_begin_block == _end_block || block_num <= _end_block, chain::plugin_exception,
                 "missed a block in ${name}.log", ("name", name));

      if (_begin_block != _end_block && block_num > _begin_block) {
         if (block_num == _end_block) {
            EOS_ASSERT(prev_id == last_block_id, chain::plugin_exception, "missed a fork change in ${name}.log",
                       ("name", name));
         } else {
            state_history_log_header prev;
            get_entry(block_num - 1, prev);
            EOS_ASSERT(prev_id == prev.block_id, chain::plugin_exception, "missed a fork change in ${name}.log",
                       ("name", name));
         }
      }

      if (block_num < _end_block)
         truncate(block_num);
      log.seekg(0, std::ios_base::end);
      uint64_t pos = log.tellg();
      write_header(header);
      write_payload(log);
      uint64_t end = log.tellg();
      EOS_ASSERT(end == pos + state_history_log_header_serial_size + header.payload_size, chain::plugin_exception,
                 "wrote payload with incorrect size to ${name}.log", ("name", name));
      log.write((char*)&pos, sizeof(pos));

      index.seekg(0, std::ios_base::end);
      index.write((char*)&pos, sizeof(pos));
      if (_begin_block == _end_block)
         _begin_block = block_num;
      _end_block    = block_num + 1;
      last_block_id = header.block_id;
   }

   // returns stream positioned at payload
   std::fstream& get_entry(uint32_t block_num, state_history_log_header& header) {
      EOS_ASSERT(block_num >= _begin_block && block_num < _end_block, chain::plugin_exception,
                 "read non-existing block in ${name}.log", ("name", name));
      log.seekg(get_pos(block_num));
      read_header(header);
      return log;
   }

   chain::block_id_type get_block_id(uint32_t block_num) {
      state_history_log_header header;
      get_entry(block_num, header);
      return header.block_id;
   }

 private:
   bool get_last_block(uint64_t size) {
      state_history_log_header header;
      uint64_t                 suffix;
      log.seekg(size - sizeof(suffix));
      log.read((char*)&suffix, sizeof(suffix));
      if (suffix > size || suffix + state_history_log_header_serial_size > size) {
         elog("corrupt ${name}.log (2)", ("name", name));
         return false;
      }
      log.seekg(suffix);
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

   void recover_blocks(uint64_t size) {
      ilog("recover ${name}.log", ("name", name));
      uint64_t pos       = 0;
      uint32_t num_found = 0;
      while (true) {
         state_history_log_header header;
         if (pos + state_history_log_header_serial_size > size)
            break;
         log.seekg(pos);
         read_header(header, false);
         uint64_t suffix;
         if (!is_ship(header.magic) || !is_ship_supported_version(header.magic) || header.payload_size > size ||
             pos + state_history_log_header_serial_size + header.payload_size + sizeof(suffix) > size) {
            EOS_ASSERT(!is_ship(header.magic) || is_ship_supported_version(header.magic), chain::plugin_exception,
                       "${name}.log has an unsupported version", ("name", name));
            break;
         }
         log.seekg(pos + state_history_log_header_serial_size + header.payload_size);
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
      log.sync();
      EOS_ASSERT(get_last_block(pos), chain::plugin_exception, "recover ${name}.log failed", ("name", name));
   }

   void open_log() {
      log.open(log_filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app);
      log.seekg(0, std::ios_base::end);
      uint64_t size = log.tellg();
      if (size >= state_history_log_header_serial_size) {
         state_history_log_header header;
         log.seekg(0);
         read_header(header, false);
         EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic) &&
                        state_history_log_header_serial_size + header.payload_size + sizeof(uint64_t) <= size,
                    chain::plugin_exception, "corrupt ${name}.log (1)", ("name", name));
         _begin_block  = chain::block_header::num_from_id(header.block_id);
         last_block_id = header.block_id;
         if (!get_last_block(size))
            recover_blocks(size);
         ilog("${name}.log has blocks ${b}-${e}", ("name", name)("b", _begin_block)("e", _end_block - 1));
      } else {
         EOS_ASSERT(!size, chain::plugin_exception, "corrupt ${name}.log (5)", ("name", name));
         ilog("${name}.log is empty", ("name", name));
      }
   }

   void open_index() {
      index.open(index_filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app);
      index.seekg(0, std::ios_base::end);
      if (index.tellg() == (static_cast<int>(_end_block) - _begin_block) * sizeof(uint64_t))
         return;
      ilog("Regenerate ${name}.index", ("name", name));
      index.close();
      index.open(index_filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

      log.seekg(0, std::ios_base::end);
      uint64_t size      = log.tellg();
      uint64_t pos       = 0;
      uint32_t num_found = 0;
      while (pos < size) {
         state_history_log_header header;
         EOS_ASSERT(pos + state_history_log_header_serial_size <= size, chain::plugin_exception,
                    "corrupt ${name}.log (6)", ("name", name));
         log.seekg(pos);
         read_header(header, false);
         uint64_t suffix_pos = pos + state_history_log_header_serial_size + header.payload_size;
         uint64_t suffix;
         EOS_ASSERT(is_ship(header.magic) && is_ship_supported_version(header.magic) &&
                        suffix_pos + sizeof(suffix) <= size,
                    chain::plugin_exception, "corrupt ${name}.log (7)", ("name", name));
         log.seekg(suffix_pos);
         log.read((char*)&suffix, sizeof(suffix));
         // ilog("block ${b} at ${pos}-${end} suffix=${suffix} file_size=${fs}",
         //      ("b", header.block_num)("pos", pos)("end", suffix_pos + sizeof(suffix))("suffix", suffix)("fs", size));
         EOS_ASSERT(suffix == pos, chain::plugin_exception, "corrupt ${name}.log (8)", ("name", name));

         index.write((char*)&pos, sizeof(pos));
         pos = suffix_pos + sizeof(suffix);
         if (!(++num_found % 10000)) {
            printf("%10u blocks found, log pos=%12llu\r", (unsigned)num_found, (unsigned long long)pos);
            fflush(stdout);
         }
      }
   }

   uint64_t get_pos(uint32_t block_num) {
      uint64_t pos;
      index.seekg((block_num - _begin_block) * sizeof(pos));
      index.read((char*)&pos, sizeof(pos));
      return pos;
   }

   void truncate(uint32_t block_num) {
      log.flush();
      index.flush();
      uint64_t num_removed = 0;
      if (block_num <= _begin_block) {
         num_removed = _end_block - _begin_block;
         log.seekg(0);
         index.seekg(0);
         boost::filesystem::resize_file(log_filename, 0);
         boost::filesystem::resize_file(index_filename, 0);
         _begin_block = _end_block = 0;
      } else {
         num_removed  = _end_block - block_num;
         uint64_t pos = get_pos(block_num);
         log.seekg(0);
         index.seekg(0);
         boost::filesystem::resize_file(log_filename, pos);
         boost::filesystem::resize_file(index_filename, (block_num - _begin_block) * sizeof(uint64_t));
         _end_block = block_num;
      }
      log.sync();
      index.sync();
      ilog("fork or replay: removed ${n} blocks from ${name}.log", ("n", num_removed)("name", name));
   }
}; // state_history_log

} // namespace eosio

FC_REFLECT(eosio::state_history_log_header, (magic)(block_id)(payload_size))
