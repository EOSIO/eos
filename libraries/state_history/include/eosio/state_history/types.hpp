#pragma once

#include <eosio/chain/trace.hpp>

namespace eosio {
namespace state_history {

using chain::block_id_type;
using chain::bytes;
using chain::extensions_type;
using chain::signature_type;
using chain::signed_transaction;
using chain::transaction_trace_ptr;

template <typename T>
struct big_vector_wrapper {
   T obj;
};

struct augmented_transaction_trace {
   transaction_trace_ptr                trace;
   chain::packed_transaction_ptr        packed_trx;

   augmented_transaction_trace()                                   = default;
   augmented_transaction_trace(const augmented_transaction_trace&) = default;
   augmented_transaction_trace(augmented_transaction_trace&&)      = default;

   augmented_transaction_trace(const transaction_trace_ptr& trace)
       : trace{trace} {}

   augmented_transaction_trace(const transaction_trace_ptr& trace,
                               const chain::packed_transaction_ptr& packed_trx)
       : trace{trace}
       , packed_trx{packed_trx} {}

   augmented_transaction_trace& operator=(const augmented_transaction_trace&) = default;
   augmented_transaction_trace& operator=(augmented_transaction_trace&&) = default;
};

struct table_delta {
   fc::unsigned_int                                                       struct_version = 0;
   std::string                                                            name{};
   state_history::big_vector_wrapper<std::vector<std::pair<bool, bytes>>> rows{};
};

struct block_position {
   uint32_t      block_num = 0;
   block_id_type block_id  = {};
};

struct get_status_request_v0 {};

struct get_status_result_v0 {
   block_position head                    = {};
   block_position last_irreversible       = {};
   uint32_t       trace_begin_block       = 0;
   uint32_t       trace_end_block         = 0;
   uint32_t       chain_state_begin_block = 0;
   uint32_t       chain_state_end_block   = 0;
   fc::sha256     chain_id                = {};
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

} // namespace state_history
} // namespace eosio

// clang-format off
FC_REFLECT(eosio::state_history::table_delta, (struct_version)(name)(rows));
FC_REFLECT(eosio::state_history::block_position, (block_num)(block_id));
FC_REFLECT_EMPTY(eosio::state_history::get_status_request_v0);
FC_REFLECT(eosio::state_history::get_status_result_v0, (head)(last_irreversible)(trace_begin_block)(trace_end_block)(chain_state_begin_block)(chain_state_end_block)(chain_id));
FC_REFLECT(eosio::state_history::get_blocks_request_v0, (start_block_num)(end_block_num)(max_messages_in_flight)(have_positions)(irreversible_only)(fetch_block)(fetch_traces)(fetch_deltas));
FC_REFLECT(eosio::state_history::get_blocks_ack_request_v0, (num_messages));
// clang-format on
