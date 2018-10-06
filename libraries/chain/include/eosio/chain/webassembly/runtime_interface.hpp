#pragma once
#include <vector>
#include <memory>

namespace eosio { namespace chain {

class apply_context;

class wasm_instantiated_module_interface {
   public:
      virtual void apply(apply_context& context) = 0;

      virtual ~wasm_instantiated_module_interface();
};

class wasm_runtime_interface {
   public:
      virtual std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory) = 0;

      virtual ~wasm_runtime_interface();
};

}}