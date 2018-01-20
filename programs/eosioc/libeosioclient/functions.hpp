#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>

namespace eosio {
namespace client {

const std::string chain_func_base = "/v1/chain";
const std::string get_info_func = chain_func_base + "/get_info";

} // namespace client
} // namespace eosio

#endif // FUNCTIONS_H
