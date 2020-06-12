#pragma once

#include <b1/rodeos/callbacks/action.hpp>
#include <b1/rodeos/callbacks/console.hpp>
#include <b1/rodeos/callbacks/kv.hpp>
#include <b1/rodeos/callbacks/query.hpp>
#include <eosio/ship_protocol.hpp>

namespace b1::rodeos::wasm_ql {

class backend_cache;

struct shared_state {
   uint32_t                                max_console_size = {};
   uint32_t                                wasm_cache_size  = {};
   uint64_t                                max_exec_time_ms = {};
   std::string                             contract_dir     = {};
   std::shared_ptr<wasm_ql::backend_cache> backend_cache    = {};
   std::shared_ptr<chain_kv::database>     db;

   shared_state(std::shared_ptr<chain_kv::database> db);
   shared_state(const shared_state&) = delete;
   ~shared_state();

   shared_state& operator=(const shared_state&) = delete;
};

struct thread_state : action_state, console_state, query_state {
   std::shared_ptr<const shared_state> shared = {};
   eosio::vm::wasm_allocator           wa     = {};
};

class thread_state_cache : public std::enable_shared_from_this<thread_state_cache> {
 private:
   std::mutex                                   mutex;
   std::shared_ptr<const wasm_ql::shared_state> shared_state;
   std::vector<std::unique_ptr<thread_state>>   states;
   uint64_t                                     num_created = 0;

 public:
   thread_state_cache(const std::shared_ptr<const wasm_ql::shared_state>& shared_state) : shared_state(shared_state) {}

   class handle {
      friend thread_state_cache;

    private:
      std::shared_ptr<thread_state_cache> cache;
      std::unique_ptr<thread_state>       state;

      handle(std::shared_ptr<thread_state_cache> cache, std::unique_ptr<thread_state> state)
          : cache{ std::move(cache) }, state{ std::move(state) } {}

    public:
      handle(const handle&) = delete;
      handle(handle&&)      = default;

      ~handle() {
         if (cache && state)
            cache->store_state(std::move(state));
      }

      handle& operator=(handle&&) = default;

      thread_state& operator*() { return *state; }
      thread_state* operator->() { return state.get(); }
   };

   handle get_state() {
      std::lock_guard<std::mutex> lock{ mutex };
      if (states.empty()) {
         try {
            auto result    = std::make_unique<thread_state>();
            result->shared = shared_state;
            ++num_created;
            return { shared_from_this(), std::move(result) };
         } catch (const eosio::vm::exception& e) {
            elog("vm::exception creating thread_state: ${w}: ${d}", ("w", e.what())("d", e.detail()));
            elog("number of thread_states created so far: ${n}", ("n", num_created));
            throw std::runtime_error(std::string("creating thread_state: ") + e.what() + ": " + e.detail());
         } catch (const std::exception& e) {
            elog("std::exception creating thread_state: ${w}", ("w", e.what()));
            elog("number of thread_states created so far: ${n}", ("n", num_created));
            throw std::runtime_error(std::string("creating thread_state: ") + e.what());
         }
      }
      auto result = std::move(states.back());
      states.pop_back();
      return { shared_from_this(), std::move(result) };
   }

   void preallocate(unsigned n) {
      std::vector<handle> handles;
      for (unsigned i = 0; i < n; ++i) //
         handles.push_back(get_state());
   }

 private:
   void store_state(std::unique_ptr<thread_state> state) {
      std::lock_guard<std::mutex> lock{ mutex };
      states.push_back(std::move(state));
   }
};

const std::vector<char>& query_get_info(wasm_ql::thread_state&   thread_state,
                                        const std::vector<char>& contract_kv_prefix);
const std::vector<char>& query_get_block(wasm_ql::thread_state&   thread_state,
                                         const std::vector<char>& contract_kv_prefix, std::string_view body);
const std::vector<char>& query_get_abi(wasm_ql::thread_state& thread_state, const std::vector<char>& contract_kv_prefix,
                                       std::string_view body);
const std::vector<char>& query_get_required_keys(wasm_ql::thread_state& thread_state, std::string_view body);
const std::vector<char>& query_send_transaction(wasm_ql::thread_state&   thread_state,
                                                const std::vector<char>& contract_kv_prefix, std::string_view body,
                                                bool return_trace_on_except);
eosio::ship_protocol::transaction_trace_v0
query_send_transaction(wasm_ql::thread_state& thread_state, const std::vector<char>& contract_kv_prefix,
                       const eosio::ship_protocol::packed_transaction& trx, const rocksdb::Snapshot* snapshot,
                       std::vector<std::vector<char>>& memory, bool return_trace_on_except);

} // namespace b1::rodeos::wasm_ql
