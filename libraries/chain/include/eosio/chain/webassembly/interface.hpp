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


#define PRECONDITIONS(...) \
   __VA_ARGS__

#define DECLARE_HOST_FUNCTION(PRECONDS, MODULE, CONSTNESS, RET_TY, NAME, ...) \
   RET_TY NAME(__VA_ARGS__) CONSTNESS; \
   inline static host_function_registrator<&interface::NAME, alias_check, early_validate_pointers, PRECONDS > NAME ## _registrator = {#MODULE, #NAME};

#define DECLARE_CF_HOST_FUNCTION(CONSTNESS, RET_TY, NAME, ...) \
   RET_TY NAME(__VA_ARGS__) CONSTNESS; \
   inline static host_function_registrator<&interface::NAME, alias_check, early_validate_pointers, context_free_check, PRECONDS > NAME ## _registrator = {"env", #NAME};

#define DECLARE(PRECONDS, RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(PRECONDS, env, , RET_TY, NAME, __VA_ARGS__)

#define DECLARE_CF(PRECONDS, RET_TY, NAME, ...) \
   DECLARE_CF_HOST_FUNCTION(PRECONDS, env, , RET_TY, NAME, __VA_ARGS__)

#define DECLARE_CONST(PRECONDS, RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(PRECONDS, env, const, RET_TY, NAME, __VA_ARGS__)

#define DECLARE_CONST_CF(PRECONDS, RET_TY, NAME, ...) \
   DECLARE_CF_HOST_FUNCTION(PRECONDS, env, const, RET_TY, NAME, __VA_ARGS__)

#define DECLARE_INJECTED(PRECONDS, RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(PRECONDS, eosio_injected, , RET_TY, NAME, __VA_ARGS__)

