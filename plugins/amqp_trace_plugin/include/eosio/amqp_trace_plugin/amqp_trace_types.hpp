#include <eosio/ship_protocol.hpp>
#include <eosio/to_bin.hpp>
#include <variant>

namespace eosio {

// publish message types
struct transaction_trace_exception {
   std::int64_t error_code; ///< fc::exception code()
   std::string error_message;
};

struct transaction_trace_message {
   std::string block_uuid;
   eosio::ship_protocol::transaction_trace trace;
};

struct block_uuid_message {
   std::string block_uuid;
   eosio::checksum256 block_id;
};

using transaction_trace_msg = std::variant<eosio::transaction_trace_exception, eosio::transaction_trace_message, block_uuid_message>;

EOSIO_REFLECT( eosio::transaction_trace_exception, error_code, error_message );
EOSIO_REFLECT( eosio::transaction_trace_message, block_uuid, trace );
EOSIO_REFLECT( eosio::block_uuid_message, block_uuid,  block_id );

} // namespace eosio

