#include <b1/rodeos/embedded_rodeos.h>
#include <b1/rodeos/rodeos.hpp>
#include <fc/scoped_exit.hpp>

struct rodeos_error_s {
   const char* msg = "no error";
   std::string buffer;

   bool set(const char* m) {
      try {
         buffer = m;
         msg    = buffer.c_str();
      } catch (...) { msg = "error storing error message"; }
      return false;
   }
};

struct rodeos_context_s : b1::rodeos::rodeos_context {};

struct rodeos_db_partition_s {
   std::shared_ptr<b1::rodeos::rodeos_db_partition> obj;
};

struct rodeos_db_snapshot_s : b1::rodeos::rodeos_db_snapshot {
   using rodeos_db_snapshot::rodeos_db_snapshot;
};

struct rodeos_filter_s : b1::rodeos::rodeos_filter {
   using rodeos_filter::rodeos_filter;
};

struct rodeos_query_handler_s : b1::rodeos::rodeos_query_handler {
   using rodeos_query_handler::rodeos_query_handler;
};

extern "C" rodeos_error* rodeos_create_error() {
   try {
      return std::make_unique<rodeos_error>().release();
   } catch (...) { return nullptr; }
}

extern "C" void rodeos_destroy_error(rodeos_error* error) { std::unique_ptr<rodeos_error>{ error }; }

extern "C" const char* rodeos_get_error(rodeos_error* error) {
   if (!error)
      return "error is null";
   return error->msg;
}

template <typename T, typename F>
auto handle_exceptions(rodeos_error* error, T errval, F f) noexcept -> decltype(f()) {
   if (!error)
      return errval;
   try {
      return f();
   } catch (std::exception& e) {
      error->set(e.what());
      return errval;
   } catch (...) {
      error->set("unknown exception");
      return errval;
   }
}

extern "C" rodeos_context* rodeos_create() {
   try {
      return std::make_unique<rodeos_context>().release();
   } catch (...) { return nullptr; }
}

extern "C" void rodeos_destroy(rodeos_context* context) { std::unique_ptr<rodeos_context>{ context }; }

extern "C" rodeos_bool rodeos_open_db(rodeos_error* error, rodeos_context* context, const char* path,
                                      rodeos_bool create_if_missing, int num_threads, int max_open_files) {
   return handle_exceptions(error, false, [&] {
      if (!context)
         return error->set("context is null");
      if (!path)
         return error->set("path is null");
      if (context->db)
         return error->set("a database is already open on this context");
      context->db = std::make_shared<b1::chain_kv::database>(
            path, create_if_missing, num_threads ? std::make_optional(num_threads) : std::nullopt,
            max_open_files ? std::make_optional(max_open_files) : std::nullopt);
      return true;
   });
}

extern "C" rodeos_db_partition* rodeos_create_partition(rodeos_error* error, rodeos_context* context,
                                                        const char* prefix, uint32_t prefix_size) {
   return handle_exceptions(error, nullptr, [&]() -> rodeos_db_partition* {
      if (!context)
         return error->set("context is null"), nullptr;
      if (!prefix)
         return error->set("prefix is null"), nullptr;
      if (!context->db)
         return error->set("database wasn't opened"), nullptr;
      auto p = std::make_unique<rodeos_db_partition>();
      p->obj = std::make_shared<b1::rodeos::rodeos_db_partition>(context->db,
                                                                 std::vector<char>{ prefix, prefix + prefix_size });
      return p.release();
   });
}

extern "C" void rodeos_destroy_partition(rodeos_db_partition* partition) {
   std::unique_ptr<rodeos_db_partition>{ partition };
}

extern "C" rodeos_db_snapshot* rodeos_create_snapshot(rodeos_error* error, rodeos_db_partition* partition,
                                                      rodeos_bool persistent) {
   return handle_exceptions(error, nullptr, [&]() -> rodeos_db_snapshot* {
      if (!partition)
         return error->set("partition is null"), nullptr;
      return std::make_unique<rodeos_db_snapshot>(partition->obj, persistent).release();
   });
}

extern "C" void rodeos_destroy_snapshot(rodeos_db_snapshot* snapshot) {
   std::unique_ptr<rodeos_db_snapshot>{ snapshot };
}

extern "C" rodeos_bool rodeos_refresh_snapshot(rodeos_error* error, rodeos_db_snapshot* snapshot) {
   return handle_exceptions(error, false, [&]() {
      if (!snapshot)
         return error->set("snapshot is null");
      snapshot->refresh();
      return true;
   });
}

template <typename F>
void with_result(const char* data, uint64_t size, F f) {
   eosio::input_stream          bin{ data, data + size };
   eosio::ship_protocol::result result;
   from_bin(result, bin);
   auto* result_v0 = std::get_if<eosio::ship_protocol::get_blocks_result_v0>(&result);
   if (result_v0)
      return f(*result_v0);

   auto* result_v1 = std::get_if<eosio::ship_protocol::get_blocks_result_v1>(&result);
   if (result_v1)
      return f(*result_v1);

   throw std::runtime_error("expected a get_blocks_result_v0 or get_blocks_result_v1");
}

extern "C" rodeos_bool rodeos_start_block(rodeos_error* error, rodeos_db_snapshot* snapshot, const char* data,
                                          uint64_t size) {
   return handle_exceptions(error, false, [&]() {
      if (!snapshot)
         return error->set("snapshot is null");
      with_result(data, size, [&](auto& result) { snapshot->start_block(result); });
      return true;
   });
}

