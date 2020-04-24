#pragma once

#include <b1/rodeos/callbacks/basic.hpp>

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
   int  is_privileged(int64_t) { return unimplemented<int>("is_privileged"); }
   void set_privileged(int64_t, int) { return unimplemented<void>("set_privileged"); }
   void preactivate_feature(int) { return unimplemented<void>("preactivate_feature"); }

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
   void eosio_assert(uint32_t test, const char* msg) {
      // todo: bounds check
      // todo: move out of unimplemented_callbacks.hpp
      if (test == 0)
         throw std::runtime_error(std::string("assertion failed: ") + msg);
   }
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

   template <typename Rft, typename Allocator>
   static void register_callbacks() {
      // privileged_api
      Rft::template add<Derived, &Derived::is_feature_active, Allocator>("env", "is_feature_active");
      Rft::template add<Derived, &Derived::activate_feature, Allocator>("env", "activate_feature");
      Rft::template add<Derived, &Derived::get_resource_limits, Allocator>("env", "get_resource_limits");
      Rft::template add<Derived, &Derived::set_resource_limits, Allocator>("env", "set_resource_limits");
      Rft::template add<Derived, &Derived::set_proposed_producers, Allocator>("env", "set_proposed_producers");
      Rft::template add<Derived, &Derived::get_blockchain_parameters_packed, Allocator>(
            "env", "get_blockchain_parameters_packed");
      Rft::template add<Derived, &Derived::set_blockchain_parameters_packed, Allocator>(
            "env", "set_blockchain_parameters_packed");
      Rft::template add<Derived, &Derived::is_privileged, Allocator>("env", "is_privileged");
      Rft::template add<Derived, &Derived::set_privileged, Allocator>("env", "set_privileged");
      Rft::template add<Derived, &Derived::preactivate_feature, Allocator>("env", "preactivate_feature");

      // producer_api
      Rft::template add<Derived, &Derived::get_active_producers, Allocator>("env", "get_active_producers");

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(IDX)                                                                         \
   Rft::template add<Derived, &Derived::db_##IDX##_store, Allocator>("env", "db_" #IDX "_store");                      \
   Rft::template add<Derived, &Derived::db_##IDX##_remove, Allocator>("env", "db_" #IDX "_remove");                    \
   Rft::template add<Derived, &Derived::db_##IDX##_update, Allocator>("env", "db_" #IDX "_update");                    \
   Rft::template add<Derived, &Derived::db_##IDX##_find_primary, Allocator>("env", "db_" #IDX "_find_primary");        \
   Rft::template add<Derived, &Derived::db_##IDX##_find_secondary, Allocator>("env", "db_" #IDX "_find_secondary");    \
   Rft::template add<Derived, &Derived::db_##IDX##_lowerbound, Allocator>("env", "db_" #IDX "_lowerbound");            \
   Rft::template add<Derived, &Derived::db_##IDX##_upperbound, Allocator>("env", "db_" #IDX "_upperbound");            \
   Rft::template add<Derived, &Derived::db_##IDX##_end, Allocator>("env", "db_" #IDX "_end");                          \
   Rft::template add<Derived, &Derived::db_##IDX##_next, Allocator>("env", "db_" #IDX "_next");                        \
   Rft::template add<Derived, &Derived::db_##IDX##_previous, Allocator>("env", "db_" #IDX "_previous");

#define DB_SECONDARY_INDEX_METHODS_ARRAY(IDX)                                                                          \
   Rft::template add<Derived, &Derived::db_##IDX##_store, Allocator>("env", "db_" #IDX "_store");                      \
   Rft::template add<Derived, &Derived::db_##IDX##_remove, Allocator>("env", "db_" #IDX "_remove");                    \
   Rft::template add<Derived, &Derived::db_##IDX##_update, Allocator>("env", "db_" #IDX "_update");                    \
   Rft::template add<Derived, &Derived::db_##IDX##_find_primary, Allocator>("env", "db_" #IDX "_find_primary");        \
   Rft::template add<Derived, &Derived::db_##IDX##_find_secondary, Allocator>("env", "db_" #IDX "_find_secondary");    \
   Rft::template add<Derived, &Derived::db_##IDX##_lowerbound, Allocator>("env", "db_" #IDX "_lowerbound");            \
   Rft::template add<Derived, &Derived::db_##IDX##_upperbound, Allocator>("env", "db_" #IDX "_upperbound");            \
   Rft::template add<Derived, &Derived::db_##IDX##_end, Allocator>("env", "db_" #IDX "_end");                          \
   Rft::template add<Derived, &Derived::db_##IDX##_next, Allocator>("env", "db_" #IDX "_next");                        \
   Rft::template add<Derived, &Derived::db_##IDX##_previous, Allocator>("env", "db_" #IDX "_previous");

      // database_api
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx128)
      DB_SECONDARY_INDEX_METHODS_ARRAY(idx256)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_double)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_long_double)

