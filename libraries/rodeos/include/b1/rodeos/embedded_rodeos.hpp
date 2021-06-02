#pragma once

#include <b1/rodeos/embedded_rodeos.h>
#include <new>
#include <stdexcept>

namespace b1::embedded_rodeos {

struct error {
   rodeos_error* obj;

   error() : obj{ rodeos_create_error() } {
      if (!obj)
         throw std::bad_alloc{};
   }

   error(const error&) = delete;

   ~error() { rodeos_destroy_error(obj); }

   error& operator=(const error&) = delete;

   operator rodeos_error*() { return obj; }

   const char* msg() { return rodeos_get_error(obj); }

   template <typename F>
   auto check(F f) -> decltype(f()) {
      auto result = f();
      if (!result)
         throw std::runtime_error(msg());
      return result;
   }
};

struct context {
   struct error    error;
   rodeos_context* obj;

   context() : obj{ rodeos_create() } {
      if (!obj)
         throw std::bad_alloc{};
   }

   context(const context&) = delete;

   ~context() { rodeos_destroy(obj); }

   context& operator=(const context&) = delete;

   operator rodeos_context*() { return obj; }

   void open_db(const char* path, bool create_if_missing, int num_threads = 0, int max_open_files = 0) {
      error.check([&] { return rodeos_open_db(error, obj, path, create_if_missing, num_threads, max_open_files); });
   }
};

struct partition {
   struct error         error;
   rodeos_db_partition* obj;

   partition(rodeos_context* context, const char* prefix, uint32_t prefix_size) {
      obj = error.check([&] { return rodeos_create_partition(error, context, prefix, prefix_size); });
   }

   partition(const partition&) = delete;

   ~partition() { rodeos_destroy_partition(obj); }

   partition& operator=(const partition&) = delete;

   operator rodeos_db_partition*() { return obj; }
};

struct snapshot {
   struct error        error;
   rodeos_db_snapshot* obj;

   snapshot(rodeos_db_partition* partition, bool persistent) {
      obj = error.check([&] { return rodeos_create_snapshot(error, partition, persistent); });
   }

   snapshot(const snapshot&) = delete;

   ~snapshot() { rodeos_destroy_snapshot(obj); }

   snapshot& operator=(const snapshot&) = delete;

   operator rodeos_db_snapshot*() { return obj; }

   void refresh() {
      error.check([&] { return rodeos_refresh_snapshot(error, obj); });
   }

   void start_block(const char* data, uint64_t size) {
      error.check([&] { return rodeos_start_block(error, obj, data, size); });
   }

   void end_block(const char* data, uint64_t size, bool force_write) {
      error.check([&] { return rodeos_end_block(error, obj, data, size, force_write); });
   }

   void write_block_info(const char* data, uint64_t size) {
      error.check([&] { return rodeos_write_block_info(error, obj, data, size); });
   }

   template <typename F>
   void write_deltas(const char* data, uint64_t size, F shutdown) {
      error.check([&] {
         return rodeos_write_deltas(
               error, obj, data, size, [](void* f) -> rodeos_bool { return (*static_cast<F*>(f))(); }, &shutdown);
      });
   }
};

struct filter {
   struct error   error;
   rodeos_filter* obj;

   filter(uint64_t name, const char* wasm_filename) {
      obj = error.check([&] { return rodeos_create_filter(error, name, wasm_filename); });
   }

   filter(const filter&) = delete;

   ~filter() { rodeos_destroy_filter(obj); }

   filter& operator=(const filter&) = delete;

   operator rodeos_filter*() { return obj; }

   void run(rodeos_db_snapshot* snapshot, const char* data, uint64_t size) {
      error.check([&] { return rodeos_run_filter(error, snapshot, obj, data, size, nullptr, nullptr); });
   }

   template <typename F>
   void run(rodeos_db_snapshot* snapshot, const char* data, uint64_t size, F push_data) {
      error.check([&] {
         return rodeos_run_filter(
               error, snapshot, obj, data, size,
               [](void* arg, const char* data, uint64_t size) -> rodeos_bool {
                  try {
                     return (*reinterpret_cast<F*>(arg))(data, size);
                  } catch (...) { return false; }
               },
               &push_data);
      });
   }
};

struct result {
   char*    data = {};
   uint64_t size = {};

   result()              = default;
   result(const result&) = delete;
   result(result&& src) { *this = std::move(src); }
   ~result() { rodeos_free_result(data); }

   result& operator=(const result& src) = delete;

   result& operator=(result&& src) {
      data     = src.data;
      size     = src.size;
      src.data = nullptr;
      src.size = 0;
      return *this;
   }
};

struct query_handler {
   struct error          error;
   rodeos_query_handler* obj;

   query_handler(rodeos_db_partition* partition, uint32_t max_console_size, uint32_t wasm_cache_size,
                 uint64_t max_exec_time_ms, const char* contract_dir) {
      obj = error.check([&] {
         return rodeos_create_query_handler(error, partition, max_console_size, wasm_cache_size, max_exec_time_ms,
                                            contract_dir);
      });
   }

   query_handler(const query_handler&) = delete;

   ~query_handler() { rodeos_destroy_query_handler(obj); }

   query_handler& operator=(const query_handler&) = delete;

   operator rodeos_query_handler*() { return obj; }

   result query_transaction(rodeos_db_snapshot* snapshot, const char* data, uint64_t size) {
      result r;
      error.check([&] { return rodeos_query_transaction(error, obj, snapshot, data, size, &r.data, &r.size); });
      return r;
   }
};

} // namespace b1::embedded_rodeos
