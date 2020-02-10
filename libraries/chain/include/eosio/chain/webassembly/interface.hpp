#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm.hpp>
#include <fc/crypto/sha1.hpp>

namespace eosio { namespace chain { namespace webassembly {

   class interface;

   template <auto HostFunction, typename... Preconditions>
   struct host_function_registrator {
      constexpr host_function_registrator(const std::string& mod_name, const std::string& fn_name) {
         using rhf_t = eosio::vm::registered_host_functions<interface>;
         rhf_t::add<HostFunction, Preconditions...>(mod_name, fn_name);
      }
   };

#define DECLARE_HOST_FUNCTION(MODULE, CONSTNESS, RET_TY, NAME, ...) \
   RET_TY NAME(__VA_ARGS__) CONSTNESS; \
   inline static host_function_registrator<&interface::NAME, alias_check, early_validate_pointers > NAME ## _registrator = {#MODULE, #NAME};

#define DECLARE_CF_HOST_FUNCTION(CONSTNESS, RET_TY, NAME, ...) \
   RET_TY NAME(__VA_ARGS__) CONSTNESS; \
   inline static host_function_registrator<&interface::NAME, alias_check, early_validate_pointers, context_free_check > NAME ## _registrator = {"env", #NAME};

#define DECLARE(RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(env, , RET_TY, NAME, __VA_ARGS__)

#define DECLARE_CF(RET_TY, NAME, ...) \
   DECLARE_CF_HOST_FUNCTION(env, , RET_TY, NAME, __VA_ARGS__)

#define DECLARE_CONST(RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(env, const, RET_TY, NAME, __VA_ARGS__)

#define DECLARE_CONST_CF(RET_TY, NAME, ...) \
   DECLARE_CF_HOST_FUNCTION(env, const, RET_TY, NAME, __VA_ARGS__)

#define DECLARE_INJECTED(RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(eosio_injected, , RET_TY, NAME, __VA_ARGS__)

#define DECLARE_INJECTED_CONST(RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(eosio_injected, const, RET_TY, NAME, __VA_ARGS__)


   // TODO maybe house these some place else
   template <typename T, typename U>
   inline static bool is_aliasing(const span<T>& a, const span<U>& b) {
      std::uintptr_t a_ui = reinterpret_cast<std::uintptr_t>(a.data());
      std::uintptr_t b_ui = reinterpret_cast<std::uintptr_t>(b.data());
      return a_ui < b_ui ? a_ui + a.size() >= b_ui : b_ui + b.size() >= a_ui;
   }
   inline static bool is_nan( const float32_t f ) {
      return f32_is_nan( f );
   }
   inline static bool is_nan( const float64_t f ) {
      return f64_is_nan( f );
   }
   inline static bool is_nan( const float128_t& f ) {
      return f128_is_nan( f );
   }

   EOS_VM_PRECONDITION(early_validate_pointers,
         EOS_VM_INVOKE_ON([&](auto arg, auto&&... rest) {
            if constexpr (is_reference_proxy_type<decltype(arg)>())
               if constexpr (is_span_type<dependent_type_t<decltype(arg)>) {
                  const auto& s = (dependent_type_t<decltype(arg)>)arg;
                  EOS_ASSERT( s.size() <= std::numeric_limits<wasm_size_t>::max() / (wasm_size_t)sizeof(T),
                        wasm_exception, "length will overflow" );
                  wasm_size_t bytes = len * sizeof(T);
                  volatile auto check = *(reinterpret_cast<const char*>(ptr) + bytes - 1);
                  ignore_unused_variable_warning(check);
               }
         }));

   EOS_VM_PRECONDITION(context_free_check,
         EOS_VM_INVOKE_ONCE([&](auto&&... args) {
            EOS_ASSERT(ctx.get_host()->context.is_context_free(), unaccessible_api, "only context free api's can be used in this context");
         }));

   EOS_VM_PRECONDITION(alias_check,
         EOS_VM_INVOKE_ON([&](auto arg, auto&&... rest) {
            using arg_type = decltype(arg);
            if constexpr (eosio::vm::is_span_type<arg_type>())
               invoke_on([&](auto arg, auto&&... rest) {
                  using arg_type = decltype(arg);
                  if constexpr (eosio::vm::is_span_type<arg_type())
                     EOS_ASSERT(!is_aliasing, wasm_exception, "arrays not allowed to alias");
               }, rest...);
         }));

