#include <eosio/chain/webassembly/eos-vm-oc/ipc_helpers.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/ipc_protocol.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/memory.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>

#include <sys/prctl.h>
#include <signal.h>
#include <sys/resource.h>

#include "IR/Module.h"
#include "IR/Validate.h"
#include "WASM/WASM.h"
#include "LLVMJIT.h"

using namespace IR;

namespace eosio { namespace chain { namespace eosvmoc {

void run_compile(wrapped_fd&& response_sock, wrapped_fd&& wasm_code) noexcept {  //noexcept; we'll just blow up if anything tries to cross this boundry
   std::vector<uint8_t> wasm = vector_for_memfd(wasm_code);

   //ideally we catch exceptions and sent them upstream as strings for easier reporting

   Module module;
   Serialization::MemoryInputStream stream(wasm.data(), wasm.size());
   WASM::scoped_skip_checks no_check;
   WASM::serialize(stream, module);
   module.userSections.clear();
   wasm_injections::wasm_binary_injection<false> injector(module);
   injector.inject();

   instantiated_code code = LLVMJIT::instantiateModule(module);

   code_compilation_result_message result_message;

   const std::map<unsigned, uintptr_t>& function_to_offsets = code.function_offsets;

   if(module.startFunctionIndex == UINTPTR_MAX)
      result_message.start = no_offset{};
   else if(module.startFunctionIndex < module.functions.imports.size()) {
      const auto& f = module.functions.imports[module.startFunctionIndex];
      const std::size_t io = get_intrinsic_ordinal(f.moduleName + "." + f.exportName);
      result_message.start = intrinsic_ordinal{io};
   }
   else
      result_message.start = code_offset{function_to_offsets.at(module.startFunctionIndex-module.functions.imports.size())};

   for(const Export& exprt : module.exports) {
      if(exprt.name == "apply")
         result_message.apply_offset = function_to_offsets.at(exprt.index-module.functions.imports.size());
   }

   result_message.starting_memory_pages = -1;
   if(module.memories.size())
      result_message.starting_memory_pages = module.memories.defs.at(0).type.size.min;

   std::vector<uint8_t> prologue(module.globals.defs.size() * 8); // Large enough to handle all mutable globals
   std::vector<uint8_t>::iterator prologue_it = prologue.end();

   //set up mutable globals
   union global_union {
      int64_t i64;
      int32_t i32;
      float f32;
      double f64;
   };

   for(const GlobalDef& global : module.globals.defs) {
      if(!global.type.isMutable)
         continue;
      prologue_it -= 8;
      global_union* const u = (global_union* const)&*prologue_it;

      switch(global.initializer.type) {
         case InitializerExpression::Type::i32_const: u->i32 = global.initializer.i32; break;
         case InitializerExpression::Type::i64_const: u->i64 = global.initializer.i64; break;
         case InitializerExpression::Type::f32_const: u->f32 = global.initializer.f32; break;
         case InitializerExpression::Type::f64_const: u->f64 = global.initializer.f64; break;
         default: break; //impossible
      }
   }

   struct table_entry {
      uintptr_t type;
      int64_t func;    //>= 0 means offset to code in wasm; < 0 means intrinsic call at offset address
   };

   for(const TableSegment& table_segment : module.tableSegments) {
      struct table_entry* table_index_0 = (struct table_entry*)(code.code.data() + code.table_offset);

      if(static_cast<uint64_t>(table_segment.baseOffset.i32) > module.tables.defs[0].type.size.min)
         return;

      if(static_cast<uint64_t>(table_segment.baseOffset.i32) > module.tables.defs[0].type.size.min)
         return;

      for(Uptr i = 0; i < table_segment.indices.size(); ++i) {
         const Uptr function_index = table_segment.indices[i];
         const uint64_t effective_table_index = table_segment.baseOffset.i32 + i;

         if(effective_table_index >= module.tables.defs[0].type.size.min)
            return;

         if(function_index < module.functions.imports.size()) {
            const auto& f = module.functions.imports[function_index];
            const std::size_t io = get_intrinsic_ordinal(f.moduleName + "." + f.exportName);
            table_index_0[effective_table_index].func = io*-8;
            table_index_0[effective_table_index].type = (uintptr_t)module.types[module.functions.imports[function_index].type.index];
         }
         else {
            table_index_0[effective_table_index].func = function_to_offsets.at(function_index - module.functions.imports.size());
            table_index_0[effective_table_index].type = (uintptr_t)module.types[module.functions.defs[function_index - module.functions.imports.size()].type.index];
         }
      }
   }

   //this is somewhat copy pasta from wasm_interface_private, with the asserts removed
   std::vector<uint8_t> initial_mem;
   for(const DataSegment& data_segment : module.dataSegments) {
      const U32 base_offset = data_segment.baseOffset.i32;

      if(base_offset + data_segment.data.size() > initial_mem.size())
         initial_mem.resize(base_offset + data_segment.data.size(), 0x00);
      memcpy(initial_mem.data() + base_offset, data_segment.data.data(), data_segment.data.size());
   }

   result_message.initdata_prologue_size = prologue.end() - prologue_it;
   std::vector<uint8_t> initdata_prep;
   std::move(prologue_it, prologue.end(), std::back_inserter(initdata_prep));
   std::move(initial_mem.begin(), initial_mem.end(), std::back_inserter(initdata_prep));

   std::vector<wrapped_fd> fds_to_send;
   fds_to_send.emplace_back(memfd_for_bytearray(code.code));
   fds_to_send.emplace_back(memfd_for_bytearray(initdata_prep));
   write_message_with_fds(response_sock, result_message, fds_to_send);
}

void run_compile_trampoline(int fd) {
   prctl(PR_SET_NAME, "oc-trampoline");
   prctl(PR_SET_PDEATHSIG, SIGKILL);

   //squelching this for now, but it means we won't have ability to get compile metrics
   struct sigaction act;
   sigset_t set;
   sigemptyset(&set);
   act.sa_handler = SIG_IGN;
   act.sa_mask = set;
   act.sa_flags = SA_NOCLDWAIT;
   act.sa_sigaction = nullptr;
   sigaction(SIGCHLD, &act, nullptr);

   while(true) {
      auto [success, message, fds] = read_message_with_fds(fd);
      if(!success)
         break;
      
      if(!std::holds_alternative<compile_wasm_message>(message) || fds.size() != 2) {
         std::cerr << "EOS VM OC compile trampoline got unexpected message; ignoring" << std::endl;
         continue;
      }

      pid_t pid = fork();
      if(pid == 0) {
         prctl(PR_SET_NAME, "oc-compile");
         prctl(PR_SET_PDEATHSIG, SIGKILL);

         struct rlimit cpu_limits = {20u, 20u};
         setrlimit(RLIMIT_CPU, &cpu_limits);

         struct rlimit vm_limits = {512u*1024u*1024u, 512u*1024u*1024u};
         setrlimit(RLIMIT_AS, &vm_limits);

         struct rlimit core_limits = {0u, 0u};
         setrlimit(RLIMIT_CORE, &core_limits);

         run_compile(std::move(fds[0]), std::move(fds[1]));
         _exit(0);
      }
      else if(pid == -1)
         std::cerr << "EOS VM OC compile trampoline failed to spawn compile task" << std::endl;
   }

   _exit(0);
}

}}}