#define DECLARE_INJECTED_CONST(PRECONDS, RET_TY, NAME, ...) \
   DECLARE_HOST_FUNCTION(PRECONDS, eosio_injected, const, RET_TY, NAME, __VA_ARGS__)


   // TODO maybe house these some place else
   template <typename T, typename U>
   inline static bool is_aliasing(const T& a, const U& b) {
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
         EOS_VM_INVOKE_ON_ALL([&](auto arg, auto&&... rest) {
            using namespace eosio::vm;
            using arg_t = decltype(arg);
            if constexpr (is_reference_proxy_type_v<arg_t>)
               if constexpr (is_span_type_v<dependent_type_t<arg_t>>) {
                  using dep_t = dependent_type_t<arg_t>;
                  const auto& s = (dep_t)arg;
                  EOS_ASSERT( s.size() <= std::numeric_limits<wasm_size_t>::max() / (wasm_size_t)sizeof(dep_t),
                        wasm_exception, "length will overflow" );
                  volatile auto check = *(reinterpret_cast<const char*>(s.data()) + s.size_bytes() - 1);
                  ignore_unused_variable_warning(check);
               }
         }));

   EOS_VM_PRECONDITION(context_free_check,
         EOS_VM_INVOKE_ONCE([&](auto&&...) {
            EOS_ASSERT(ctx.get_host()->context.is_context_free(), unaccessible_api, "only context free api's can be used in this context");
         }));

   EOS_VM_PRECONDITION(alias_check,
         EOS_VM_INVOKE_ON_ALL(([&](auto arg, auto&&... rest) {
            using namespace eosio::vm;
            using arg_t = decltype(arg);
            if constexpr (is_span_type_v<arg_t>) {
               eosio::vm::invoke_on<false, eosio::vm::detail::invoke_on_all_t>([&](auto narg, auto&&... nrest) {
                  using nested_arg_t = decltype(arg);
                  if constexpr (eosio::vm::is_span_type_v<nested_arg_t>)
                     EOS_ASSERT(!is_aliasing(arg, narg), wasm_exception, "arrays not allowed to alias");
               }, rest...);
            }
         })));

   EOS_VM_PRECONDITION(is_nan_check,
         EOS_VM_INVOKE_ON(float, [&](auto arg, auto&&.. rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         });
         EOS_VM_INVOKE_ON(double, [&](auto arg, auto&&.. rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         });
         EOS_VM_INVOKE_ON(const float128_t&, [&](const auto& arg, auto&&.. rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         })
         );

   EOS_VM_PRECONDITION(legacy_static_check_wl_args,
         EOS_VM_INVOKE_ONCE([&](auto&&... args) {
            static_assert(is_whitelisted_legacy_type_v<args> && ..., "legacy whitelisted type violation");
         }));

   EOS_VM_PRECONDITION(static_check_wl_args,
         EOS_VM_INVOKE_ONCE([&](auto&&... args) {
            static_assert(is_whitelisted_type_v<args> && ..., "whitelisted type violation");
         }));

   class interface {
      public:
         // checktime api
         DECLARE_INJECTED( PRECONDITIONS(legacy_static_check_wl_args),
               void, checktime)

         // context free api
         DECLARE_CONST_CF( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, get_context_free_data, uint32_t, legacy_array_ptr<char>)

         // privileged api
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  is_feature_active,                int64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     activate_feature,                 int64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,     preactivate_feature,              const digest_type&)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,     set_resource_limits,              account_name, int64_t, int64_t, int64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     get_resource_limits,              account_name, legacy_ptr<int64_t>, legacy_ptr<int64_t>, legacy_ptr<int64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int64_t,  set_proposed_producers,           legacy_array_ptr<char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int64_t,  set_proposed_producers_ex,        uint64_t, legacy_array_ptr<char>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint32_t, get_blockchain_parameters_packed, legacy_array_ptr<char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,     set_blockchain_parameters_packed, legacy_array_ptr<char>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     is_privileged,                    account_name)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,     set_privileged,                   account_name, bool)

         // softfloat api
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_add,        float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_sub,        float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_div,        float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_mul,        float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_min,        float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_max,        float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_copysign,   float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_abs,        float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_neg,        float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_sqrt,       float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_ceil,       float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_floor,      float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_trunc,      float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f32_nearest,    float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f32_eq,         float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f32_ne,         float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f32_lt,         float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f32_le,         float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f32_gt,         float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f32_ge,         float, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_add,        double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_sub,        double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_div,        double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_mul,        double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_min,        double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_max,        double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_copysign,   double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_abs,        double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_neg,        double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_sqrt,       double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_ceil,       double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_floor,      double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_trunc,      double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f64_nearest,    double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f64_eq,         double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f64_ne,         double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f64_lt,         double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f64_le,         double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f64_gt,         double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     _eosio_f64_ge,         double, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_f32_promote,    float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_f64_demote,     double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  _eosio_f32_trunc_i32s, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  _eosio_f64_trunc_i32s, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint32_t, _eosio_f32_trunc_i32u, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint32_t, _eosio_f64_trunc_i32u, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int64_t,  _eosio_f32_trunc_i64s, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int64_t,  _eosio_f64_trunc_i64s, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint64_t, _eosio_f32_trunc_i64u, float)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint64_t, _eosio_f64_trunc_i64u, double)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_i32_to_f32,     int32_t)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_i64_to_f32,     int64_t)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_ui32_to_f32,    uint32_t)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    _eosio_ui64_to_f32,    uint64_t)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_i32_to_f64,     int32_t)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_i64_to_f64,     int64_t)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_ui32_to_f64,    uint32_t)
         DECLARE_INJECTED_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   _eosio_ui64_to_f64,    uint64_t)

         // producer api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, get_active_producers, legacy_array_ptr<account_name>)

         // crypto api
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    assert_recover_key, legacy_ptr<const fc::sha256>, legacy_array_ptr<char>, legacy_array_ptr<char>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, recover_key,        legacy_ptr<const fc::sha256>, legacy_array_ptr<char>, legacy_array_ptr<char>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    assert_sha256,      legacy_array_ptr<char>, legacy_ptr<const fc::sha256>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    assert_sha1,        legacy_array_ptr<char>, legacy_ptr<const fc::sha1>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    assert_sha512,      legacy_array_ptr<char>, legacy_ptr<const fc::sha512>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    assert_ripemd160,   legacy_array_ptr<char>, legacy_ptr<const fc::ripemd160>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    sha256,             legacy_array_ptr<char>, legacy_ptr<fc::sha256>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    sha1,               legacy_array_ptr<char>, legacy_ptr<fc::sha1>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    sha512,             legacy_array_ptr<char>, legacy_ptr<fc::sha512>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,    ripemd160,          legacy_array_ptr<char>, legacy_ptr<fc::ripemd160>)

         // permission api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               bool,    check_transaction_authorization, legacy_array_ptr<char>, legacy_array_ptr<char>, legacy_array_ptr<char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               bool,    check_permission_authorization,  account_name, permission_name, legacy_array_ptr<char>, legacy_array_ptr<char>, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int64_t, get_permission_last_used,        account_name, permission_name)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int64_t, get_account_creation_time,       account_name)

         // authorization api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,       require_auth,      account_name)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,       require_auth2,     account_name, permission_name)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool, has_auth,          account_name)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,       require_recipient, account_name)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool, is_account,        account_name)

         // system api
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint64_t, current_time)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint64_t, publication_time)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               bool,     is_feature_activated, legacy_ptr<const digest_type>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               name,     get_sender)

         // context-free system api
         DECLARE_CF( PRECONDITIONS(legacy_static_check_wl_args),
               void, abort)
         DECLARE_CF( PRECONDITIONS(legacy_static_check_wl_args),
               void, eosio_assert,         bool, null_terminated_ptr)
         DECLARE_CF( PRECONDITIONS(legacy_static_check_wl_args),
               void, eosio_assert_message, bool, legacy_array_ptr<const char>)
         DECLARE_CF( PRECONDITIONS(legacy_static_check_wl_args),
               void, eosio_assert_code,    bool, uint64_t)
         DECLARE_CF( PRECONDITIONS(legacy_static_check_wl_args),
               void, eosio_exit,           int32_t)

         // action api
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, read_action_data,        legacy_array_ptr<char>)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, action_data_size)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               name,    current_receiver)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,          set_action_return_value, legacy_array_ptr<char>)

         // console api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, prints,     null_terminated_ptr)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, prints_l,   legacy_array_ptr<const char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printi,     int64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printui,    uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printi128,  legacy_ptr<const __int128>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printui128, legacy_ptr<const unsigned __int128>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printsf,    float)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printdf,    double)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printqf,    legacy_ptr<const float128_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, printn,     name)
         DECLARE( PRECONDITIONS(),
               void, printhex,   legacy_array_ptr<const char>)

         // TODO still need to abstract the DECLAREs a bit to allow for abitrary preconditions
         // database api
         // primary index api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_store_i64,      uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_update_i64,     int32_t, uint64_t, legacy_array_ptr<const char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_remove_i64,     int32_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_get_i64,        int32_t, legacy_array_ptr<char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_next_i64,       int32_t, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_previous_i64,   int32_t, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_find_i64,       uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_lowerbound_i64, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_upperbound_i64, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_end_i64,        uint64_t, uint64_t, uint64_t)

         // uint64_t secondary index api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_store,          uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx64_update,         int32_t, uint64_t, legacy_ptr<const uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx64_remove,         int32_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint64_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_find_primary,   uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_end,            uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_next,           int32_t, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx64_previous,       int32_t, legacy_ptr<uint64_t>)

         // uint128_t secondary index api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_store,          uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint128_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx128_update,         int32_t, uint64_t, legacy_ptr<const uint128_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx128_remove,         int32_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint128_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_find_primary,   uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_end,            uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_next,           int32_t, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx128_previous,       int32_t, legacy_ptr<uint64_t>)

         // 256-bit secondary index api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_store,          uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx256_update,         int32_t, uint64_t, legacy_array_ptr<const uint128_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx256_remove,         int32_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_find_secondary, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, uint64_t&)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_find_primary,   uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_upperbound,     uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_end,            uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_next,           int32_t, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx256_previous,       int32_t, legacy_ptr<uint64_t>)

         // double secondary index api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_store,          uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const float64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx_double_update,         int32_t, uint64_t, legacy_ptr<const float64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx_double_remove,         int32_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const float64_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_find_primary,   uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_end,            uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_next,           int32_t, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_double_previous,       int32_t, legacy_ptr<uint64_t>)

         // long double secondary index api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_store,          uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const float128_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx_long_double_update,         int32_t, uint64_t, legacy_ptr<const float128_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void,    db_idx_long_double_remove,         int32_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_find_secondary, uint64_t, uint64_t, uint64_t, legacy_ptr<const float128_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_find_primary,   uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_lowerbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_upperbound,     uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>,  legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_end,            uint64_t, uint64_t, uint64_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_next,           int32_t, legacy_ptr<uint64_t>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, db_idx_long_double_previous,       int32_t, legacy_ptr<uint64_t>)

         // memory api
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               char*,   memcpy,  char*, const char*, wasm_size_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               char*,   memmove, char*, const char*, wasm_size_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, memcmp,  const char*, const char*, wasm_size_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               char*,   memset, char*, int32_t, wasm_size_t)

         // transaction api
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, send_inline,              legacy_array_ptr<char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, send_context_free_inline, legacy_array_ptr<char>)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               void, send_deferred,            legacy_ptr<const uint128_t>, account_name, legacy_array_ptr<char>, uint32_t)
         DECLARE( PRECONDITIONS(legacy_static_check_wl_args),
               bool, cancel_deferred,          legacy_ptr<const uint128_t>)

         // context-free transaction api
         DECLARE_CONST_CF( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, read_transaction, legacy_array_ptr<char>)
         DECLARE_CONST_CF( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, transaction_size)
         DECLARE_CONST_CF( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, expiration)
         DECLARE_CONST_CF( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, tapos_block_num)
         DECLARE_CONST_CF( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, tapos_block_prefix)
         DECLARE_CONST_CF( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t, get_action, uint32_t, uint32_t, legacy_array_ptr<char>)

         // compiler builtins api
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __ashlti3,      legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __ashrti3,      legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __lshlti3,      legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __lshrti3,      legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __divti3,       legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __udivti3,      legacy_ptr<uint128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __multi3,       legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __modti3,       legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __umodti3,      legacy_ptr<uint128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __addtf3,       legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __subtf3,       legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __multf3,       legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __divtf3,       legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __negtf2,       legacy_ptr<float128_t>, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __extendsftf2,  legacy_ptr<float128_t>, float)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __extenddftf2,  legacy_ptr<float128_t>, double)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   __trunctfdf2,   uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               float,    __trunctfsf2,   uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __fixtfsi,      uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int64_t,  __fixtfdi,      uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __fixtfti,      legacy_ptr<int128_t>, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint32_t, __fixunstfsi,   uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               uint64_t, __fixunstfdi,   uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __fixunstfti,   legacy_ptr<uint128_t>, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __fixsfti,      legacy_ptr<int128_t>, float)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __fixdfti,      legacy_ptr<int128_t>, double)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __fixunssfti,   legacy_ptr<uint128_t>, float)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __fixunsdfti,   legacy_ptr<uint128_t>, double)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   __floatsidf,    int32_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __floatsitf,    legacy_ptr<float128_t>, int32_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __floatditf,    legacy_ptr<float128_t>, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __floatunsitf,  legacy_ptr<float128_t>, uint32_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               void,     __floatunditf,  legacy_ptr<float128_t>, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   __floattidf,    uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               double,   __floatuntidf,  uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __cmptf2,   uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __eqtf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __netf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __getf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __gttf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __letf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __lttf2,    uint64_t, uint64_t, uint64_t, uint64_t)
         DECLARE_CONST( PRECONDITIONS(legacy_static_check_wl_args),
               int32_t,  __unordtf2, uint64_t, uint64_t, uint64_t, uint64_t)

         // call depth api
         DECLARE_INJECTED( PRECONDITIONS(legacy_static_check_wl_args),
               void, call_depth_assert)

      private:
         apply_context& context;
   };

}}} // ns eosio::chain::webassembly

#undef PRECONDITIONS
#undef DECLARE
#undef DECLARE_CONST
#undef DECLARE_INJECTED
#undef DECLARE_INJECTED_CONST
#undef DECLARE_HOST_FUNCTION
