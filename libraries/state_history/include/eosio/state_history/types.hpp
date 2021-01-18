#pragma once

#include <eosio/chain/trace.hpp>
#include <variant>

namespace eosio {
namespace state_history {

using chain::block_id_type;
using chain::bytes;
using chain::extensions_type;
using chain::signature_type;
using chain::signed_transaction;
using chain::transaction_trace_ptr;

using compression_type = chain::packed_transaction::prunable_data_type::compression_type;

template <typename T>
struct big_vector_wrapper {
   T obj;
};

template <typename ST>
inline void pack_varuint64(ST& ds, uint64_t val) {
   do {
      uint8_t b = uint8_t(val) & 0x7f;
      val >>= 7;
      b |= ((val > 0) << 7);
      ds.write((char*)&b, 1);
   } while (val);
}

template <typename ST>
void pack_big_bytes(ST& ds, const eosio::chain::bytes& v) {
   pack_varuint64(ds, v.size());
   if (v.size())
      ds.write(&v.front(), v.size());
}

template <typename ST>
void pack_big_bytes(ST& ds, const std::optional<eosio::chain::bytes>& v) {
   fc::raw::pack(ds, v.has_value());
   if (v)
      pack_big_bytes(ds, *v);
}

template <typename T>
class opaque {
   std::vector<char> data;

 public:
   opaque() = default;
   explicit opaque(std::vector<char>&& serialized)
       : data(std::move(serialized)) {}
   opaque(const T& value) { data = fc::raw::pack(value); }

   opaque& operator=(const T& value) {
      data = fc::raw::pack(value);
      return *this;
   }
   opaque& operator=(std::vector<char>&& serialized) {
      data = std::move(serialized);
      return *this;
   }

   template <typename ST>
   void pack_to(ST& ds) const {
      // we need to pack as big vector because it can be used to hold the state delta object
      // which would be as large as the eos snapshot when the nodeos restarted from a snapshot.
      pack_big_bytes(ds, this->data);
   }
};

struct augmented_transaction_trace {
   transaction_trace_ptr         trace;
   chain::packed_transaction_ptr packed_trx;

   augmented_transaction_trace()                                   = default;
   augmented_transaction_trace(const augmented_transaction_trace&) = default;
   augmented_transaction_trace(augmented_transaction_trace&&)      = default;

   augmented_transaction_trace(const transaction_trace_ptr& trace)
       : trace{trace} {}

   augmented_transaction_trace(const transaction_trace_ptr& trace, const chain::packed_transaction_ptr& packed_trx)
       : trace{trace}
       , packed_trx{packed_trx} {}

   augmented_transaction_trace& operator=(const augmented_transaction_trace&) = default;
   augmented_transaction_trace& operator=(augmented_transaction_trace&&) = default;
};

struct table_delta {
   fc::unsigned_int                                                          struct_version = 1;
   std::string                                                               name{};
   state_history::big_vector_wrapper<std::vector<std::pair<uint8_t, bytes>>> rows{};
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
   block_position                head;
   block_position                last_irreversible;
   std::optional<block_position> this_block;
   std::optional<block_position> prev_block;
   std::optional<bytes>          block;
   std::optional<bytes>          traces;
   std::optional<bytes>          deltas;
};

using state_request = std::variant<get_status_request_v0, get_blocks_request_v0, get_blocks_ack_request_v0>;

struct account_auth_sequence {
   uint64_t account  = {};
   uint64_t sequence = {};
};

struct account_delta {
   uint64_t account = {};
   uint64_t delta   = {};
};

struct permission_level {
   uint64_t actor      = {};
   uint64_t permission = {};
};

struct action_receipt_v0 {
   uint64_t                           receiver        = {};
   eosio::chain::digest_type          act_digest      = {};
   uint64_t                           global_sequence = {};
   uint64_t                           recv_sequence   = {};
   std::vector<account_auth_sequence> auth_sequence   = {};
   fc::unsigned_int                   code_sequence   = {};
   fc::unsigned_int                   abi_sequence    = {};
};

using action_receipt = std::variant<action_receipt_v0>;

struct action {
   uint64_t                      account       = {};
   uint64_t                      name          = {};
   std::vector<permission_level> authorization = {};
   bytes                         data          = {};
};

struct action_trace_v0 {
   fc::unsigned_int              action_ordinal         = {};
   fc::unsigned_int              creator_action_ordinal = {};
   std::optional<action_receipt> receipt                = {};
   uint64_t                      receiver               = {};
   action                        act                    = {};
   bool                          context_free           = {};
   int64_t                       elapsed                = {};
   std::string                   console                = {};
   std::vector<account_delta>    account_ram_deltas     = {};
   std::optional<std::string>    except                 = {};
   std::optional<uint64_t>       error_code             = {};
};

struct action_trace_v1 {
   fc::unsigned_int              action_ordinal         = {};
   fc::unsigned_int              creator_action_ordinal = {};
   std::optional<action_receipt> receipt                = {};
   uint64_t                      receiver               = {};
   action                        act                    = {};
   bool                          context_free           = {};
   int64_t                       elapsed                = {};
   std::string                   console                = {};
   std::vector<account_delta>    account_ram_deltas     = {};
   std::vector<account_delta>    account_disk_deltas    = {};
   std::optional<std::string>    except                 = {};
   std::optional<uint64_t>       error_code             = {};
   bytes                         return_value           = {};
};

using action_trace = std::variant<action_trace_v0, action_trace_v1>;

struct partial_transaction_v0 {
   eosio::chain::time_point_sec               expiration             = {};
   uint16_t                                   ref_block_num          = {};
   uint32_t                                   ref_block_prefix       = {};
   fc::unsigned_int                           max_net_usage_words    = {};
   uint8_t                                    max_cpu_usage_ms       = {};
   fc::unsigned_int                           delay_sec              = {};
   eosio::chain::extensions_type              transaction_extensions = {};
   std::vector<eosio::chain::signature_type>  signatures             = {};
   std::vector<bytes>                         context_free_data      = {};
};

using prunable_data_type = eosio::chain::packed_transaction::prunable_data_type;
struct partial_transaction_v1 {
   eosio::chain::time_point_sec               expiration             = {};
   uint16_t                                   ref_block_num          = {};
   uint32_t                                   ref_block_prefix       = {};
   fc::unsigned_int                           max_net_usage_words    = {};
   uint8_t                                    max_cpu_usage_ms       = {};
   fc::unsigned_int                           delay_sec              = {};
   eosio::chain::extensions_type              transaction_extensions = {};
   std::optional<prunable_data_type>          prunable_data          = {};
};

using partial_transaction = std::variant<partial_transaction_v0, partial_transaction_v1>;

struct transaction_trace_recurse;

struct transaction_trace_v0 {
   using transaction_trace                        = std::variant<transaction_trace_v0>;
   eosio::chain::digest_type    id                = {};
   uint8_t                      status            = {};
   uint32_t                     cpu_usage_us      = {};
   fc::unsigned_int             net_usage_words   = {};
   int64_t                      elapsed           = {};
   uint64_t                     net_usage         = {};
   bool                         scheduled         = {};
   std::vector<action_trace>    action_traces     = {};
   std::optional<account_delta> account_ram_delta = {};
   std::optional<std::string>   except            = {};
   std::optional<uint64_t>      error_code        = {};

