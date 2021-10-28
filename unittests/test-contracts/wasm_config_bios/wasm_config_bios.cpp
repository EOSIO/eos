#include <eosio/contract.hpp>

extern "C" __attribute__((eosio_wasm_import)) void set_wasm_parameters_packed(const void*, std::size_t);
extern "C" __attribute__((eosio_wasm_import)) uint32_t get_wasm_parameters_packed(void*, std::size_t, uint32_t);
#ifdef USE_EOSIO_CDT_1_7_X
extern "C" __attribute__((eosio_wasm_import)) uint32_t read_action_data( void* msg, uint32_t len );
extern "C" __attribute__((eosio_wasm_import))    uint32_t action_data_size();
#endif

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

   [[eosio::action]] void getwparams(const wasm_config& cfg) {
      uint32_t buffer[12];

      int sz = get_wasm_parameters_packed(nullptr, 0, 0);
      eosio::check(sz == 48, "wrong wasm parameters size");

      std::fill_n(buffer, sizeof(buffer)/sizeof(buffer[0]), 0xFFFFFFFFu);

      get_wasm_parameters_packed(buffer, sizeof(buffer), 0);

      eosio::check(buffer[0] == 0, "wrong version");
      eosio::check(cfg.max_mutable_global_bytes == buffer[1], "wrong max_mutable_global_bytes");
      eosio::check(cfg.max_table_elements == buffer[2], "wrong max_table_elements");
      eosio::check(cfg.max_section_elements == buffer[3], "wrong max_section_elements");
      eosio::check(cfg.max_linear_memory_init == buffer[4], "wrong max_linear_memory_init");
      eosio::check(cfg.max_func_local_bytes == buffer[5], "wrong max_func_local_bytes");
      eosio::check(cfg.max_nested_structures == buffer[6], "wrong max_nested_structures");
      eosio::check(cfg.max_symbol_bytes == buffer[7], "wrong max_symbol_bytes");
      eosio::check(cfg.max_module_bytes == buffer[8], "wrong max_module_bytes");
      eosio::check(cfg.max_code_bytes == buffer[9], "wrong max_code_bytes");
      eosio::check(cfg.max_pages == buffer[10], "wrong max_pages");
      eosio::check(cfg.max_call_depth == buffer[11], "wrong max_call_depth");
   }
};
