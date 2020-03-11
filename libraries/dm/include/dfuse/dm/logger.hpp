#include <fc/log/logger.hpp>

#define dmlog( LOGGER, FORMAT, ... ) \
  (LOGGER)->log( FC_LOG_MESSAGE( debug, FORMAT, __VA_ARGS__ ) )