#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
//eos-vm includes
#include <eosio/vm/backend.hpp>

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace eosio::vm;

namespace wasm_constraints = eosio::chain::wasm_constraints;

namespace {

  struct checktime_watchdog {
     checktime_watchdog(transaction_checktime_timer& timer) : _timer(timer) {}
     template<typename F>
     struct guard {
        guard(transaction_checktime_timer& timer, F&& func)
           : _timer(timer), _func(static_cast<F&&>(func)) {
           _timer.set_expiration_callback(&callback, this);
           if(_timer.expired) {
              _func(); // it's harmless if _func is invoked twice
           }
        }
        ~guard() {
           _timer.set_expiration_callback(nullptr, nullptr);
        }
        static void callback(void* data) {
           guard* self = static_cast<guard*>(data);
           self->_func();
        }
        transaction_checktime_timer& _timer;
        F _func;
     };
     template<typename F>
     guard<F> scoped_run(F&& func) {
        return guard{_timer, static_cast<F&&>(func)};
     }
     transaction_checktime_timer& _timer;
  };
}

// Used on setcode.  Must not reject anything that WAVM accepts
// For the moment, this runs after WAVM validation, as I am not
// sure that eos-vm will replicate WAVM's parsing exactly.
struct setcode_options {
   static constexpr bool forbid_export_mutable_globals = false;
   static constexpr bool allow_code_after_function_end = true;
   static constexpr bool allow_u32_limits_flags = true;
   static constexpr bool allow_invalid_empty_local_set = true;
   static constexpr bool allow_zero_blocktype = true;
};

void validate(const bytes& code, const allowlisted_intrinsics_type& intrinsics) {
   wasm_code_ptr code_ptr((uint8_t*)code.data(), code.size());
   try {
      eos_vm_null_backend_t<setcode_options> bkend(code_ptr, code.size(), nullptr);
      // check import signatures
       eos_vm_host_functions_t::resolve(bkend.get_module());
      // check that the imports are all currently enabled
      const auto& imports = bkend.get_module().imports;
      for(std::uint32_t i = 0; i < imports.size(); ++i) {
         EOS_ASSERT(std::string_view((char*)imports[i].module_str.raw(), imports[i].module_str.size()) == "env" &&
                    is_intrinsic_allowlisted(intrinsics, std::string_view((char*)imports[i].field_str.raw(), imports[i].field_str.size())),
                    wasm_serialization_error, "${module}.${fn} unresolveable",
                    ("module", std::string((char*)imports[i].module_str.raw(), imports[i].module_str.size()))
                    ("fn", std::string((char*)imports[i].field_str.raw(), imports[i].field_str.size())));
      }
   } catch(vm::exception& e) {
      EOS_THROW(wasm_serialization_error, e.detail());
   }
}

void validate( const bytes& code, const wasm_config& cfg, const allowlisted_intrinsics_type& intrinsics ) {
   EOS_ASSERT(code.size() <= cfg.max_module_bytes, wasm_serialization_error, "Code too large");
   wasm_code_ptr code_ptr((uint8_t*)code.data(), code.size());
   try {
      eos_vm_null_backend_t<wasm_config> bkend(code_ptr, code.size(), nullptr, cfg);
      // check import signatures
      eos_vm_host_functions_t::resolve(bkend.get_module());
      // check that the imports are all currently enabled
      const auto& imports = bkend.get_module().imports;
      for(std::uint32_t i = 0; i < imports.size(); ++i) {
         EOS_ASSERT(std::string_view((char*)imports[i].module_str.raw(), imports[i].module_str.size()) == "env" &&
                    is_intrinsic_allowlisted(intrinsics, std::string_view((char*)imports[i].field_str.raw(), imports[i].field_str.size())),
                    wasm_serialization_error, "${module}.${fn} unresolveable",
                    ("module", std::string((char*)imports[i].module_str.raw(), imports[i].module_str.size()))
                    ("fn", std::string((char*)imports[i].field_str.raw(), imports[i].field_str.size())));
      }
      // check apply
      uint32_t apply_idx = bkend.get_module().get_exported_function("apply");
      EOS_ASSERT(apply_idx < std::numeric_limits<uint32_t>::max(), wasm_serialization_error, "apply not exported");
      const vm::func_type& apply_type = bkend.get_module().get_function_type(apply_idx);
      EOS_ASSERT((apply_type == vm::host_function{{vm::i64, vm::i64, vm::i64}, {}}), wasm_serialization_error, "apply has wrong type");
   } catch(vm::exception& e) {
      EOS_THROW(wasm_serialization_error, e.detail());
   }
}

