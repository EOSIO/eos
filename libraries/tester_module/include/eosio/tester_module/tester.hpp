#pragma once

#include <eosio/module/module_mapping.hpp>

#include <cstdint>

namespace eosio { namespace tester_module {
   EOSIO_MODULE_DECL(void, assert_version, int64_t, int64_t, int64_t, uint8_t);
}} // ns eosio::tester_module
