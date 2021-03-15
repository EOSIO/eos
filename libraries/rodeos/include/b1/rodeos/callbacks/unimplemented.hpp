#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>

namespace b1::rodeos {

template <typename Derived>
struct unimplemented_callbacks {
   template <typename T>
   T unimplemented(const char* name) {
      throw std::runtime_error("wasm called " + std::string(name) + ", which is unimplemented");
   }

   // privileged_api
   int  is_feature_active(int64_t) { return unimplemented<int>("is_feature_active"); }
   void activate_feature(int64_t) { return unimplemented<void>("activate_feature"); }
   void get_resource_limits(int64_t, int, int, int) { return unimplemented<void>("get_resource_limits"); }
   void set_resource_limits(int64_t, int64_t, int64_t, int64_t) { return unimplemented<void>("set_resource_limits"); }
   int64_t set_proposed_producers(int, int) { return unimplemented<int64_t>("set_proposed_producers"); }
   int     get_blockchain_parameters_packed(int, int) { return unimplemented<int>("get_blockchain_parameters_packed"); }
   void set_blockchain_parameters_packed(int, int) { return unimplemented<void>("set_blockchain_parameters_packed"); }
   uint32_t get_parameters_packed(eosio::vm::span<const char>, eosio::vm::span<char>) const { return unimplemented<uint32_t>("get_parameters_packed"); }
   void set_parameters_packed( eosio::vm::span<const char> ) { return unimplemented<uint32_t>("set_parameters_packed"); }

   int  is_privileged(int64_t) { return unimplemented<int>("is_privileged"); }
   void set_privileged(int64_t, int) { return unimplemented<void>("set_privileged"); }
   void preactivate_feature(int) { return unimplemented<void>("preactivate_feature"); }
   bool set_transaction_resource_payer(int64_t, int64_t, int64_t) { return unimplemented<bool>("set_transaction_resource_payer"); }

   // producer_api
   int get_active_producers(int, int) { return unimplemented<int>("get_active_producers"); }

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(IDX)                                                                         \
   int  db_##IDX##_store(int64_t, int64_t, int64_t, int64_t, int) { return unimplemented<int>("db_" #IDX "_store"); }  \
   void db_##IDX##_remove(int) { return unimplemented<void>("db_" #IDX "_remove"); }                                   \
   void db_##IDX##_update(int, int64_t, int) { return unimplemented<void>("db_" #IDX "_update"); }                     \
   int  db_##IDX##_find_primary(int64_t, int64_t, int64_t, int, int64_t) {                                             \
      return unimplemented<int>("db_" #IDX "_find_primary");                                                          \
   }                                                                                                                   \
   int db_##IDX##_find_secondary(int64_t, int64_t, int64_t, int, int) {                                                \
      return unimplemented<int>("db_" #IDX "_find_secondary");                                                         \
   }                                                                                                                   \
   int db_##IDX##_lowerbound(int64_t, int64_t, int64_t, int, int) {                                                    \
      return unimplemented<int>("db_" #IDX "_lowerbound");                                                             \
   }                                                                                                                   \
   int db_##IDX##_upperbound(int64_t, int64_t, int64_t, int, int) {                                                    \
      return unimplemented<int>("db_" #IDX "_upperbound");                                                             \
   }                                                                                                                   \
   int db_##IDX##_end(int64_t, int64_t, int64_t) { return unimplemented<int>("db_" #IDX "_end"); }                     \
   int db_##IDX##_next(int, int) { return unimplemented<int>("db_" #IDX "_next"); }                                    \
   int db_##IDX##_previous(int, int) { return unimplemented<int>("db_" #IDX "_previous"); }

#define DB_SECONDARY_INDEX_METHODS_ARRAY(IDX)                                                                          \
   int db_##IDX##_store(int64_t, int64_t, int64_t, int64_t, int, int) {                                                \
      return unimplemented<int>("db_" #IDX "_store");                                                                  \
   }                                                                                                                   \
   void db_##IDX##_remove(int) { return unimplemented<void>("db_" #IDX "_remove"); }                                   \
   void db_##IDX##_update(int, int64_t, int, int) { return unimplemented<void>("db_" #IDX "_update"); }                \
   int  db_##IDX##_find_primary(int64_t, int64_t, int64_t, int, int, int64_t) {                                        \
      return unimplemented<int>("db_" #IDX "_find_primary");                                                          \
   }                                                                                                                   \
   int db_##IDX##_find_secondary(int64_t, int64_t, int64_t, int, int, int) {                                           \
      return unimplemented<int>("db_" #IDX "_find_secondary");                                                         \
   }                                                                                                                   \
   int db_##IDX##_lowerbound(int64_t, int64_t, int64_t, int, int, int) {                                               \
      return unimplemented<int>("db_" #IDX "_lowerbound");                                                             \
   }                                                                                                                   \
   int db_##IDX##_upperbound(int64_t, int64_t, int64_t, int, int, int) {                                               \
      return unimplemented<int>("db_" #IDX "_upperbound");                                                             \
   }                                                                                                                   \
   int db_##IDX##_end(int64_t, int64_t, int64_t) { return unimplemented<int>("db_" #IDX "_end"); }                     \
   int db_##IDX##_next(int, int) { return unimplemented<int>("db_" #IDX "_next"); }                                    \
   int db_##IDX##_previous(int, int) { return unimplemented<int>("db_" #IDX "_previous"); }

