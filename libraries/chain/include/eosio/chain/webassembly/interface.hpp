#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm.hpp>
#include <fc/crypto/sha1.hpp>

namespace eosio { namespace chain { namespace webassembly {

   class interface;

   template <auto HostFunction>
   struct host_function_registrator {
      constexpr host_function_registrator(const std::string& mod_name, const std::string& fn_name) {
         using rhf_t = eosio::vm::registered_host_functions<interface>;
         rhf_t::add<interface, HostFunction, eosio::vm::wasm_allocator>(mod_name, fn_name);
      }
   };

#define DECLARE_HOST_FUNCTION(RET_TY, NAME, ...) \
   RET_TY NAME (__VA_ARGS__);                   \
   inline static host_function_registrator<&interface::NAME> NAME ## _registrator = {"env", #NAME};

#define DECLARE_INJECTED_HOST_FUNCTION(RET_TY, NAME, ...) \
   RET_TY NAME (__VA_ARGS__);                            \
   inline static host_function_registrator<&interface::NAME> NAME ## _registrator = {"eosio_injected", #NAME};

   class interface {
      public:
         inline explicit interface(apply_context& ctx) : context(ctx) {}

         // checktime api
         DECLARE_INJECTED_HOST_FUNCTION(void, checktime)

         // context free api
         DECLARE_HOST_FUNCTION(int, get_context_free_data, uint32_t, legacy_array_ptr<char>, uint32_t)

