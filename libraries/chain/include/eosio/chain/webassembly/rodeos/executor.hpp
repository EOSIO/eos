#pragma once

#include <stdint.h>
#include <stddef.h>

namespace eosio { namespace chain {

class apply_context;

namespace rodeos {

class code_cache;
class memory;
struct code_descriptor;

class executor {
   public:
      executor(const code_cache& cc);

      void execute(const code_descriptor& code, const memory& mem, apply_context& context);

   private:
      uint8_t* code_mapping;
      size_t code_mapping_size;
      bool mapping_is_executable;
};

}}}