   // database_api
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64)
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx128)
   DB_SECONDARY_INDEX_METHODS_ARRAY(idx256)
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_double)
   DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_long_double)

#undef DB_SECONDARY_INDEX_METHODS_SIMPLE
#undef DB_SECONDARY_INDEX_METHODS_ARRAY

   // crypto_api
   void assert_recover_key(int, int, int, int, int) { return unimplemented<void>("assert_recover_key"); }
   int  recover_key(int, int, int, int, int) { return unimplemented<int>("recover_key"); }
   void assert_sha256(int, int, int) { return unimplemented<void>("assert_sha256"); }
   void assert_sha1(int, int, int) { return unimplemented<void>("assert_sha1"); }
   void assert_sha512(int, int, int) { return unimplemented<void>("assert_sha512"); }
   void assert_ripemd160(int, int, int) { return unimplemented<void>("assert_ripemd160"); }
   void sha1(int, int, int) { return unimplemented<void>("sha1"); }
   void sha256(int, int, int) { return unimplemented<void>("sha256"); }
   void sha512(int, int, int) { return unimplemented<void>("sha512"); }
   void ripemd160(int, int, int) { return unimplemented<void>("ripemd160"); }

   // permission_api
   int check_transaction_authorization(int, int, int, int, int, int) {
      return unimplemented<int>("check_transaction_authorization");
   }
   int check_permission_authorization(int64_t, int64_t, int, int, int, int, int64_t) {
      return unimplemented<int>("check_permission_authorization");
   }
   int64_t get_permission_last_used(int64_t, int64_t) { return unimplemented<int64_t>("get_permission_last_used"); }
   int64_t get_account_creation_time(int64_t) { return unimplemented<int64_t>("get_account_creation_time"); }

   // system_api
   int64_t publication_time() { return unimplemented<int64_t>("publication_time"); }
   int     is_feature_activated(int) { return unimplemented<int>("is_feature_activated"); }
   int64_t get_sender() { return unimplemented<int64_t>("get_sender"); }

   // context_free_system_api
   void eosio_assert_code(int, int64_t) { return unimplemented<void>("eosio_assert_code"); }
   void eosio_exit(int) { return unimplemented<void>("eosio_exit"); }

   // authorization_api
   void require_recipient(int64_t) { return unimplemented<void>("require_recipient"); }
   void require_auth(int64_t) { return unimplemented<void>("require_auth"); }
   void require_auth2(int64_t, int64_t) { return unimplemented<void>("require_auth2"); }
   int  has_auth(int64_t) { return unimplemented<int>("has_auth"); }
   int  is_account(int64_t) { return unimplemented<int>("is_account"); }

   // context_free_transaction_api
   int read_transaction(int, int) { return unimplemented<int>("read_transaction"); }
   int transaction_size() { return unimplemented<int>("transaction_size"); }
   int expiration() { return unimplemented<int>("expiration"); }
   int tapos_block_prefix() { return unimplemented<int>("tapos_block_prefix"); }
   int tapos_block_num() { return unimplemented<int>("tapos_block_num"); }
   int get_action(int, int, int, int) { return unimplemented<int>("get_action"); }

   // transaction_api
   void send_inline(int, int) { return unimplemented<void>("send_inline"); }
   void send_context_free_inline(int, int) { return unimplemented<void>("send_context_free_inline"); }
   void send_deferred(int, int64_t, int, int, int32_t) { return unimplemented<void>("send_deferred"); }
   int  cancel_deferred(int) { return unimplemented<int>("cancel_deferred"); }

   // context_free_api
   int get_context_free_data(int, int, int) { return unimplemented<int>("get_context_free_data"); }

   template <typename Rft>
   static void register_callbacks() {
      // todo: preconditions

      // privileged_api
      Rft::template add<&Derived::is_feature_active>("env", "is_feature_active");
      Rft::template add<&Derived::activate_feature>("env", "activate_feature");
      Rft::template add<&Derived::get_resource_limits>("env", "get_resource_limits");
      Rft::template add<&Derived::set_resource_limits>("env", "set_resource_limits");
      Rft::template add<&Derived::set_proposed_producers>("env", "set_proposed_producers");
      Rft::template add<&Derived::get_blockchain_parameters_packed>("env", "get_blockchain_parameters_packed");
      Rft::template add<&Derived::set_blockchain_parameters_packed>("env", "set_blockchain_parameters_packed");
      Rft::template add<&Derived::is_privileged>("env", "is_privileged");
      Rft::template add<&Derived::set_privileged>("env", "set_privileged");
      Rft::template add<&Derived::preactivate_feature>("env", "preactivate_feature");
      Rft::template add<&Derived::set_transaction_resource_payer>("env", "set_transaction_resource_payer");

      // producer_api
      Rft::template add<&Derived::get_active_producers>("env", "get_active_producers");

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(IDX)                                                                         \
   Rft::template add<&Derived::db_##IDX##_store>("env", "db_" #IDX "_store");                                          \
   Rft::template add<&Derived::db_##IDX##_remove>("env", "db_" #IDX "_remove");                                        \
   Rft::template add<&Derived::db_##IDX##_update>("env", "db_" #IDX "_update");                                        \
   Rft::template add<&Derived::db_##IDX##_find_primary>("env", "db_" #IDX "_find_primary");                            \
   Rft::template add<&Derived::db_##IDX##_find_secondary>("env", "db_" #IDX "_find_secondary");                        \
   Rft::template add<&Derived::db_##IDX##_lowerbound>("env", "db_" #IDX "_lowerbound");                                \
   Rft::template add<&Derived::db_##IDX##_upperbound>("env", "db_" #IDX "_upperbound");                                \
   Rft::template add<&Derived::db_##IDX##_end>("env", "db_" #IDX "_end");                                              \
   Rft::template add<&Derived::db_##IDX##_next>("env", "db_" #IDX "_next");                                            \
   Rft::template add<&Derived::db_##IDX##_previous>("env", "db_" #IDX "_previous");

#define DB_SECONDARY_INDEX_METHODS_ARRAY(IDX)                                                                          \
   Rft::template add<&Derived::db_##IDX##_store>("env", "db_" #IDX "_store");                                          \
   Rft::template add<&Derived::db_##IDX##_remove>("env", "db_" #IDX "_remove");                                        \
   Rft::template add<&Derived::db_##IDX##_update>("env", "db_" #IDX "_update");                                        \
   Rft::template add<&Derived::db_##IDX##_find_primary>("env", "db_" #IDX "_find_primary");                            \
   Rft::template add<&Derived::db_##IDX##_find_secondary>("env", "db_" #IDX "_find_secondary");                        \
   Rft::template add<&Derived::db_##IDX##_lowerbound>("env", "db_" #IDX "_lowerbound");                                \
   Rft::template add<&Derived::db_##IDX##_upperbound>("env", "db_" #IDX "_upperbound");                                \
   Rft::template add<&Derived::db_##IDX##_end>("env", "db_" #IDX "_end");                                              \
   Rft::template add<&Derived::db_##IDX##_next>("env", "db_" #IDX "_next");                                            \
   Rft::template add<&Derived::db_##IDX##_previous>("env", "db_" #IDX "_previous");

      // database_api
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx128)
      DB_SECONDARY_INDEX_METHODS_ARRAY(idx256)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_double)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_long_double)

