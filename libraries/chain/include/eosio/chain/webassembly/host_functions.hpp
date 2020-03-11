#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/vm/allocator.hpp>
#include <eosio/vm/host_function.hpp>

namespace eosio { namespace chain { namespace webassembly {
   class interface; // forward declaration

   template <auto HostFunction>
   struct host_function_registrator {
      host_function_registrator( const std::string& mod_name, const std::string& fn_name ) {
         using rhf_t = eosio::vm::registered_host_functions<interface>;
         rhf_t::add<interface, HostFunction, eosio::vm::wasm_allocator>(mod_name, fn_name);
      }
   };

}}} // ns eosio::chain::webassembly

#define EOSIO_REGISTER_HOST_FUNCTION(IS_INJECTED, RET_TY, HF_NAME, ...) \
   host_function_registrator<&interface::HF_NAME> eosio_host_function_ ## HF_NAME = {IS_INJECTED ? "eosio_injected" : "env", #HF_NAME};

#define EOSIO_DECLARE_HOST_FUNCTION(_, RET_TY, HF_NAME, ...) \
   RET_TY HF_NAME(__VA_ARGS__);

// HOST_FUNCTION_MACRO(false,  IS INJECTED, PREDICATE, RETURN TYPE, INTRINSIC NAME, ...
// /* Arg types */

