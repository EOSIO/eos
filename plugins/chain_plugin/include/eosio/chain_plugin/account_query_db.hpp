#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio::chain_apis {
   /**
    * This class manages the ephemeral indices and data that provide the `get_accounts_by_authorizers` RPC call
    * There is no persistence and the indices/caches are recreated when the class is instantiated based on the
    * current state of the chain.
    */
   class account_query_db {
   public:

      /**
       * Instantiate a new account query DB from the given chain controller
       * The caller is expected to manage lifetimes such that this controller reference does not go stale
       * for the life of the account query DB
       * @param chain - controller to read data from
       */
      account_query_db( const class eosio::chain::controller& chain );
      ~account_query_db();

      /**
       * Allow moving the account query DB (including by assignment)
       */
      account_query_db(account_query_db&&);
      account_query_db& operator=(account_query_db&&);

      /**
       * Add a transaction trace to the account query DB that has been applied to the contoller even though it may
       * not yet be committed to by a block.
       *
       * @param trace
       */
      void cache_transaction_trace( const chain::transaction_trace_ptr& trace );

      /**
       * Add a block to the account query DB, committing all the cached traces it represents and dumping any
       * uncommitted traces.
       * @param block
       */
      void commit_block(const chain::block_state_ptr& block );

      /**
       * parameters for the get_accounts_by_authorizers RPC
       */
      struct get_accounts_by_authorizers_params{
         std::vector<chain::name> accounts;
         std::vector<chain::public_key_type> keys;
      };

      /**
       * Result of the get_accounts_by_authorizers RPC
       */
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
      /**
       * Given a set of account names and public keys, find all account permission authorities that are, in part or whole,
       * satisfiable.
       *
       * @param args
       * @return
       */
      get_accounts_by_authorizers_result get_accounts_by_authorizers( const get_accounts_by_authorizers_params& args) const;

   private:
      std::unique_ptr<struct account_query_db_impl> _impl;
   };

}

FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_params, (accounts)(keys))
FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_result::account_result, (account_name)(permission_name)(authorizer)(weight)(threshold))
FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_result, (accounts))
