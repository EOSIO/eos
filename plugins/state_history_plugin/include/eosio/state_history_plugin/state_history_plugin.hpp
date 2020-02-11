/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>

template <typename T>
struct history_serial_big_vector_wrapper {
   T obj;
};

namespace fc {
class variant;
}

namespace eosio {
using chain::bytes;
using std::shared_ptr;

typedef shared_ptr<struct state_history_plugin_impl> state_history_ptr;

struct partial_transaction {
   chain::time_point_sec         expiration             = {};
   uint16_t                      ref_block_num          = {};
   uint32_t                      ref_block_prefix       = {};
   fc::unsigned_int              max_net_usage_words    = {};
   uint8_t                       max_cpu_usage_ms       = {};
   fc::unsigned_int              delay_sec              = {};
   chain::extensions_type        transaction_extensions = {};
   vector<chain::signature_type> signatures             = {};
   vector<bytes>                 context_free_data      = {};

   partial_transaction(const chain::signed_transaction& t)
       : expiration(t.expiration)
       , ref_block_num(t.ref_block_num)
       , ref_block_prefix(t.ref_block_prefix)
       , max_net_usage_words(t.max_net_usage_words)
       , max_cpu_usage_ms(t.max_cpu_usage_ms)
       , delay_sec(t.delay_sec)
       , transaction_extensions(t.transaction_extensions)
       , signatures(t.signatures)
       , context_free_data(t.context_free_data) {}
};

struct augmented_transaction_trace {
   chain::transaction_trace_ptr         trace;
   std::shared_ptr<partial_transaction> partial;

   augmented_transaction_trace()                                   = default;
   augmented_transaction_trace(const augmented_transaction_trace&) = default;
   augmented_transaction_trace(augmented_transaction_trace&&)      = default;

   augmented_transaction_trace(const chain::transaction_trace_ptr& trace)
       : trace{trace} {}

   augmented_transaction_trace(const chain::transaction_trace_ptr&         trace,
                               const std::shared_ptr<partial_transaction>& partial)
       : trace{trace}
       , partial{partial} {}

   augmented_transaction_trace(const chain::transaction_trace_ptr& trace, const chain::signed_transaction& t)
       : trace{trace}
       , partial{std::make_shared<partial_transaction>(t)} {}

   augmented_transaction_trace& operator=(const augmented_transaction_trace&) = default;
   augmented_transaction_trace& operator=(augmented_transaction_trace&&) = default;
};

struct table_delta {
   fc::unsigned_int                                                       struct_version = 0;
   std::string                                                            name{};
   history_serial_big_vector_wrapper<std::vector<std::pair<bool, bytes>>> rows{};
};

struct block_position {
   uint32_t             block_num = 0;
   chain::block_id_type block_id  = {};
};

struct get_status_request_v0 {};

struct get_status_result_v0 {
   block_position head                    = {};
   block_position last_irreversible       = {};
   uint32_t       trace_begin_block       = 0;
   uint32_t       trace_end_block         = 0;
   uint32_t       chain_state_begin_block = 0;
   uint32_t       chain_state_end_block   = 0;
};

struct get_blocks_request_v0 {
   uint32_t                    start_block_num        = 0;
   uint32_t                    end_block_num          = 0;
   uint32_t                    max_messages_in_flight = 0;
   std::vector<block_position> have_positions         = {};
   bool                        irreversible_only      = false;
   bool                        fetch_block            = false;
   bool                        fetch_traces           = false;
   bool                        fetch_deltas           = false;
};

struct get_blocks_ack_request_v0 {
   uint32_t num_messages = 0;
};

struct get_blocks_result_v0 {
   block_position               head;
   block_position               last_irreversible;
   fc::optional<block_position> this_block;
   fc::optional<block_position> prev_block;
   fc::optional<bytes>          block;
   fc::optional<bytes>          traces;
   fc::optional<bytes>          deltas;
};

using state_request = fc::static_variant<get_status_request_v0, get_blocks_request_v0, get_blocks_ack_request_v0>;
using state_result  = fc::static_variant<get_status_result_v0, get_blocks_result_v0>;

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

// clang-format off
FC_REFLECT(eosio::table_delta, (struct_version)(name)(rows));
FC_REFLECT(eosio::block_position, (block_num)(block_id));
FC_REFLECT_EMPTY(eosio::get_status_request_v0);
FC_REFLECT(eosio::get_status_result_v0, (head)(last_irreversible)(trace_begin_block)(trace_end_block)(chain_state_begin_block)(chain_state_end_block));
FC_REFLECT(eosio::get_blocks_request_v0, (start_block_num)(end_block_num)(max_messages_in_flight)(have_positions)(irreversible_only)(fetch_block)(fetch_traces)(fetch_deltas));
FC_REFLECT(eosio::get_blocks_ack_request_v0, (num_messages));
// clang-format on
