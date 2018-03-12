#pragma once
#include <vector>
#include <memory>

namespace eosio { namespace chain {

class apply_context;

class wasm_instaniated_module_interface {
   public:
      virtual void apply(apply_context& context) = 0;
      virtual void error(apply_context& context) = 0;

      virtual ~wasm_instaniated_module_interface();
};

class wasm_runtime_interface {
   public:
      virtual std::unique_ptr<wasm_instaniated_module_interface> instaniate_module(const shared_vector<char>& code, std::vector<uint8_t> initial_memory) = 0;

      virtual ~wasm_runtime_interface();
};

}}