#define EOSIO_HOST_FUNCTIONS(HOST_FUNCTION_MACRO)                              \
  /* checktime api */                                                          \
  HOST_FUNCTION_MACRO(false, void, checktime)                                  \
                                                                               \
  /* context free api */                                                       \
  HOST_FUNCTION_MACRO(false, int, get_context_free_data, uint32_t,             \
                      legacy_array_ptr<char>, uint32_t)                        \
                                                                               \
  /* privileged api */                                                         \
  HOST_FUNCTION_MACRO(false, int, is_feature_active, int64_t)                  \
  HOST_FUNCTION_MACRO(false, void, activate_feature, int64_t)                  \
  HOST_FUNCTION_MACRO(false, void, preactivate_feature, const digest_type &)   \
  HOST_FUNCTION_MACRO(false, void, set_resource_limits, account_name, int64_t, \
                      int64_t, int64_t)                                        \
  HOST_FUNCTION_MACRO(false, void, get_resource_limits, account_name,          \
                      int64_t &, int64_t &, int64_t &)                         \
  HOST_FUNCTION_MACRO(false, int64_t, set_proposed_producers,                  \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, int64_t, set_proposed_producers_ex, uint64_t,     \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, uint32_t, get_blockchain_parameters_packed,       \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, void, set_blockchain_parameters_packed,           \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, bool, is_privileged, account_name)                \
  HOST_FUNCTION_MACRO(false, void, set_privileged, account_name, bool)         \
                                                                               \
  /* softfloat api */                                                          \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_add, float, float)               \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_sub, float, float)               \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_div, float, float)               \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_mul, float, float)               \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_min, float, float)               \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_max, float, float)               \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_copysign, float, float)          \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_abs, float)                      \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_neg, float)                      \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_sqrt, float)                     \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_ceil, float)                     \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_floor, float)                    \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_trunc, float)                    \
  HOST_FUNCTION_MACRO(true, float, _eosio_f32_nearest, float)                  \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f32_eq, float, float)                 \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f32_ne, float, float)                 \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f32_lt, float, float)                 \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f32_le, float, float)                 \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f32_gt, float, float)                 \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f32_ge, float, float)                 \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_add, double, double)            \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_sub, double, double)            \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_div, double, double)            \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_mul, double, double)            \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_min, double, double)            \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_max, double, double)            \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_copysign, double, double)       \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_abs, double)                    \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_neg, double)                    \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_sqrt, double)                   \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_ceil, double)                   \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_floor, double)                  \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_trunc, double)                  \
  HOST_FUNCTION_MACRO(true, double, _eosio_f64_nearest, double)                \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f64_eq, double, double)               \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f64_ne, double, double)               \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f64_lt, double, double)               \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f64_le, double, double)               \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f64_gt, double, double)               \
  HOST_FUNCTION_MACRO(true, bool, _eosio_f64_ge, double, double)               \
  HOST_FUNCTION_MACRO(true, double, _eosio_f32_promote, float)                 \
  HOST_FUNCTION_MACRO(true, float, _eosio_f64_demote, double)                  \
  HOST_FUNCTION_MACRO(true, int32_t, _eosio_f32_trunc_i32s, float)             \
  HOST_FUNCTION_MACRO(true, int32_t, _eosio_f64_trunc_i32s, double)            \
  HOST_FUNCTION_MACRO(true, uint32_t, _eosio_f32_trunc_i32u, float)            \
  HOST_FUNCTION_MACRO(true, uint32_t, _eosio_f64_trunc_i32u, double)           \
  HOST_FUNCTION_MACRO(true, int64_t, _eosio_f32_trunc_i64s, float)             \
  HOST_FUNCTION_MACRO(true, int64_t, _eosio_f64_trunc_i64s, double)            \
  HOST_FUNCTION_MACRO(true, uint64_t, _eosio_f32_trunc_i64u, float)            \
  HOST_FUNCTION_MACRO(true, uint64_t, _eosio_f64_trunc_i64u, double)           \
  HOST_FUNCTION_MACRO(true, float, _eosio_i32_to_f32, int32_t)                 \
  HOST_FUNCTION_MACRO(true, float, _eosio_i64_to_f32, int64_t)                 \
  HOST_FUNCTION_MACRO(true, float, _eosio_ui32_to_f32, uint32_t)               \
  HOST_FUNCTION_MACRO(true, float, _eosio_ui64_to_f32, uint64_t)               \
  HOST_FUNCTION_MACRO(true, double, _eosio_i32_to_f64, int32_t)                \
  HOST_FUNCTION_MACRO(true, double, _eosio_i64_to_f64, int64_t)                \
  HOST_FUNCTION_MACRO(true, double, _eosio_ui32_to_f64, uint32_t)              \
  HOST_FUNCTION_MACRO(true, double, _eosio_ui64_to_f64, uint64_t)              \
                                                                               \
  /* producer api */                                                           \
  HOST_FUNCTION_MACRO(false, int32_t, get_active_producers,                    \
                      legacy_array_ptr<account_name>, uint32_t)                \
                                                                               \
  /* crypto api */                                                             \
  HOST_FUNCTION_MACRO(false, void, assert_recover_key, const sha256 &,         \
                      legacy_array_ptr<char>, uint32_t,                        \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, int32_t, recover_key, const sha256 &,             \
                      legacy_array_ptr<char>, uint32_t,                        \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, void, assert_sha256, legacy_array_ptr<char>,      \
                      uint32_t, const sha256 &)                                \
  HOST_FUNCTION_MACRO(false, void, assert_sha1, legacy_array_ptr<char>,        \
                      uint32_t, const sha1 &)                                  \
  HOST_FUNCTION_MACRO(false, void, assert_sha512, legacy_array_ptr<char>,      \
                      uint32_t, const sha512 &)                                \
  HOST_FUNCTION_MACRO(false, void, assert_ripemd160, legacy_array_ptr<char>,   \
                      uint32_t, const ripemd160 &)                             \
  HOST_FUNCTION_MACRO(false, void, sha256, legacy_array_ptr<char>, uint32_t,   \
                      sha256 &)                                                \
  HOST_FUNCTION_MACRO(false, void, sha1, legacy_array_ptr<char>, uint32_t,     \
                      sha1 &)                                                  \
  HOST_FUNCTION_MACRO(false, void, sha512, legacy_array_ptr<char>, uint32_t,   \
                      sha512 &)                                                \
  HOST_FUNCTION_MACRO(false, void, ripemd160, legacy_array_ptr<char>,          \
                      uint32_t, ripemd160 &)                                   \
                                                                               \
  /* permission api */                                                         \
  HOST_FUNCTION_MACRO(false, bool, check_transaction_authorization,            \
                      legacy_array_ptr<char>, uint32_t,                        \
                      legacy_array_ptr<char>, uint32_t,                        \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, bool, check_permission_authorization,             \
                      account_name, legacy_array_ptr<char>, uint32_t,          \
                      legacy_array_ptr<char>, uint32_t,                        \
                      legacy_array_ptr<char>, uint32_t, uint64_t)              \
  HOST_FUNCTION_MACRO(false, int64_t, get_permission_last_used, account_name,  \
                      permission_name)                                         \
  HOST_FUNCTION_MACRO(false, int64_t, get_account_creation_time, account_name) \
                                                                               \
  /* authorization api */                                                      \
  HOST_FUNCTION_MACRO(false, void, require_auth, account_name)                 \
  HOST_FUNCTION_MACRO(false, void, require_auth2, account_name,                \
                      permission_name)                                         \
  HOST_FUNCTION_MACRO(false, void, has_auth, account_name)                     \
  HOST_FUNCTION_MACRO(false, void, require_recipient, account_name)            \
  HOST_FUNCTION_MACRO(false, void, is_account, account_name)                   \
                                                                               \
  /* system api */                                                             \
  HOST_FUNCTION_MACRO(false, uint64_t, current_time)                           \
  HOST_FUNCTION_MACRO(false, uint64_t, publication_time)                       \
  HOST_FUNCTION_MACRO(false, bool, is_feature_activated, const digest_type &)  \
  HOST_FUNCTION_MACRO(false, name, get_sender)                                 \
                                                                               \
  /* context free system api */                                                \
  HOST_FUNCTION_MACRO(false, void, abort)                                      \
  HOST_FUNCTION_MACRO(false, void, eosio_assert, bool, null_terminated_ptr)    \
  HOST_FUNCTION_MACRO(false, void, eosio_assert_message, bool,                 \
                      legacy_array_ptr<const char>, uint32_t)                  \
  HOST_FUNCTION_MACRO(false, void, eosio_assert_code, bool, uint64_t)          \
  HOST_FUNCTION_MACRO(false, void, eosio_exit, uint32_t)                       \
                                                                               \
  /* action api */                                                             \
  HOST_FUNCTION_MACRO(false, int32_t, read_action_data,                        \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, int32_t, action_data_size)                        \
  HOST_FUNCTION_MACRO(false, name, current_receiver)                           \
  HOST_FUNCTION_MACRO(false, void, set_action_return_value,                    \
                      legacy_array_ptr<char>, uint32_t)                        \
                                                                               \
  /* console api */                                                            \
  HOST_FUNCTION_MACRO(false, void, prints, null_terminated_ptr)                \
  HOST_FUNCTION_MACRO(false, void, prints_l, legacy_array_ptr<const char>,     \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, printi, int64_t)                            \
  HOST_FUNCTION_MACRO(false, void, printui, uint64_t)                          \
  HOST_FUNCTION_MACRO(false, void, printi128, const __int128 &)                \
  HOST_FUNCTION_MACRO(false, void, printui128, const unsigned __int128 &)      \
  HOST_FUNCTION_MACRO(false, void, printsf, float)                             \
  HOST_FUNCTION_MACRO(false, void, printdf, double)                            \
  HOST_FUNCTION_MACRO(false, void, printqf, const float128_t &)                \
  HOST_FUNCTION_MACRO(false, void, printn, name)                               \
  HOST_FUNCTION_MACRO(false, void, printhex, legacy_array_ptr<const char>,     \
                      uint32_t)                                                \
                                                                               \
  /* database api */                                                           \
  HOST_FUNCTION_MACRO(false, int32_t, db_store_i64, uint64_t, uint64_t,        \
                      uint64_t, uint64_t, legacy_array_ptr<const char>,        \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, db_update_i64, int32_t, uint64_t,           \
                      legacy_array_ptr<const char>, uint32_t)                  \
  HOST_FUNCTION_MACRO(false, void, db_remove_i64, int32_t)                     \
  HOST_FUNCTION_MACRO(false, int32_t, db_get_i64, int32_t,                     \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, int32_t, db_next_i64, int32_t, uint64_t &)        \
  HOST_FUNCTION_MACRO(false, int32_t, db_previous_i64, int32_t, uint64_t &)    \
  HOST_FUNCTION_MACRO(false, int32_t, db_find_i64, uint64_t, uint64_t,         \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, int32_t, db_lowerbound_i64, uint64_t, uint64_t,   \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, int32_t, db_upperbound_i64, uint64_t, uint64_t,   \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, int32_t, db_end_i64, uint64_t, uint64_t,          \
                      uint64_t)                                                \
                                                                               \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_store, uint64_t, uint64_t,      \
                      uint64_t, uint64_t, const uint64_t &)                    \
  HOST_FUNCTION_MACRO(false, void, db_idx64_update, int32_t, uint64_t,         \
                      const uint64_t &)                                        \
  HOST_FUNCTION_MACRO(false, void, db_idx64_remove, int32_t)                   \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_find_secondary, uint64_t,       \
                      uint64_t, uint64_t, const uint64_t &, uint64_t &)        \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_find_primary, uint64_t,         \
                      uint64_t, uint64_t, uint64_t &, uint64_t &)              \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_lowerbound, uint64_t, uint64_t, \
                      uint64_t, uint64_t &, uint64_t &)                        \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_upperbound, uint64_t, uint64_t, \
                      uint64_t, uint64_t &, uint64_t &)                        \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_end, uint64_t, uint64_t,        \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_next, int32_t, uint64_t &)      \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx64_previous, int32_t, uint64_t &)  \
                                                                               \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_store, uint64_t, uint64_t,     \
                      uint64_t, uint64_t, const uint128_t &)                   \
  HOST_FUNCTION_MACRO(false, void, db_idx128_update, int32_t, uint64_t,        \
                      const uint128_t &)                                       \
  HOST_FUNCTION_MACRO(false, void, db_idx128_remove, int32_t)                  \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_find_secondary, uint64_t,      \
                      uint64_t, uint64_t, const uint128_t &, uint64_t &)       \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_find_primary, uint64_t,        \
                      uint64_t, uint64_t, uint128_t &, uint64_t &)             \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_lowerbound, uint64_t,          \
                      uint64_t, uint64_t, uint128_t &, uint64_t &)             \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_upperbound, uint64_t,          \
                      uint64_t, uint64_t, uint128_t &, uint64_t &)             \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_end, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_next, int32_t, uint64_t &)     \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx128_previous, int32_t, uint64_t &) \
                                                                               \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_store, uint64_t, uint64_t,     \
                      uint64_t, uint64_t, legacy_array_ptr<const uint128_t>,   \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, db_idx256_update, int32_t, uint64_t,        \
                      legacy_array_ptr<const uint128_t>, uint32_t)             \
  HOST_FUNCTION_MACRO(false, void, db_idx256_remove, int32_t)                  \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_find_secondary, uint64_t,      \
                      uint64_t, uint64_t, legacy_array_ptr<const uint128_t>,   \
                      uint32_t, uint64_t &)                                    \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_find_primary, uint64_t,        \
                      uint64_t, uint64_t, legacy_array_ptr<uint128_t>,         \
                      uint32_t, uint64_t &)                                    \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_lowerbound, uint64_t,          \
                      uint64_t, uint64_t, legacy_array_ptr<uint128_t>,         \
                      uint32_t, uint64_t &)                                    \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_upperbound, uint64_t,          \
                      uint64_t, uint64_t, legacy_array_ptr<uint128_t>,         \
                      uint32_t, uint64_t &)                                    \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_end, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_next, int32_t, uint64_t &)     \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx256_previous, int32_t, uint64_t &) \
                                                                               \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_store, uint64_t, uint64_t, \
                      uint64_t, uint64_t, const float64_t &)                   \
  HOST_FUNCTION_MACRO(false, void, db_idx_double_update, int32_t, uint64_t,    \
                      const float64_t &)                                       \
  HOST_FUNCTION_MACRO(false, void, db_idx_double_remove, int32_t)              \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_find_secondary, uint64_t,  \
                      uint64_t, uint64_t, const float64_t &, uint64_t &)       \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_find_primary, uint64_t,    \
                      uint64_t, uint64_t, float64_t &, uint64_t &)             \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_lowerbound, uint64_t,      \
                      uint64_t, uint64_t, float64_t &, uint64_t &)             \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_upperbound, uint64_t,      \
                      uint64_t, uint64_t, float64_t &, uint64_t &)             \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_end, uint64_t, uint64_t,   \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_next, int32_t, uint64_t &) \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_double_previous, int32_t,         \
                      uint64_t &)                                              \
                                                                               \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_store, uint64_t,      \
                      uint64_t, uint64_t, uint64_t, const float128_t &)        \
  HOST_FUNCTION_MACRO(false, void, db_idx_long_double_update, int32_t,         \
                      uint64_t, const float128_t &)                            \
  HOST_FUNCTION_MACRO(false, void, db_idx_long_double_remove, int32_t)         \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_find_secondary,       \
                      uint64_t, uint64_t, uint64_t, const float128_t &,        \
                      uint64_t &)                                              \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_find_primary,         \
                      uint64_t, uint64_t, uint64_t, float128_t &, uint64_t &)  \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_lowerbound, uint64_t, \
                      uint64_t, uint64_t, float128_t &, uint64_t &)            \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_upperbound, uint64_t, \
                      uint64_t, uint64_t, float128_t &, uint64_t &)            \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_end, uint64_t,        \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_next, int32_t,        \
                      uint64_t &)                                              \
  HOST_FUNCTION_MACRO(false, int32_t, db_idx_long_double_previous, int32_t,    \
                      uint64_t &)                                              \
                                                                               \
  /* memory api */                                                             \
  HOST_FUNCTION_MACRO(false, char *, memcpy, legacy_array_ptr<char>,           \
                      legacy_array_ptr<const char>, uint32_t)                  \
  HOST_FUNCTION_MACRO(false, char *, memmove, legacy_array_ptr<char>,          \
                      legacy_array_ptr<const char>, uint32_t)                  \
  HOST_FUNCTION_MACRO(false, int32_t, memcmp, legacy_array_ptr<const char>,    \
                      legacy_array_ptr<const char>, uint32_t)                  \
  HOST_FUNCTION_MACRO(false, char *, memset, legacy_array_ptr<char>, uint32_t) \
                                                                               \
  /* transaction api */                                                        \
  HOST_FUNCTION_MACRO(false, void, send_inline, legacy_array_ptr<char>,        \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, send_context_free_inline,                   \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, void, send_deferred, const uint128_t &,           \
                      account_name, legacy_array_ptr<char>, uint32_t,          \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, cancel_deferred, const uint128_t &)         \
                                                                               \
  /* context free transaction api */                                           \
  HOST_FUNCTION_MACRO(false, int32_t, read_transaction,                        \
                      legacy_array_ptr<char>, uint32_t)                        \
  HOST_FUNCTION_MACRO(false, int32_t, transaction_size)                        \
  HOST_FUNCTION_MACRO(false, int32_t, expiration)                              \
  HOST_FUNCTION_MACRO(false, int32_t, tapos_block_num)                         \
  HOST_FUNCTION_MACRO(false, int32_t, tapos_block_prefix)                      \
  HOST_FUNCTION_MACRO(false, int32_t, get_action, uint32_t, uint32_t,          \
                      legacy_array_ptr<char>, uint32_t)                        \
                                                                               \
  /* compiler builtins */                                                      \
  HOST_FUNCTION_MACRO(false, void, __ashlti3, __int128 &, uint64_t, uint64_t,  \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, __ashrti3, __int128 &, uint64_t, uint64_t,  \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, __lshlti3, __int128 &, uint64_t, uint64_t,  \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, __lshrti3, __int128 &, uint64_t, uint64_t,  \
                      uint32_t)                                                \
  HOST_FUNCTION_MACRO(false, void, __divti3, __int128 &, uint64_t, uint64_t,   \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, void, __udivti3, __int128 &, uint64_t, uint64_t,  \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, void, __multi3, __int128 &, uint64_t, uint64_t,   \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, void, __modti3, __int128 &, uint64_t, uint64_t,   \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, void, __umodti3, __int128 &, uint64_t, uint64_t,  \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, void, __addtf3, __float128_t &, uint64_t,         \
                      uint64_t, uint64_t, uint64_t)                            \
  HOST_FUNCTION_MACRO(false, void, __subtf3, __float128_t &, uint64_t,         \
                      uint64_t, uint64_t, uint64_t)                            \
  HOST_FUNCTION_MACRO(false, void, __multf3, __float128_t &, uint64_t,         \
                      uint64_t, uint64_t, uint64_t)                            \
  HOST_FUNCTION_MACRO(false, void, __divtf3, __float128_t &, uint64_t,         \
                      uint64_t, uint64_t, uint64_t)                            \
  HOST_FUNCTION_MACRO(false, void, __negtf2, __float128_t &, uint64_t,         \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, void, __extendsftf2, __float128_t &, float)       \
  HOST_FUNCTION_MACRO(false, void, __extenddftf2, __float128_t &, double)      \
  HOST_FUNCTION_MACRO(false, double, __trunctfdf2, uint64_t, uint64_t)         \
  HOST_FUNCTION_MACRO(false, float, __trunctfsf2, uint64_t, uint64_t)          \
  HOST_FUNCTION_MACRO(false, int32_t, __fixtfsi, uint64_t, uint64_t)           \
  HOST_FUNCTION_MACRO(false, int64_t, __fixtfdi, uint64_t, uint64_t)           \
  HOST_FUNCTION_MACRO(false, void, __fixtfti, __int128 &, uint64_t, uint64_t)  \
  HOST_FUNCTION_MACRO(false, uint32_t, __fixunstfsi, uint64_t, uint64_t)       \
  HOST_FUNCTION_MACRO(false, uint64_t, __fixunstfdi, uint64_t, uint64_t)       \
  HOST_FUNCTION_MACRO(false, void, __fixunstfti, unsigned __int128 &,          \
                      uint64_t, uint64_t)                                      \
  HOST_FUNCTION_MACRO(false, void, __fixsfti, __int128 &, float)               \
  HOST_FUNCTION_MACRO(false, void, __fixdfti, __int128 &, double)              \
  HOST_FUNCTION_MACRO(false, void, __fixunssfti, unsigned __int128 &, float)   \
  HOST_FUNCTION_MACRO(false, void, __fixunsdfti, unsigned __int128 &, double)  \
  HOST_FUNCTION_MACRO(false, double, __floatsidf, int32_t)                     \
  HOST_FUNCTION_MACRO(false, void, __floatsitf, float128_t &, int32_t)         \
  HOST_FUNCTION_MACRO(false, void, __floatditf, float128_t &, int64_t)         \
  HOST_FUNCTION_MACRO(false, void, __floatunsitf, float128_t &, uint32_t)      \
  HOST_FUNCTION_MACRO(false, void, __floatunditf, float128_t &, uint64_t)      \
  HOST_FUNCTION_MACRO(false, double, __floattidf, uint64_t, uint64_t)          \
  HOST_FUNCTION_MACRO(false, double, __floatuntidf, uint64_t, uint64_t)        \
  HOST_FUNCTION_MACRO(false, int, __cmptf2, uint64_t, uint64_t, uint64_t,      \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int, __eqtf2, uint64_t, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int, __netf2, uint64_t, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int, __getf2, uint64_t, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int, __gttf2, uint64_t, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int, __letf2, uint64_t, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int, __lttf2, uint64_t, uint64_t, uint64_t,       \
                      uint64_t)                                                \
  HOST_FUNCTION_MACRO(false, int, __unordtf2, uint64_t, uint64_t, uint64_t,    \
                      uint64_t)                                                \
                                                                               \
  /* call depth api */                                                         \
  HOST_FUNCTION_MACRO(true, void, call_depth_assert)                           \
