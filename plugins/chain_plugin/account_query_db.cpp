#include <eosio/chain_plugin/account_query_db.hpp>

#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/permission_object.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/set_of.hpp>

using namespace eosio;
using namespace boost::multi_index;
using namespace boost::bimaps;

namespace {
   /**
    * Structure to hold indirect reference to a `property_object` via {owner,name} as well as a non-standard
    * index over `last_updated` for roll-back support
    */
   struct permission_info {
      chain::name owner;
      chain::name name;
      fc::time_point last_updated;

      using cref = std::reference_wrapper<const permission_info>;
   };

   struct by_owner_name;
   struct by_last_updated;

   /**
    * Multi-index providing fast lookup for {owner,name} as well as {last_updated}
    */
   using permission_info_index_t = multi_index_container<
      permission_info,
      indexed_by<
         ordered_unique<
            tag<by_owner_name>,
            composite_key<permission_info,
               member<permission_info, chain::name, &permission_info::owner>,
               member<permission_info, chain::name, &permission_info::name>
            >
         >,
         ordered_non_unique<
            tag<by_last_updated>,
            member<permission_info, fc::time_point, &permission_info::last_updated>
         >
      >
   >;

   /**
    * Utility function to identify on-block action
    * @param p
    * @return
    */
   bool is_onblock(const chain::transaction_trace_ptr& p) {
      if (p->action_traces.size() != 1)
         return false;
      const auto& act = p->action_traces[0].act;
      if (act.account != eosio::chain::config::system_account_name || act.name != N(onblock) ||
          act.authorization.size() != 1)
         return false;
      const auto& auth = act.authorization[0];
      return auth.actor == eosio::chain::config::system_account_name &&
             auth.permission == eosio::chain::config::active_name;
   }
}

namespace std {
   /**
    * support for using `permission_info::cref` in ordered containers
    */
   template<>
   struct less<permission_info::cref> {
      bool operator()( const permission_info::cref& lhs, const permission_info::cref& rhs ) const {
         return std::uintptr_t(&lhs.get()) < std::uintptr_t(&rhs.get());
      }
   };
}

namespace eosio::chain_apis {
   /**
    * Implementation details of the account query DB
    */
   struct account_query_db_impl {
      account_query_db_impl(const chain::controller& controller)
      :controller(controller)
      {}

      /**
       * Build the initial database from the chain controller by extracting the information contained in the
       * blockchain state at the current HEAD
       */
      void build_account_query_map() {
         ilog("Building account query maps");
         auto start = fc::time_point::now();
         const auto& index = controller.db().get_index<chain::permission_index>().indices().get<by_id>();

         for (const auto& po : index ) {
            const auto& pi = permission_info_index.emplace( permission_info{ po.owner, po.name, po.last_updated } ).first;
            add_to_bimaps(*pi, po);
         }
         auto duration = fc::time_point::now() - start;
         ilog("Finished building account query maps in ${sec}", ("sec", (duration.count() / 1'000'000.0 )));
      }

      /**
       * Add a permission to the bimaps for keys and accounts
       * @param pi - the ephemeral permission info structure being added
       * @param po - the chain data associted with this permission
       */
      void add_to_bimaps( const permission_info& pi, const chain::permission_object& po ) {
         // For each account, add this permission info's non-owning reference to the bimap for accounts
         for (const auto& a : po.auth.accounts) {
            name_bimap.insert(name_bimap_t::value_type {a.permission.actor, pi});
         }

         // for each key, add this permission info's non-owning reference to the bimap for keys
         for (const auto& k: po.auth.keys) {
            key_bimap.insert(key_bimap_t::value_type {(chain::public_key_type)k.key, pi});
         }
      }

      /**
       * Remove a permission from the bimaps for keys and accounts
       * @param pi - the ephemeral permission info structure being removed
       */
      void remove_from_bimaps( const permission_info& pi ) {
         // remove all entries from the name bimap that refer to this permission_info's reference
         const auto name_range = name_bimap.right.equal_range(pi);
         name_bimap.right.erase(name_range.first, name_range.second);

         // remove all entries from the key bimap that refer to this permission_info's reference
         const auto key_range = key_bimap.right.equal_range(pi);
         key_bimap.right.erase(key_range.first, key_range.second);
      }

