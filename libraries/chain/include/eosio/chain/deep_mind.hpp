#pragma once
#include <fc/string.hpp>
#include <fc/variant_object.hpp>
#include <fc/log/logger_config.hpp>

#define RAM_EVENT_ID( FORMAT, ... ) \
   fc::format_string( FORMAT, fc::mutable_variant_object()__VA_ARGS__ )

namespace eosio { namespace chain {

   fc::logging_config add_deep_mind_logger(fc::logging_config config);

   void set_dmlog_appender_stdout_unbuffered();

} } /// namespace eosio::chain
