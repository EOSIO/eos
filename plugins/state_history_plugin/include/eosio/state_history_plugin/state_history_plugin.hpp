/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>

namespace fc {
class variant;
}

namespace eosio {
using chain::bytes;
using std::shared_ptr;

typedef shared_ptr<struct state_history_plugin_impl> state_history_ptr;

struct table_delta {
   fc::unsigned_int                    struct_version = 0;
   std::string                         name{};
   std::vector<std::pair<bool, bytes>> rows{};
};

struct get_status_request_v0 {};

struct get_status_result_v0 {
   uint32_t             head_block_num              = 0;
   chain::block_id_type head_block_id               = {};
   uint32_t             last_irreversible_block_num = 0;
   chain::block_id_type last_irreversible_block_id  = {};
   uint32_t             block_state_begin_block     = 0;
   uint32_t             block_state_end_block       = 0;
   uint32_t             trace_begin_block           = 0;
   uint32_t             trace_end_block             = 0;
   uint32_t             chain_state_begin_block     = 0;
   uint32_t             chain_state_end_block       = 0;
};

struct get_block_request_v0 {
   uint32_t block_num         = 0;
   bool     fetch_block       = false;
   bool     fetch_block_state = false;
   bool     fetch_traces      = false;
   bool     fetch_deltas      = false;
};

struct get_block_result_v0 {
   uint32_t            block_num = 0;
   fc::optional<bytes> block;
   fc::optional<bytes> block_state;
   fc::optional<bytes> traces;
   fc::optional<bytes> deltas;
};

using state_request = fc::static_variant<get_status_request_v0, get_block_request_v0>;
using state_result  = fc::static_variant<get_status_result_v0, get_block_result_v0>;

class state_history_plugin : public plugin<state_history_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   state_history_plugin();
   virtual ~state_history_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

 private:
   state_history_ptr my;
};

} // namespace eosio

FC_REFLECT(eosio::table_delta, (struct_version)(name)(rows));
FC_REFLECT_EMPTY(eosio::get_status_request_v0);
FC_REFLECT(
    eosio::get_status_result_v0,
    (head_block_num)(head_block_id)(last_irreversible_block_num)(last_irreversible_block_id)(block_state_begin_block)(
        block_state_end_block)(trace_begin_block)(trace_end_block)(chain_state_begin_block)(chain_state_end_block));
FC_REFLECT(eosio::get_block_request_v0, (block_num)(fetch_block)(fetch_block_state)(fetch_traces)(fetch_deltas));
FC_REFLECT(eosio::get_block_result_v0, (block_num)(block)(block_state)(traces)(deltas));