      /**
       * Given a time_point, remove all permissions that were last updated at or after that time_point
       * this will effectively remove any updates that happened at or after that time point
       *
       * For each removed entry, this will create a new entry if there exists an equivalent {owner, name} permission
       * at the HEAD state of the chain.
       * @param bsp - the block to rollback before
       */
      void rollback_to_before( const chain::block_state_ptr& bsp ) {
         const auto t = bsp->block->timestamp.to_time_point();
         auto& index = permission_info_index.get<by_last_updated>();
         const auto& permission_by_owner = controller.db().get_index<chain::permission_index>().indices().get<chain::by_owner>();

         while (!index.empty()) {
            const auto& pi = (*index.rbegin());
            if (pi.last_updated < t) {
               break;
            }

            // remove this entry from the bimaps
            remove_from_bimaps(pi);

            auto itr = permission_by_owner.find(std::make_tuple(pi.owner, pi.name));
            if (itr == permission_by_owner.end()) {
               // this permission does not exist at this point in the chains history
               index.erase(index.iterator_to(pi));
            } else {
               const auto& po = *itr;
               index.modify(index.iterator_to(pi), [&po](auto& mutable_pi) {
                  mutable_pi.last_updated = po.last_updated;
               });
               add_to_bimaps(pi, po);
            }
         }
      }

      /**
       * Store a potentially relevant transaction trace in a short lived cache so that it can be processed if its
       * committed to by a block
       * @param trace
       */
      void cache_transaction_trace( const chain::transaction_trace_ptr& trace ) {
         if( !trace->receipt ) return;
         // include only executed transactions; soft_fail included so that onerror (and any inlines via onerror) are included
         if((trace->receipt->status != chain::transaction_receipt_header::executed &&
             trace->receipt->status != chain::transaction_receipt_header::soft_fail)) {
            return;
         }
         if( is_onblock( trace )) {
            onblock_trace.emplace( trace );
         } else if( trace->failed_dtrx_trace ) {
            cached_trace_map[trace->failed_dtrx_trace->id] = trace;
         } else {
            cached_trace_map[trace->id] = trace;
         }
      }

      /**
       * Commit a block of transactions to the account query DB
       * transaction traces need to be in the cache prior to this call
       * @param bsp
       */
      void commit_block(const chain::block_state_ptr& bsp ) {
         using permission_set_t = std::set<name_pair_t>;

         permission_set_t updated;
         permission_set_t deleted;

         /**
          * process traces to find `updateauth` and `deleteauth` calls maintaining a final set of
          * permissions to either update or delete.  Intra-block changes are discarded
          */
         auto process_trace = [&](const chain::transaction_trace_ptr& trace) {
            for( const auto& at : trace->action_traces ) {
               if (std::tie(at.receiver, at.act.account) != std::tie(chain::config::system_account_name,chain::config::system_account_name)) {
                  continue;
               }

               if (at.act.name == chain::updateauth::get_name()) {
                  auto data = at.act.data_as<chain::updateauth>();
                  auto itr = updated.emplace(data.account, data.permission).first;
                  deleted.erase(*itr);
               } else if (at.act.name == chain::deleteauth::get_name()) {
                  auto data = at.act.data_as<chain::deleteauth>();
                  auto itr = deleted.emplace(data.account, data.permission).first;
                  updated.erase(*itr);
               }
            }
         };

         rollback_to_before(bsp);

         if( onblock_trace )
            process_trace(*onblock_trace);

         for( const auto& r : bsp->block->transactions ) {
            chain::transaction_id_type id;
            if( r.trx.contains<chain::transaction_id_type>()) {
               id = r.trx.get<chain::transaction_id_type>();
            } else {
               id = r.trx.get<chain::packed_transaction>().id();
            }

            const auto it = cached_trace_map.find( id );
            if( it != cached_trace_map.end() ) {
               process_trace( it->second );
            }
         }

         auto& index = permission_info_index.get<by_owner_name>();
         const auto& permission_by_owner = controller.db().get_index<chain::permission_index>().indices().get<chain::by_owner>();

         // for each updated permission, find the new values and update the account query db
         for (const auto& up: updated) {
            auto source_itr = permission_by_owner.find(up);
            EOS_ASSERT(source_itr != permission_by_owner.end(), chain::plugin_exception, "chain data is missing");
            auto itr = index.find(up);
            if (itr == index.end()) {
               const auto& po = *source_itr;
               itr = index.emplace(permission_info{ po.owner, po.name, po.last_updated }).first;
            } else {
               remove_from_bimaps(*itr);
               index.modify(itr, [&](auto& mutable_pi){
                  mutable_pi.last_updated = source_itr->last_updated;
               });
            }

            add_to_bimaps(*itr, *source_itr);
         }

         // for all deleted permissions, process their removal from the account query DB
         for (const auto& dp: deleted) {
            auto itr = index.find(dp);
            if (itr != index.end()) {
               remove_from_bimaps(*itr);
               index.erase(itr);
            }
         }

         // drop any unprocessed cached traces
         cached_trace_map.clear();
         onblock_trace.reset();
      }

