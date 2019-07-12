#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat_types.h>

//eos-vm includes
#include <eosio/vm/host_function.hpp>
namespace eosio { namespace vm {
   template <>
   struct registered_host_functions<eosio::chain::apply_context>;
} }
#include <eosio/vm/backend.hpp>

// eosio specific specializations
namespace eosio { namespace vm {
   template <>
   struct reduce_type<chain::name> {
      typedef uint64_t type;
   };

   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc*, T&& val) 
         -> std::enable_if_t<std::is_same_v<i64_const_t, T> && std::is_same_v<chain::name, std::decay_t<S>>, S> {
      return std::move(chain::name{(uint64_t)val.data.ui});
   } 

   // we can clean these up if we go with custom vms
   template <typename T>
   struct reduce_type<eosio::chain::array_ptr<T>> {
      typedef uint32_t type;
   };
   
   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* walloc, T&& val) 
         -> std::enable_if_t<std::is_same_v<i32_const_t, T> && 
         std::is_same_v< eosio::chain::array_ptr<typename S::type>, S> &&
         !std::is_lvalue_reference_v<S> && !std::is_pointer_v<S>, S> {
      using ptr_ty = typename S::type;
      return eosio::chain::array_ptr<ptr_ty>((ptr_ty*)((walloc->template get_base_ptr<char>())+val.data.ui));
   }

   template <typename Ctx>
   struct construct_derived<eosio::chain::transaction_context, Ctx> {
      static auto &value(Ctx& ctx) { return ctx.trx_context; }
   };
   
   template <>
   struct construct_derived<eosio::chain::apply_context, eosio::chain::apply_context> {
      static auto &value(eosio::chain::apply_context& ctx) { return ctx; }
   };

   template <>
   struct reduce_type<eosio::chain::null_terminated_ptr> {
      typedef uint32_t type;
   };
   
   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* walloc, T&& val) 
         -> std::enable_if_t<std::is_same_v<i32_const_t, T> && 
         std::is_same_v< eosio::chain::null_terminated_ptr, S> &&
         !std::is_lvalue_reference_v<S> && !std::is_pointer_v<S>, S> {
      return eosio::chain::null_terminated_ptr((char*)(walloc->template get_base_ptr<uint8_t>()+val.data.ui));
   }

}} // ns eosio::vm

