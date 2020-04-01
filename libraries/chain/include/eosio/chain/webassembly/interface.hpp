#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm.hpp>
#include <fc/crypto/sha1.hpp>

namespace eosio { namespace chain { namespace webassembly {

   template <auto HostFunction, typename... Preconditions>
   struct host_function_registrator {
      constexpr host_function_registrator(const std::string& mod_name, const std::string& fn_name) {
         using rhf_t = eos_vm_host_functions_t;
         rhf_t::add<HostFunction, Preconditions...>(mod_name, fn_name);
      }
   };


#define REGISTER_INJECTED_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, alias_check, early_validate_pointers, ##__VA_ARGS__> NAME ## _registrator = {eosio_injected_module_name, #NAME};

#define REGISTER_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, alias_check, early_validate_pointers, ##__VA_ARGS__> NAME ## _registrator = {"env", #NAME};

#define REGISTER_CF_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, alias_check, early_validate_pointers, context_free_check, ##__VA_ARGS__> NAME ## _registrator = {"env", #NAME};


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
                  const auto& s = (dep_t&)arg;
                  EOS_ASSERT( s.size() <= std::numeric_limits<wasm_size_t>::max() / (wasm_size_t)sizeof(dependent_type_t<dep_t>),
                        wasm_exception, "length will overflow" );
                  volatile auto check = *(reinterpret_cast<const char*>(s.data()) + s.size_bytes() - 1);
                  ignore_unused_variable_warning(check);
               }
         }));

   EOS_VM_PRECONDITION(context_free_check,
         EOS_VM_INVOKE_ONCE([&](auto&&...) {
            EOS_ASSERT(ctx.get_host().get_context().is_context_free(), unaccessible_api, "only context free api's can be used in this context");
         }));

   EOS_VM_PRECONDITION(alias_check,
         EOS_VM_INVOKE_ON_ALL(([&](auto arg, auto&&... rest) {
            using namespace eosio::vm;
            using arg_t = decltype(arg);
            if constexpr (is_span_type_v<arg_t>) {
               eosio::vm::invoke_on<false, eosio::vm::invoke_on_all_t>([&](auto narg, auto&&... nrest) {
                  using nested_arg_t = decltype(arg);
                  if constexpr (eosio::vm::is_span_type_v<nested_arg_t>)
                     EOS_ASSERT(!is_aliasing(arg, narg), wasm_exception, "arrays not allowed to alias");
               }, rest...);
            }
         })));

   EOS_VM_PRECONDITION(is_nan_check,
         EOS_VM_INVOKE_ON(float, [&](auto arg, auto&&... rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         });
         EOS_VM_INVOKE_ON(double, [&](auto arg, auto&&... rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         });
         EOS_VM_INVOKE_ON(const float128_t&, [&](const auto& arg, auto&&... rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         })
         );

   EOS_VM_PRECONDITION(legacy_static_check_wl_args,
         EOS_VM_INVOKE_ONCE([&](auto&&... args) {
            static_assert( are_whitelisted_legacy_types_v<std::decay_t<decltype(args)>...>, "legacy whitelisted type violation");
         }));

   EOS_VM_PRECONDITION(static_check_wl_args,
         EOS_VM_INVOKE_ONCE([&](auto&&... args) {
            static_assert( are_whitelisted_types_v<std::decay_t<decltype(args)>...>, "whitelisted type violation");
         }));

   class interface {
      public:
         interface(apply_context& ctx) : context(ctx) {}

         inline apply_context& get_context() { return context; }
         inline const apply_context& get_context() const { return context; }

         // checktime api
         void checktime();
         REGISTER_HOST_FUNCTION(checktime);

         // context free api
         int32_t get_context_free_data(uint32_t index, legacy_array_ptr<char> buffer) const;
         REGISTER_CF_HOST_FUNCTION(get_context_free_data, legacy_static_check_wl_args)

         // privileged api
         int32_t is_feature_active(int64_t feature_name) const;
         void activate_feature(int64_t feature_name) const;
         void preactivate_feature(legacy_ptr<const digest_type>);
         void set_resource_limits(account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
         void get_resource_limits(account_name account, legacy_ptr<int64_t> ram_bytes, legacy_ptr<int64_t> net_weight, legacy_ptr<int64_t> cpu_weight) const;
         int64_t set_proposed_producers(legacy_array_ptr<char> packed_producer_schedule);
         int64_t set_proposed_producers_ex(uint64_t packed_producer_format, legacy_array_ptr<char> packed_producer_schedule);
         uint32_t get_blockchain_parameters_packed(legacy_array_ptr<char> packed_blockchain_parameters) const;
         void set_blockchain_parameters_packed(legacy_array_ptr<char> packed_blockchain_parameters);
         bool is_privileged(account_name account) const;
         void set_privileged(account_name account, bool is_priv);

         REGISTER_HOST_FUNCTION(is_feature_active, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(activate_feature, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(preactivate_feature, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(set_resource_limits, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(get_resource_limits, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(set_proposed_producers, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(set_proposed_producers_ex, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(get_blockchain_parameters_packed, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(set_blockchain_parameters_packed, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(is_privileged, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(set_privileged, legacy_static_check_wl_args);

         // softfloat api
         float _eosio_f32_add(float, float) const;
         float _eosio_f32_sub(float, float) const;
         float _eosio_f32_div(float, float) const;
         float _eosio_f32_mul(float, float) const;
         float _eosio_f32_min(float, float) const;
         float _eosio_f32_max(float, float) const;
         float _eosio_f32_copysign(float, float) const;
         float _eosio_f32_abs(float) const;
         float _eosio_f32_neg(float) const;
         float _eosio_f32_sqrt(float) const;
         float _eosio_f32_ceil(float) const;
         float _eosio_f32_floor(float) const;
         float _eosio_f32_trunc(float) const;
         float _eosio_f32_nearest(float) const;
         bool _eosio_f32_eq(float, float) const;
         bool _eosio_f32_ne(float, float) const;
         bool _eosio_f32_lt(float, float) const;
         bool _eosio_f32_le(float, float) const;
         bool _eosio_f32_gt(float, float) const;
         bool _eosio_f32_ge(float, float) const;
         double _eosio_f64_add(double, double) const;
         double _eosio_f64_sub(double, double) const;
         double _eosio_f64_div(double, double) const;
         double _eosio_f64_mul(double, double) const;
         double _eosio_f64_min(double, double) const;
         double _eosio_f64_max(double, double) const;
         double _eosio_f64_copysign(double, double) const;
         double _eosio_f64_abs(double) const;
         double _eosio_f64_neg(double) const;
         double _eosio_f64_sqrt(double) const;
         double _eosio_f64_ceil(double) const;
         double _eosio_f64_floor(double) const;
         double _eosio_f64_trunc(double) const;
         double _eosio_f64_nearest(double) const;
         bool _eosio_f64_eq(double, double) const;
         bool _eosio_f64_ne(double, double) const;
         bool _eosio_f64_lt(double, double) const;
         bool _eosio_f64_le(double, double) const;
         bool _eosio_f64_gt(double, double) const;
         bool _eosio_f64_ge(double, double) const;
         double _eosio_f32_promote(float) const;
         float _eosio_f64_demote(double) const;
         int32_t _eosio_f32_trunc_i32s(float) const;
         int32_t _eosio_f64_trunc_i32s(double) const;
         uint32_t _eosio_f32_trunc_i32u(float) const;
         uint32_t _eosio_f64_trunc_i32u(double) const;
         int64_t _eosio_f32_trunc_i64s(float) const;
         int64_t _eosio_f64_trunc_i64s(double) const;
         uint64_t _eosio_f32_trunc_i64u(float) const;
         uint64_t _eosio_f64_trunc_i64u(double) const;
         float _eosio_i32_to_f32(int32_t) const;
         float _eosio_i64_to_f32(int64_t) const;
         float _eosio_ui32_to_f32(uint32_t) const;
         float _eosio_ui64_to_f32(uint64_t) const;
         double _eosio_i32_to_f64(int32_t) const;
         double _eosio_i64_to_f64(int64_t) const;
         double _eosio_ui32_to_f64(uint32_t) const;
         double _eosio_ui64_to_f64(uint64_t) const;

         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_add, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_sub, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_div, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_mul, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_min, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_max, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_copysign, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_abs, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_neg, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_sqrt, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_ceil, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_floor, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_nearest, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_eq, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_ne, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_lt, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_le, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_gt, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_ge, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_add, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_sub, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_div, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_mul, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_min, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_max, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_copysign, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_abs, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_neg, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_sqrt, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_ceil, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_floor, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_nearest, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_eq, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_ne, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_lt, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_le, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_gt, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_ge, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_promote, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_demote, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i32s, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i32s, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i32u, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i32u, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i64s, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i64s, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i64u, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i64u, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i32_to_f32, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i64_to_f32, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui32_to_f32, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui64_to_f32, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i32_to_f64, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i64_to_f64, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui32_to_f64, legacy_static_check_wl_args);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui64_to_f64, legacy_static_check_wl_args);

         // producer api
         int32_t get_active_producers(legacy_array_ptr<account_name>) const;
         REGISTER_HOST_FUNCTION(get_active_producers, legacy_static_check_wl_args);

         // crypto api
         void assert_recover_key(legacy_ptr<const fc::sha256>, legacy_array_ptr<char>, legacy_array_ptr<char>) const;
         int32_t recover_key(legacy_ptr<const fc::sha256>, legacy_array_ptr<char>, legacy_array_ptr<char>) const;
         void assert_sha256(legacy_array_ptr<char>, legacy_ptr<const fc::sha256>) const;
         void assert_sha1(legacy_array_ptr<char>, legacy_ptr<const fc::sha1>) const;
         void assert_sha512(legacy_array_ptr<char>, legacy_ptr<const fc::sha512>) const;
         void assert_ripemd160(legacy_array_ptr<char>, legacy_ptr<const fc::ripemd160>) const;
         void sha256(legacy_array_ptr<char>, legacy_ptr<fc::sha256>) const;
         void sha1(legacy_array_ptr<char>, legacy_ptr<fc::sha1>) const;
         void sha512(legacy_array_ptr<char>, legacy_ptr<fc::sha512>) const;
         void ripemd160(legacy_array_ptr<char>, legacy_ptr<fc::ripemd160>) const;

         REGISTER_HOST_FUNCTION(assert_recover_key, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(recover_key, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(assert_sha256, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(assert_sha1, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(assert_sha512, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(assert_ripemd160, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(sha256, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(sha1, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(sha512, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(ripemd160, legacy_static_check_wl_args);

         // permission api
         bool check_transaction_authorization(legacy_array_ptr<char>, legacy_array_ptr<char>, legacy_array_ptr<char>);
         bool check_permission_authorization(account_name, permission_name, legacy_array_ptr<char>, legacy_array_ptr<char>, uint64_t);
         int64_t get_permission_last_used(account_name, permission_name) const;
         int64_t get_account_creation_time(account_name) const;

         REGISTER_HOST_FUNCTION(check_transaction_authorization, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(check_permission_authorization, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(get_permission_last_used, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(get_account_creation_time, legacy_static_check_wl_args);

         // authorization api
         void require_auth(account_name);
         void require_auth2(account_name, permission_name);
         bool has_auth(account_name) const;
         void require_recipient(account_name);
         bool is_account(account_name) const;

         REGISTER_HOST_FUNCTION(require_auth, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(require_auth2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(has_auth, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(require_recipient, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(is_account, legacy_static_check_wl_args);

         // system api
         uint64_t current_time() const;
         uint64_t publication_time() const;
         bool is_feature_activated(legacy_ptr<const digest_type>) const;
         name get_sender() const;

         REGISTER_HOST_FUNCTION(current_time);
         REGISTER_HOST_FUNCTION(publication_time);
         REGISTER_HOST_FUNCTION(is_feature_activated, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(get_sender);

         // context-free system api
         void abort();
         void eosio_assert(bool, null_terminated_ptr);
         void eosio_assert_message(bool, legacy_array_ptr<const char>);
         void eosio_assert_code(bool, uint64_t);
         void eosio_exit(int32_t);

         REGISTER_CF_HOST_FUNCTION(abort)
         REGISTER_CF_HOST_FUNCTION(eosio_assert, legacy_static_check_wl_args)
         REGISTER_CF_HOST_FUNCTION(eosio_assert_message, legacy_static_check_wl_args)
         REGISTER_CF_HOST_FUNCTION(eosio_assert_code, legacy_static_check_wl_args)
         REGISTER_CF_HOST_FUNCTION(eosio_exit, legacy_static_check_wl_args)

         // action api
         int32_t read_action_data(legacy_array_ptr<char>) const;
         int32_t action_data_size() const;
         name current_receiver() const;
         void set_action_return_value(legacy_array_ptr<char>);

         REGISTER_HOST_FUNCTION(read_action_data, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(action_data_size);
         REGISTER_HOST_FUNCTION(current_receiver);
         REGISTER_HOST_FUNCTION(set_action_return_value, legacy_static_check_wl_args);

         // console api
         void prints(null_terminated_ptr);
         void prints_l(legacy_array_ptr<const char>);
         void printi(int64_t);
         void printui(uint64_t);
         void printi128(legacy_ptr<const __int128>);
         void printui128(legacy_ptr<const unsigned __int128>);
         void printsf(float);
         void printdf(double);
         void printqf(legacy_ptr<const float128_t>);
         void printn(name);
         void printhex(legacy_array_ptr<const char>);

         REGISTER_HOST_FUNCTION(prints, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(prints_l, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printi, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printui, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printi128, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printui128, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printsf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printdf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printqf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printn, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(printhex, legacy_static_check_wl_args);

         // database api
         // primary index api
         int32_t db_store_i64(uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const char>);
         void db_update_i64(int32_t, uint64_t, legacy_array_ptr<const char>);
         void db_remove_i64(int32_t);
         int32_t db_get_i64(int32_t, legacy_array_ptr<char>);
         int32_t db_next_i64(int32_t, legacy_ptr<uint64_t>);
         int32_t db_previous_i64(int32_t, legacy_ptr<uint64_t>);
         int32_t db_find_i64(uint64_t, uint64_t, uint64_t, uint64_t);
         int32_t db_lowerbound_i64(uint64_t, uint64_t, uint64_t, uint64_t);
         int32_t db_upperbound_i64(uint64_t, uint64_t, uint64_t, uint64_t);
         int32_t db_end_i64(uint64_t, uint64_t, uint64_t);

         REGISTER_HOST_FUNCTION(db_store_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_update_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_remove_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_get_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_next_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_previous_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_find_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_lowerbound_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_upperbound_i64, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_end_i64, legacy_static_check_wl_args);


         // uint64_t secondary index api
         int32_t db_idx64_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint64_t>);
         void db_idx64_update(int32_t, uint64_t, legacy_ptr<const uint64_t>);
         void db_idx64_remove(int32_t);
         int32_t db_idx64_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const uint64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx64_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, uint64_t);
         int32_t db_idx64_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx64_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx64_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx64_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx64_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_HOST_FUNCTION(db_idx64_store, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_update, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_remove, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_find_secondary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_find_primary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_lowerbound, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_upperbound, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_end, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_next, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx64_previous, legacy_static_check_wl_args);

         // uint128_t secondary index api
         int32_t db_idx128_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint128_t>);
         void db_idx128_update(int32_t, uint64_t, legacy_ptr<const uint128_t>);
         void db_idx128_remove(int32_t);
         int32_t db_idx128_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx128_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, uint64_t);
         int32_t db_idx128_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx128_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx128_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx128_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx128_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_HOST_FUNCTION(db_idx128_store, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_update, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_remove, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_find_secondary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_find_primary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_lowerbound, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_upperbound, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_end, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_next, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx128_previous, legacy_static_check_wl_args);

         // 256-bit secondary index api
         int32_t db_idx256_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>);
         void db_idx256_update(int32_t, uint64_t, legacy_array_ptr<const uint128_t>);
         void db_idx256_remove(int32_t);
         int32_t db_idx256_find_secondary(uint64_t, uint64_t, uint64_t, legacy_array_ptr<const uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx256_find_primary(uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, uint64_t);
         int32_t db_idx256_lowerbound(uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx256_upperbound(uint64_t, uint64_t, uint64_t, legacy_array_ptr<uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx256_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx256_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx256_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_HOST_FUNCTION(db_idx256_store, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_update, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_remove, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_find_secondary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_find_primary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_lowerbound, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_upperbound, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_end, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_next, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx256_previous, legacy_static_check_wl_args);

         // double secondary index api
         int32_t db_idx_double_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const float64_t>);
         void db_idx_double_update(int32_t, uint64_t, legacy_ptr<const float64_t>);
         void db_idx_double_remove(int32_t);
         int32_t db_idx_double_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const float64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx_double_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, uint64_t);
         int32_t db_idx_double_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx_double_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx_double_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx_double_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx_double_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_HOST_FUNCTION(db_idx_double_store, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_double_update, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_double_remove, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_double_find_secondary, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_double_find_primary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_double_lowerbound, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_double_upperbound, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_double_end, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_double_next, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_double_previous, legacy_static_check_wl_args);


         // long double secondary index api
         int32_t db_idx_long_double_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const float128_t>);
         void db_idx_long_double_update(int32_t, uint64_t, legacy_ptr<const float128_t>);
         void db_idx_long_double_remove(int32_t);
         int32_t db_idx_long_double_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const float128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx_long_double_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>, uint64_t);
         int32_t db_idx_long_double_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx_long_double_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>,  legacy_ptr<uint64_t>);
         int32_t db_idx_long_double_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx_long_double_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx_long_double_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_HOST_FUNCTION(db_idx_long_double_store, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_long_double_update, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_long_double_remove, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_long_double_find_secondary, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_long_double_find_primary, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_long_double_lowerbound, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_long_double_upperbound, legacy_static_check_wl_args, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_long_double_end, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_long_double_next, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(db_idx_long_double_previous, legacy_static_check_wl_args);

         // memory api
         char* memcpy(unvalidated_ptr<char>, unvalidated_ptr<const char>, wasm_size_t) const;
         char* memmove(unvalidated_ptr<char>, unvalidated_ptr<const char>, wasm_size_t) const;
         int32_t memcmp(unvalidated_ptr<const char>, unvalidated_ptr<const char>, wasm_size_t) const;
         char* memset(unvalidated_ptr<char>, int32_t, wasm_size_t) const;

         REGISTER_HOST_FUNCTION(memcpy, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(memmove, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(memcmp, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(memset, legacy_static_check_wl_args);

         // transaction api
         void send_inline(legacy_array_ptr<char>);
         void send_context_free_inline(legacy_array_ptr<char>);
         void send_deferred(legacy_ptr<const uint128_t>, account_name, legacy_array_ptr<char>, uint32_t);
         bool cancel_deferred(legacy_ptr<const uint128_t>);

         REGISTER_HOST_FUNCTION(send_inline, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(send_context_free_inline, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(send_deferred, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(cancel_deferred, legacy_static_check_wl_args);

         // context-free transaction api
         int32_t read_transaction(legacy_array_ptr<char>) const;
         int32_t transaction_size() const;
         int32_t expiration() const;
         int32_t tapos_block_num() const;
         int32_t tapos_block_prefix() const;
         int32_t get_action(uint32_t, uint32_t, legacy_array_ptr<char>) const;

         REGISTER_HOST_FUNCTION(read_transaction, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(transaction_size, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(expiration, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(tapos_block_num, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(tapos_block_prefix, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(get_action, legacy_static_check_wl_args);

         // compiler builtins api
         void __ashlti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __ashrti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __lshlti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __lshrti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint32_t) const;
         void __divti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __udivti3(legacy_ptr<uint128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __multi3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __modti3(legacy_ptr<int128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __umodti3(legacy_ptr<uint128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __addtf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __subtf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __multf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __divtf3(legacy_ptr<float128_t>, uint64_t, uint64_t, uint64_t, uint64_t) const;
         void __negtf2(legacy_ptr<float128_t>, uint64_t, uint64_t) const;
         void __extendsftf2(legacy_ptr<float128_t>, float) const;
         void __extenddftf2(legacy_ptr<float128_t>, double) const;
         double __trunctfdf2(uint64_t, uint64_t) const;
         float __trunctfsf2(uint64_t, uint64_t) const;
         int32_t __fixtfsi(uint64_t, uint64_t) const;
         int64_t __fixtfdi(uint64_t, uint64_t) const;
         void __fixtfti(legacy_ptr<int128_t>, uint64_t, uint64_t) const;
         uint32_t __fixunstfsi(uint64_t, uint64_t) const;
         uint64_t __fixunstfdi(uint64_t, uint64_t) const;
         void __fixunstfti(legacy_ptr<uint128_t>, uint64_t, uint64_t) const;
         void __fixsfti(legacy_ptr<int128_t>, float) const;
         void __fixdfti(legacy_ptr<int128_t>, double) const;
         void __fixunssfti(legacy_ptr<uint128_t>, float) const;
         void __fixunsdfti(legacy_ptr<uint128_t>, double) const;
         double __floatsidf(int32_t) const;
         void __floatsitf(legacy_ptr<float128_t>, int32_t) const;
         void __floatditf(legacy_ptr<float128_t>, uint64_t) const;
         void __floatunsitf(legacy_ptr<float128_t>, uint32_t) const;
         void __floatunditf(legacy_ptr<float128_t>, uint64_t) const;
         double __floattidf(uint64_t, uint64_t) const;
         double __floatuntidf(uint64_t, uint64_t) const;
         int32_t __cmptf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __eqtf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __netf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __getf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __gttf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __letf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __lttf2(uint64_t, uint64_t, uint64_t, uint64_t) const;
         int32_t __unordtf2(uint64_t, uint64_t, uint64_t, uint64_t) const;

         REGISTER_HOST_FUNCTION(__ashlti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__ashrti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__lshlti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__lshrti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__divti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__udivti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__multi3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__modti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__umodti3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__addtf3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__subtf3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__multf3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__divtf3, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__negtf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__extendsftf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__extenddftf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__trunctfdf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__trunctfsf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixtfsi, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixtfdi, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixtfti, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixunstfsi, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixunstfdi, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixunstfti, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixsfti, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixdfti, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixunssfti, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__fixunsdfti, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__floatsidf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__floatsitf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__floatditf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__floatunsitf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__floatunditf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__floattidf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__floatuntidf, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__cmptf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__eqtf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__netf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__getf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__gttf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__letf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__lttf2, legacy_static_check_wl_args);
         REGISTER_HOST_FUNCTION(__unordtf2, legacy_static_check_wl_args);

         // call depth api
         void call_depth_assert();
         REGISTER_INJECTED_HOST_FUNCTION(call_depth_assert);

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