   EOS_VM_PRECONDITION(is_nan_check,
         EOS_VM_INVOKE_ON([&](auto arg, auto&&.. rest) {
            if constexpr (std::is_same_v<decltype(arg), float> || std::is_same_v<decltype(arg), double> || std::is_same_v<decltype(arg), float128_t>)
               EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         }));

   class interface {
      public:
         // checktime api
         DECLARE_INJECTED(void, checktime)

         // context free api
         DECLARE_CONST_CF(int32_t, get_context_free_data, uint32_t, legacy_array_ptr<char>, uint32_t)

         // privileged api
         DECLARE_CONST(int32_t,  is_feature_active,                int64_t)
         DECLARE_CONST(void,     activate_feature,                 int64_t)
         DECLARE(void,           preactivate_feature,              const digest_type&)
         DECLARE(void,           set_resource_limits,              account_name, int64_t, int64_t, int64_t)
         DECLARE_CONST(void,     get_resource_limits,              account_name, legacy_ptr<int64_t>, legacy_ptr<int64_t>, legacy_ptr<int64_t>)
         DECLARE(int64_t,        set_proposed_producers,           legacy_array_ptr<char>, uint32_t)
         DECLARE(int64_t,        set_proposed_producers_ex,        uint64_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_CONST(uint32_t, get_blockchain_parameters_packed, legacy_array_ptr<char>, uint32_t)
         DECLARE(void,           set_blockchain_parameters_packed, legacy_array_ptr<char>, uint32_t)
         DECLARE_CONST(bool,     is_privileged,                    account_name)
         DECLARE(void,           set_privileged,                   account_name, bool)

         // softfloat api
         DECLARE_INJECTED_CONST(float,    _eosio_f32_add,        float, float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_sub,        float, float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_div,        float, float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_mul,        float, float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_min,        float, float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_max,        float, float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_copysign,   float, float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_abs,        float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_neg,        float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_sqrt,       float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_ceil,       float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_floor,      float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_trunc,      float)
         DECLARE_INJECTED_CONST(float,    _eosio_f32_nearest,    float)
         DECLARE_INJECTED_CONST(bool,     _eosio_f32_eq,         float, float)
         DECLARE_INJECTED_CONST(bool,     _eosio_f32_ne,         float, float)
         DECLARE_INJECTED_CONST(bool,     _eosio_f32_lt,         float, float)
         DECLARE_INJECTED_CONST(bool,     _eosio_f32_le,         float, float)
         DECLARE_INJECTED_CONST(bool,     _eosio_f32_gt,         float, float)
         DECLARE_INJECTED_CONST(bool,     _eosio_f32_ge,         float, float)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_add,        double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_sub,        double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_div,        double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_mul,        double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_min,        double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_max,        double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_copysign,   double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_abs,        double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_neg,        double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_sqrt,       double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_ceil,       double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_floor,      double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_trunc,      double)
         DECLARE_INJECTED_CONST(double,   _eosio_f64_nearest,    double)
         DECLARE_INJECTED_CONST(bool,     _eosio_f64_eq,         double, double)
         DECLARE_INJECTED_CONST(bool,     _eosio_f64_ne,         double, double)
         DECLARE_INJECTED_CONST(bool,     _eosio_f64_lt,         double, double)
         DECLARE_INJECTED_CONST(bool,     _eosio_f64_le,         double, double)
         DECLARE_INJECTED_CONST(bool,     _eosio_f64_gt,         double, double)
         DECLARE_INJECTED_CONST(bool,     _eosio_f64_ge,         double, double)
         DECLARE_INJECTED_CONST(double,   _eosio_f32_promote,    float)
         DECLARE_INJECTED_CONST(float,    _eosio_f64_demote,     double)
         DECLARE_INJECTED_CONST(int32_t,  _eosio_f32_trunc_i32s, float)
         DECLARE_INJECTED_CONST(int32_t,  _eosio_f64_trunc_i32s, double)
         DECLARE_INJECTED_CONST(uint32_t, _eosio_f32_trunc_i32u, float)
         DECLARE_INJECTED_CONST(uint32_t, _eosio_f64_trunc_i32u, double)
         DECLARE_INJECTED_CONST(int64_t,  _eosio_f32_trunc_i64s, float)
         DECLARE_INJECTED_CONST(int64_t,  _eosio_f64_trunc_i64s, double)
         DECLARE_INJECTED_CONST(uint64_t, _eosio_f32_trunc_i64u, float)
         DECLARE_INJECTED_CONST(uint64_t, _eosio_f64_trunc_i64u, double)
         DECLARE_INJECTED_CONST(float,    _eosio_i32_to_f32,     int32_t)
         DECLARE_INJECTED_CONST(float,    _eosio_i64_to_f32,     int64_t)
         DECLARE_INJECTED_CONST(float,    _eosio_ui32_to_f32,    uint32_t)
         DECLARE_INJECTED_CONST(float,    _eosio_ui64_to_f32,    uint64_t)
         DECLARE_INJECTED_CONST(double,   _eosio_i32_to_f64,     int32_t)
         DECLARE_INJECTED_CONST(double,   _eosio_i64_to_f64,     int64_t)
         DECLARE_INJECTED_CONST(double,   _eosio_ui32_to_f64,    uint32_t)
         DECLARE_INJECTED_CONST(double,   _eosio_ui64_to_f64,    uint64_t)

