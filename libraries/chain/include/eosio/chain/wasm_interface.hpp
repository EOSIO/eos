#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/exceptions.hpp>
#include "Runtime/Linker.h"
#include "Runtime/Runtime.h"

namespace eosio { namespace chain {

   class apply_context;
   class wasm_runtime_interface;
   class controller;

   struct wasm_exit {
      int32_t code = 0;
   };

   namespace webassembly { namespace common {
      class intrinsics_accessor;

      struct root_resolver : Runtime::Resolver {
         //when validating is true; only allow "env" imports. Otherwise allow any imports. This resolver is used
         //in two cases: once by the generic validating code where we only want "env" to pass; and then second in the
         //wavm runtime where we need to allow linkage to injected functions
         root_resolver(bool validating = false) : validating(validating) {}
         bool validating;

         bool resolve(const string& mod_name,
                      const string& export_name,
                      IR::ObjectType type,
                      Runtime::ObjectInstance*& out) override {
      try {
         //protect access to "private" injected functions; so for now just simply allow "env" since injected functions
         //  are in a different module
         if(validating && mod_name != "env")
            EOS_ASSERT( false, wasm_exception, "importing from module that is not 'env': ${module}.${export}", ("module",mod_name)("export",export_name) );

         // Try to resolve an intrinsic first.
         if(Runtime::IntrinsicResolver::singleton.resolve(mod_name,export_name,type, out)) {
            return true;
         }

         EOS_ASSERT( false, wasm_exception, "${module}.${export} unresolveable", ("module",mod_name)("export",export_name) );
         return false;
      } FC_CAPTURE_AND_RETHROW( (mod_name)(export_name) ) }
      };
   } }

   /**
    * @class wasm_interface
    *
    */
   class wasm_interface {
      public:
         enum class vm_type {
            wavm,
            wabt
         };

         wasm_interface(vm_type vm);
         ~wasm_interface();

         //validates code -- does a WASM validation pass and checks the wasm against EOSIO specific constraints
         static void validate(const controller& control, const bytes& code);

         //indicate when code has changed on an account, effectively reducing the reference count on
         //the old one and increasing the reference count on the new one
         void account_code_change(const digest_type& from, const digest_type& to, const uint32_t pending_block_num);

         void prune_wasm_cache(const uint32_t through_block_num);

         //Calls apply or error on a given code
         void apply(const digest_type& code_id, const shared_string& code, apply_context& context);

         //Immediately exits currently running wasm. UB is called when no wasm running
         void exit();

      private:
         unique_ptr<struct wasm_interface_impl> my;
         friend class eosio::chain::webassembly::common::intrinsics_accessor;
   };

} } // eosio::chain

namespace eosio{ namespace chain {
   std::istream& operator>>(std::istream& in, wasm_interface::vm_type& runtime);
}}

FC_REFLECT_ENUM( eosio::chain::wasm_interface::vm_type, (wavm)(wabt) )