   // semantically, this should be std::optional<transaction_trace>;
   // it is represented as vector because optional cannot be used for incomplete type
   std::vector<transaction_trace_recurse> failed_dtrx_trace = {};
   std::optional<partial_transaction>     partial           = {};
};

using transaction_trace = std::variant<transaction_trace_v0>;
struct transaction_trace_recurse {
   transaction_trace recurse;
};

using optional_signed_block = std::variant<chain::signed_block_v0_ptr, chain::signed_block_ptr>;

struct get_blocks_result_v1 {
   block_position                head;
   block_position                last_irreversible;
   std::optional<block_position> this_block;
   std::optional<block_position> prev_block;
   optional_signed_block         block; // packed as std::optional<fc::static_variant<signed_block_v0, signed_block>>
   opaque<std::vector<transaction_trace>> traces;
   opaque<std::vector<table_delta>>       deltas;
};

using state_result = std::variant<get_status_result_v0, get_blocks_result_v0, get_blocks_result_v1>;

} // namespace state_history
} // namespace eosio

// clang-format off
FC_REFLECT(eosio::state_history::table_delta, (struct_version)(name)(rows));
FC_REFLECT(eosio::state_history::block_position, (block_num)(block_id));
FC_REFLECT_EMPTY(eosio::state_history::get_status_request_v0);
FC_REFLECT(eosio::state_history::get_status_result_v0, (head)(last_irreversible)(trace_begin_block)(trace_end_block)(chain_state_begin_block)(chain_state_end_block)(chain_id));
FC_REFLECT(eosio::state_history::get_blocks_request_v0, (start_block_num)(end_block_num)(max_messages_in_flight)(have_positions)(irreversible_only)(fetch_block)(fetch_traces)(fetch_deltas));
FC_REFLECT(eosio::state_history::get_blocks_ack_request_v0, (num_messages));

FC_REFLECT(eosio::state_history::account_auth_sequence, (account)(sequence));
FC_REFLECT(eosio::state_history::account_delta, (account)(delta));
FC_REFLECT(eosio::state_history::permission_level,(actor)(permission));
FC_REFLECT(eosio::state_history::action_receipt_v0, (receiver)(act_digest)(global_sequence)(recv_sequence)(auth_sequence)(code_sequence)(abi_sequence));
FC_REFLECT(eosio::state_history::action, (account)(name)(authorization)(data));
FC_REFLECT(eosio::state_history::action_trace_v0, (action_ordinal)(creator_action_ordinal)(receipt)(receiver)(act)(context_free)(elapsed)(console)(account_ram_deltas)(except)(error_code));
FC_REFLECT(eosio::state_history::action_trace_v1, (action_ordinal)(creator_action_ordinal)(receipt)(receiver)(act)(context_free)(elapsed)(console)(account_ram_deltas)(account_disk_deltas)(except)(error_code)(return_value));
FC_REFLECT(eosio::state_history::partial_transaction_v0, (expiration)(ref_block_num)(ref_block_prefix)(max_net_usage_words)(max_cpu_usage_ms)(delay_sec)(transaction_extensions)(signatures)(context_free_data));
FC_REFLECT(eosio::state_history::partial_transaction_v1, (expiration)(ref_block_num)(ref_block_prefix)(max_net_usage_words)(max_cpu_usage_ms)(delay_sec)(transaction_extensions)(prunable_data));
FC_REFLECT(eosio::state_history::transaction_trace_v0,(id)(status)(cpu_usage_us)(net_usage_words)(elapsed)(net_usage)(scheduled)(action_traces)(account_ram_delta)(except)(error_code)(failed_dtrx_trace)(partial));
FC_REFLECT(eosio::state_history::transaction_trace_recurse, (recurse));
// clang-format on

namespace fc {
template <typename ST, typename T>
ST& operator<<(ST& strm, const eosio::state_history::opaque<T>& v) {
   v.pack_to(strm);
   return strm;
}
} // namespace fc
