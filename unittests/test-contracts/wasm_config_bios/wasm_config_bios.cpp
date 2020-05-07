#include <eosio/contract.hpp>

extern "C" __attribute__((eosio_wasm_import)) void set_wasm_parameters_packed(const void*, std::size_t);

struct wasm_config {
   std::uint32_t max_mutable_global_bytes;
   std::uint32_t max_table_elements;
   std::uint32_t max_section_elements;
   std::uint32_t max_linear_memory_init;
   std::uint32_t max_func_local_bytes;
   std::uint32_t max_nested_structures;
   std::uint32_t max_symbol_bytes;
   std::uint32_t max_module_bytes;
   std::uint32_t max_code_bytes;
   std::uint32_t max_pages;
   std::uint32_t max_call_depth;
};

struct internal_config {
   uint32_t version;
   wasm_config config;
};

class [[eosio::contract]] wasm_config_bios : public eosio::contract {
 public:
   using contract::contract;
   [[eosio::action]] void setwparams(const wasm_config& cfg) {
      internal_config config{0, cfg};
      set_wasm_parameters_packed(&config, sizeof(config));
   }
};
