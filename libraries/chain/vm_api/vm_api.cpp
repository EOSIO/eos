#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
/*
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain/wasm_eosio_validation.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
*/
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/symbol.hpp>

#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw.hpp>

#include <softfloat.hpp>
#include <compiler_builtins.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace eosio::chain;
#include <fstream>
#include <dlfcn.h>
#include "xxhash.h"
//vm_api.cpp
extern "C" void vm_api_init();

class init_vm_api {
public:
   init_vm_api() {
      vm_api_init();
   }
};

static init_vm_api _init_vm_api;
static apply_context *s_ctx = nullptr;

//eosio.token.cpp
void token_create( uint64_t issuer, const char* maximum_supply, size_t size);
void token_issue( uint64_t to, const char* quantity, size_t size1, const char* memo, size_t size2 );
void token_transfer( uint64_t from, uint64_t to, const char* quantity, size_t size1, const char* memo, size_t size2);

namespace eosio {
namespace chain {

void set_apply_context(apply_context *ctx) {
   s_ctx = ctx;
}

apply_context *get_apply_context() {
   return s_ctx;
}

static inline apply_context& ctx() {
   return *s_ctx;
}

#include <eosiolib_native/vm_api.h>

#include "action.cpp"
#include "chain.cpp"
#include "system.cpp"
#include "crypto.cpp"
#include "db.cpp"
#include "privileged.cpp"
#include "transaction.cpp"
#include "print.cpp"
#include "permission.cpp"

static struct vm_api _vm_api = {
};

extern "C" void vm_api_init() {
   static bool s_init = false;
   if (!s_init){
      s_init = true;
      // _vm_api.vm_apply = vm_apply;
      _vm_api.read_action_data = read_action_data;
      _vm_api.action_data_size = action_data_size;
      _vm_api.get_action_info = get_action_info;
      
      _vm_api.require_recipient = require_recipient;
      _vm_api.require_auth = require_auth;
      _vm_api.require_auth2 = require_auth2;
      _vm_api.has_auth = has_auth;
      _vm_api.is_account = is_account;

      _vm_api.send_inline = send_inline;
      _vm_api.send_context_free_inline = send_context_free_inline;
      _vm_api.publication_time = publication_time;
      _vm_api.current_receiver = current_receiver;
      _vm_api.get_active_producers = get_active_producers;

      _vm_api.assert_sha256 = assert_sha256;
      _vm_api.assert_sha1 = assert_sha1;
      _vm_api.assert_sha512 = assert_sha512;
      _vm_api.assert_ripemd160 = assert_ripemd160;
      _vm_api.assert_recover_key = assert_recover_key;
      _vm_api.sha256 = sha256;
      _vm_api.sha1 = sha1;
      _vm_api.sha512 = sha512;
      _vm_api.ripemd160 = ripemd160;
      _vm_api.recover_key = recover_key;
      _vm_api.sha3 = nullptr;

      _vm_api.db_store_i64 = db_store_i64;
      _vm_api.db_update_i64 = db_update_i64;
      _vm_api.db_remove_i64 = db_remove_i64;
      _vm_api.db_get_i64 = db_get_i64;
      _vm_api.db_next_i64 = db_next_i64;

      _vm_api.db_previous_i64 = db_previous_i64;
      _vm_api.db_find_i64 = db_find_i64;
      _vm_api.db_lowerbound_i64 = db_lowerbound_i64;
      _vm_api.db_upperbound_i64 = db_upperbound_i64;
      _vm_api.db_end_i64 = db_end_i64;

      _vm_api.db_idx64_store = db_idx64_store;
      _vm_api.db_idx64_update = db_idx64_update;

      _vm_api.db_idx64_remove = db_idx64_remove;
      _vm_api.db_idx64_next = db_idx64_next;
      _vm_api.db_idx64_previous = db_idx64_previous;
      _vm_api.db_idx64_find_primary = db_idx64_find_primary;
      _vm_api.db_idx64_find_secondary = db_idx64_find_secondary;
      _vm_api.db_idx64_lowerbound = db_idx64_lowerbound;
      _vm_api.db_idx64_upperbound = db_idx64_upperbound;
      _vm_api.db_idx64_end = db_idx64_end;
      _vm_api.db_idx128_store = db_idx128_store;

      _vm_api.db_idx128_update = db_idx128_update;
      _vm_api.db_idx128_remove = db_idx128_remove;
      _vm_api.db_idx128_next = db_idx128_next;
      _vm_api.db_idx128_previous = db_idx128_previous;
      _vm_api.db_idx128_find_primary = db_idx128_find_primary;
      _vm_api.db_idx128_find_secondary = db_idx128_find_secondary;
      _vm_api.db_idx128_lowerbound = db_idx128_lowerbound;
      _vm_api.db_idx128_upperbound = db_idx128_upperbound;

      _vm_api.db_idx128_end = db_idx128_end;
      _vm_api.db_idx256_store = db_idx256_store;
      _vm_api.db_idx256_update = db_idx256_update;
      _vm_api.db_idx256_remove = db_idx256_remove;
      _vm_api.db_idx256_next = db_idx256_next;

      _vm_api.db_idx256_previous = db_idx256_previous;
      _vm_api.db_idx256_find_primary = db_idx256_find_primary;
      _vm_api.db_idx256_find_secondary = db_idx256_find_secondary;
      _vm_api.db_idx256_lowerbound = db_idx256_lowerbound;
      _vm_api.db_idx256_upperbound = db_idx256_upperbound;
      _vm_api.db_idx256_end = db_idx256_end;
      _vm_api.db_idx_double_store = db_idx_double_store;
      _vm_api.db_idx_double_update = db_idx_double_update;
      _vm_api.db_idx_double_remove = db_idx_double_remove;
      _vm_api.db_idx_double_next = db_idx_double_next;
      _vm_api.db_idx_double_previous = db_idx_double_previous;
      _vm_api.db_idx_double_find_primary = db_idx_double_find_primary;
      _vm_api.db_idx_double_find_secondary = db_idx_double_find_secondary;
      _vm_api.db_idx_double_lowerbound = db_idx_double_lowerbound;
      _vm_api.db_idx_double_upperbound = db_idx_double_upperbound;
      _vm_api.db_idx_double_end = db_idx_double_end;
      _vm_api.db_idx_long_double_store = db_idx_long_double_store;
      _vm_api.db_idx_long_double_update = db_idx_long_double_update;
      _vm_api.db_idx_long_double_remove = db_idx_long_double_remove;
      _vm_api.db_idx_long_double_next = db_idx_long_double_next;
      _vm_api.db_idx_long_double_previous = db_idx_long_double_previous;
      _vm_api.db_idx_long_double_find_primary = db_idx_long_double_find_primary;
      _vm_api.db_idx_long_double_find_secondary = db_idx_long_double_find_secondary;
      _vm_api.db_idx_long_double_lowerbound = db_idx_long_double_lowerbound;
      _vm_api.db_idx_long_double_upperbound = db_idx_long_double_upperbound;
      _vm_api.db_idx_long_double_end = db_idx_long_double_end;

      _vm_api.check_transaction_authorization = check_transaction_authorization;
      _vm_api.check_permission_authorization = check_permission_authorization;
      _vm_api.get_permission_last_used = get_permission_last_used;
      _vm_api.get_account_creation_time = get_account_creation_time;

      _vm_api.prints = prints;
      _vm_api.prints_l = prints_l;
      _vm_api.printi = printi;
      _vm_api.printui = printui;
      _vm_api.printi128 = printi128;
      _vm_api.printui128 = printui128;
      _vm_api.printsf = printsf;
      _vm_api.printdf = printdf;
      _vm_api.printqf = printqf;
      _vm_api.printn = printn;
      _vm_api.printhex = printhex;

      _vm_api.set_resource_limits = set_resource_limits;
      _vm_api.get_resource_limits = get_resource_limits;
      _vm_api.set_proposed_producers = set_proposed_producers;
      _vm_api.is_privileged = is_privileged;
      _vm_api.set_privileged = set_privileged;
      _vm_api.set_blockchain_parameters_packed = set_blockchain_parameters_packed;
      _vm_api.get_blockchain_parameters_packed = get_blockchain_parameters_packed;
      _vm_api.activate_feature = activate_feature;

      _vm_api.eosio_abort = eosio_abort;
      _vm_api.eosio_assert = eosio_assert;
      _vm_api.eosio_assert_message = eosio_assert_message;
      _vm_api.eosio_assert_code = eosio_assert_code;
      _vm_api.eosio_exit = eosio_exit;
      _vm_api.current_time = current_time;
      _vm_api.now = now;

      _vm_api.checktime = checktime;
      _vm_api.check_context_free = check_context_free;

      _vm_api.send_deferred = _send_deferred;
      _vm_api.cancel_deferred = _cancel_deferred;
      _vm_api.read_transaction = read_transaction;
      _vm_api.transaction_size = transaction_size;

      _vm_api.tapos_block_num = tapos_block_num;
      _vm_api.tapos_block_prefix = tapos_block_prefix;
      _vm_api.expiration = expiration;
      _vm_api.get_action = get_action;

      _vm_api.assert_privileged = assert_privileged;
      _vm_api.assert_context_free = assert_context_free;
      _vm_api.get_context_free_data = get_context_free_data;


      _vm_api.token_create = token_create;
      _vm_api.token_issue = token_issue;
      _vm_api.token_transfer = token_transfer;
   }
   vm_register_api(&_vm_api);
}

}}
