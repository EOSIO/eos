/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>

namespace fc { class variant; }

namespace eosio {
   using chain::transaction_id_type;
   using std::shared_ptr;
   using namespace appbase;
   using chain::name;
   using fc::optional;
   using chain::uint128_t;

   typedef shared_ptr<class history_plugin_impl> history_ptr;
   typedef shared_ptr<const class history_plugin_impl> history_const_ptr;

namespace history_apis {

class read_only {
   history_const_ptr history;

   public:
      read_only(history_const_ptr&& history)
         : history(history) {}


      struct get_actions_params {
         chain::account_name account_name;
         optional<int32_t>   pos; /// a absolute sequence positon -1 is the end/last action
         optional<int32_t>   offset; ///< the number of actions relative to pos, negative numbers return [pos-offset,pos), positive numbers return [pos,pos+offset)
      };

      struct ordered_action_result {
         uint64_t                     global_action_seq = 0;
         int32_t                      account_action_seq = 0;
         uint32_t                     block_num;
         chain::block_timestamp_type  block_time;
         fc::variant                  action_trace;
      };

      struct get_actions_result {
         vector<ordered_action_result> actions;
         uint32_t                      last_irreversible_block;
         optional<bool>                time_limit_exceeded_error;
      };


      get_actions_result get_actions( const get_actions_params& )const;


      struct get_transaction_params {
         string                        id;
         optional<uint32_t>            block_num_hint;
      };

      struct get_transaction_result {
         transaction_id_type                   id;
         fc::variant                           trx;
         chain::block_timestamp_type           block_time;
         uint32_t                              block_num = 0;
         uint32_t                              last_irreversible_block = 0;
         vector<fc::variant>                   traces;
      };

      get_transaction_result get_transaction( const get_transaction_params& )const;




      /*
      struct ordered_transaction_results {
         uint32_t                    seq_num;
         chain::transaction_id_type  transaction_id;
         fc::variant                 transaction;
      };

      get_transactions_results get_transactions(const get_transactions_params& params) const;
      */


      struct get_key_accounts_params {
         chain::public_key_type     public_key;
      };
      struct get_key_accounts_results {
         vector<chain::account_name> account_names;
      };
      get_key_accounts_results get_key_accounts(const get_key_accounts_params& params) const;


      struct get_controlled_accounts_params {
         chain::account_name     controlling_account;
      };
      struct get_controlled_accounts_results {
         vector<chain::account_name> controlled_accounts;
      };
      get_controlled_accounts_results get_controlled_accounts(const get_controlled_accounts_params& params) const;
};


} // namespace history_apis


/**
 *  This plugin tracks all actions and keys associated with a set of configured accounts. It enables
 *  wallets to paginate queries for history.
 *
 *  An action will be included in the account's history if any of the following:
 *     - receiver
 *     - any account named in auth list
 *
 *  A key will be linked to an account if the key is referneced in authorities of updateauth or newaccount
 */
class history_plugin : public plugin<history_plugin> {
   public:
      APPBASE_PLUGIN_REQUIRES((chain_plugin))

      history_plugin();
      virtual ~history_plugin();

      virtual void set_program_options(options_description& cli, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      history_apis::read_only  get_read_only_api()const { return history_apis::read_only(history_const_ptr(my)); }

   private:
      history_ptr my;
};

} /// namespace eosio

FC_REFLECT( eosio::history_apis::read_only::get_actions_params, (account_name)(pos)(offset) )
FC_REFLECT( eosio::history_apis::read_only::get_actions_result, (actions)(last_irreversible_block)(time_limit_exceeded_error) )
FC_REFLECT( eosio::history_apis::read_only::ordered_action_result, (global_action_seq)(account_action_seq)(block_num)(block_time)(action_trace) )

FC_REFLECT( eosio::history_apis::read_only::get_transaction_params, (id)(block_num_hint) )
FC_REFLECT( eosio::history_apis::read_only::get_transaction_result, (id)(trx)(block_time)(block_num)(last_irreversible_block)(traces) )
/*
FC_REFLECT(eosio::history_apis::read_only::get_transaction_params, (transaction_id) )
FC_REFLECT(eosio::history_apis::read_only::get_transaction_results, (transaction_id)(transaction) )
FC_REFLECT(eosio::history_apis::read_only::get_transactions_params, (account_name)(skip_seq)(num_seq) )
FC_REFLECT(eosio::history_apis::read_only::ordered_transaction_results, (seq_num)(transaction_id)(transaction) )
FC_REFLECT(eosio::history_apis::read_only::get_transactions_results, (transactions)(time_limit_exceeded_error) )
*/
FC_REFLECT(eosio::history_apis::read_only::get_key_accounts_params, (public_key) )
FC_REFLECT(eosio::history_apis::read_only::get_key_accounts_results, (account_names) )
FC_REFLECT(eosio::history_apis::read_only::get_controlled_accounts_params, (controlling_account) )
FC_REFLECT(eosio::history_apis::read_only::get_controlled_accounts_results, (controlled_accounts) )
