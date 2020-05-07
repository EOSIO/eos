#pragma once
#include <fc/variant.hpp>
#include <eosio/trace_api/trace.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/protocol_feature_activation.hpp>

namespace eosio { namespace trace_api {

   using data_log_entry = fc::static_variant<
      block_trace_v0,
      block_trace_v1
   >;

}}
