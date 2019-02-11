/*
 * vm_api.hpp
 *
 *  Created on: Oct 13, 2018
 *      Author: newworld
 */

#ifndef CONTRACTS_EOSIOLIB_NATIVE_VM_API_HPP_
#define CONTRACTS_EOSIOLIB_NATIVE_VM_API_HPP_

#include <stdint.h>
#include <string>

using namespace std;

struct vm_api_cpp
{
   int64_t (*get_current_exception)(std::string& what);
   void (*n2str)(uint64_t n, string& str_name);
   uint64_t (*str2n)(string& str_name);
};

void vm_register_api_cpp(struct vm_api_cpp* api);
struct vm_api_cpp* get_vm_api_cpp();


#endif /* CONTRACTS_EOSIOLIB_NATIVE_VM_API_HPP_ */
