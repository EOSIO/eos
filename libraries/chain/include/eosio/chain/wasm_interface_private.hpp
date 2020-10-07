#pragma once

#include <eosio/chain/wasm_interface.hpp>
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
#include <eosio/chain/webassembly/eos-vm-oc.hpp>
#else
#define _REGISTER_EOSVMOC_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)
#endif
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/scoped_exit.hpp>

#include "IR/Module.h"
#include "Runtime/Intrinsics.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Validate.h"

#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/vm/allocator.hpp>

using namespace fc;
using namespace eosio::chain::webassembly;
using namespace IR;
using namespace Runtime;

using boost::multi_index_container;

namespace eosio { namespace chain {

   namespace eosvmoc { struct config; }

   struct wasm_interface_impl {
      struct wasm_cache_entry {
         digest_type                                          code_hash;
         uint32_t                                             first_block_num_used;
         uint32_t                                             last_block_num_used;
         std::unique_ptr<wasm_instantiated_module_interface>  module;
         uint8_t                                              vm_type = 0;
         uint8_t                                              vm_version = 0;
      };
      struct by_hash;
      struct by_first_block_num;
      struct by_last_block_num;

#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
      struct eosvmoc_tier {
         eosvmoc_tier(const boost::filesystem::path& d, const eosvmoc::config& c, const chainbase::database& db)
           : cc(d, c, webassembly::eosvmoc::make_code_finder(db)), exec(cc),
            mem(wasm_constraints::maximum_linear_memory/wasm_constraints::wasm_page_size, webassembly::eosvmoc::get_intrinsic_map()) {}
         eosvmoc::code_cache_async cc;
         eosvmoc::executor exec;
         eosvmoc::memory mem;
      };
#endif

      wasm_interface_impl(wasm_interface::vm_type vm, bool eosvmoc_tierup, const chainbase::database& d, const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config) : db(d), wasm_runtime_time(vm) {
#ifdef EOSIO_EOS_VM_RUNTIME_ENABLED
         if(vm == wasm_interface::vm_type::eos_vm)
            runtime_interface = std::make_unique<webassembly::eos_vm_runtime::eos_vm_runtime<eosio::vm::interpreter>>();
#endif
#ifdef EOSIO_EOS_VM_JIT_RUNTIME_ENABLED
         if(vm == wasm_interface::vm_type::eos_vm_jit)
            runtime_interface = std::make_unique<webassembly::eos_vm_runtime::eos_vm_runtime<eosio::vm::jit>>();
#endif
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
         if(vm == wasm_interface::vm_type::eos_vm_oc)
            runtime_interface = std::make_unique<webassembly::eosvmoc::eosvmoc_runtime>(data_dir, eosvmoc_config, d);
#endif
         if(!runtime_interface)
            EOS_THROW(wasm_exception, "${r} wasm runtime not supported on this platform and/or configuration", ("r", vm));

#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
         if(eosvmoc_tierup) {
            EOS_ASSERT(vm != wasm_interface::vm_type::eos_vm_oc, wasm_exception, "You can't use EOS VM OC as the base runtime when tier up is activated");
            eosvmoc.emplace(data_dir, eosvmoc_config, d);
         }
#endif
      }

      ~wasm_interface_impl() {
         if(is_shutting_down)
            for(wasm_cache_index::iterator it = wasm_instantiation_cache.begin(); it != wasm_instantiation_cache.end(); ++it)
               wasm_instantiation_cache.modify(it, [](wasm_cache_entry& e) {
                  e.module.release();
               });
      }

      std::vector<uint8_t> parse_initial_memory(const Module& module) {
         std::vector<uint8_t> mem_image;

         for(const DataSegment& data_segment : module.dataSegments) {
            EOS_ASSERT(data_segment.baseOffset.type == InitializerExpression::Type::i32_const, wasm_exception, "");
            EOS_ASSERT(module.memories.defs.size(), wasm_exception, "");
            const U32 base_offset = data_segment.baseOffset.i32;
            const Uptr memory_size = (module.memories.defs[0].type.size.min << IR::numBytesPerPageLog2);
            if(base_offset >= memory_size || base_offset + data_segment.data.size() > memory_size)
               FC_THROW_EXCEPTION(wasm_execution_error, "WASM data segment outside of valid memory range");
            if(base_offset + data_segment.data.size() > mem_image.size())
               mem_image.resize(base_offset + data_segment.data.size(), 0x00);
            memcpy(mem_image.data() + base_offset, data_segment.data.data(), data_segment.data.size());
         }

         return mem_image;
      }

