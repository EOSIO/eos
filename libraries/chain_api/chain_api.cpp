#include <chain_api.hpp>

static chain_api_cpp* s_api = nullptr;
void register_chain_api_cpp(struct chain_api_cpp* api) {
   s_api = api;
}

struct chain_api_cpp* get_chain_api_cpp(void) {
   if (!s_api) {
      throw std::runtime_error("chain api not specified!");
   }
   return s_api;
}