extern "C" rodeos_bool rodeos_end_block(rodeos_error* error, rodeos_db_snapshot* snapshot, const char* data,
                                        uint64_t size, bool force_write) {
   return handle_exceptions(error, false, [&]() {
      if (!snapshot)
         return error->set("snapshot is null");
      with_result(data, size, [&](auto& result) { snapshot->end_block(result, force_write); });
      return true;
   });
}

extern "C" rodeos_bool rodeos_write_block_info(rodeos_error* error, rodeos_db_snapshot* snapshot, const char* data,
                                               uint64_t size) {
   return handle_exceptions(error, false, [&]() {
      if (!snapshot)
         return error->set("snapshot is null");
      with_result(data, size, [&](auto& result) { snapshot->write_block_info(result); });
      return true;
   });
}

extern "C" rodeos_bool rodeos_write_deltas(rodeos_error* error, rodeos_db_snapshot* snapshot, const char* data,
                                           uint64_t size, rodeos_bool (*shutdown)(void*), void* shutdown_arg) {
   return handle_exceptions(error, false, [&]() {
      if (!snapshot)
         return error->set("snapshot is null");
      with_result(data, size, [&](auto& result) {
         snapshot->write_deltas(result, [=]() -> bool {
            if (shutdown)
               return shutdown(shutdown_arg);
            else
               return false;
         });
      });
      return true;
   });
}

extern "C" rodeos_filter* rodeos_create_filter(rodeos_error* error, uint64_t name, const char* wasm_filename) {
   return handle_exceptions(error, nullptr, [&]() -> rodeos_filter* { //
      return std::make_unique<rodeos_filter>(eosio::name{ name }, wasm_filename).release();
   });
}

extern "C" void rodeos_destroy_filter(rodeos_filter* filter) { std::unique_ptr<rodeos_filter>{ filter }; }

extern "C" rodeos_bool rodeos_run_filter(rodeos_error* error, rodeos_db_snapshot* snapshot, rodeos_filter* filter,
                                         const char* data, uint64_t size,
                                         rodeos_bool (*push_data)(void* arg, const char* data, uint64_t size),
                                         void* push_data_arg) {
   return handle_exceptions(error, false, [&]() {
      if (!snapshot)
         return error->set("snapshot is null");
      if (!filter)
         return error->set("filter is null");
      with_result(data, size, [&](auto& result) {
         filter->process(*snapshot, result, { data, data + size }, [&](const char* data, uint64_t size) {
            if (push_data && !push_data(push_data_arg, data, size))
               throw std::runtime_error("push_data returned false");
         });
      });
      return true;
   });
}

extern "C" rodeos_query_handler* rodeos_create_query_handler(rodeos_error* error, rodeos_db_partition* partition,
                                                             uint32_t max_console_size, uint32_t wasm_cache_size,
                                                             uint64_t max_exec_time_ms, const char* contract_dir) {
   return handle_exceptions(error, nullptr, [&]() -> rodeos_query_handler* {
      if (!partition)
         return error->set("partition is null"), nullptr;
      auto shared_state              = std::make_shared<b1::rodeos::wasm_ql::shared_state>(partition->obj->db);
      shared_state->max_console_size = max_console_size;
      shared_state->wasm_cache_size  = wasm_cache_size;
      shared_state->max_exec_time_ms = max_exec_time_ms;
      shared_state->contract_dir     = contract_dir ? contract_dir : "";
      return std::make_unique<rodeos_query_handler>(partition->obj, shared_state).release();
   });
}

void rodeos_destroy_query_handler(rodeos_query_handler* handler) { std::unique_ptr<rodeos_query_handler>{ handler }; }

rodeos_bool rodeos_query_transaction(rodeos_error* error, rodeos_query_handler* handler, rodeos_db_snapshot* snapshot,
                                     const char* data, uint64_t size, char** result, uint64_t* result_size) {
   return handle_exceptions(error, false, [&]() {
      if (!handler)
         return error->set("handler is null");
      if (!result)
         return error->set("result is null");
      if (!result_size)
         return error->set("result_size is null");
      *result      = nullptr;
      *result_size = 0;

      std::vector<std::vector<char>> memory;
      eosio::input_stream            s{ data, size };
      auto                           trx = eosio::from_bin<eosio::ship_protocol::packed_transaction>(s);

      auto                                    thread_state = handler->state_cache.get_state();
      eosio::ship_protocol::transaction_trace tt;
      if (snapshot->snap) {
         tt = query_send_transaction(*thread_state, snapshot->partition->contract_kv_prefix, trx,
                                     snapshot->snap->snapshot(), memory, true);
      } else {
         tt = query_send_transaction(*thread_state, snapshot->partition->contract_kv_prefix, trx, nullptr, memory,
                                     true);
      }

      handler->state_cache.store_state(std::move(thread_state));

      eosio::size_stream ss;
      eosio::to_bin(tt, ss);
      *result = (char*)malloc(ss.size);
      if (!result)
         throw std::bad_alloc();
      auto                    free_on_except = fc::make_scoped_exit([&] {
         free(*result);
         *result = nullptr;
      });
      eosio::fixed_buf_stream fbs(*result, ss.size);
      to_bin(tt, fbs);
      if (fbs.pos != fbs.end) {
         eosio::check(false, eosio::convert_stream_error(eosio::stream_error::underrun));
      }
      *result_size = ss.size;
      free_on_except.cancel();
      return true;
   });
}

void rodeos_free_result(char* result) {
   if (result)
      free(result);
}
