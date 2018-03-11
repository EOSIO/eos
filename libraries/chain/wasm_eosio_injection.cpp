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
/*
std::string to_string( ControlStructureImm imm ) {
   return "result type : "+std::to_string(uint32_t(imm.resultType));
}
std::string to_string( BranchImm imm ) {
   return "branch target : "+std::to_string(uint32_t(imm.targetDepth));
}
*/

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

void tables_injection_visitor::inject( Module& m ) {
}
void tables_injection_visitor::initializer() {
}

void globals_injection_visitor::inject( Module& m ) {

}
void globals_injection_visitor::initializer() {
}

uint32_t instruction_counter::icnt = 0;
int32_t  checktime_injector::checktime_idx = -1;

}}} // namespace eosio, chain, injectors