      account_query_db::get_accounts_by_authorizers_result
      get_accounts_by_authorizers( const account_query_db::get_accounts_by_authorizers_params& args) const {
         using result_t = account_query_db::get_accounts_by_authorizers_result;
         result_t result;

         // deduplicate inputs
         auto account_set = std::set<chain::name>(args.accounts.begin(), args.accounts.end());
         auto key_set = std::set<chain::public_key_type>(args.keys.begin(), args.keys.end());

         // build a set of permissions that include any of our keys/accounts
         auto permission_set = std::set<name_pair_t>();

         for (const auto& a: account_set) {
            auto range = name_bimap.left.equal_range(a);
            for (auto itr = range.first; itr != range.second; ++itr) {
               const auto& pi = itr->second.get();
               permission_set.emplace(std::make_tuple(pi.owner, pi.name));
            }
         }

         for (const auto& k: key_set) {
            auto range = key_bimap.left.equal_range(k);
            for (auto itr = range.first; itr != range.second; ++itr) {
               const auto& pi = itr->second.get();
               permission_set.emplace(std::make_tuple(pi.owner, pi.name));
            }
         }

         /**
          * Add a single result entry
          */
         auto push_result = [&result](const auto& po, const auto& authorizer, auto weight) {
            fc::variant v;
            fc::to_variant(authorizer, v);
            result.accounts.emplace_back(result_t::account_result{
                  po.owner,
                  po.name,
                  v,
                  weight,
                  po.auth.threshold
            });
         };

         // for each permission that included at least one of the accounts/keys, add entries to the result to indicate
         // all the relevant authorization data.
         const auto& permission_by_owner = controller.db().get_index<chain::permission_index>().indices().get<chain::by_owner>();
         for (const auto& p: permission_set) {
            const auto& iter = permission_by_owner.find(p);
            EOS_ASSERT(iter != permission_by_owner.end(), chain::plugin_exception, "chain data mismatch");
            const auto& po = *iter;

            for (const auto& a : po.auth.accounts) {
               if (account_set.count(a.permission.actor)) {
                  push_result(po, a.permission, a.weight);
               }
            }

            for (const auto& k: po.auth.keys) {
               auto pk = (chain::public_key_type)k.key;
               if (key_set.count(pk)) {
                  push_result(po, pk, k.weight);
               }
            }
         }

         return result;
      }

      /**
       * Convenience aliases
       */
      using name_pair_t = std::tuple<chain::name, chain::name>;
      using name_bimap_t = bimap<multiset_of<chain::name>, multiset_of<permission_info::cref>>;
      using key_bimap_t = bimap<multiset_of<chain::public_key_type>, multiset_of<permission_info::cref>>;
      using cached_trace_map_t = std::map<chain::transaction_id_type, chain::transaction_trace_ptr>;
      using onblock_trace_t = std::optional<chain::transaction_trace_ptr>;

      const chain::controller&   controller;               ///< the controller to read data from
      permission_info_index_t    permission_info_index;    ///< multi-index that holds ephemeral indices
      name_bimap_t               name_bimap;               ///< many:many bimap of names:permission_infos
      key_bimap_t                key_bimap;                ///< many:many bimap of keys:permission_infos
      cached_trace_map_t         cached_trace_map;         ///< temporary cache of uncommitted traces
      onblock_trace_t            onblock_trace;            ///< temporary cache of on_block trace
   };

   account_query_db::account_query_db( const chain::controller& controller )
   :_impl(std::make_unique<account_query_db_impl>(controller))
   {
      _impl->build_account_query_map();
   }

   account_query_db::~account_query_db() = default;
   account_query_db & account_query_db::operator=(account_query_db &&) = default;

   void account_query_db::cache_transaction_trace( const chain::transaction_trace_ptr& trace ) {
      try {
         _impl->cache_transaction_trace(trace);
      } FC_LOG_AND_DROP(("ACCOUNT DB cache_transaction_trace ERROR"));
   }

   void account_query_db::commit_block(const chain::block_state_ptr& block ) {
      try {
         _impl->commit_block(block);
      } FC_LOG_AND_DROP(("ACCOUNT DB commit_block ERROR"));
   }

   account_query_db::get_accounts_by_authorizers_result account_query_db::get_accounts_by_authorizers( const account_query_db::get_accounts_by_authorizers_params& args) const {
      return _impl->get_accounts_by_authorizers(args);
   }

}
