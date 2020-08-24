
#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/chain/webassembly/preconditions.hpp>
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
#include <eosio/chain/webassembly/eos-vm-oc.hpp>
#endif
#include <eosio/chain/webassembly/interface.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/equal.hpp>

namespace eosio {
namespace chain {
namespace webassembly {

template <auto HostFunction, typename... Preconditions>
struct host_function_registrator {
   template <typename Mod, typename Name>
   constexpr host_function_registrator(Mod mod_name, Name fn_name) {
      using rhf_t = eos_vm_host_functions_t;
      rhf_t::add<HostFunction, Preconditions...>(mod_name.c_str(), fn_name.c_str());
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
      constexpr bool is_injected = (Mod() == BOOST_HANA_STRING(EOSIO_INJECTED_MODULE_NAME));
      eosvmoc::register_eosvm_oc<HostFunction, is_injected, std::tuple<Preconditions...>>(
          mod_name + BOOST_HANA_STRING(".") + fn_name);
#endif
   }
};

#define REGISTER_INJECTED_HOST_FUNCTION(NAME, ...)                                                                     \
   static host_function_registrator<&interface::NAME, ##__VA_ARGS__> NAME##_registrator_impl() {                       \
      return {BOOST_HANA_STRING(EOSIO_INJECTED_MODULE_NAME), BOOST_HANA_STRING(#NAME)};                                \
   }                                                                                                                   \
   inline static auto NAME##_registrator = NAME##_registrator_impl();

#define REGISTER_HOST_FUNCTION(NAME, ...)                                                                              \
   static host_function_registrator<&interface::NAME, core_precondition, context_aware_check, ##__VA_ARGS__>           \
       NAME##_registrator_impl() {                                                                                     \
      return {BOOST_HANA_STRING("env"), BOOST_HANA_STRING(#NAME)};                                                     \
   }                                                                                                                   \
   inline static auto NAME##_registrator = NAME##_registrator_impl();

#define REGISTER_CF_HOST_FUNCTION(NAME, ...)                                                                           \
   static host_function_registrator<&interface::NAME, core_precondition, ##__VA_ARGS__> NAME##_registrator_impl() {    \
      return {BOOST_HANA_STRING("env"), BOOST_HANA_STRING(#NAME)};                                                     \
   }                                                                                                                   \
   inline static auto NAME##_registrator = NAME##_registrator_impl();

#define REGISTER_LEGACY_HOST_FUNCTION(NAME, ...)                                                                       \
   static host_function_registrator<&interface::NAME, legacy_static_check_wl_args, context_aware_check, ##__VA_ARGS__> \
       NAME##_registrator_impl() {                                                                                     \
      return {BOOST_HANA_STRING("env"), BOOST_HANA_STRING(#NAME)};                                                     \
   }                                                                                                                   \
   inline static auto NAME##_registrator = NAME##_registrator_impl();

#define REGISTER_LEGACY_CF_HOST_FUNCTION(NAME, ...)                                                                    \
   static host_function_registrator<&interface::NAME, legacy_static_check_wl_args, ##__VA_ARGS__>                      \
       NAME##_registrator_impl() {                                                                                     \
      return {BOOST_HANA_STRING("env"), BOOST_HANA_STRING(#NAME)};                                                     \
   }                                                                                                                   \
   inline static auto NAME##_registrator = NAME##_registrator_impl();

#define REGISTER_LEGACY_CF_ONLY_HOST_FUNCTION(NAME, ...)                                                               \
   static host_function_registrator<&interface::NAME, legacy_static_check_wl_args, context_free_check, ##__VA_ARGS__>  \
       NAME##_registrator_impl() {                                                                                     \
      return {BOOST_HANA_STRING("env"), BOOST_HANA_STRING(#NAME)};                                                     \
   }                                                                                                                   \
   inline static auto NAME##_registrator = NAME##_registrator_impl();

// context free api
REGISTER_LEGACY_CF_ONLY_HOST_FUNCTION(get_context_free_data)

// privileged api
REGISTER_HOST_FUNCTION(is_feature_active, privileged_check);
REGISTER_HOST_FUNCTION(activate_feature, privileged_check);
REGISTER_LEGACY_HOST_FUNCTION(preactivate_feature, privileged_check);
REGISTER_HOST_FUNCTION(set_resource_limits, privileged_check);
REGISTER_LEGACY_HOST_FUNCTION(get_resource_limits, privileged_check);
REGISTER_HOST_FUNCTION(set_resource_limit, privileged_check);
REGISTER_HOST_FUNCTION(get_resource_limit, privileged_check);
REGISTER_HOST_FUNCTION(get_wasm_parameters_packed, privileged_check);
REGISTER_HOST_FUNCTION(set_wasm_parameters_packed, privileged_check);
REGISTER_LEGACY_HOST_FUNCTION(set_proposed_producers, privileged_check);
REGISTER_LEGACY_HOST_FUNCTION(set_proposed_producers_ex, privileged_check);
REGISTER_LEGACY_HOST_FUNCTION(get_blockchain_parameters_packed, privileged_check);
REGISTER_LEGACY_HOST_FUNCTION(set_blockchain_parameters_packed, privileged_check);
REGISTER_HOST_FUNCTION(get_parameters_packed, privileged_check);
REGISTER_HOST_FUNCTION(set_parameters_packed, privileged_check);
REGISTER_HOST_FUNCTION(get_kv_parameters_packed, privileged_check);
REGISTER_HOST_FUNCTION(set_kv_parameters_packed, privileged_check);
REGISTER_HOST_FUNCTION(is_privileged, privileged_check);
REGISTER_HOST_FUNCTION(set_privileged, privileged_check);

// softfloat api
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
REGISTER_LEGACY_HOST_FUNCTION(get_active_producers);

// crypto api
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
REGISTER_LEGACY_HOST_FUNCTION(check_transaction_authorization);
REGISTER_LEGACY_HOST_FUNCTION(check_permission_authorization);
REGISTER_HOST_FUNCTION(get_permission_last_used);
REGISTER_HOST_FUNCTION(get_account_creation_time);

// authorization api
REGISTER_HOST_FUNCTION(require_auth);
REGISTER_HOST_FUNCTION(require_auth2);
REGISTER_HOST_FUNCTION(has_auth);
REGISTER_HOST_FUNCTION(require_recipient);
REGISTER_HOST_FUNCTION(is_account);

// system api
REGISTER_HOST_FUNCTION(current_time);
REGISTER_HOST_FUNCTION(publication_time);
REGISTER_LEGACY_HOST_FUNCTION(is_feature_activated);
REGISTER_HOST_FUNCTION(get_sender);

// context-free system api
REGISTER_CF_HOST_FUNCTION(abort)
REGISTER_LEGACY_CF_HOST_FUNCTION(eosio_assert)
REGISTER_LEGACY_CF_HOST_FUNCTION(eosio_assert_message)
REGISTER_CF_HOST_FUNCTION(eosio_assert_code)
REGISTER_CF_HOST_FUNCTION(eosio_exit)

// action api
REGISTER_LEGACY_CF_HOST_FUNCTION(read_action_data);
REGISTER_CF_HOST_FUNCTION(action_data_size);
REGISTER_CF_HOST_FUNCTION(current_receiver);
REGISTER_HOST_FUNCTION(set_action_return_value);

// console api
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
REGISTER_LEGACY_CF_HOST_FUNCTION(memcpy);
REGISTER_LEGACY_CF_HOST_FUNCTION(memmove);
REGISTER_LEGACY_CF_HOST_FUNCTION(memcmp);
REGISTER_LEGACY_CF_HOST_FUNCTION(memset);

// transaction api
REGISTER_LEGACY_HOST_FUNCTION(send_inline);
REGISTER_LEGACY_HOST_FUNCTION(send_context_free_inline);
REGISTER_LEGACY_HOST_FUNCTION(send_deferred);
REGISTER_LEGACY_HOST_FUNCTION(cancel_deferred);

// context-free transaction api
REGISTER_LEGACY_CF_HOST_FUNCTION(read_transaction);
REGISTER_CF_HOST_FUNCTION(transaction_size);
REGISTER_CF_HOST_FUNCTION(expiration);
REGISTER_CF_HOST_FUNCTION(tapos_block_num);
REGISTER_CF_HOST_FUNCTION(tapos_block_prefix);
REGISTER_LEGACY_CF_HOST_FUNCTION(get_action);

// compiler builtins api
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

} // namespace webassembly
} // namespace chain
} // namespace eosio