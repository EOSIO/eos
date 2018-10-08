/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <boost/filesystem.hpp>
#include <fstream>
#include <stdint.h>

#include <eosio/chain/exceptions.hpp>
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
 *    uint32_t    block_num
 *    uint64_t    size of payload
 *    uint8_t     version
 *                payload
 *
 * irreversible_state payload:
 *    uint32_t    size of deltas
 *    char[]      deltas
 */

struct history_log_header {
   uint32_t block_num    = 0;
   uint64_t payload_size = 0;
   uint16_t version      = 0;
};

class history_log {
 private:
   const char* const name     = "";
   bool              _is_open = false;
   std::string       log_filename;
   std::string       index_filename;
   std::fstream      log;
   std::fstream      index;
   uint32_t          _begin_block = 0;
   uint32_t          _end_block   = 0;

 public:
   history_log(const char* const name)
       : name(name) {}

   bool     is_open() const { return _is_open; }
   uint32_t begin_block() const { return _begin_block; }
   uint32_t end_block() const { return _end_block; }

   void open(std::string log_filename, std::string index_filename) {
      this->log_filename   = std::move(log_filename);
      this->index_filename = std::move(index_filename);
      open_log();
      open_index();
      _is_open = true;
   }

   void close() {
      log.close();
      index.close();
      _is_open = false;
   }

   template <typename F>
   void write_entry(history_log_header header, F write_payload) {
      EOS_ASSERT(_is_open, chain::plugin_exception, "writing to closed ${name}.log", ("name", name));
      EOS_ASSERT(_begin_block == _end_block || header.block_num <= _end_block, chain::plugin_exception,
                 "missed a block writing to ${name}.log", ("name", name));
      if (header.block_num < _end_block)
         truncate(header.block_num);
      log.seekg(0, std::ios_base::end);
      uint64_t pos = log.tellg();
      log.write((char*)&header, sizeof(header));
      write_payload(log);
      uint64_t end = log.tellg();
      EOS_ASSERT(end == pos + sizeof(header) + header.payload_size, chain::plugin_exception,
                 "wrote payload with incorrect size to ${name}.log", ("name", name));
      log.write((char*)&pos, sizeof(pos));

      index.seekg(0, std::ios_base::end);
      index.write((char*)&pos, sizeof(pos));
      if (_begin_block == _end_block)
         _begin_block = header.block_num;
      _end_block = header.block_num + 1;
   }

   // returns stream positioned at payload
   std::fstream& get_entry(uint32_t block_num, history_log_header& header) {
      EOS_ASSERT(_is_open, chain::plugin_exception, "reading from closed ${name}.log", ("name", name));
      EOS_ASSERT(block_num >= _begin_block && block_num < _end_block, chain::plugin_exception,
                 "read non-existing block in ${name}.log", ("name", name));
      log.seekg(get_pos(block_num));
      log.read((char*)&header, sizeof(header));
      return log;
   }

 private:
   void open_log() {
      log.open(log_filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app);
      uint64_t size = log.tellg();
      if (size >= sizeof(history_log_header)) {
         history_log_header header;
         log.seekg(0);
         log.read((char*)&header, sizeof(header));
         EOS_ASSERT(header.version == 0 && sizeof(header) + header.payload_size + sizeof(uint64_t) <= size,
                    chain::plugin_exception, "corrupt ${name}.log (1)", ("name", name));
         _begin_block = header.block_num;

         uint64_t end_pos;
         log.seekg(size - sizeof(end_pos));
         log.read((char*)&end_pos, sizeof(end_pos));
         EOS_ASSERT(end_pos <= size && end_pos + sizeof(header) <= size, chain::plugin_exception,
                    "corrupt ${name}.log (2)", ("name", name));
         log.seekg(end_pos);
         log.read((char*)&header, sizeof(header));
         EOS_ASSERT(end_pos + sizeof(header) + header.payload_size + sizeof(uint64_t) == size, chain::plugin_exception,
                    "corrupt ${name}.log (3)", ("name", name));
         _end_block = header.block_num + 1;

         EOS_ASSERT(_begin_block < _end_block, chain::plugin_exception, "corrupt ${name}.log (4)", ("name", name));
         ilog("${name}.log has blocks ${b}-${e}", ("name", name)("b", _begin_block)("e", _end_block - 1));
      } else {
         EOS_ASSERT(!size, chain::plugin_exception, "corrupt ${name}.log (5)", ("name", name));
         ilog("${name}.log is empty", ("name", name));
      }
   }

   void open_index() {
      index.open(index_filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app);
      if (index.tellg() == (_end_block - _begin_block) * sizeof(uint64_t))
         return;
      ilog("Regenerate ${name}.index", ("name", name));
      index.close();
      index.open(index_filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

      log.seekg(0, std::ios_base::end);
      uint64_t size = log.tellg();
      uint64_t pos  = 0;
      while (pos < size) {
         index.write((char*)&pos, sizeof(pos));
         history_log_header header;
         EOS_ASSERT(pos + sizeof(header) <= size, chain::plugin_exception, "corrupt ${name}.log (6)", ("name", name));
         log.seekg(pos);
         log.read((char*)&header, sizeof(header));
         uint64_t suffix_pos = pos + sizeof(header) + header.payload_size;
         uint64_t suffix;
         EOS_ASSERT(suffix_pos + sizeof(suffix) <= size, chain::plugin_exception, "corrupt ${name}.log (7)",
                    ("name", name));
         log.seekg(suffix_pos);
         log.read((char*)&suffix, sizeof(suffix));
         // ilog("block ${b} at ${pos}-${end} suffix=${suffix} file_size=${fs}",
         //      ("b", header.block_num)("pos", pos)("end", suffix_pos + sizeof(suffix))("suffix", suffix)("fs", size));
         EOS_ASSERT(suffix == pos, chain::plugin_exception, "corrupt ${name}.log (8)", ("name", name));
         pos = suffix_pos + sizeof(suffix);
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
         boost::filesystem::resize_file(index_filename, (block_num - _begin_block) * sizeof(pos));
         _end_block = block_num;
      }
      log.sync();
      index.sync();
      ilog("fork or replay: removed ${n} blocks from ${name}.log", ("n", num_removed)("name", name));
   }
}; // history_log

} // namespace eosio
