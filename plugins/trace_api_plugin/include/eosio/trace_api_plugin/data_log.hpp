#pragma once
#include <fc/variant.hpp>
#include <eosio/trace_api_plugin/trace.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/protocol_feature_activation.hpp>

namespace eosio { namespace trace_api_plugin {

   using data_log_entry = fc::variant<
      block_trace
   >;

}}
