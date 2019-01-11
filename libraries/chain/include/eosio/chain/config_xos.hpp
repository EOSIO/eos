/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/wasm_interface.hpp>
#include <fc/time.hpp>

#pragma GCC diagnostic ignored "-Wunused-variable"

namespace eosio { namespace chain { namespace config {

//guaranteed minimum resources  which is abbreviated  gmr
const static uint32_t   default_gmr_cpu_limit         = 200'000; /// free cpu usage in microseconds
const static uint32_t   default_gmr_net_limit         = 10 * 1024;   // 10 KB
const static uint32_t   default_gmr_ram_limit         = 0;   // 0 KB
const static uint16_t   default_gmr_resource_limit_per_day                 = 1000;



} } } // namespace eosio::chain::config


