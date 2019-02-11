#include "vm_api.hpp"
#include <stdexcept>

static vm_api_cpp* s_api = nullptr;
void vm_register_api_cpp(struct vm_api_cpp* api) {
   s_api = api;
}

struct vm_api_cpp* get_vm_api_cpp(void) {
   if (!s_api) {
      throw std::runtime_error("vm api not specified!");
   }
   return s_api;
}
