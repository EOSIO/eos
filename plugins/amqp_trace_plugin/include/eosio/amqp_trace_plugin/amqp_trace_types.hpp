#include <eosio/ship_protocol.hpp>
#include <eosio/to_bin.hpp>
#include <variant>

namespace eosio {

// publish message types
struct transaction_trace_exception {
   std::int64_t error_code; ///< fc::exception code()
   std::string error_message;
};

using transaction_trace_msg = std::variant<eosio::transaction_trace_exception, eosio::ship_protocol::transaction_trace>;

EOSIO_REFLECT( eosio::transaction_trace_exception, error_code, error_message );

} // namespace eosio

