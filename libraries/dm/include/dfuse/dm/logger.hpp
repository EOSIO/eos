#include <fc/log/logger.hpp>

#define dmlog( FORMAT, ... ) \
  FC_MULTILINE_MACRO_BEGIN \
   if( (fc::logger::get("deep-mind")).is_enabled( fc::log_level::debug ) ) \
      (fc::logger::get("deep-mind")).log( FC_LOG_MESSAGE( debug, FORMAT, __VA_ARGS__ ) ); \
  FC_MULTILINE_MACRO_END