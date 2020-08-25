#include <eosio/chain/webassembly/eos-vm-oc/executor.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/memory.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic_mapping.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.h>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/hana/equal.hpp>

#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#if defined(__has_feature)
#if __has_feature(shadow_call_stack)
#error EOS VM OC is not compatible with Clang ShadowCallStack
#endif
#endif

extern "C" int arch_prctl(int code, unsigned long* addr);

namespace eosio { namespace chain { namespace eosvmoc {

static constexpr auto signal_sentinel = 0x4D56534F45534559ul;

static void(*chained_handler)(int,siginfo_t*,void*);
static void segv_handler(int sig, siginfo_t* info, void* ctx)  {
   control_block* cb_in_main_segment;

   //a 0 GS value is an indicator an executor hasn't been active on this thread recently
   uint64_t current_gs;
   syscall(SYS_arch_prctl, ARCH_GET_GS, &current_gs);
   if(current_gs == 0)
      goto notus;

   cb_in_main_segment = reinterpret_cast<control_block*>(current_gs - memory::cb_offset);

   //as a double check that the control block pointer is what we expect, look for the magic
   if(cb_in_main_segment->magic != signal_sentinel)
      goto notus;

   //was wasm running? If not, this SEGV was not due to us
   if(cb_in_main_segment->is_running == false)
      goto notus;

   //was the segfault within code?
   if((uintptr_t)info->si_addr >= cb_in_main_segment->execution_thread_code_start &&
      (uintptr_t)info->si_addr < cb_in_main_segment->execution_thread_code_start+cb_in_main_segment->execution_thread_code_length)
         siglongjmp(*cb_in_main_segment->jmp, EOSVMOC_EXIT_CHECKTIME_FAIL);

   //was the segfault within data?
   if((uintptr_t)info->si_addr >= cb_in_main_segment->execution_thread_memory_start &&
      (uintptr_t)info->si_addr < cb_in_main_segment->execution_thread_memory_start+cb_in_main_segment->execution_thread_memory_length)
         siglongjmp(*cb_in_main_segment->jmp, EOSVMOC_EXIT_SEGV);

notus:
   if(chained_handler) {
      chained_handler(sig, info, ctx);
      return;
   }
   ::signal(sig, SIG_DFL);
   ::raise(sig);
   __builtin_unreachable();
}

static intrinsic grow_memory_intrinsic EOSVMOC_INTRINSIC_INIT_PRIORITY("eosvmoc_internal.grow_memory", IR::FunctionType::get(IR::ResultType::i32,{IR::ValueType::i32,IR::ValueType::i32}),
  (void*)&eos_vm_oc_grow_memory,
  std::integral_constant<std::size_t, find_intrinsic_index("eosvmoc_internal.grow_memory")>::value
);

//This is effectively overriding the eosio_exit intrinsic in wasm_interface
static void eosio_exit(int32_t code) {
   siglongjmp(*eos_vm_oc_get_jmp_buf(), EOSVMOC_EXIT_CLEAN_EXIT);
   __builtin_unreachable();
}
static intrinsic eosio_exit_intrinsic("env.eosio_exit", IR::FunctionType::get(IR::ResultType::none,{IR::ValueType::i32}), (void*)&eosio_exit,
  std::integral_constant<std::size_t, find_intrinsic_index("env.eosio_exit")>::value
);

static void throw_internal_exception(const char* const s) {
   *reinterpret_cast<std::exception_ptr*>(eos_vm_oc_get_exception_ptr()) = std::make_exception_ptr(wasm_execution_error(FC_LOG_MESSAGE(error, s)));
   siglongjmp(*eos_vm_oc_get_jmp_buf(), EOSVMOC_EXIT_EXCEPTION);
   __builtin_unreachable();
}

#define DEFINE_EOSVMOC_TRAP_INTRINSIC(module,name) \
	void name(); \
	static intrinsic name##Function EOSVMOC_INTRINSIC_INIT_PRIORITY(#module "." #name,IR::FunctionType::get(),(void*)&name, \
     std::integral_constant<std::size_t, find_intrinsic_index(#module "." #name)>::value \
   ); \
	void name()

DEFINE_EOSVMOC_TRAP_INTRINSIC(eosvmoc_internal,depth_assert) {
   throw_internal_exception("Exceeded call depth maximum");
}

DEFINE_EOSVMOC_TRAP_INTRINSIC(eosvmoc_internal,div0_or_overflow) {
   throw_internal_exception("Division by 0 or integer overflow trapped");
}

DEFINE_EOSVMOC_TRAP_INTRINSIC(eosvmoc_internal,indirect_call_mismatch) {
   throw_internal_exception("Indirect call function type mismatch");
}

DEFINE_EOSVMOC_TRAP_INTRINSIC(eosvmoc_internal,indirect_call_oob) {
   throw_internal_exception("Indirect call index out of bounds");
}

DEFINE_EOSVMOC_TRAP_INTRINSIC(eosvmoc_internal,unreachable) {
   throw_internal_exception("Unreachable reached");
}

struct executor_signal_init {
   executor_signal_init() {
      struct sigaction sig_action, old_sig_action;
      sig_action.sa_sigaction = segv_handler;
      sigemptyset(&sig_action.sa_mask);
      sig_action.sa_flags = SA_SIGINFO | SA_NODEFER;
      sigaction(SIGSEGV, &sig_action, &old_sig_action);
      if(old_sig_action.sa_flags & SA_SIGINFO)
         chained_handler = old_sig_action.sa_sigaction;
      else if(old_sig_action.sa_handler != SIG_IGN && old_sig_action.sa_handler != SIG_DFL)
         chained_handler = (void (*)(int,siginfo_t*,void*))old_sig_action.sa_handler;
   }
};

executor::executor(const code_cache_base& cc) {
   //if we're the first executor created, go setup the signal handling. For now we'll just leave this attached forever
   static executor_signal_init the_executor_signal_init;

   uint64_t current_gs;
   if(arch_prctl(ARCH_GET_GS, &current_gs) || current_gs)
      wlog("x86_64 GS register is not set as expected. EOS VM OC may not run correctly on this platform");

   struct stat s;
   FC_ASSERT(fstat(cc.fd(), &s) == 0, "executor failed to get code cache size");
   code_mapping = (uint8_t*)mmap(nullptr, s.st_size, PROT_EXEC|PROT_READ, MAP_SHARED, cc.fd(), 0);
   FC_ASSERT(code_mapping != MAP_FAILED, "failed to map code cache in to executor");
   code_mapping_size = s.st_size;
   mapping_is_executable = true;
}

void executor::execute(const code_descriptor& code, memory& mem, apply_context& context) {
   if(mapping_is_executable == false) {
      mprotect(code_mapping, code_mapping_size, PROT_EXEC|PROT_READ);
      mapping_is_executable = true;
   }

   uint64_t max_call_depth = eosio::chain::wasm_constraints::maximum_call_depth+1;
   uint64_t max_pages = eosio::chain::wasm_constraints::maximum_linear_memory/eosio::chain::wasm_constraints::wasm_page_size;
   if(context.control.is_builtin_activated(builtin_protocol_feature_t::configurable_wasm_limits)) {
      const wasm_config& config = context.control.get_global_properties().wasm_configuration;
      max_call_depth = config.max_call_depth;
      max_pages = config.max_pages;
   }
   stack.reset(max_call_depth);
   EOS_ASSERT(code.starting_memory_pages <= (int)max_pages, wasm_execution_error, "Initial memory out of range");

   //prepare initial memory, mutable globals, and table data
   if(code.starting_memory_pages > 0 ) {
      uint64_t initial_page_offset = std::min(static_cast<std::size_t>(code.starting_memory_pages), mem.size_of_memory_slice_mapping()/memory::stride - 1);
      if(initial_page_offset < code.starting_memory_pages) {
         mprotect(mem.full_page_memory_base() + initial_page_offset * eosio::chain::wasm_constraints::wasm_page_size,
                  (code.starting_memory_pages - initial_page_offset) * eosio::chain::wasm_constraints::wasm_page_size, PROT_READ | PROT_WRITE);
      }
      arch_prctl(ARCH_SET_GS, (unsigned long*)(mem.zero_page_memory_base()+initial_page_offset*memory::stride));
      memset(mem.full_page_memory_base(), 0, 64u*1024u*code.starting_memory_pages);
   }
   else
      arch_prctl(ARCH_SET_GS, (unsigned long*)mem.zero_page_memory_base());

   void* globals;
   if(code.initdata_prologue_size > memory::max_prologue_size) {
      globals_buffer.resize(code.initdata_prologue_size);
      memcpy(globals_buffer.data(), code_mapping + code.initdata_begin, code.initdata_prologue_size);
      memcpy(mem.full_page_memory_base() - memory::max_prologue_size,
             code_mapping + code.initdata_begin + code.initdata_prologue_size - memory::max_prologue_size,
             code.initdata_size - code.initdata_prologue_size + memory::max_prologue_size);
      globals = globals_buffer.data() + globals_buffer.size();
   } else {
      memcpy(mem.full_page_memory_base() - code.initdata_prologue_size, code_mapping + code.initdata_begin, code.initdata_size);
      globals = mem.full_page_memory_base();
   }

   control_block* const cb = mem.get_control_block();
   cb->magic = signal_sentinel;
   cb->execution_thread_code_start = (uintptr_t)code_mapping;
   cb->execution_thread_code_length = code_mapping_size;
   cb->execution_thread_memory_start = (uintptr_t)mem.start_of_memory_slices();
   cb->execution_thread_memory_length = mem.size_of_memory_slice_mapping();
   cb->ctx = &context;
   executors_exception_ptr = nullptr;
   cb->eptr = &executors_exception_ptr;
   cb->current_call_depth_remaining = max_call_depth + 1;
   cb->current_linear_memory_pages = code.starting_memory_pages;
   cb->max_linear_memory_pages = max_pages;
   cb->first_invalid_memory_address = code.starting_memory_pages*64*1024;
   cb->full_linear_memory_start = (char*)mem.full_page_memory_base();
   cb->jmp = &executors_sigjmp_buf;
   cb->bounce_buffers = &executors_bounce_buffers;
   cb->running_code_base = (uintptr_t)(code_mapping + code.code_begin);
   cb->is_running = true;
   cb->globals = globals;

   context.trx_context.transaction_timer.set_expiration_callback([](void* user) {
      executor* self = (executor*)user;
      syscall(SYS_mprotect, self->code_mapping, self->code_mapping_size, PROT_NONE);
      self->mapping_is_executable = false;
   }, this);
   context.trx_context.checktime(); //catch any expiration that might have occurred before setting up callback

   auto cleanup = fc::make_scoped_exit([cb, &tt=context.trx_context.transaction_timer, &mem=mem](){
      cb->is_running = false;
      cb->bounce_buffers->clear();
      tt.set_expiration_callback(nullptr, nullptr);

      uint64_t base_pages = mem.size_of_memory_slice_mapping()/memory::stride - 1;
      if(cb->current_linear_memory_pages > base_pages) {
         mprotect(mem.full_page_memory_base() + base_pages * eosio::chain::wasm_constraints::wasm_page_size,
                  (cb->current_linear_memory_pages - base_pages) * eosio::chain::wasm_constraints::wasm_page_size, PROT_NONE);
      }
   });

   void(*apply_func)(uint64_t, uint64_t, uint64_t) = (void(*)(uint64_t, uint64_t, uint64_t))(cb->running_code_base + code.apply_offset);

   switch(sigsetjmp(*cb->jmp, 0)) {
      case 0:
         stack.run([&]{
            std::visit(overloaded {
               [&](const no_offset&) {},
               [&](const intrinsic_ordinal& i) {
                  void(*start_func)() = (void(*)())(*(uintptr_t*)((uintptr_t)mem.zero_page_memory_base() - memory::first_intrinsic_offset - i.ordinal*8));
                  start_func();
               },
               [&](const code_offset& offs) {
                  void(*start_func)() = (void(*)())(cb->running_code_base + offs.offset);
                  start_func();
               }
            }, code.start);
            apply_func(context.get_receiver().to_uint64_t(), context.get_action().account.to_uint64_t(), context.get_action().name.to_uint64_t());
         });
         break;
      //case 1: clean eosio_exit
      case EOSVMOC_EXIT_CHECKTIME_FAIL:
         context.trx_context.checktime();
         break;
      case EOSVMOC_EXIT_SEGV:
         EOS_ASSERT(false, wasm_execution_error, "access violation");
         break;
      case EOSVMOC_EXIT_EXCEPTION: //exception
         std::rethrow_exception(*cb->eptr);
         break;
   }
}

executor::~executor() {
   arch_prctl(ARCH_SET_GS, nullptr);
}

}}}
