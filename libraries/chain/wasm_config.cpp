#include <eosio/chain/wasm_config.hpp>
#include <eosio/chain/exceptions.hpp>

using namespace eosio::chain;

void wasm_config::validate() const {
   EOS_ASSERT(max_section_elements >= 4, action_validate_exception, "max_section_elements cannot be less than 4");
   EOS_ASSERT(max_func_local_bytes >= 8, action_validate_exception, "max_func_local_bytes cannot be less than 8");
   EOS_ASSERT(max_nested_structures >= 1, action_validate_exception, "max_nested_structures cannot be less than 1");
   EOS_ASSERT(max_symbol_bytes >= 32, action_validate_exception, "max_symbol_bytes cannot be less than 32");
   EOS_ASSERT(max_module_bytes >= 256, action_validate_exception, "max_module_bytes cannot be less than 256");
   EOS_ASSERT(max_code_bytes >= 32, action_validate_exception, "max_code_bytes cannot be less than 32");
   EOS_ASSERT(max_pages >= 1, action_validate_exception, "max_pages cannot be less than 1");
   EOS_ASSERT(max_call_depth >= 2, action_validate_exception, "max_call_depth cannot be less than 2");
}
