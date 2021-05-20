#pragma once

#include <b1/rodeos/callbacks/basic.hpp>
#include <b1/rodeos/callbacks/chaindb.hpp>
#include <b1/rodeos/callbacks/compiler_builtins.hpp>
#include <b1/rodeos/callbacks/console.hpp>
#include <b1/rodeos/callbacks/filter.hpp>
#include <b1/rodeos/callbacks/memory.hpp>
#include <b1/rodeos/callbacks/unimplemented.hpp>
#include <b1/rodeos/callbacks/unimplemented_filter.hpp>

// todo: configure limits
// todo: timeout
namespace b1::rodeos::filter {

struct callbacks;
using rhf_t     = registered_host_functions<callbacks>;
using backend_t = eosio::vm::backend<rhf_t, eosio::vm::jit>;

struct filter_state : b1::rodeos::data_state<backend_t>, b1::rodeos::console_state, b1::rodeos::filter_callback_state {
   eosio::vm::wasm_allocator wa = {};
};

struct callbacks : b1::rodeos::chaindb_callbacks<callbacks>,
                   b1::rodeos::compiler_builtins_callbacks<callbacks>,
                   b1::rodeos::console_callbacks<callbacks>,
                   b1::rodeos::context_free_system_callbacks<callbacks>,
                   b1::rodeos::data_callbacks<callbacks>,
                   b1::rodeos::db_callbacks<callbacks>,
                   b1::rodeos::filter_callbacks<callbacks>,
                   b1::rodeos::memory_callbacks<callbacks>,
                   b1::rodeos::unimplemented_callbacks<callbacks>,
                   b1::rodeos::unimplemented_filter_callbacks<callbacks> {
   filter::filter_state&      filter_state;
   b1::rodeos::chaindb_state& chaindb_state;
   b1::rodeos::db_view_state& db_view_state;

   callbacks(filter::filter_state& filter_state, b1::rodeos::chaindb_state& chaindb_state,
             b1::rodeos::db_view_state& db_view_state)
       : filter_state{ filter_state }, chaindb_state{ chaindb_state }, db_view_state{ db_view_state } {}

   auto& get_state() { return filter_state; }
   auto& get_filter_callback_state() { return filter_state; }
   auto& get_chaindb_state() { return chaindb_state; }
   auto& get_db_view_state() { return db_view_state; }
};

inline void register_callbacks() {
   b1::rodeos::chaindb_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::compiler_builtins_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::console_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::context_free_system_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::data_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::db_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::filter_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::memory_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::unimplemented_callbacks<callbacks>::register_callbacks<rhf_t>();
   b1::rodeos::unimplemented_filter_callbacks<callbacks>::register_callbacks<rhf_t>();
}

} // namespace b1::rodeos::filter
