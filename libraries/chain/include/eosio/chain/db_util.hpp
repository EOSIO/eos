#pragma once
#include <b1/chain_kv/chain_kv.hpp>
#include <chainbase/chainbase.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/chain_snapshot.hpp>
#include <eosio/chain/genesis_state.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/snapshot.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/backing_store.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/database_header_object.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/genesis_intrinsics.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/protocol_state_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/whitelisted_intrinsics.hpp>
#include <eosio/chain/controller.hpp>

#include <b1/session/session.hpp>

namespace eosio { namespace chain { 
   
   class apply_context;

   namespace backing_store {
      struct db_context;
   }

namespace db_util {
   using controller_index_set =
         index_set<account_index, account_metadata_index, account_ram_correction_index, global_property_multi_index,
                   protocol_state_multi_index, dynamic_global_property_multi_index, block_summary_multi_index,
                   transaction_multi_index, generated_transaction_multi_index, table_id_multi_index, code_index,
                   database_header_multi_index, kv_db_config_index>;

   using contract_database_index_set = index_set<key_value_index, index64_index, index128_index, index256_index,
                                                 index_double_index, index_long_double_index>;

   using db_context = backing_store::db_context;

   class maybe_session {
    public:
      maybe_session() = default;

      maybe_session(chainbase::database& cb_database);
      maybe_session(maybe_session&& src) noexcept;

      maybe_session& operator=(const maybe_session& src) = delete;

      void push();

      void squash();

      void undo();

    private:
      std::unique_ptr<chainbase::database::session> cb_session    = {};
   };

   // Save the backing_store setting to the chainbase
   // Currently deprecated, left just for backward compatibility
   void check_backing_store_setting(chainbase::database& db, bool clean_startup);

   void destroy(const fc::path& p);

   std::unique_ptr<kv_context> create_kv_context(chainbase::database& db, 
                                                 name receiver, 
                                                 kv_resource_manager resource_manager,
                                                 const kv_database_config& limits);

   std::unique_ptr<db_context> create_db_context(apply_context& context, name receiver);

   void add_to_snapshot(const chainbase::database&                                    db, 
                        const eosio::chain::snapshot_writer_ptr&                      snapshot, 
                        const eosio::chain::block_state&                              head,
                        const eosio::chain::authorization_manager&                    authorization,
                        const eosio::chain::resource_limits::resource_limits_manager& resource_limits);

   void read_from_snapshot(chainbase::database&                                    db, 
                           const snapshot_reader_ptr&                              snapshot, 
                           uint32_t                                                blog_start, 
                           uint32_t                                                blog_end,
                           eosio::chain::authorization_manager&                    authorization,
                           eosio::chain::resource_limits::resource_limits_manager& resource_limits,
                           eosio::chain::block_state_ptr&                          head, 
                           uint32_t&                                               snapshot_head_block,
                           const eosio::chain::chain_id_type&                      chain_id);

   std::optional<eosio::chain::genesis_state> extract_legacy_genesis_state(snapshot_reader& snapshot, uint32_t version);
}}} // namespace eosio::chain::db_util
