/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <functional>

#include <appbase/application.hpp>

#include <eosio/chain/asset.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/types.hpp>

#include <eosio/chain_plugin/chain_plugin_params.hpp>
#include <eosio/chain_plugin/chain_plugin_results.hpp>

#include <boost/container/flat_set.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <fc/static_variant.hpp>

#include <eosio/http_plugin/http_plugin.hpp>

namespace fc { class variant; }

namespace eosio {

class chain_plugin : public appbase::plugin<chain_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((http_plugin))

   chain_plugin();
   virtual ~chain_plugin();

   virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;

   void plugin_initialize(const appbase::variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   void accept_block( const chain::signed_block_ptr& block );
   void accept_transaction(const chain::packed_transaction& trx, chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);
   void accept_transaction(const chain::transaction_metadata_ptr& trx, chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);

   bool block_is_on_preferred_chain(const chain::block_id_type& block_id);

   static bool recover_reversible_blocks( const fc::path& db_dir,
                                          uint32_t cache_size,
                                          fc::optional<fc::path> new_db_dir = fc::optional<fc::path>(),
                                          uint32_t truncate_at_block = 0
                                        );

   static bool import_reversible_blocks( const fc::path& reversible_dir,
                                         uint32_t cache_size,
                                         const fc::path& reversible_blocks_file
                                       );

   static bool export_reversible_blocks( const fc::path& reversible_dir,
                                        const fc::path& reversible_blocks_file
                                       );

   // Only call this after plugin_initialize()!
   chain::controller& chain();
   // Only call this after plugin_initialize()!
   const chain::controller& chain() const;

   chain::chain_id_type get_chain_id() const;

   fc::microseconds get_abi_serializer_max_time() const;

   void handle_guard_exception(const chain::guard_exception& e) const;

   static void handle_db_exhaustion();

private:
   void log_guard_exception(const chain::guard_exception& e) const;
   void init_request_handler();

   fc::unique_ptr<class chain_plugin_impl> my;
};

}
