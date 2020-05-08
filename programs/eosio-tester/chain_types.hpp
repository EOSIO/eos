// TODO: this is a duplicate of a file in CDT

#pragma once

#include <eosio/check.hpp>
#include <eosio/crypto.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/from_bin.hpp>
#include <eosio/name.hpp>
#include <eosio/reflection.hpp>
#include <eosio/stream.hpp>
#include <eosio/time.hpp>
#include <eosio/to_bin.hpp>
#include <eosio/varint.hpp>

namespace chain_types {

struct extension {
   uint16_t            type = {};
   eosio::input_stream data = {};
};

EOSIO_REFLECT(extension, type, data);

enum class transaction_status : uint8_t {
   executed  = 0, // succeed, no error handler executed
   soft_fail = 1, // objectively failed (not executed), error handler executed
   hard_fail = 2, // objectively failed and error handler objectively failed thus no state change
   delayed   = 3, // transaction delayed/deferred/scheduled for future execution
   expired   = 4, // transaction expired and storage space refunded to user
};

inline std::string to_string(transaction_status status) {
   switch (status) {
      case transaction_status::executed: return "executed";
      case transaction_status::soft_fail: return "soft_fail";
      case transaction_status::hard_fail: return "hard_fail";
      case transaction_status::delayed: return "delayed";
      case transaction_status::expired: return "expired";
   }

   eosio::check(false, "unknown status: " + std::to_string((uint8_t)status));
   return {}; // suppress warning
}

inline transaction_status get_transaction_status(const std::string& s) {
   if (s == "executed")
      return transaction_status::executed;
   if (s == "soft_fail")
      return transaction_status::soft_fail;
   if (s == "hard_fail")
      return transaction_status::hard_fail;
   if (s == "delayed")
      return transaction_status::delayed;
   if (s == "expired")
      return transaction_status::expired;

   eosio::check(false, "unknown status: " + s);
   return {}; // suppress warning
}

struct permission_level {
   eosio::name actor      = {};
   eosio::name permission = {};
};

EOSIO_REFLECT(permission_level, actor, permission);

struct account_auth_sequence {
   eosio::name account  = {};
   uint64_t    sequence = {};
};

EOSIO_REFLECT(account_auth_sequence, account, sequence);
EOSIO_COMPARE(account_auth_sequence);

struct account_delta {
   eosio::name account = {};
   int64_t     delta   = {};
};

EOSIO_REFLECT(account_delta, account, delta);
EOSIO_COMPARE(account_delta);

struct action_receipt_v0 {
   eosio::name                        receiver        = {};
   eosio::checksum256                 act_digest      = {};
   uint64_t                           global_sequence = {};
   uint64_t                           recv_sequence   = {};
   std::vector<account_auth_sequence> auth_sequence   = {};
   eosio::varuint32                   code_sequence   = {};
   eosio::varuint32                   abi_sequence    = {};
};

EOSIO_REFLECT(action_receipt_v0, receiver, act_digest, global_sequence, recv_sequence, auth_sequence, code_sequence,
              abi_sequence);

using action_receipt = std::variant<action_receipt_v0>;

struct action {
   eosio::name                   account       = {};
   eosio::name                   name          = {};
   std::vector<permission_level> authorization = {};
   eosio::input_stream           data          = {};
};

EOSIO_REFLECT(action, account, name, authorization, data);

struct action_trace_v0 {
   eosio::varuint32              action_ordinal         = {};
   eosio::varuint32              creator_action_ordinal = {};
   std::optional<action_receipt> receipt                = {};
   eosio::name                   receiver               = {};
   action                        act                    = {};
   bool                          context_free           = {};
   int64_t                       elapsed                = {};
   std::string                   console                = {};
   std::vector<account_delta>    account_ram_deltas     = {};
   std::optional<std::string>    except                 = {};
   std::optional<uint64_t>       error_code             = {};
};

EOSIO_REFLECT(action_trace_v0, action_ordinal, creator_action_ordinal, receipt, receiver, act, context_free, elapsed,
              console, account_ram_deltas, except, error_code);

struct action_trace_v1 {
   eosio::varuint32              action_ordinal         = {};
   eosio::varuint32              creator_action_ordinal = {};
   std::optional<action_receipt> receipt                = {};
   eosio::name                   receiver               = {};
   action                        act                    = {};
   bool                          context_free           = {};
   int64_t                       elapsed                = {};
   std::string                   console                = {};
   std::vector<account_delta>    account_ram_deltas     = {};
   std::vector<account_delta>    account_disk_deltas    = {};
   std::optional<std::string>    except                 = {};
   std::optional<uint64_t>       error_code             = {};
   eosio::input_stream           return_value           = {};
};

EOSIO_REFLECT(action_trace_v1, action_ordinal, creator_action_ordinal, receipt, receiver, act, context_free, elapsed,
              console, account_ram_deltas, account_disk_deltas, except, error_code, return_value)

using action_trace = std::variant<action_trace_v0, action_trace_v1>;

struct partial_transaction_v0 {
   eosio::time_point_sec            expiration             = {};
   uint16_t                         ref_block_num          = {};
   uint32_t                         ref_block_prefix       = {};
   eosio::varuint32                 max_net_usage_words    = {};
   uint8_t                          max_cpu_usage_ms       = {};
   eosio::varuint32                 delay_sec              = {};
   std::vector<extension>           transaction_extensions = {};
   std::vector<eosio::signature>    signatures             = {};
   std::vector<eosio::input_stream> context_free_data      = {};
};

EOSIO_REFLECT(partial_transaction_v0, expiration, ref_block_num, ref_block_prefix, max_net_usage_words,
              max_cpu_usage_ms, delay_sec, transaction_extensions, signatures, context_free_data);

using partial_transaction = std::variant<partial_transaction_v0>;

struct transaction_trace_v0;
using transaction_trace = std::variant<transaction_trace_v0>;

struct transaction_trace_v0 {
   eosio::checksum256                 id                  = {};
   transaction_status                 status              = {};
   uint32_t                           cpu_usage_us        = {};
   eosio::varuint32                   net_usage_words     = {};
   int64_t                            elapsed             = {};
   uint64_t                           net_usage           = {};
   bool                               scheduled           = {};
   std::vector<action_trace>          action_traces       = {};
   std::optional<account_delta>       account_ram_delta   = {};
   std::optional<std::string>         except              = {};
   std::optional<uint64_t>            error_code          = {};
   std::vector<transaction_trace>     failed_dtrx_trace   = {};
   std::optional<partial_transaction> reserved_do_not_use = {};
};

EOSIO_REFLECT(transaction_trace_v0, id, status, cpu_usage_us, net_usage_words, elapsed, net_usage, scheduled,
              action_traces, account_ram_delta, except, error_code, failed_dtrx_trace, reserved_do_not_use);

struct producer_key {
   eosio::name       producer_name     = {};
   eosio::public_key block_signing_key = {};
};

EOSIO_REFLECT(producer_key, producer_name, block_signing_key);

struct producer_schedule {
   uint32_t                  version   = {};
   std::vector<producer_key> producers = {};
};

EOSIO_REFLECT(producer_schedule, version, producers);

struct transaction_receipt_header {
   transaction_status status          = {};
   uint32_t           cpu_usage_us    = {};
   eosio::varuint32   net_usage_words = {};
};

EOSIO_REFLECT(transaction_receipt_header, status, cpu_usage_us, net_usage_words);

struct packed_transaction {
   std::vector<eosio::signature> signatures               = {};
   uint8_t                       compression              = {};
   eosio::input_stream           packed_context_free_data = {};
   eosio::input_stream           packed_trx               = {};
};

EOSIO_REFLECT(packed_transaction, signatures, compression, packed_context_free_data, packed_trx);

using transaction_variant = std::variant<eosio::checksum256, packed_transaction>;

struct transaction_receipt : transaction_receipt_header {
   transaction_variant trx = {};
};

EOSIO_REFLECT(transaction_receipt, base transaction_receipt_header, trx);

struct block_header {
   eosio::block_timestamp           timestamp;
   eosio::name                      producer          = {};
   uint16_t                         confirmed         = {};
   eosio::checksum256               previous          = {};
   eosio::checksum256               transaction_mroot = {};
   eosio::checksum256               action_mroot      = {};
   uint32_t                         schedule_version  = {};
   std::optional<producer_schedule> new_producers     = {};
   std::vector<extension>           header_extensions = {};
};

EOSIO_REFLECT(block_header, timestamp, producer, confirmed, previous, transaction_mroot, action_mroot, schedule_version,
              new_producers, header_extensions)

struct signed_block_header : block_header {
   eosio::signature producer_signature = {};
};

EOSIO_REFLECT(signed_block_header, base block_header, producer_signature);

struct signed_block : signed_block_header {
   std::vector<transaction_receipt> transactions     = {};
   std::vector<extension>           block_extensions = {};
};

EOSIO_REFLECT(signed_block, base signed_block_header, transactions, block_extensions);

struct block_info {
   uint32_t               block_num = {};
   eosio::checksum256     block_id  = {};
   eosio::block_timestamp timestamp;
};

EOSIO_REFLECT(block_info, block_num, block_id, timestamp);

} // namespace chain_types
