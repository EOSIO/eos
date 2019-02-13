/*
 * chain_api.hpp
 *
 *  Created on: Oct 13, 2018
 *      Author: newworld
 */

#ifndef CHAIN_API_HPP_
#define CHAIN_API_HPP_

#include <stdint.h>
#include <string>
#include <eosio/chain/types.hpp>

using namespace std;
using namespace eosio::chain;

struct chain_api_cpp
{
   int64_t (*get_current_exception)(std::string& what);
   void (*n2str)(uint64_t n, string& str_name);
   uint64_t (*str2n)(string& str_name);
   bool (*get_code)(uint64_t contract, digest_type& code_id, const eosio::chain::shared_string** code);
   const char* (*get_code_ex)( uint64_t receiver, size_t* size );
   string (*get_state_dir)();
   bool (*contracts_console)();
   void (*resume_billing_timer)(void);
   void (*pause_billing_timer)(void);
   bool (*is_producing_block)();

//vm_exceptions.cpp
   void (*throw_exception)(int type, const char* fmt, ...);

};

void register_chain_api_cpp(struct chain_api_cpp* api);
struct chain_api_cpp* get_chain_api_cpp();

#endif /* CHAIN_API_HPP_ */

