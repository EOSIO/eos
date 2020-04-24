#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/chain/webassembly/preconditions.hpp>
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
   inline static host_function_registrator<&interface::NAME, ##__VA_ARGS__> NAME ## _registrator = {eosio_injected_module_name, #NAME};

#define REGISTER_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, core_precondition, context_aware_check, ##__VA_ARGS__> NAME ## _registrator = {"env", #NAME};

#define REGISTER_CF_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, core_precondition, ##__VA_ARGS__> NAME ## _registrator = {"env", #NAME};

#define REGISTER_LEGACY_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, null_pointer_check, legacy_static_check_wl_args, context_aware_check, ##__VA_ARGS__> NAME ## _registrator = {"env", #NAME};

#define REGISTER_LEGACY_CF_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, null_pointer_check, legacy_static_check_wl_args, ##__VA_ARGS__> NAME ## _registrator = {"env", #NAME};

#define REGISTER_LEGACY_CF_ONLY_HOST_FUNCTION(NAME, ...) \
   inline static host_function_registrator<&interface::NAME, null_pointer_check, legacy_static_check_wl_args, context_free_check, ##__VA_ARGS__> NAME ## _registrator = {"env", #NAME};

   class interface {
      public:
         interface(apply_context& ctx) : context(ctx) {}

         inline apply_context& get_context() { return context; }
         inline const apply_context& get_context() const { return context; }

         // context free api
         int32_t get_context_free_data(uint32_t index, legacy_span<char> buffer) const;
         REGISTER_LEGACY_CF_ONLY_HOST_FUNCTION(get_context_free_data)

         // privileged api
         int32_t is_feature_active(int64_t feature_name) const;
         void activate_feature(int64_t feature_name) const;
         void preactivate_feature(legacy_ptr<const digest_type>);
         void set_resource_limits(account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
         void get_resource_limits(account_name account, legacy_ptr<int64_t, 8> ram_bytes, legacy_ptr<int64_t, 8> net_weight, legacy_ptr<int64_t, 8> cpu_weight) const;
         void set_resource_limit(account_name, name, int64_t);
         int64_t get_resource_limit(account_name, name) const;
         int64_t set_proposed_producers(legacy_span<const char> packed_producer_schedule);
         int64_t set_proposed_producers_ex(uint64_t packed_producer_format, legacy_span<const char> packed_producer_schedule);
         uint32_t get_blockchain_parameters_packed(legacy_span<char> packed_blockchain_parameters) const;
         void set_blockchain_parameters_packed(legacy_span<const char> packed_blockchain_parameters);
         uint32_t get_kv_parameters_packed(name, span<char>, uint32_t) const;
         void set_kv_parameters_packed(name, span<const char>);
         bool is_privileged(account_name account) const;
         void set_privileged(account_name account, bool is_priv);

         REGISTER_HOST_FUNCTION(is_feature_active, privileged_check);
         REGISTER_HOST_FUNCTION(activate_feature, privileged_check);
         REGISTER_LEGACY_HOST_FUNCTION(preactivate_feature, privileged_check);
         REGISTER_HOST_FUNCTION(set_resource_limits, privileged_check);
         REGISTER_LEGACY_HOST_FUNCTION(get_resource_limits, privileged_check);
         REGISTER_HOST_FUNCTION(set_resource_limit, privileged_check);
         REGISTER_HOST_FUNCTION(get_resource_limit, privileged_check);
         REGISTER_LEGACY_HOST_FUNCTION(set_proposed_producers, privileged_check);
         REGISTER_LEGACY_HOST_FUNCTION(set_proposed_producers_ex, privileged_check);
         REGISTER_LEGACY_HOST_FUNCTION(get_blockchain_parameters_packed, privileged_check);
         REGISTER_LEGACY_HOST_FUNCTION(set_blockchain_parameters_packed, privileged_check);
         REGISTER_HOST_FUNCTION(get_kv_parameters_packed, privileged_check);
         REGISTER_HOST_FUNCTION(set_kv_parameters_packed, privileged_check);
         REGISTER_HOST_FUNCTION(is_privileged, privileged_check);
         REGISTER_HOST_FUNCTION(set_privileged, privileged_check);

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

         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_add);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_sub);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_div);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_mul);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_min);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_max);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_copysign);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_abs);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_neg);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_sqrt);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_ceil);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_floor);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_nearest);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_eq);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_ne);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_lt);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_le);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_gt);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_ge);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_add);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_sub);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_div);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_mul);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_min);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_max);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_copysign);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_abs);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_neg);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_sqrt);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_ceil);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_floor);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_nearest);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_eq);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_ne);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_lt);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_le);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_gt);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_ge);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_promote);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_demote);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i32s);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i32s);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i32u);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i32u);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i64s);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i64s);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f32_trunc_i64u);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_f64_trunc_i64u);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i32_to_f32);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i64_to_f32);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui32_to_f32);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui64_to_f32);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i32_to_f64);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_i64_to_f64);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui32_to_f64);
         REGISTER_INJECTED_HOST_FUNCTION(_eosio_ui64_to_f64);

         // producer api
         int32_t get_active_producers(legacy_span<account_name>) const;
         REGISTER_LEGACY_HOST_FUNCTION(get_active_producers);

         // crypto api
         void assert_recover_key(legacy_ptr<const fc::sha256>, legacy_span<const char>, legacy_span<const char>) const;
         int32_t recover_key(legacy_ptr<const fc::sha256>, legacy_span<const char>, legacy_span<char>) const;
         void assert_sha256(legacy_span<const char>, legacy_ptr<const fc::sha256>) const;
         void assert_sha1(legacy_span<const char>, legacy_ptr<const fc::sha1>) const;
         void assert_sha512(legacy_span<const char>, legacy_ptr<const fc::sha512>) const;
         void assert_ripemd160(legacy_span<const char>, legacy_ptr<const fc::ripemd160>) const;
         void sha256(legacy_span<const char>, legacy_ptr<fc::sha256>) const;
         void sha1(legacy_span<const char>, legacy_ptr<fc::sha1>) const;
         void sha512(legacy_span<const char>, legacy_ptr<fc::sha512>) const;
         void ripemd160(legacy_span<const char>, legacy_ptr<fc::ripemd160>) const;

         REGISTER_LEGACY_CF_HOST_FUNCTION(assert_recover_key);
         REGISTER_LEGACY_CF_HOST_FUNCTION(recover_key);
         REGISTER_LEGACY_CF_HOST_FUNCTION(assert_sha256);
         REGISTER_LEGACY_CF_HOST_FUNCTION(assert_sha1);
         REGISTER_LEGACY_CF_HOST_FUNCTION(assert_sha512);
         REGISTER_LEGACY_CF_HOST_FUNCTION(assert_ripemd160);
         REGISTER_LEGACY_CF_HOST_FUNCTION(sha256);
         REGISTER_LEGACY_CF_HOST_FUNCTION(sha1);
         REGISTER_LEGACY_CF_HOST_FUNCTION(sha512);
         REGISTER_LEGACY_CF_HOST_FUNCTION(ripemd160);

         // permission api
         bool check_transaction_authorization(legacy_span<const char>, legacy_span<const char>, legacy_span<const char>) const;
         bool check_permission_authorization(account_name, permission_name, legacy_span<const char>, legacy_span<const char>, uint64_t) const;
         int64_t get_permission_last_used(account_name, permission_name) const;
         int64_t get_account_creation_time(account_name) const;

         REGISTER_LEGACY_HOST_FUNCTION(check_transaction_authorization);
         REGISTER_LEGACY_HOST_FUNCTION(check_permission_authorization);
         REGISTER_HOST_FUNCTION(get_permission_last_used);
         REGISTER_HOST_FUNCTION(get_account_creation_time);

         // authorization api
         void require_auth(account_name) const;
         void require_auth2(account_name, permission_name) const;
         bool has_auth(account_name) const;
         void require_recipient(account_name);
         bool is_account(account_name) const;

         REGISTER_HOST_FUNCTION(require_auth);
         REGISTER_HOST_FUNCTION(require_auth2);
         REGISTER_HOST_FUNCTION(has_auth);
         REGISTER_HOST_FUNCTION(require_recipient);
         REGISTER_HOST_FUNCTION(is_account);

         // system api
         uint64_t current_time() const;
         uint64_t publication_time() const;
         bool is_feature_activated(legacy_ptr<const digest_type>) const;
         name get_sender() const;

         REGISTER_HOST_FUNCTION(current_time);
         REGISTER_HOST_FUNCTION(publication_time);
         REGISTER_LEGACY_HOST_FUNCTION(is_feature_activated);
         REGISTER_HOST_FUNCTION(get_sender);

         // context-free system api
         void abort() const;
         void eosio_assert(bool, null_terminated_ptr) const;
         void eosio_assert_message(bool, legacy_span<const char>) const;
         void eosio_assert_code(bool, uint64_t) const;
         void eosio_exit(int32_t) const;

         REGISTER_CF_HOST_FUNCTION(abort)
         REGISTER_LEGACY_CF_HOST_FUNCTION(eosio_assert)
         REGISTER_LEGACY_CF_HOST_FUNCTION(eosio_assert_message)
         REGISTER_CF_HOST_FUNCTION(eosio_assert_code)
         REGISTER_CF_HOST_FUNCTION(eosio_exit)

         // action api
         int32_t read_action_data(legacy_span<char>) const;
         int32_t action_data_size() const;
         name current_receiver() const;
         void set_action_return_value(span<const char>);

         REGISTER_LEGACY_CF_HOST_FUNCTION(read_action_data);
         REGISTER_CF_HOST_FUNCTION(action_data_size);
         REGISTER_CF_HOST_FUNCTION(current_receiver);
         REGISTER_HOST_FUNCTION(set_action_return_value);

         // console api
         void prints(null_terminated_ptr);
         void prints_l(legacy_span<const char>);
         void printi(int64_t);
         void printui(uint64_t);
         void printi128(legacy_ptr<const __int128>);
         void printui128(legacy_ptr<const unsigned __int128>);
         void printsf(float32_t);
         void printdf(float64_t);
         void printqf(legacy_ptr<const float128_t>);
         void printn(name);
         void printhex(legacy_span<const char>);

         REGISTER_LEGACY_CF_HOST_FUNCTION(prints);
         REGISTER_LEGACY_CF_HOST_FUNCTION(prints_l);
         REGISTER_CF_HOST_FUNCTION(printi);
         REGISTER_CF_HOST_FUNCTION(printui);
         REGISTER_LEGACY_CF_HOST_FUNCTION(printi128);
         REGISTER_LEGACY_CF_HOST_FUNCTION(printui128);
         REGISTER_CF_HOST_FUNCTION(printsf);
         REGISTER_CF_HOST_FUNCTION(printdf);
         REGISTER_LEGACY_CF_HOST_FUNCTION(printqf);
         REGISTER_CF_HOST_FUNCTION(printn);
         REGISTER_LEGACY_CF_HOST_FUNCTION(printhex);

         // database api
         // primary index api
         int32_t db_store_i64(uint64_t, uint64_t, uint64_t, uint64_t, legacy_span<const char>);
         void db_update_i64(int32_t, uint64_t, legacy_span<const char>);
         void db_remove_i64(int32_t);
         int32_t db_get_i64(int32_t, legacy_span<char>);
         int32_t db_next_i64(int32_t, legacy_ptr<uint64_t>);
         int32_t db_previous_i64(int32_t, legacy_ptr<uint64_t>);
         int32_t db_find_i64(uint64_t, uint64_t, uint64_t, uint64_t);
         int32_t db_lowerbound_i64(uint64_t, uint64_t, uint64_t, uint64_t);
         int32_t db_upperbound_i64(uint64_t, uint64_t, uint64_t, uint64_t);
         int32_t db_end_i64(uint64_t, uint64_t, uint64_t);

         REGISTER_LEGACY_HOST_FUNCTION(db_store_i64);
         REGISTER_LEGACY_HOST_FUNCTION(db_update_i64);
         REGISTER_HOST_FUNCTION(db_remove_i64);
         REGISTER_LEGACY_HOST_FUNCTION(db_get_i64);
         REGISTER_LEGACY_HOST_FUNCTION(db_next_i64);
         REGISTER_LEGACY_HOST_FUNCTION(db_previous_i64);
         REGISTER_HOST_FUNCTION(db_find_i64);
         REGISTER_HOST_FUNCTION(db_lowerbound_i64);
         REGISTER_HOST_FUNCTION(db_upperbound_i64);
         REGISTER_HOST_FUNCTION(db_end_i64);


         // uint64_t secondary index api
         int32_t db_idx64_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint64_t>);
         void db_idx64_update(int32_t, uint64_t, legacy_ptr<const uint64_t>);
         void db_idx64_remove(int32_t);
         int32_t db_idx64_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const uint64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx64_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t>, uint64_t);
         int32_t db_idx64_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t, 8>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx64_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint64_t, 8>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx64_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx64_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx64_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_store);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_update);
         REGISTER_HOST_FUNCTION(db_idx64_remove);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_find_secondary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_find_primary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_lowerbound);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_upperbound);
         REGISTER_HOST_FUNCTION(db_idx64_end);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_next);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx64_previous);

         // uint128_t secondary index api
         int32_t db_idx128_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const uint128_t>);
         void db_idx128_update(int32_t, uint64_t, legacy_ptr<const uint128_t>);
         void db_idx128_remove(int32_t);
         int32_t db_idx128_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx128_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t>, uint64_t);
         int32_t db_idx128_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t, 16>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx128_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<uint128_t, 16>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx128_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx128_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx128_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_store);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_update);
         REGISTER_HOST_FUNCTION(db_idx128_remove);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_find_secondary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_find_primary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_lowerbound);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_upperbound);
         REGISTER_HOST_FUNCTION(db_idx128_end);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_next);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx128_previous);

         // 256-bit secondary index api
         int32_t db_idx256_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_span<const uint128_t>);
         void db_idx256_update(int32_t, uint64_t, legacy_span<const uint128_t>);
         void db_idx256_remove(int32_t);
         int32_t db_idx256_find_secondary(uint64_t, uint64_t, uint64_t, legacy_span<const uint128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx256_find_primary(uint64_t, uint64_t, uint64_t, legacy_span<uint128_t>, uint64_t);
         int32_t db_idx256_lowerbound(uint64_t, uint64_t, uint64_t, legacy_span<uint128_t, 16>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx256_upperbound(uint64_t, uint64_t, uint64_t, legacy_span<uint128_t, 16>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx256_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx256_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx256_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_store);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_update);
         REGISTER_HOST_FUNCTION(db_idx256_remove);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_find_secondary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_find_primary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_lowerbound);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_upperbound);
         REGISTER_HOST_FUNCTION(db_idx256_end);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_next);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx256_previous);

         // double secondary index api
         int32_t db_idx_double_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const float64_t>);
         void db_idx_double_update(int32_t, uint64_t, legacy_ptr<const float64_t>);
         void db_idx_double_remove(int32_t);
         int32_t db_idx_double_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const float64_t>, legacy_ptr<uint64_t>);
         int32_t db_idx_double_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t>, uint64_t);
         int32_t db_idx_double_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t, 8>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx_double_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float64_t, 8>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx_double_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx_double_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx_double_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_store, is_nan_check);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_update, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_double_remove);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_find_secondary, is_nan_check);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_find_primary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_lowerbound, is_nan_check);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_upperbound, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_double_end);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_next);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_double_previous);


         // long double secondary index api
         int32_t db_idx_long_double_store(uint64_t, uint64_t, uint64_t, uint64_t, legacy_ptr<const float128_t>);
         void db_idx_long_double_update(int32_t, uint64_t, legacy_ptr<const float128_t>);
         void db_idx_long_double_remove(int32_t);
         int32_t db_idx_long_double_find_secondary(uint64_t, uint64_t, uint64_t, legacy_ptr<const float128_t>, legacy_ptr<uint64_t>);
         int32_t db_idx_long_double_find_primary(uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t>, uint64_t);
         int32_t db_idx_long_double_lowerbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t, 8>, legacy_ptr<uint64_t, 8>);
         int32_t db_idx_long_double_upperbound(uint64_t, uint64_t, uint64_t, legacy_ptr<float128_t, 8>,  legacy_ptr<uint64_t, 8>);
         int32_t db_idx_long_double_end(uint64_t, uint64_t, uint64_t);
         int32_t db_idx_long_double_next(int32_t, legacy_ptr<uint64_t>);
         int32_t db_idx_long_double_previous(int32_t, legacy_ptr<uint64_t>);

         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_store, is_nan_check);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_update, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_long_double_remove);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_find_secondary, is_nan_check);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_find_primary);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_lowerbound, is_nan_check);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_upperbound, is_nan_check);
         REGISTER_HOST_FUNCTION(db_idx_long_double_end);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_next);
         REGISTER_LEGACY_HOST_FUNCTION(db_idx_long_double_previous);

         // kv database api
         int64_t  kv_erase(uint64_t, uint64_t, span<const char>);
         int64_t  kv_set(uint64_t, uint64_t, span<const char>, span<const char>);
         bool     kv_get(uint64_t, uint64_t, span<const char>, uint32_t*);
         uint32_t kv_get_data(uint64_t, uint32_t, span<char>);
         uint32_t kv_it_create(uint64_t, uint64_t, span<const char>);
         void     kv_it_destroy(uint32_t);
         int32_t  kv_it_status(uint32_t);
         int32_t  kv_it_compare(uint32_t, uint32_t);
         int32_t  kv_it_key_compare(uint32_t, span<const char>);
         int32_t  kv_it_move_to_end(uint32_t);
         int32_t  kv_it_next(uint32_t, uint32_t* found_key_size, uint32_t* found_value_size);
         int32_t  kv_it_prev(uint32_t, uint32_t* found_key_size, uint32_t* found_value_size);
         int32_t  kv_it_lower_bound(uint32_t, span<const char>, uint32_t* found_key_size, uint32_t* found_value_size);
         int32_t  kv_it_key(uint32_t, uint32_t, span<char>, uint32_t*);
         int32_t  kv_it_value(uint32_t, uint32_t, span<char> dest, uint32_t*);

         REGISTER_HOST_FUNCTION(kv_erase);
         REGISTER_HOST_FUNCTION(kv_set);
         REGISTER_HOST_FUNCTION(kv_get);
         REGISTER_HOST_FUNCTION(kv_get_data);
         REGISTER_HOST_FUNCTION(kv_it_create);
         REGISTER_HOST_FUNCTION(kv_it_destroy);
         REGISTER_HOST_FUNCTION(kv_it_status);
         REGISTER_HOST_FUNCTION(kv_it_compare);
         REGISTER_HOST_FUNCTION(kv_it_key_compare);
         REGISTER_HOST_FUNCTION(kv_it_move_to_end);
         REGISTER_HOST_FUNCTION(kv_it_next);
         REGISTER_HOST_FUNCTION(kv_it_prev);
         REGISTER_HOST_FUNCTION(kv_it_lower_bound);
         REGISTER_HOST_FUNCTION(kv_it_key);
         REGISTER_HOST_FUNCTION(kv_it_value);

         // memory api
         void* memcpy(unvalidated_ptr<char>, unvalidated_ptr<const char>, wasm_size_t) const;
         void* memmove(unvalidated_ptr<char>, unvalidated_ptr<const char>, wasm_size_t) const;
         int32_t memcmp(unvalidated_ptr<const char>, unvalidated_ptr<const char>, wasm_size_t) const;
         void* memset(unvalidated_ptr<char>, int32_t, wasm_size_t) const;

         REGISTER_LEGACY_CF_HOST_FUNCTION(memcpy);
         REGISTER_LEGACY_CF_HOST_FUNCTION(memmove);
         REGISTER_LEGACY_CF_HOST_FUNCTION(memcmp);
         REGISTER_LEGACY_CF_HOST_FUNCTION(memset);

         // transaction api
         void send_inline(legacy_span<const char>);
         void send_context_free_inline(legacy_span<const char>);
         void send_deferred(legacy_ptr<const uint128_t>, account_name, legacy_span<const char>, uint32_t);
         bool cancel_deferred(legacy_ptr<const uint128_t>);

         REGISTER_LEGACY_HOST_FUNCTION(send_inline);
         REGISTER_LEGACY_HOST_FUNCTION(send_context_free_inline);
         REGISTER_LEGACY_HOST_FUNCTION(send_deferred);
         REGISTER_LEGACY_HOST_FUNCTION(cancel_deferred);

         // context-free transaction api
         int32_t read_transaction(legacy_span<char>) const;
         int32_t transaction_size() const;
         int32_t expiration() const;
         int32_t tapos_block_num() const;
         int32_t tapos_block_prefix() const;
         int32_t get_action(uint32_t, uint32_t, legacy_span<char>) const;

         REGISTER_LEGACY_CF_HOST_FUNCTION(read_transaction);
         REGISTER_CF_HOST_FUNCTION(transaction_size);
         REGISTER_CF_HOST_FUNCTION(expiration);
         REGISTER_CF_HOST_FUNCTION(tapos_block_num);
         REGISTER_CF_HOST_FUNCTION(tapos_block_prefix);
         REGISTER_LEGACY_CF_HOST_FUNCTION(get_action);

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

         REGISTER_LEGACY_CF_HOST_FUNCTION(__ashlti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__ashrti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__lshlti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__lshrti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__divti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__udivti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__multi3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__modti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__umodti3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__addtf3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__subtf3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__multf3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__divtf3);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__negtf2);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__extendsftf2);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__extenddftf2);
         REGISTER_CF_HOST_FUNCTION(__trunctfdf2);
         REGISTER_CF_HOST_FUNCTION(__trunctfsf2);
         REGISTER_CF_HOST_FUNCTION(__fixtfsi);
         REGISTER_CF_HOST_FUNCTION(__fixtfdi);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__fixtfti);
         REGISTER_CF_HOST_FUNCTION(__fixunstfsi);
         REGISTER_CF_HOST_FUNCTION(__fixunstfdi);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__fixunstfti);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__fixsfti);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__fixdfti);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__fixunssfti);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__fixunsdfti);
         REGISTER_CF_HOST_FUNCTION(__floatsidf);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__floatsitf);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__floatditf);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__floatunsitf);
         REGISTER_LEGACY_CF_HOST_FUNCTION(__floatunditf);
         REGISTER_CF_HOST_FUNCTION(__floattidf);
         REGISTER_CF_HOST_FUNCTION(__floatuntidf);
         REGISTER_CF_HOST_FUNCTION(__cmptf2);
         REGISTER_CF_HOST_FUNCTION(__eqtf2);
         REGISTER_CF_HOST_FUNCTION(__netf2);
         REGISTER_CF_HOST_FUNCTION(__getf2);
         REGISTER_CF_HOST_FUNCTION(__gttf2);
         REGISTER_CF_HOST_FUNCTION(__letf2);
         REGISTER_CF_HOST_FUNCTION(__lttf2);
         REGISTER_CF_HOST_FUNCTION(__unordtf2);

      private:
         apply_context& context;
   };

}}} // ns eosio::chain::webassembly

#undef REGISTER_LEGACY_CF_ONLY_HOST_FUNCTION
#undef REGISTER_LEGACY_CF_HOST_FUNCTION
#undef REGISTER_LEGACY_HOST_FUNCTION
#undef REGISTER_CF_HOST_FUNCTION
#undef REGISTER_HOST_FUNCTION
#undef REGISTER_INJECTED_HOST_FUNCTION
