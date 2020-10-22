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
      RODEOS_REGISTER_CALLBACK(Rft, Derived, is_feature_active);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, activate_feature);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_resource_limits);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, set_resource_limits);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, set_proposed_producers);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_blockchain_parameters_packed);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, set_blockchain_parameters_packed);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, is_privileged);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, set_privileged);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, preactivate_feature);

      // producer_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_active_producers);

#define DB_SECONDARY_INDEX_METHODS_SIMPLE(IDX)                                                                         \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_store);                                                           \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_remove);                                                          \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_update);                                                          \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_find_primary);                                                    \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_find_secondary);                                                  \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_lowerbound);                                                      \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_upperbound);                                                      \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_end);                                                             \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_next);                                                            \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_previous);

#define DB_SECONDARY_INDEX_METHODS_ARRAY(IDX)                                                                          \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_store);                                                           \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_remove);                                                          \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_update);                                                          \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_find_primary);                                                    \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_find_secondary);                                                  \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_lowerbound);                                                      \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_upperbound);                                                      \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_end);                                                             \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_next);                                                            \
   RODEOS_REGISTER_CALLBACK(Rft, Derived, db_##IDX##_previous);

      // database_api
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx64)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx128)
      DB_SECONDARY_INDEX_METHODS_ARRAY(idx256)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_double)
      DB_SECONDARY_INDEX_METHODS_SIMPLE(idx_long_double)

#undef DB_SECONDARY_INDEX_METHODS_SIMPLE
#undef DB_SECONDARY_INDEX_METHODS_ARRAY

      // permission_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, check_transaction_authorization);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, check_permission_authorization);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_permission_last_used);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_account_creation_time);

      // system_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, publication_time);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, is_feature_activated);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_sender);

      // context_free_system_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, eosio_assert_code);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, eosio_exit);

      // authorization_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, require_recipient);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, require_auth);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, require_auth2);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, has_auth);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, is_account);

      // context_free_transaction_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, read_transaction);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, transaction_size);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, expiration);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, tapos_block_prefix);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, tapos_block_num);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_action);

      // transaction_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, send_inline);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, send_context_free_inline);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, send_deferred);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, cancel_deferred);

      // context_free_api
      RODEOS_REGISTER_CALLBACK(Rft, Derived, get_context_free_data);
   } // register_callbacks()
};   // unimplemented_callbacks

} // namespace b1::rodeos
