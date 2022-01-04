#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/wasm_eosio_binary_ops.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_injections {
using namespace IR;
using namespace eosio::chain::wasm_constraints;

std::map<std::vector<uint16_t>, uint32_t> injector_utils::type_slots;
std::map<std::string, uint32_t>           injector_utils::registered_injected;
std::map<uint32_t, uint32_t>              injector_utils::injected_index_mapping;
uint32_t                                  injector_utils::next_injected_index;


void noop_injection_visitor::inject( Module& m ) { /* just pass */ }
void noop_injection_visitor::initializer() { /* just pass */ }

void memories_injection_visitor::inject( Module& m ) {
}
void memories_injection_visitor::initializer() {
}

void data_segments_injection_visitor::inject( Module& m ) {
}
void data_segments_injection_visitor::initializer() {
}

int32_t  call_depth_check_and_insert_checktime::global_idx = -1;
uint32_t instruction_counter::icnt = 0;
uint32_t instruction_counter::tcnt = 0;
uint32_t instruction_counter::bcnt = 0;
std::queue<uint32_t> instruction_counter::fcnts;

int32_t  checktime_injection::idx = 0;
int32_t  checktime_injection::chktm_idx = 0;
std::stack<size_t>                   checktime_block_type::block_stack;
std::stack<size_t>                   checktime_block_type::type_stack;
std::queue<std::vector<size_t>>      checktime_block_type::orderings;
std::queue<std::map<size_t, size_t>> checktime_block_type::bcnt_tables;
size_t  checktime_function_end::fcnt = 0;

}}} // namespace eosio, chain, injectors