      void code_block_num_last_used(const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version, const uint32_t& block_num) {
         wasm_cache_index::iterator it = wasm_instantiation_cache.find(boost::make_tuple(code_hash, vm_type, vm_version));
         if(it != wasm_instantiation_cache.end())
            wasm_instantiation_cache.modify(it, [block_num](wasm_cache_entry& e) {
               e.last_block_num_used = block_num;
            });
      }

      void current_lib(uint32_t lib) {
         //anything last used before or on the LIB can be evicted
         const auto first_it = wasm_instantiation_cache.get<by_last_block_num>().begin();
         const auto last_it  = wasm_instantiation_cache.get<by_last_block_num>().upper_bound(lib);
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
         if(eosvmoc) for(auto it = first_it; it != last_it; it++)
            eosvmoc->cc.free_code(it->code_hash, it->vm_version);
#endif
         wasm_instantiation_cache.get<by_last_block_num>().erase(first_it, last_it);
      }

      const std::unique_ptr<wasm_instantiated_module_interface>& get_instantiated_module( const digest_type& code_hash, const uint8_t& vm_type,
                                                                                 const uint8_t& vm_version, transaction_context& trx_context )
      {
         wasm_cache_index::iterator it = wasm_instantiation_cache.find(
                                             boost::make_tuple(code_hash, vm_type, vm_version) );
         const code_object* codeobject = nullptr;
         if(it == wasm_instantiation_cache.end()) {
            codeobject = &db.get<code_object,by_code_hash>(boost::make_tuple(code_hash, vm_type, vm_version));

            it = wasm_instantiation_cache.emplace( wasm_interface_impl::wasm_cache_entry{
                                                      .code_hash = code_hash,
                                                      .first_block_num_used = codeobject->first_block_used,
                                                      .last_block_num_used = UINT32_MAX,
                                                      .module = nullptr,
                                                      .vm_type = vm_type,
                                                      .vm_version = vm_version
                                                   } ).first;
         }

         if(!it->module) {
            if(!codeobject)
               codeobject = &db.get<code_object,by_code_hash>(boost::make_tuple(code_hash, vm_type, vm_version));

            auto timer_pause = fc::make_scoped_exit([&](){
               trx_context.resume_billing_timer();
            });
            trx_context.pause_billing_timer();
            IR::Module module;
            std::vector<U8> bytes = {
                (const U8*)codeobject->code.data(),
                (const U8*)codeobject->code.data() + codeobject->code.size()};
            try {
               Serialization::MemoryInputStream stream((const U8*)bytes.data(),
                                                       bytes.size());
               WASM::scoped_skip_checks no_check;
               WASM::serialize(stream, module);
               module.userSections.clear();
            } catch (const Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            } catch (const IR::ValidationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }
            if (runtime_interface->inject_module(module)) {
               try {
                  Serialization::ArrayOutputStream outstream;
                  WASM::serialize(outstream, module);
                  bytes = outstream.getBytes();
               } catch (const Serialization::FatalSerializationException& e) {
                  EOS_ASSERT(false, wasm_serialization_error,
                             e.message.c_str());
               } catch (const IR::ValidationException& e) {
                  EOS_ASSERT(false, wasm_serialization_error,
                             e.message.c_str());
               }
            }

            wasm_instantiation_cache.modify(it, [&](auto& c) {
               c.module = runtime_interface->instantiate_module((const char*)bytes.data(), bytes.size(), parse_initial_memory(module), code_hash, vm_type, vm_version);
            });
         }
         return it->module;
      }

      bool is_shutting_down = false;
      std::unique_ptr<wasm_runtime_interface> runtime_interface;

      typedef boost::multi_index_container<
         wasm_cache_entry,
         indexed_by<
            ordered_unique<tag<by_hash>,
               composite_key< wasm_cache_entry,
                  member<wasm_cache_entry, digest_type, &wasm_cache_entry::code_hash>,
                  member<wasm_cache_entry, uint8_t,     &wasm_cache_entry::vm_type>,
                  member<wasm_cache_entry, uint8_t,     &wasm_cache_entry::vm_version>
               >
            >,
            ordered_non_unique<tag<by_first_block_num>, member<wasm_cache_entry, uint32_t, &wasm_cache_entry::first_block_num_used>>,
            ordered_non_unique<tag<by_last_block_num>, member<wasm_cache_entry, uint32_t, &wasm_cache_entry::last_block_num_used>>
         >
      > wasm_cache_index;
      wasm_cache_index wasm_instantiation_cache;

      const chainbase::database& db;
      const wasm_interface::vm_type wasm_runtime_time;

#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
      fc::optional<eosvmoc_tier> eosvmoc;
#endif
   };

} } // eosio::chain