         // privileged api
         DECLARE_HOST_FUNCTION(int,      is_feature_active,                int64_t)
         DECLARE_HOST_FUNCTION(void,     activate_feature,                 int64_t)
         DECLARE_HOST_FUNCTION(void,     preactivate_feature,              const digest_type&)
         DECLARE_HOST_FUNCTION(void,     set_resource_limits,              account_name, int64_t, int64_t, int64_t)
         DECLARE_HOST_FUNCTION(void,     get_resource_limits,              account_name, int64_t&, int64_t&, int64_t&)
         DECLARE_HOST_FUNCTION(int64_t,  set_proposed_producers,           legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(int64_t,  set_proposed_producers_ex,        uint64_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(uint32_t, get_blockchain_parameters_packed, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(void,     set_blockchain_parameters_packed, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(bool,     is_privileged,                    account_name)
         DECLARE_HOST_FUNCTION(void,     set_privileged,                   account_name, bool)

         // softfloat api
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_add,        float, float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_sub,        float, float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_div,        float, float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_mul,        float, float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_min,        float, float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_max,        float, float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_copysign,   float, float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_abs,        float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_neg,        float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_sqrt,       float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_ceil,       float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_floor,      float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_trunc,      float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f32_nearest,    float)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f32_eq,         float, float)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f32_ne,         float, float)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f32_lt,         float, float)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f32_le,         float, float)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f32_gt,         float, float)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f32_ge,         float, float)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_add,        double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_sub,        double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_div,        double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_mul,        double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_min,        double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_max,        double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_copysign,   double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_abs,        double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_neg,        double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_sqrt,       double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_ceil,       double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_floor,      double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_trunc,      double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f64_nearest,    double)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f64_eq,         double, double)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f64_ne,         double, double)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f64_lt,         double, double)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f64_le,         double, double)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f64_gt,         double, double)
         DECLARE_INJECTED_HOST_FUNCTION(bool,     _eosio_f64_ge,         double, double)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_f32_promote,    float)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_f64_demote,     double)
         DECLARE_INJECTED_HOST_FUNCTION(int32_t,  _eosio_f32_trunc_i32s, float)
         DECLARE_INJECTED_HOST_FUNCTION(int32_t,  _eosio_f64_trunc_i32s, double)
         DECLARE_INJECTED_HOST_FUNCTION(uint32_t, _eosio_f32_trunc_i32u, float)
         DECLARE_INJECTED_HOST_FUNCTION(uint32_t, _eosio_f64_trunc_i32u, double)
         DECLARE_INJECTED_HOST_FUNCTION(int64_t,  _eosio_f32_trunc_i64s, float)
         DECLARE_INJECTED_HOST_FUNCTION(int64_t,  _eosio_f64_trunc_i64s, double)
         DECLARE_INJECTED_HOST_FUNCTION(uint64_t, _eosio_f32_trunc_i64u, float)
         DECLARE_INJECTED_HOST_FUNCTION(uint64_t, _eosio_f64_trunc_i64u, double)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_i32_to_f32,     int32_t)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_i64_to_f32,     int64_t)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_ui32_to_f32,    uint32_t)
         DECLARE_INJECTED_HOST_FUNCTION(float,    _eosio_ui64_to_f32,    uint64_t)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_i32_to_f64,     int32_t)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_i64_to_f64,     int64_t)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_ui32_to_f64,    uint32_t)
         DECLARE_INJECTED_HOST_FUNCTION(double,   _eosio_ui64_to_f64,    uint64_t)

         // producer api
         DECLARE_HOST_FUNCTION(int32_t, get_active_producers, legacy_array_ptr<account_name>, uint32_t)

         // crypto api
         DECLARE_HOST_FUNCTION(void,    assert_recover_key, const fc::sha256&, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(int32_t, recover_key,        const fc::sha256&, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(void,    assert_sha256,      legacy_array_ptr<char>, uint32_t, const fc::sha256&)
         DECLARE_HOST_FUNCTION(void,    assert_sha1,        legacy_array_ptr<char>, uint32_t, const fc::sha1&)
         DECLARE_HOST_FUNCTION(void,    assert_sha512,      legacy_array_ptr<char>, uint32_t, const fc::sha512&)
         DECLARE_HOST_FUNCTION(void,    assert_ripemd160,   legacy_array_ptr<char>, uint32_t, const fc::ripemd160&)
         DECLARE_HOST_FUNCTION(void,    sha256,             legacy_array_ptr<char>, uint32_t, fc::sha256&)
         DECLARE_HOST_FUNCTION(void,    sha1,               legacy_array_ptr<char>, uint32_t, fc::sha1&)
         DECLARE_HOST_FUNCTION(void,    sha512,             legacy_array_ptr<char>, uint32_t, fc::sha512&)
         DECLARE_HOST_FUNCTION(void,    ripemd160,          legacy_array_ptr<char>, uint32_t, fc::ripemd160&)

         // permission api
         DECLARE_HOST_FUNCTION(bool, check_transaction_authorization, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(bool, check_permission_authorization, account_name, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t, uint64_t)
         DECLARE_HOST_FUNCTION(int64_t, get_permission_last_used, account_name, permission_name)
         DECLARE_HOST_FUNCTION(int64_t, get_account_creation_time, account_name)

         // authorization api
         DECLARE_HOST_FUNCTION(void, require_auth,      account_name)
         DECLARE_HOST_FUNCTION(void, require_auth2,     account_name, permission_name)
         DECLARE_HOST_FUNCTION(void, has_auth,          account_name)
         DECLARE_HOST_FUNCTION(void, require_recipient, account_name)
         DECLARE_HOST_FUNCTION(void, is_account,        account_name)

         // system api
         DECLARE_HOST_FUNCTION(uint64_t, current_time)
         DECLARE_HOST_FUNCTION(uint64_t, publication_time)
         DECLARE_HOST_FUNCTION(bool,     is_feature_activated, const digest_type&)
         DECLARE_HOST_FUNCTION(name,     get_sender)

         // context free system api
         DECLARE_HOST_FUNCTION(void, abort)
         DECLARE_HOST_FUNCTION(void, eosio_assert, bool, null_terminated_ptr)
         DECLARE_HOST_FUNCTION(void, eosio_assert_message, bool, legacy_array_ptr<const char>, uint32_t)
         DECLARE_HOST_FUNCTION(void, eosio_assert_code, bool, uint64_t)
         DECLARE_HOST_FUNCTION(void, eosio_exit, uint32_t)

         // action api
         DECLARE_HOST_FUNCTION(int32_t, read_action_data,        legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(int32_t, action_data_size)
         DECLARE_HOST_FUNCTION(name,    current_receiver)
         DECLARE_HOST_FUNCTION(void,    set_action_return_value, legacy_array_ptr<char>, uint32_t)

         // console api
         DECLARE_HOST_FUNCTION(void, prints, null_terminated_ptr)
         DECLARE_HOST_FUNCTION(void, prints_l, legacy_array_ptr<const char>, uint32_t)
         DECLARE_HOST_FUNCTION(void, printi, int64_t)
         DECLARE_HOST_FUNCTION(void, printui, uint64_t)
         DECLARE_HOST_FUNCTION(void, printi128, const __int128&)
         DECLARE_HOST_FUNCTION(void, printui128, const unsigned __int128&)
         DECLARE_HOST_FUNCTION(void, printsf, float)
         DECLARE_HOST_FUNCTION(void, printdf, double)
         DECLARE_HOST_FUNCTION(void, printqf, const float128_t&)
         DECLARE_HOST_FUNCTION(void, printn, name)
         DECLARE_HOST_FUNCTION(void, printhex, legacy_array_ptr<const char>, uint32_t)

         // database api
         DECLARE_HOST_FUNCTION(int32_t, db_store_i64,      uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const char>, uint32_t)
         DECLARE_HOST_FUNCTION(void,    db_update_i64,     int32_t, uint64_t, legacy_array_ptr<const char>, uint32_t)
         DECLARE_HOST_FUNCTION(void,    db_remove_i64,     int32_t)
         DECLARE_HOST_FUNCTION(int32_t, db_get_i64,        int32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(int32_t, db_next_i64,       int32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_previous_i64,   int32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_find_i64,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_lowerbound_i64, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_upperbound_i64, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_end_i64,        uint64_t, uint64_t, uint64_t)

         DECLARE_HOST_FUNCTION(int32_t, db_idx64_store,          uint64_t, uint64_t, uint64_t, uint64_t, const uint64_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx64_update,         int32_t, uint64_t, const uint64_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx64_remove,         int32_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx64_find_secondary, uint64_t, uint64_t, uint64_t, const uint64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx64_find_primary,   uint64_t, uint64_t, uint64_t, uint64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx64_lowerbound,     uint64_t, uint64_t, uint64_t, uint64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx64_upperbound,     uint64_t, uint64_t, uint64_t, uint64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx64_end,            uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx64_next,           int32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx64_previous,       int32_t, uint64_t&)

         DECLARE_HOST_FUNCTION(int32_t, db_idx128_store,          uint64_t, uint64_t, uint64_t, uint64_t, const uint128_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx128_update,         int32_t, uint64_t, const uint128_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx128_remove,         int32_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx128_find_secondary, uint64_t, uint64_t, uint64_t, const uint128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx128_find_primary,   uint64_t, uint64_t, uint64_t, uint128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx128_lowerbound,     uint64_t, uint64_t, uint64_t, uint128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx128_upperbound,     uint64_t, uint64_t, uint64_t, uint128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx128_end,            uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx128_next,           int32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx128_previous,       int32_t, uint64_t&)

         DECLARE_HOST_FUNCTION(int32_t, db_idx256_store,          uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t)
         DECLARE_HOST_FUNCTION(void,    db_idx256_update,         int32_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t)
         DECLARE_HOST_FUNCTION(void,    db_idx256_remove,         int32_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx256_find_secondary, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx256_find_primary,   uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx256_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx256_upperbound,     uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx256_end,            uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx256_next,           int32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx256_previous,       int32_t, uint64_t&)

         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_store,          uint64_t, uint64_t, uint64_t, uint64_t, const float64_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx_double_update,         int32_t, uint64_t, const float64_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx_double_remove,         int32_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_find_secondary, uint64_t, uint64_t, uint64_t, const float64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_find_primary,   uint64_t, uint64_t, uint64_t, float64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_lowerbound,     uint64_t, uint64_t, uint64_t, float64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_upperbound,     uint64_t, uint64_t, uint64_t, float64_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_end,            uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_next,           int32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_double_previous,       int32_t, uint64_t&)

         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_store,          uint64_t, uint64_t, uint64_t, uint64_t, const float128_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx_long_double_update,         int32_t, uint64_t, const float128_t&)
         DECLARE_HOST_FUNCTION(void,    db_idx_long_double_remove,         int32_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_find_secondary, uint64_t, uint64_t, uint64_t, const float128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_find_primary,   uint64_t, uint64_t, uint64_t, float128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_lowerbound,     uint64_t, uint64_t, uint64_t, float128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_upperbound,     uint64_t, uint64_t, uint64_t, float128_t&, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_end,            uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_next,           int32_t, uint64_t&)
         DECLARE_HOST_FUNCTION(int32_t, db_idx_long_double_previous,       int32_t, uint64_t&)

         // memory api
         DECLARE_HOST_FUNCTION(char*,   memcpy,  legacy_array_ptr<char>, legacy_array_ptr<const char>, uint32_t)
         DECLARE_HOST_FUNCTION(char*,   memmove, legacy_array_ptr<char>, legacy_array_ptr<const char>, uint32_t)
         DECLARE_HOST_FUNCTION(int32_t, memcmp,  legacy_array_ptr<const char>, legacy_array_ptr<const char>, uint32_t)
         DECLARE_HOST_FUNCTION(char*,   memset,  legacy_array_ptr<char>, uint32_t)

         // transaction api
         DECLARE_HOST_FUNCTION(void, send_inline,              legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(void, send_context_free_inline, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(void, send_deferred,            const uint128_t&, account_name, legacy_array_ptr<char>, uint32_t, uint32_t)
         DECLARE_HOST_FUNCTION(void, cancel_deferred,          const uint128_t&)

         // context free transaction api
         DECLARE_HOST_FUNCTION(int32_t, read_transaction, legacy_array_ptr<char>, uint32_t)
         DECLARE_HOST_FUNCTION(int32_t, transaction_size)
         DECLARE_HOST_FUNCTION(int32_t, expiration)
         DECLARE_HOST_FUNCTION(int32_t, tapos_block_num)
         DECLARE_HOST_FUNCTION(int32_t, tapos_block_prefix)
         DECLARE_HOST_FUNCTION(int32_t, get_action, uint32_t, uint32_t, legacy_array_ptr<char>, uint32_t)

         // compiler builtins
         DECLARE_HOST_FUNCTION(void,     __ashlti3,     __int128&, uint64_t, uint64_t, uint32_t)
         DECLARE_HOST_FUNCTION(void,     __ashrti3,     __int128&, uint64_t, uint64_t, uint32_t)
         DECLARE_HOST_FUNCTION(void,     __lshlti3,     __int128&, uint64_t, uint64_t, uint32_t)
         DECLARE_HOST_FUNCTION(void,     __lshrti3,     __int128&, uint64_t, uint64_t, uint32_t)
         DECLARE_HOST_FUNCTION(void,     __divti3,      __int128&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __udivti3,     __int128&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __multi3,      __int128&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __modti3,      __int128&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __umodti3,     __int128&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __addtf3,      float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __subtf3,      float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __multf3,      float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __divtf3,      float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __negtf2,      float128_t&, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __extendsftf2, float128_t&, float)
         DECLARE_HOST_FUNCTION(void,     __extenddftf2, float128_t&, double)
         DECLARE_HOST_FUNCTION(double,   __trunctfdf2,  uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(float,    __trunctfsf2,  uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int32_t,  __fixtfsi,     uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int64_t,  __fixtfdi,     uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __fixtfti,     __int128&, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(uint32_t, __fixunstfsi,  uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(uint64_t, __fixunstfdi,  uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __fixunstfti,  unsigned __int128&, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(void,     __fixsfti,     __int128&, float)
         DECLARE_HOST_FUNCTION(void,     __fixdfti,     __int128&, double)
         DECLARE_HOST_FUNCTION(void,     __fixunssfti,  unsigned __int128&, float)
         DECLARE_HOST_FUNCTION(void,     __fixunsdfti,  unsigned __int128&, double)
         DECLARE_HOST_FUNCTION(double,   __floatsidf,   int32_t)
         DECLARE_HOST_FUNCTION(void,     __floatsitf,   float128_t&, int32_t)
         DECLARE_HOST_FUNCTION(void,     __floatditf,   float128_t&, int64_t)
         DECLARE_HOST_FUNCTION(void,     __floatunsitf, float128_t&, uint32_t)
         DECLARE_HOST_FUNCTION(void,     __floatunditf, float128_t&, uint64_t)
         DECLARE_HOST_FUNCTION(double,   __floattidf,   uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(double,   __floatuntidf, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __cmptf2,      uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __eqtf2,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __netf2,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __getf2,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __gttf2,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __letf2,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __lttf2,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_HOST_FUNCTION(int,      __unordtf2,    uint64_t, uint64_t, uint64_t, uint64_t)

         // call depth api
         DECLARE_INJECTED_HOST_FUNCTION(void, call_depth_assert)

      private:
         apply_context& context;
   };

}}} // ns eosio::chain::webassembly
