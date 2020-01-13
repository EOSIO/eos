#pragma once

#include <stdint.h>
#include <stddef.h>
#include <exception>
#include <setjmp.h>

#include <list>
#include <vector>
#include <cstddef>

namespace eosio { namespace chain {

class apply_context;

namespace eosvmoc {

class code_cache_base;
class memory;
struct code_descriptor;

class executor {
   public:
      executor(const code_cache_base& cc);
      ~executor();

      void execute(const code_descriptor& code, const memory& mem, apply_context& context);

   private:
      uint8_t* code_mapping;
      size_t code_mapping_size;
      bool mapping_is_executable;

      std::exception_ptr executors_exception_ptr;
      sigjmp_buf executors_sigjmp_buf;
      std::list<std::vector<std::byte>> executors_bounce_buffers;
};

}}}
