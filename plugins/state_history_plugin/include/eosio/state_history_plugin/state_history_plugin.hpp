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
   fc::unsigned_int                        struct_version = 0;
   std::string                             name{};
   std::vector<std::pair<uint64_t, bytes>> rows{};
   std::vector<uint64_t>                   removed{};
};

struct get_state_request_v0 {};

struct get_state_result_v0 {
   uint32_t last_block_num = 0;
};

struct get_block_request_v0 {
   uint32_t block_num = 0;
};

struct get_block_result_v0 {
   uint32_t block_num = 0;
   bool     found     = false;
   bytes    deltas;
};

using state_request = fc::static_variant<get_state_request_v0, get_block_request_v0>;
using state_result  = fc::static_variant<get_state_result_v0, get_block_result_v0>;

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

FC_REFLECT(eosio::table_delta, (struct_version)(name)(rows)(removed));
FC_REFLECT_EMPTY(eosio::get_state_request_v0);
FC_REFLECT(eosio::get_state_result_v0, (last_block_num));
FC_REFLECT(eosio::get_block_request_v0, (block_num));
FC_REFLECT(eosio::get_block_result_v0, (block_num)(found)(deltas));