// Be permissive on apply.
struct apply_options {
   std::uint32_t max_pages = wasm_constraints::maximum_linear_memory/wasm_constraints::wasm_page_size;
   std::uint32_t max_call_depth = wasm_constraints::maximum_call_depth+1;
   static constexpr bool forbid_export_mutable_globals = false;
   static constexpr bool allow_code_after_function_end = false;
   static constexpr bool allow_u32_limits_flags = true;
   static constexpr bool allow_invalid_empty_local_set = true;
   static constexpr bool allow_zero_blocktype = true;
};

template<typename Impl>
class eos_vm_instantiated_module : public wasm_instantiated_module_interface {
   using backend_t = eos_vm_backend_t<Impl>;
   public:

      eos_vm_instantiated_module(eos_vm_runtime<Impl>* runtime, std::unique_ptr<backend_t> mod) :
         _runtime(runtime),
         _instantiated_module(std::move(mod)) {}

      void apply(apply_context& context) override {
         _instantiated_module->set_wasm_allocator(&context.control.get_wasm_allocator());
         _runtime->_bkend = _instantiated_module.get();
         apply_options opts;
         if(context.control.is_builtin_activated(builtin_protocol_feature_t::configurable_wasm_limits)) {
            const wasm_config& config = context.control.get_global_properties().wasm_configuration;
            opts = {config.max_pages, config.max_call_depth};
         }
         auto fn = [&]() {
            eosio::chain::webassembly::interface iface(context);
            _runtime->_bkend->initialize(&iface, opts);
            _runtime->_bkend->call(
                iface, "env", "apply",
                context.get_receiver().to_uint64_t(),
                context.get_action().account.to_uint64_t(),
                context.get_action().name.to_uint64_t());
         };
         try {
            checktime_watchdog wd(context.trx_context.transaction_timer);
            _runtime->_bkend->timed_run(wd, fn);
         } catch(eosio::vm::timeout_exception&) {
            context.trx_context.checktime();
         } catch(eosio::vm::wasm_memory_exception& e) {
            FC_THROW_EXCEPTION(wasm_execution_error, "access violation");
         } catch(eosio::vm::exception& e) {
            FC_THROW_EXCEPTION(wasm_execution_error, "eos-vm system failure");
         }
         _runtime->_bkend = nullptr;
      }

   private:
      eos_vm_runtime<Impl>*            _runtime;
      std::unique_ptr<backend_t> _instantiated_module;
};

template<typename Impl>
eos_vm_runtime<Impl>::eos_vm_runtime() {}

template<typename Impl>
void eos_vm_runtime<Impl>::immediately_exit_currently_running_module() {
   throw wasm_exit{};
}

template<typename Impl>
bool eos_vm_runtime<Impl>::inject_module(IR::Module& module) {
   return false;
}

template<typename Impl>
std::unique_ptr<wasm_instantiated_module_interface> eos_vm_runtime<Impl>::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>,
                                                                                             const digest_type&, const uint8_t&, const uint8_t&) {

   using backend_t = eos_vm_backend_t<Impl>;
   try {
      wasm_code_ptr code((uint8_t*)code_bytes, code_size);
      apply_options options = { .max_pages = 65536,
                                .max_call_depth = 0 };
      std::unique_ptr<backend_t> bkend = std::make_unique<backend_t>(code, code_size, nullptr, options);
      eos_vm_host_functions_t::resolve(bkend->get_module());
      return std::make_unique<eos_vm_instantiated_module<Impl>>(this, std::move(bkend));
   } catch(eosio::vm::exception& e) {
      FC_THROW_EXCEPTION(wasm_execution_error, "Error building eos-vm interp: ${e}", ("e", e.what()));
   }
}

template class eos_vm_runtime<eosio::vm::interpreter>;
template class eos_vm_runtime<eosio::vm::jit>;

}}}}
