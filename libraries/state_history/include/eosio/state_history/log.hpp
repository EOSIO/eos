#pragma once

#include <boost/filesystem.hpp>
#include <fstream>
#include <stdint.h>

#include <eosio/chain/block_header.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <fc/io/cfile.hpp>
#include <fc/log/logger.hpp>
#include <eosio/state_history/trace_converter.hpp>

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

inline uint64_t       ship_magic(uint32_t version) { return N(ship).to_uint64_t() | version; }
inline bool           is_ship(uint64_t magic) { return (magic & 0xffff'ffff'0000'0000) == N(ship).to_uint64_t(); }
inline uint32_t       get_ship_version(uint64_t magic) { return magic; }
inline bool           is_ship_supported_version(uint64_t magic) { return get_ship_version(magic) <= 1; }
static const uint32_t ship_current_version = 1;

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
   fc::cfile            log;
   fc::cfile            index;
   uint32_t             _begin_block = 0;
   uint32_t             _end_block   = 0;
   chain::block_id_type last_block_id;
   uint32_t             version = ship_current_version;
 public:
   // The type aliases below help to make it obvious about the meanings of member function return values.
   using block_num_type     = uint32_t;
   using version_type       = uint32_t;
   using file_position_type = uint64_t;

   state_history_log(const char* const name, std::string log_filename, std::string index_filename);

   block_num_type begin_block() const { return _begin_block; }
   block_num_type end_block() const { return _end_block; }

   void read_header(state_history_log_header& header, bool assert_version = true);
   void write_header(const state_history_log_header& header);

   template <typename F>
   void write_entry(const state_history_log_header& header, const chain::block_id_type& prev_id, F write_payload) {
      auto [pos, block_num] = write_entry_header(header, prev_id);
      write_payload(log);
      write_entry_position(header, pos, block_num);
   }

   /// @returns cfile positioned at payload
   fc::cfile&           get_entry_header(block_num_type block_num, state_history_log_header& header);
   chain::block_id_type get_block_id(block_num_type block_num);

   /**
    * @returns the entry payload and its version if existed
    **/
   fc::optional<std::pair<chain::bytes, version_type>> get_entry(block_num_type block_num);

   template <typename Converter>
   fc::optional<chain::bytes> get_entry(uint32_t block_num, Converter conv) {
      auto entry = this->get_entry(block_num);
      if (entry)
         return conv(entry->first, entry->second);
      return {};
   }

 protected:
   fc::cfile& get_log() { return log; }
 private:
   bool               get_last_block(uint64_t size);
   void               recover_blocks(uint64_t size);
   void               open_log();
   void               open_index();
   file_position_type get_pos(block_num_type block_num);
   void               truncate(block_num_type block_num);

   /**
    *  @returns the file position of the written entry and its block num
    **/
   std::pair<file_position_type, block_num_type> write_entry_header(const state_history_log_header& header,
                                                                    const chain::block_id_type&     prev_id);
   void write_entry_position(const state_history_log_header& header, file_position_type pos, block_num_type block_num);
}; // state_history_log

class state_history_traces_log : public state_history_log {
   state_history::trace_converter trace_convert;

 public:
   using compression_type            = chain::packed_transaction::prunable_data_type::compression_type;
   bool             trace_debug_mode = false;

   state_history_traces_log(fc::path state_history_dir);

   static bool exists(fc::path state_history_dir);

   void add_transaction(const chain::transaction_trace_ptr& trace, const chain::packed_transaction_ptr& transaction) {
      trace_convert.add_transaction(trace, transaction);
   }

   fc::optional<chain::bytes> get_log_entry(block_num_type block_num);

   void store(const chainbase::database& db, const chain::block_state_ptr& block_state);

   /**
    *  @param[in,out] ids The ids to been pruned and returns the ids not found in the specified block
    **/
   void prune_transactions(block_num_type block_num, std::vector<chain::transaction_id_type>& ids);
};

class state_history_chain_state_log : public state_history_log {
 public:
   state_history_chain_state_log(fc::path state_history_dir);
   
   fc::optional<chain::bytes> get_log_entry(block_num_type block_num);

   void store(const chainbase::database& db, const chain::block_state_ptr& block_state);
};

} // namespace eosio

FC_REFLECT(eosio::state_history_log_header, (magic)(block_id)(payload_size))
