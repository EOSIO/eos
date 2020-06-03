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
         /**
          * This structure is an concrete alias of `chain::permission_level` to facilitate
          * specialized rules when transforming to/from variants.
          */
         struct permission_level : public chain::permission_level {
         };

         std::vector<permission_level> accounts;
         std::vector<chain::public_key_type> keys;
      };

      /**
       * Result of the get_accounts_by_authorizers RPC
       */
      struct get_accounts_by_authorizers_result{
         struct account_result {
            chain::name                            account_name;
            chain::name                            permission_name;
            fc::optional<chain::permission_level>  authorizing_account;
            fc::optional<chain::public_key_type>   authorizing_key;
            chain::weight_type                     weight;
            uint32_t                               threshold;
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

namespace fc {
   using params = eosio::chain_apis::account_query_db::get_accounts_by_authorizers_params;
   /**
    * Overloaded to_variant so that permission is only present if it is set
    * @param a
    * @param v
    */
   inline void to_variant(const params::permission_level& a, fc::variant& v) {
      if (a.permission.empty()) {
         v = a.actor.to_string();
      } else {
         v = mutable_variant_object()
            ("actor", a.actor.to_string())
            ("permission", a.permission.to_string());
      }
   }

   /**
    * Overloaded from_variant to allow parsing an account with a permission wildcard (empty) from a string
    * instead of an object
    * @param v
    * @param a
    */
   inline void from_variant(const fc::variant& v, params::permission_level& a) {
      if (v.is_string()) {
         from_variant(v, a.actor);
         a.permission = {};
      } else if (v.is_object()) {
         const auto& vo = v.get_object();
         if(vo.contains("actor"))
            from_variant(vo["actor"], a.actor);
         else
            EOS_THROW(eosio::chain::invalid_http_request, "Missing Actor field");

         if(vo.contains("permission") && vo.size() == 2)
            from_variant(vo["permission"], a.permission);
         else if (vo.size() == 1)
            a.permission = {};
         else
            EOS_THROW(eosio::chain::invalid_http_request, "Unrecognized fields in account");
      }
   }
}

FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_params, (accounts)(keys))
FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_result::account_result, (account_name)(permission_name)(authorizing_account)(authorizing_key)(weight)(threshold))
FC_REFLECT( eosio::chain_apis::account_query_db::get_accounts_by_authorizers_result, (accounts))