namespace eosio { namespace vm {
   template <typename WAlloc, typename Cls, typename Cls2, auto F, typename R, typename Args, size_t... Is>
   auto create_logging_function(std::index_sequence<Is...>) {
      return std::function<void(Cls*, WAlloc*, operand_stack&)>{ [](Cls* self, WAlloc* walloc, operand_stack& os) {
         size_t i = sizeof...(Is) - 1;
         auto& intrinsic_log = self->control.get_intrinsic_debug_log();
	 /*
         if (intrinsic_log) {
            eosio::chain::digest_type::encoder enc;
            enc.write(walloc->template get_base_ptr<char>(), walloc->get_current_page() * 64 * 1024);
            intrinsic_log->record_intrinsic(
               eosio::chain::calc_arguments_hash(
                  get_value<typename std::tuple_element<Is, Args>::type, Args>(
                        walloc, get_value<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
                                       os.get_back(i - Is)))...),
               enc.result());
         }
	 */
         if constexpr (!std::is_same_v<R, void>) {
            if constexpr (std::is_same_v<Cls2, std::nullptr_t>) {
               R res = std::invoke(F, get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                            walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
               os.trim(sizeof...(Is));
               os.push(resolve_result<R>(std::move(res), walloc));
            } else {
               R res = std::invoke(F, construct_derived<Cls2, Cls>::value(*self),
                                   get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                         walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
               os.trim(sizeof...(Is));
               os.push(resolve_result<R>(std::move(res), walloc));
            }
         } else {
            if constexpr (std::is_same_v<Cls2, std::nullptr_t>) {
               std::invoke(F, get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                    walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
            } else {
               std::invoke(F, construct_derived<Cls2, Cls>::value(*self),
                           get_value<typename std::tuple_element<Is, Args>::type, Args>(
                                 walloc, std::move(os.get_back(i - Is).get<to_wasm_t<typename std::tuple_element<Is, Args>::type>>()))...);
            }
            os.trim(sizeof...(Is));
         }
      } };
   }

   template <>
   struct registered_host_functions<eosio::chain::apply_context> {
      using Cls = eosio::chain::apply_context;

      template <typename WAlloc>
      struct mappings {
         std::unordered_map<std::pair<std::string, std::string>, uint32_t, host_func_pair_hash> named_mapping;
         std::vector<host_function>                                                             host_functions;
         std::vector<std::function<void(Cls*, WAlloc*, operand_stack&)>>                        functions;
         size_t                                                                                 current_index = 0;
      };

      template <typename WAlloc>
      static mappings<WAlloc>& get_mappings() {
         static mappings<WAlloc> _mappings;
         return _mappings;
      }

      template <typename Cls2, auto Func, typename WAlloc>
      static void add(const std::string& mod, const std::string& name) {
         using deduced_full_ts                         = decltype(get_args_full(Func));
         using deduced_ts                              = decltype(get_args(Func));
         using res_t                                   = typename decltype(get_return_t(Func))::type;
         static constexpr auto is                      = std::make_index_sequence<std::tuple_size<deduced_ts>::value>();
         auto&                 current_mappings        = get_mappings<WAlloc>();
         current_mappings.named_mapping[{ mod, name }] = current_mappings.current_index++;
         current_mappings.functions.push_back(create_logging_function<WAlloc, Cls, Cls2, Func, res_t, deduced_full_ts>(is));
      }

      template <typename Module>
      static void resolve(Module& mod) {
         decltype(mod.import_functions) imports          = { mod.allocator, mod.get_imported_functions_size() };
         auto&                          current_mappings = get_mappings<wasm_allocator>();
         for (int i = 0; i < mod.imports.size(); i++) {
            std::string mod_name =
                  std::string((char*)mod.imports[i].module_str.raw(), mod.imports[i].module_str.size());
            std::string fn_name = std::string((char*)mod.imports[i].field_str.raw(), mod.imports[i].field_str.size());
            EOS_WB_ASSERT(current_mappings.named_mapping.count({ mod_name, fn_name }), wasm_link_exception,
                          "no mapping for imported function");
            imports[i] = current_mappings.named_mapping[{ mod_name, fn_name }];
         }
         mod.import_functions = std::move(imports);
      }

      template <typename Execution_Context>
      void operator()(Cls* host, Execution_Context& ctx, uint32_t index) {
         const auto& _func = get_mappings<wasm_allocator>().functions[index];
         std::invoke(_func, host, ctx.get_wasm_allocator(), ctx.get_operand_stack());
      }
   };
} } // eosio::vm

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace fc;
using namespace eosio::vm;
using namespace eosio::chain::webassembly::common;

class eos_vm_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eos_vm_runtime();
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>) override;

      void immediately_exit_currently_running_module() override {
         if (_bkend)
            _bkend->exit({});
      }

   private:
      // todo: managing this will get more complicated with sync calls;
      //       immediately_exit_currently_running_module() should probably
      //       move from wasm_runtime_interface to wasm_instantiated_module_interface.
      backend<apply_context>* _bkend = nullptr;  // non owning pointer to allow for immediate exit

   friend class eos_vm_instantiated_module;
};

} } } }// eosio::chain::webassembly::wabt_runtime

#define __EOS_VM_INTRINSIC_NAME(LBL, SUF) LBL##SUF
#define _EOS_VM_INTRINSIC_NAME(LBL, SUF) __INTRINSIC_NAME(LBL, SUF)

#define _REGISTER_EOS_VM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG) \
   eosio::vm::registered_function<eosio::chain::apply_context, CLS, &CLS::METHOD> _EOS_VM_INTRINSIC_NAME(__eos_vm_intrinsic_fn, __COUNTER__)(std::string(MOD), std::string(NAME));