#undef DB_SECONDARY_INDEX_METHODS_SIMPLE
#undef DB_SECONDARY_INDEX_METHODS_ARRAY

      // crypto_api
      Rft::template add<&Derived::assert_recover_key>("env", "assert_recover_key");
      Rft::template add<&Derived::recover_key>("env", "recover_key");
      Rft::template add<&Derived::assert_sha256>("env", "assert_sha256");
      Rft::template add<&Derived::assert_sha1>("env", "assert_sha1");
      Rft::template add<&Derived::assert_sha512>("env", "assert_sha512");
      Rft::template add<&Derived::assert_ripemd160>("env", "assert_ripemd160");
      Rft::template add<&Derived::sha1>("env", "sha1");
      Rft::template add<&Derived::sha256>("env", "sha256");
      Rft::template add<&Derived::sha512>("env", "sha512");
      Rft::template add<&Derived::ripemd160>("env", "ripemd160");

      // permission_api
      Rft::template add<&Derived::check_transaction_authorization>("env", "check_transaction_authorization");
      Rft::template add<&Derived::check_permission_authorization>("env", "check_permission_authorization");
      Rft::template add<&Derived::get_permission_last_used>("env", "get_permission_last_used");
      Rft::template add<&Derived::get_account_creation_time>("env", "get_account_creation_time");

      // system_api
      Rft::template add<&Derived::publication_time>("env", "publication_time");
      Rft::template add<&Derived::is_feature_activated>("env", "is_feature_activated");
      Rft::template add<&Derived::get_sender>("env", "get_sender");

      // context_free_system_api
      Rft::template add<&Derived::eosio_assert_code>("env", "eosio_assert_code");
      Rft::template add<&Derived::eosio_exit>("env", "eosio_exit");

      // authorization_api
      Rft::template add<&Derived::require_recipient>("env", "require_recipient");
      Rft::template add<&Derived::require_auth>("env", "require_auth");
      Rft::template add<&Derived::require_auth2>("env", "require_auth2");
      Rft::template add<&Derived::has_auth>("env", "has_auth");
      Rft::template add<&Derived::is_account>("env", "is_account");

      // context_free_transaction_api
      Rft::template add<&Derived::read_transaction>("env", "read_transaction");
      Rft::template add<&Derived::transaction_size>("env", "transaction_size");
      Rft::template add<&Derived::expiration>("env", "expiration");
      Rft::template add<&Derived::tapos_block_prefix>("env", "tapos_block_prefix");
      Rft::template add<&Derived::tapos_block_num>("env", "tapos_block_num");
      Rft::template add<&Derived::get_action>("env", "get_action");

      // transaction_api
      Rft::template add<&Derived::send_inline>("env", "send_inline");
      Rft::template add<&Derived::send_context_free_inline>("env", "send_context_free_inline");
      Rft::template add<&Derived::send_deferred>("env", "send_deferred");
      Rft::template add<&Derived::cancel_deferred>("env", "cancel_deferred");

      // context_free_api
      Rft::template add<&Derived::get_context_free_data>("env", "get_context_free_data");
   } // register_callbacks()
};   // unimplemented_callbacks

} // namespace b1::rodeos
