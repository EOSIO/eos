#pragma once

#include <eosio/chain/webassembly/eos-vm-oc/stack.hpp>

#include <stdint.h>
#include <stddef.h>
#include <exception>
#include <setjmp.h>

#include <list>
#include <vector>
#include <functional>
#include <cstddef>

namespace eosio { namespace chain {

namespace eosvmoc {

class code_cache_base;
class memory;
struct code_descriptor;

struct timer_base {
   virtual void set_expiration_callback(void(*)(void*), void*) {}
   virtual void checktime() {}
};

void throw_internal_exception(const char* const s);

class executor {
   public:
      executor(const code_cache_base& cc);
      ~executor();

      void execute(const code_descriptor& code, memory& mem, void* context, uint64_t max_call_depth, uint64_t max_pages, timer_base* timer, uint64_t receiver, uint64_t account, uint64_t action);

   private:
      uint8_t* code_mapping;
      size_t code_mapping_size;
      bool mapping_is_executable;

      std::exception_ptr executors_exception_ptr;
      sigjmp_buf executors_sigjmp_buf;
      std::list<std::vector<std::byte>> executors_bounce_buffers;
      std::vector<std::byte> globals_buffer;
      execution_stack stack;
};

}}}
