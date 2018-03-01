#include <eosio/chain/wasm_binary_ops.hpp>

namespace eosio { namespace chain { namespace wasm_ops {
std::unordered_map<uint8_t, wasm_op_generator> = {
   { UNREACHABLE, []( std::vector<uint8_t>& vec, size_t index ){ return std::make_shared<unreachable

}}} // namespace eosio, chain, wasm_ops
