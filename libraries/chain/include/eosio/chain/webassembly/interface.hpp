#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <fc/crypto/sha1.hpp>
#include <boost/hana/string.hpp>

namespace eosio { namespace chain {
class apply_context;
namespace webassembly {

   class interface {
      public:
         interface(apply_context& ctx) : context(ctx) {}

         inline apply_context& get_context() { return context; }
         inline const apply_context& get_context() const { return context; }

         // context free api
         int32_t get_context_free_data(uint32_t index, legacy_span<char> buffer) const;

         // privileged api
         int32_t is_feature_active(int64_t feature_name) const;
         void activate_feature(int64_t feature_name) const;
         void preactivate_feature(legacy_ptr<const digest_type>);
         void set_resource_limits(account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
         void get_resource_limits(account_name account, legacy_ptr<int64_t, 8> ram_bytes, legacy_ptr<int64_t, 8> net_weight, legacy_ptr<int64_t, 8> cpu_weight) const;
         void set_resource_limit(account_name, name, int64_t);
         uint32_t get_wasm_parameters_packed( span<char> packed_parameters, uint32_t max_version ) const;
         void set_wasm_parameters_packed( span<const char> packed_parameters );
         int64_t get_resource_limit(account_name, name) const;
         int64_t set_proposed_producers(legacy_span<const char> packed_producer_schedule);
         int64_t set_proposed_producers_ex(uint64_t packed_producer_format, legacy_span<const char> packed_producer_schedule);
         uint32_t get_blockchain_parameters_packed(legacy_span<char> packed_blockchain_parameters) const;
         void set_blockchain_parameters_packed(legacy_span<const char> packed_blockchain_parameters);
         uint32_t get_parameters_packed( span<const char> packed_parameter_ids, span<char> packed_parameters) const;
         void set_parameters_packed( span<const char> packed_parameters );
         uint32_t get_kv_parameters_packed(span<char>, uint32_t) const;
         void set_kv_parameters_packed(span<const char>);
         bool is_privileged(account_name account) const;
         void set_privileged(account_name account, bool is_priv);

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

         // producer api
         int32_t get_active_producers(legacy_span<account_name>) const;

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

         // permission api
         bool check_transaction_authorization(legacy_span<const char>, legacy_span<const char>, legacy_span<const char>) const;
         bool check_permission_authorization(account_name, permission_name, legacy_span<const char>, legacy_span<const char>, uint64_t) const;
         int64_t get_permission_last_used(account_name, permission_name) const;
         int64_t get_account_creation_time(account_name) const;

         // authorization api
         void require_auth(account_name) const;
         void require_auth2(account_name, permission_name) const;
         bool has_auth(account_name) const;
         void require_recipient(account_name);
         bool is_account(account_name) const;

         // system api
         uint64_t current_time() const;
         uint64_t publication_time() const;
         bool is_feature_activated(legacy_ptr<const digest_type>) const;
         name get_sender() const;

         // context-free system api
         void abort() const;
         void eosio_assert(bool, null_terminated_ptr) const;
         void eosio_assert_message(bool, legacy_span<const char>) const;
         void eosio_assert_code(bool, uint64_t) const;
         void eosio_exit(int32_t) const;

         // action api
         int32_t read_action_data(legacy_span<char>) const;
         int32_t action_data_size() const;
         name current_receiver() const;
         void set_action_return_value(span<const char>);

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

         // kv database api
         int64_t  kv_erase(uint64_t, span<const char>);
         int64_t  kv_set(uint64_t, span<const char>, span<const char>, account_name payer);
         bool     kv_get(uint64_t, span<const char>, uint32_t*);
         uint32_t kv_get_data(uint32_t, span<char>);
         uint32_t kv_it_create(uint64_t, span<const char>);
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

         // memory api
         void* memcpy(memcpy_params) const;
         void* memmove(memcpy_params) const;
         int32_t memcmp(memcmp_params) const;
         void* memset(memset_params) const;

         // transaction api
         void send_inline(legacy_span<const char>);
         void send_context_free_inline(legacy_span<const char>);
         void send_deferred(legacy_ptr<const uint128_t>, account_name, legacy_span<const char>, uint32_t);
         bool cancel_deferred(legacy_ptr<const uint128_t>);

         // context-free transaction api
         int32_t read_transaction(legacy_span<char>) const;
         int32_t transaction_size() const;
         int32_t expiration() const;
         int32_t tapos_block_num() const;
         int32_t tapos_block_prefix() const;
         int32_t get_action(uint32_t, uint32_t, legacy_span<char>) const;

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

      private:
         apply_context& context;
   };

}}} // ns eosio::chain::webassembly
