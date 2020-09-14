#pragma once
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/whitelisted_intrinsics.hpp>
#include <eosio/chain/exceptions.hpp>
#if defined(EOSIO_EOS_VM_RUNTIME_ENABLED) || defined(EOSIO_EOS_VM_JIT_RUNTIME_ENABLED)
#include <eosio/vm/allocator.hpp>
#endif
#include "Runtime/Linker.h"
#include "Runtime/Runtime.h"

namespace eosio { namespace chain {

   class apply_context;
   class wasm_runtime_interface;
   class controller;
   namespace eosvmoc { struct config; }

   struct wasm_exit {
      int32_t code = 0;
   };

   /**
    * @class wasm_interface
    *
    */
   class wasm_interface {
      public:
         enum class vm_type {
            eos_vm,
            eos_vm_jit,
            eos_vm_oc
         };

         //return string description of vm_type
         static std::string vm_type_string(vm_type vmtype) {
             switch (vmtype) {
             case vm_type::eos_vm:
                return "eos-vm";
             case vm_type::eos_vm_oc:
                return "eos-vm-oc";
             default:
                 return "eos-vm-jit";
             }
         }

         wasm_interface(vm_type vm, bool eosvmoc_tierup, const chainbase::database& d, const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, bool profile);
         ~wasm_interface();

         //call before dtor to skip what can be minutes of dtor overhead with some runtimes; can cause leaks
         void indicate_shutting_down();

         //validates code -- does a WASM validation pass and checks the wasm against EOSIO specific constraints
         static void validate(const controller& control, const bytes& code);

         //indicate that a particular code probably won't be used after given block_num
         void code_block_num_last_used(const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version, const uint32_t& block_num);

         //indicate the current LIB. evicts old cache entries
         void current_lib(const uint32_t lib);

         //Calls apply or error on a given code
         void apply(const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version, apply_context& context);

         //Immediately exits currently running wasm. UB is called when no wasm running
         void exit();

      private:
         unique_ptr<struct wasm_interface_impl> my;
   };

} } // eosio::chain

namespace eosio{ namespace chain {
   std::istream& operator>>(std::istream& in, wasm_interface::vm_type& runtime);
}}

FC_REFLECT_ENUM( eosio::chain::wasm_interface::vm_type, (eos_vm)(eos_vm_jit)(eos_vm_oc) )
