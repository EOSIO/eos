#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat.hpp>
#include "IR/Types.h"

#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/memory.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/executor.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/config.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic_interface.hpp>

#include <boost/hana/equal.hpp>

namespace eosio { namespace chain { namespace webassembly { namespace eosvmoc {

using namespace IR;
using namespace Runtime;
using namespace fc;

using namespace eosio::chain::eosvmoc;

class eosvmoc_instantiated_module;

struct timer : eosvmoc::timer_base {
   timer(apply_context& context) : context(context) {}
   void set_expiration_callback(void (*fn)(void*), void* data) override;
   void checktime() override;
   apply_context& context;
};

inline auto make_code_finder(const chainbase::database& db) {
   return [&db](const digest_type& id, uint8_t vm_version) -> std::string_view {
      auto * p = db.find<code_object,by_code_hash>(boost::make_tuple(id, 0, vm_version));
      if(p) return { p->code.data(), p->code.size() };
      else return {};
   };
}

class eosvmoc_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eosvmoc_runtime(const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db);
      ~eosvmoc_runtime();
      bool inject_module(IR::Module&) override { return false; }
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory,
                                                                             const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) override;

      void immediately_exit_currently_running_module() override;

      friend eosvmoc_instantiated_module;
      eosvmoc::code_cache_sync cc;
      eosvmoc::executor exec;
      eosvmoc::memory mem;
};

struct eos_vm_oc_type_converter : public basic_type_converter<eos_vm_oc_execution_interface> {
   using base_type = basic_type_converter<eos_vm_oc_execution_interface>;
   using base_type::basic_type_converter;
   using base_type::to_wasm;

   vm::wasm_ptr_t to_wasm(void*&& ptr) {
      return convert_native_to_wasm(static_cast<char*>(ptr));
   }

   template<typename T>
   inline decltype(auto) as_value(const vm::native_value& val) const {
      if constexpr (std::is_integral_v<T> && sizeof(T) == 4)
         return static_cast<T>(val.i32);
      else if constexpr (std::is_integral_v<T> && sizeof(T) == 8)
         return static_cast<T>(val.i64);
      else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4)
         return static_cast<T>(val.f32);
      else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8)
         return static_cast<T>(val.f64);
      // No direct pointer support
      else
         return vm::no_match_t{};
   }
};

inline intrinsic_map_t& get_intrinsic_map() {
   static intrinsic_map_t instance;
   return instance;
}

template<auto F, typename Preconditions, typename Name>
void register_eosvm_oc(Name n) {
   constexpr auto fn = create_function<F, webassembly::interface, eos_vm_oc_type_converter, Preconditions>();
   auto& map = get_intrinsic_map();
   map.insert({n.c_str(),
      {
         reinterpret_cast<void*>(fn),
         ::boost::hana::index_if(intrinsic_table, ::boost::hana::equal.to(n)).value()
      }
   });
}

} } } }// eosio::chain::webassembly::eosvmoc