#undef DB_SECONDARY_INDEX_METHODS_SIMPLE
#undef DB_SECONDARY_INDEX_METHODS_ARRAY

      // crypto_api
      Rft::template add<Derived, &Derived::assert_recover_key, Allocator>("env", "assert_recover_key");
      Rft::template add<Derived, &Derived::recover_key, Allocator>("env", "recover_key");
      Rft::template add<Derived, &Derived::assert_sha256, Allocator>("env", "assert_sha256");
      Rft::template add<Derived, &Derived::assert_sha1, Allocator>("env", "assert_sha1");
      Rft::template add<Derived, &Derived::assert_sha512, Allocator>("env", "assert_sha512");
      Rft::template add<Derived, &Derived::assert_ripemd160, Allocator>("env", "assert_ripemd160");
      Rft::template add<Derived, &Derived::sha1, Allocator>("env", "sha1");
      Rft::template add<Derived, &Derived::sha256, Allocator>("env", "sha256");
      Rft::template add<Derived, &Derived::sha512, Allocator>("env", "sha512");
      Rft::template add<Derived, &Derived::ripemd160, Allocator>("env", "ripemd160");

      // permission_api
      Rft::template add<Derived, &Derived::check_transaction_authorization, Allocator>(
            "env", "check_transaction_authorization");
      Rft::template add<Derived, &Derived::check_permission_authorization, Allocator>("env",
                                                                                      "check_permission_authorization");
      Rft::template add<Derived, &Derived::get_permission_last_used, Allocator>("env", "get_permission_last_used");
      Rft::template add<Derived, &Derived::get_account_creation_time, Allocator>("env", "get_account_creation_time");

      // system_api
      Rft::template add<Derived, &Derived::publication_time, Allocator>("env", "publication_time");
      Rft::template add<Derived, &Derived::is_feature_activated, Allocator>("env", "is_feature_activated");
      Rft::template add<Derived, &Derived::get_sender, Allocator>("env", "get_sender");

      // context_free_system_api
      Rft::template add<Derived, &Derived::eosio_assert, Allocator>("env", "eosio_assert");
      Rft::template add<Derived, &Derived::eosio_assert_code, Allocator>("env", "eosio_assert_code");
      Rft::template add<Derived, &Derived::eosio_exit, Allocator>("env", "eosio_exit");

      // authorization_api
      Rft::template add<Derived, &Derived::require_recipient, Allocator>("env", "require_recipient");
      Rft::template add<Derived, &Derived::require_auth, Allocator>("env", "require_auth");
      Rft::template add<Derived, &Derived::require_auth2, Allocator>("env", "require_auth2");
      Rft::template add<Derived, &Derived::has_auth, Allocator>("env", "has_auth");
      Rft::template add<Derived, &Derived::is_account, Allocator>("env", "is_account");

      // context_free_transaction_api
      Rft::template add<Derived, &Derived::read_transaction, Allocator>("env", "read_transaction");
      Rft::template add<Derived, &Derived::transaction_size, Allocator>("env", "transaction_size");
      Rft::template add<Derived, &Derived::expiration, Allocator>("env", "expiration");
      Rft::template add<Derived, &Derived::tapos_block_prefix, Allocator>("env", "tapos_block_prefix");
      Rft::template add<Derived, &Derived::tapos_block_num, Allocator>("env", "tapos_block_num");
      Rft::template add<Derived, &Derived::get_action, Allocator>("env", "get_action");

      // transaction_api
      Rft::template add<Derived, &Derived::send_inline, Allocator>("env", "send_inline");
      Rft::template add<Derived, &Derived::send_context_free_inline, Allocator>("env", "send_context_free_inline");
      Rft::template add<Derived, &Derived::send_deferred, Allocator>("env", "send_deferred");
      Rft::template add<Derived, &Derived::cancel_deferred, Allocator>("env", "cancel_deferred");

      // context_free_api
      Rft::template add<Derived, &Derived::get_context_free_data, Allocator>("env", "get_context_free_data");
   } // register_callbacks()
};   // unimplemented_callbacks

} // namespace b1::rodeos
