#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio::chain_apis {
   class account_query_db {
   public:
      account_query_db( const class eosio::chain::controller& chain );
      ~account_query_db();

      account_query_db(account_query_db&&);
      account_query_db& operator=(account_query_db&&);

      void cache_transaction_trace( const chain::transaction_trace_ptr& trace );
      void commit_block(const chain::block_state_ptr& block );

      struct get_accounts_by_authorizers_params{
         std::vector<chain::name> accounts;
         std::vector<chain::public_key_type> keys;
      };

      struct get_accounts_by_authorizers_result{
         struct account_result {
            chain::name         account_name;
            chain::name         permission_name;
            fc::variant         authorizer;
            chain::weight_type  weight;
            uint32_t            threshold;
         };

         std::vector<account_result> accounts;
      };
      get_accounts_by_authorizers_result get_accounts_by_authorizers( const get_accounts_by_authorizers_params& args) const;

   private:
      std::unique_ptr<struct account_query_db_impl> _impl;
   };

}

FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_params, (accounts)(keys))
FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_result::account_result, (account_name)(permission_name)(authorizer)(weight)(threshold))
FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_result, (accounts))
