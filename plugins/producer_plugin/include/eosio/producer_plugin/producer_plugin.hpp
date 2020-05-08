#pragma once

#include <appbase/application.hpp>

#include <boost/container/flat_set.hpp>

#include <eosio/chain/types.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>

#include <functional>
#include <string>

namespace eosio {

// class producer_plugin;
// static appbase::abstract_plugin& _producer_plugin = app().register_plugin<producer_plugin>();
// 
class producer_plugin : public appbase::plugin<producer_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin)(eosio::http_client_plugin))

   struct runtime_options {
      fc::optional<int32_t>   max_transaction_time;
      fc::optional<int32_t>   max_irreversible_block_age;
      fc::optional<int32_t>   produce_time_offset_us;
      fc::optional<int32_t>   last_block_time_offset_us;
      fc::optional<int32_t>   max_scheduled_transaction_time_per_block_ms;
      fc::optional<int32_t>   subjective_cpu_leeway_us;
      fc::optional<double>    incoming_defer_ratio;
      fc::optional<uint32_t>  greylist_limit;
   };

   struct whitelist_blacklist {
      fc::optional< boost::container::flat_set<eosio::chain::account_name> > actor_whitelist;
      fc::optional< boost::container::flat_set<eosio::chain::account_name> > actor_blacklist;
      fc::optional< boost::container::flat_set<eosio::chain::account_name> > contract_whitelist;
      fc::optional< boost::container::flat_set<eosio::chain::account_name> > contract_blacklist;
      fc::optional< boost::container::flat_set< std::pair<eosio::chain::account_name, eosio::chain::action_name> > > action_blacklist;
      fc::optional< boost::container::flat_set<eosio::chain::public_key_type> > key_blacklist;
   };

   struct greylist_params {
      std::vector<eosio::chain::account_name> accounts;
   };

   struct integrity_hash_information {
      eosio::chain::block_id_type head_block_id;
      eosio::chain::digest_type   integrity_hash;
   };

   struct snapshot_information {
       eosio::chain::block_id_type head_block_id;
       std::string          snapshot_name;
   };

   struct scheduled_protocol_feature_activations {
      std::vector<eosio::chain::digest_type> protocol_features_to_activate;
   };

   struct get_supported_protocol_features_params {
      bool exclude_disabled = false;
      bool exclude_unactivatable = false;
   };

   struct get_account_ram_corrections_params {
      fc::optional<eosio::chain::account_name>  lower_bound;
      fc::optional<eosio::chain::account_name>  upper_bound;
      uint32_t                limit = 10;
      bool                    reverse = false;
   };

   struct get_account_ram_corrections_result {
      std::vector<fc::variant> rows;
       fc::optional<eosio::chain::account_name>   more;
   };

   template<typename T>
   using next_function = std::function<void(const fc::static_variant<fc::exception_ptr, T>&)>;

   producer_plugin();
   virtual ~producer_plugin();

   virtual void set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;

   bool                  is_producer_key(const chain::public_key_type& key) const;
   chain::signature_type sign_compact(const chain::public_key_type& key, const fc::sha256& digest) const;

   virtual void plugin_initialize(const boost::program_options::variables_map& options);
   virtual void plugin_startup();
   virtual void plugin_shutdown();
   void handle_sighup() override;

   void pause();
   void resume();
   bool paused() const;
   void update_runtime_options(const runtime_options& options);
   runtime_options get_runtime_options() const;

   void add_greylist_accounts(const greylist_params& params);
   void remove_greylist_accounts(const greylist_params& params);
   greylist_params get_greylist() const;

   whitelist_blacklist get_whitelist_blacklist() const;
   void set_whitelist_blacklist(const whitelist_blacklist& params);

   integrity_hash_information get_integrity_hash() const;
   void create_snapshot(next_function<snapshot_information> next);

   scheduled_protocol_feature_activations get_scheduled_protocol_feature_activations() const;
   void schedule_protocol_feature_activations(const scheduled_protocol_feature_activations& schedule);

   fc::variants get_supported_protocol_features( const get_supported_protocol_features_params& params ) const;

   get_account_ram_corrections_result  get_account_ram_corrections( const get_account_ram_corrections_params& params ) const;

private:
   std::shared_ptr<class producer_plugin_impl> my;
};
};

} //eosio

FC_REFLECT(eosio::producer_plugin::runtime_options, (max_transaction_time)(max_irreversible_block_age)(produce_time_offset_us)(last_block_time_offset_us)(max_scheduled_transaction_time_per_block_ms)(subjective_cpu_leeway_us)(incoming_defer_ratio)(greylist_limit));
FC_REFLECT(eosio::producer_plugin::greylist_params, (accounts));
FC_REFLECT(eosio::producer_plugin::whitelist_blacklist, (actor_whitelist)(actor_blacklist)(contract_whitelist)(contract_blacklist)(action_blacklist)(key_blacklist) )
FC_REFLECT(eosio::producer_plugin::integrity_hash_information, (head_block_id)(integrity_hash))
FC_REFLECT(eosio::producer_plugin::snapshot_information, (head_block_id)(snapshot_name))
FC_REFLECT(eosio::producer_plugin::scheduled_protocol_feature_activations, (protocol_features_to_activate))
FC_REFLECT(eosio::producer_plugin::get_supported_protocol_features_params, (exclude_disabled)(exclude_unactivatable))
FC_REFLECT(eosio::producer_plugin::get_account_ram_corrections_params, (lower_bound)(upper_bound)(limit)(reverse))
FC_REFLECT(eosio::producer_plugin::get_account_ram_corrections_result, (rows)(more))
