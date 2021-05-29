#pragma once
#include <rocksdb/env.h>

namespace b1 {

struct rdb_stdout_logger : public rocksdb::Logger {
 public:
   explicit rdb_stdout_logger(const rocksdb::InfoLogLevel log_level = rocksdb::InfoLogLevel::INFO_LEVEL);

   // Brings overloaded Logv()s into scope so they're not hidden when we override
   // a subset of them.
   using rocksdb::Logger::Logv;

   virtual void Logv(const char* format, va_list ap) override;
};
}