#include "rdb_stdout_logger.hpp"
#include <stdio.h>
namespace b1 {
rdb_stdout_logger::rdb_stdout_logger(const rocksdb::InfoLogLevel log_level)
      : Logger(log_level) {}

void rdb_stdout_logger::Logv(const char* format, va_list ap) {
    vprintf(format, ap);
    printf("\n");
  }

}