         // producer api
         DECLARE(int32_t, get_active_producers, legacy_array_ptr<account_name>, uint32_t)

         // crypto api
         DECLARE_CONST(void,    assert_recover_key, const fc::sha256&, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_CONST(int32_t, recover_key,        const fc::sha256&, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE_CONST(void,    assert_sha256,      legacy_array_ptr<char>, uint32_t, const fc::sha256&)
         DECLARE_CONST(void,    assert_sha1,        legacy_array_ptr<char>, uint32_t, const fc::sha1&)
         DECLARE_CONST(void,    assert_sha512,      legacy_array_ptr<char>, uint32_t, const fc::sha512&)
         DECLARE_CONST(void,    assert_ripemd160,   legacy_array_ptr<char>, uint32_t, const fc::ripemd160&)
         DECLARE_CONST(void,    sha256,             legacy_array_ptr<char>, uint32_t, fc::sha256&)
         DECLARE_CONST(void,    sha1,               legacy_array_ptr<char>, uint32_t, fc::sha1&)
         DECLARE_CONST(void,    sha512,             legacy_array_ptr<char>, uint32_t, fc::sha512&)
         DECLARE_CONST(void,    ripemd160,          legacy_array_ptr<char>, uint32_t, fc::ripemd160&)

         // permission api
         DECLARE(bool,    check_transaction_authorization, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE(bool,    check_permission_authorization,  account_name, permission_name, legacy_array_ptr<char>, uint32_t, legacy_array_ptr<char>, uint32_t, uint64_t)
         DECLARE_CONST(int64_t, get_permission_last_used,        account_name, permission_name)
         DECLARE_CONST(int64_t, get_account_creation_time,       account_name)

         // authorization api
         DECLARE(void,       require_auth,      account_name)
         DECLARE(void,       require_auth2,     account_name, permission_name)
         DECLARE_CONST(bool, has_auth,          account_name)
         DECLARE(void,       require_recipient, account_name)
         DECLARE_CONST(bool, is_account,        account_name)

         // system api
         DECLARE_CONST(uint64_t, current_time)
         DECLARE_CONST(uint64_t, publication_time)
         DECLARE_CONST(bool,     is_feature_activated, const digest_type&)
         DECLARE_CONST(name,     get_sender)

         // context-free system api
         DECLARE_CF(void, abort)
         DECLARE_CF(void, eosio_assert,         bool, null_terminated_ptr)
         DECLARE_CF(void, eosio_assert_message, bool, legacy_array_ptr<const char>, uint32_t)
         DECLARE_CF(void, eosio_assert_code,    bool, uint64_t)
         DECLARE_CF(void, eosio_exit,           int32_t)

         // action api
         DECLARE_CONST(int32_t, read_action_data,        legacy_array_ptr<char>, uint32_t)
         DECLARE_CONST(int32_t, action_data_size)
         DECLARE_CONST(name,    current_receiver)
         DECLARE(void,          set_action_return_value, legacy_array_ptr<char>, uint32_t)

         // console api
         DECLARE(void, prints,     null_terminated_ptr)
         DECLARE(void, prints_l,   legacy_array_ptr<const char>, uint32_t)
         DECLARE(void, printi,     int64_t)
         DECLARE(void, printui,    uint64_t)
         DECLARE(void, printi128,  const __int128&)
         DECLARE(void, printui128, const unsigned __int128&)
         DECLARE(void, printsf,    float)
         DECLARE(void, printdf,    double)
         DECLARE(void, printqf,    const float128_t&)
         DECLARE(void, printn,     name)
         DECLARE(void, printhex,   legacy_array_ptr<const char>, uint32_t)

         // TODO still need to abstract the DECLAREs a bit to allow for abitrary preconditions
         // database api
         // primary index api
         DECLARE(int32_t, db_store_i64,      uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const char>, uint32_t)
         DECLARE(void,    db_update_i64,     int32_t, uint64_t, legacy_array_ptr<const char>, uint32_t)
         DECLARE(void,    db_remove_i64,     int32_t)
         DECLARE(int32_t, db_get_i64,        int32_t, legacy_array_ptr<char>, uint32_t)
         DECLARE(int32_t, db_next_i64,       int32_t, uint64_t&)
         DECLARE(int32_t, db_previous_i64,   int32_t, uint64_t&)
         DECLARE(int32_t, db_find_i64,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_lowerbound_i64, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_upperbound_i64, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_end_i64,        uint64_t, uint64_t, uint64_t)

         // uint64_t secondary index api
         DECLARE(int32_t, db_idx64_store,          uint64_t, uint64_t, uint64_t, uint64_t, const uint64_t&)
         DECLARE(void,    db_idx64_update,         int32_t, uint64_t, const uint64_t&)
         DECLARE(void,    db_idx64_remove,         int32_t)
         DECLARE(int32_t, db_idx64_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint64_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx64_find_primary,   uint64_t, uint64_t, uint64_t, uint64_t&, uint64_t)
         DECLARE(int32_t, db_idx64_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx64_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx64_end,            uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_idx64_next,           int32_t, uint64_t&)
         DECLARE(int32_t, db_idx64_previous,       int32_t, uint64_t&)

         // uint128_t secondary index api
         DECLARE(int32_t, db_idx128_store,          uint64_t, uint64_t, uint64_t, uint64_t, const uint128_t&)
         DECLARE(void,    db_idx128_update,         int32_t, uint64_t, const uint128_t&)
         DECLARE(void,    db_idx128_remove,         int32_t)
         DECLARE(int32_t, db_idx128_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint128_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx128_find_primary,   uint64_t, uint64_t, uint64_t, uint128_t&, uint64_t)
         DECLARE(int32_t, db_idx128_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx128_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx128_end,            uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_idx128_next,           int32_t, uint64_t&)
         DECLARE(int32_t, db_idx128_previous,       int32_t, uint64_t&)

         // 256-bit secondary index api
         DECLARE(int32_t, db_idx256_store,          uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t)
         DECLARE(void,    db_idx256_update,         int32_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t)
         DECLARE(void,    db_idx256_remove,         int32_t)
         DECLARE(int32_t, db_idx256_find_secondary, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, uint32_t, uint64_t&)
         DECLARE(int32_t, db_idx256_find_primary,   uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, uint64_t)
         DECLARE(int32_t, db_idx256_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx256_upperbound,     uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint32_t, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx256_end,            uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_idx256_next,           int32_t, uint64_t&)
         DECLARE(int32_t, db_idx256_previous,       int32_t, uint64_t&)

         // double secondary index api
         DECLARE(int32_t, db_idx_double_store,          uint64_t, uint64_t, uint64_t, uint64_t, const float64_t&)
         DECLARE(void,    db_idx_double_update,         int32_t, uint64_t, const float64_t&)
         DECLARE(void,    db_idx_double_remove,         int32_t)
         DECLARE(int32_t, db_idx_double_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const float64_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx_double_find_primary,   uint64_t, uint64_t, uint64_t, float64_t&, uint64_t)
         DECLARE(int32_t, db_idx_double_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx_double_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx_double_end,            uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_idx_double_next,           int32_t, uint64_t&)
         DECLARE(int32_t, db_idx_double_previous,       int32_t, uint64_t&)

         // long double secondary index api
         DECLARE(int32_t, db_idx_long_double_store,          uint64_t, uint64_t, uint64_t, uint64_t, const float128_t&)
         DECLARE(void,    db_idx_long_double_update,         int32_t, uint64_t, const float128_t&)
         DECLARE(void,    db_idx_long_double_remove,         int32_t)
         DECLARE(int32_t, db_idx_long_double_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const float128_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx_long_double_find_primary,   uint64_t, uint64_t, uint64_t, float128_t&, uint64_t)
         DECLARE(int32_t, db_idx_long_double_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>, legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx_long_double_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>,  legacy_ptr<uint64_t>)
         DECLARE(int32_t, db_idx_long_double_end,            uint64_t, uint64_t, uint64_t)
         DECLARE(int32_t, db_idx_long_double_next,           int32_t, uint64_t&)
         DECLARE(int32_t, db_idx_long_double_previous,       int32_t, uint64_t&)

         // memory api
         DECLARE_CONST(char*,   memcpy,  legacy_array_ptr<char>, legacy_array_ptr<const char>, uint32_t)
         DECLARE_CONST(char*,   memmove, legacy_array_ptr<char>, legacy_array_ptr<const char>, uint32_t)
         DECLARE_CONST(int32_t, memcmp,  legacy_array_ptr<const char>, legacy_array_ptr<const char>, uint32_t)
         DECLARE_CONST(char*,   memset,  legacy_array_ptr<char>, int32_t, uint32_t)

         // transaction api
         DECLARE(void, send_inline,              legacy_array_ptr<char>, uint32_t)
         DECLARE(void, send_context_free_inline, legacy_array_ptr<char>, uint32_t)
         DECLARE(void, send_deferred,            legacy_ptr<const uint128_t>, account_name, legacy_array_ptr<char>, uint32_t, uint32_t)
         DECLARE(bool, cancel_deferred,          const uint128_t&)

         // context-free transaction api
         DECLARE_CONST_CF(int32_t, read_transaction, legacy_array_ptr<char>, uint32_t)
         DECLARE_CONST_CF(int32_t, transaction_size)
         DECLARE_CONST_CF(int32_t, expiration)
         DECLARE_CONST_CF(int32_t, tapos_block_num)
         DECLARE_CONST_CF(int32_t, tapos_block_prefix)
         DECLARE_CONST_CF(int32_t, get_action, uint32_t, uint32_t, legacy_array_ptr<char>, uint32_t)

         // compiler builtins api
         DECLARE_CONST(void,     __ashlti3,      int128_t&, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST(void,     __ashrti3,      int128_t&, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST(void,     __lshlti3,      int128_t&, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST(void,     __lshrti3,      int128_t&, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST(void,     __divti3,       int128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __udivti3,      uint128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __multi3,       int128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __modti3,       int128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __umodti3,      uint128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __addtf3,       float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __subtf3,       float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __multf3,       float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __divtf3,       float128_t&, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(void,     __negtf2,       float128_t&, uint64_t, uint64_t)
         DECLARE_CONST(void,     __extendsftf2,  float128_t&, float)
         DECLARE_CONST(void,     __extenddftf2,  float128_t&, double)
         DECLARE_CONST(double,   __trunctfdf2,   uint64_t, uint64_t)
         DECLARE_CONST(float,    __trunctfsf2,   uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __fixtfsi,      uint64_t, uint64_t)
         DECLARE_CONST(int64_t,  __fixtfdi,      uint64_t, uint64_t)
         DECLARE_CONST(void,     __fixtfti,      int128_t&, uint64_t, uint64_t)
         DECLARE_CONST(uint32_t, __fixunstfsi,   uint64_t, uint64_t)
         DECLARE_CONST(uint64_t, __fixunstfdi,   uint64_t, uint64_t)
         DECLARE_CONST(void,     __fixunstfti,   uint128_t&, uint64_t, uint64_t)
         DECLARE_CONST(void,     __fixsfti,      int128_t&, float)
         DECLARE_CONST(void,     __fixdfti,      int128_t&, double)
         DECLARE_CONST(void,     __fixunssfti,   uint128_t&, float)
         DECLARE_CONST(void,     __fixunsdfti,   uint128_t&, double)
         DECLARE_CONST(double,   __floatsidf,    int32_t)
         DECLARE_CONST(void,     __floatsitf,    float128_t&, int32_t)
         DECLARE_CONST(void,     __floatditf,    float128_t&, uint64_t)
         DECLARE_CONST(void,     __floatunsitf,  float128_t&, uint32_t)
         DECLARE_CONST(void,     __floatunditf,  float128_t&, uint64_t)
         DECLARE_CONST(double,   __floattidf,    uint64_t, uint64_t)
         DECLARE_CONST(double,   __floatuntidf,  uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __cmptf2,   uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __eqtf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __netf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __getf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __gttf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __letf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __lttf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST(int32_t,  __unordtf2, uint64_t, uint64_t, uint64_t, uint64_t)

         // call depth api
         DECLARE_INJECTED(void, call_depth_assert)

      private:
         apply_context& context;
   };

}}} // ns eosio::chain::webassembly

#undef DECLARE
#undef DECLARE_CONST
#undef DECLARE_INJECTED
#undef DECLARE_INJECTED_CONST
#undef DECLARE_HOST_FUNCTION
