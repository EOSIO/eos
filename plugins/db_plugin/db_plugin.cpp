#include <eos/db_plugin/db_plugin.hpp>
#include <eos/chain/chain_controller.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/chain/types.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <eos/chain/multi_index_includes.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

namespace fc { class variant; }

namespace eos {

using chain::AccountName;
using chain::block_id_type;
using chain::PermissionName;
using chain::ProcessedTransaction;
using chain::signed_block;
using boost::multi_index_container;
using chain::transaction_id_type;
using namespace boost::multi_index;

class db_plugin_impl {
public:
   void applied_irreversible_block(const signed_block&);

   chain_plugin* chain_plug;
   std::set<AccountName> filter_on;

private:
   struct block_comp
   {
      bool operator()(const block_id_type& a, const block_id_type& b) const
      {
         return chain::block_header::num_from_id(a) > chain::block_header::num_from_id(b);
      }
   };
   typedef std::multimap<block_id_type, transaction_id_type, block_comp> block_transaction_id_map;

   bool is_scope_relevant(const eos::types::Vector<AccountName>& scope);
   static void add(chainbase::database& db, const vector<types::KeyPermissionWeight>& keys, const AccountName& account_name, const PermissionName& permission);

   template<typename MultiIndex, typename LookupType>
   static void remove(chainbase::database& db, const AccountName& account_name, const PermissionName& permission)
   {
      const auto& idx = db.get_index<MultiIndex, LookupType>();
      auto& mutatable_idx = db.get_mutable_index<MultiIndex>();
      auto range = idx.equal_range( boost::make_tuple( account_name, permission ) );

      for (auto acct_perm = range.first; acct_perm != range.second; ++acct_perm)
      {
         mutatable_idx.remove(*acct_perm);
      }
   }

   static void add(chainbase::database& db, const vector<types::AccountPermissionWeight>& controlling_accounts, const AccountName& account_name, const PermissionName& permission);
   static const AccountName NEW_ACCOUNT;
   static const AccountName UPDATE_AUTH;
   static const AccountName DELETE_AUTH;
   static const PermissionName OWNER;
   static const PermissionName ACTIVE;
   static const PermissionName RECOVERY;
};
const AccountName db_plugin_impl::NEW_ACCOUNT = "newaccount";
const AccountName db_plugin_impl::UPDATE_AUTH = "updateauth";
const AccountName db_plugin_impl::DELETE_AUTH = "deleteauth";
const PermissionName db_plugin_impl::OWNER = "owner";
const PermissionName db_plugin_impl::ACTIVE = "active";
const PermissionName db_plugin_impl::RECOVERY = "recovery";

void db_plugin_impl::applied_irreversible_block(const signed_block& block)
{
   const auto block_id = block.id();
   ilog("block ${bid}", ("bid", block_id));
//   auto& db = chain_plug->chain().get_mutable_database();
   const bool check_relevance = filter_on.size();
   for (const auto& cycle : block.cycles)
   {
      for (const auto& thread : cycle)
      {
         for (const auto& trx : thread.user_input)
         {
            if (check_relevance && !is_scope_relevant(trx.scope))
               continue;

            ilog("block ${bid} : ${tid}", ("bid", block_id)("tid", trx.id()));

//            db.create<transaction_history_object>([&block_id,&trx](transaction_history_object& transaction_history) {
//               transaction_history.block_id = block_id;
//               transaction_history.transaction_id = trx.id();
//            });

            for (const auto& account_name : trx.scope)
            {
//               db.create<account_transaction_history_object>([&trx,&account_name](account_transaction_history_object& account_transaction_history) {
//                  account_transaction_history.account_name = account_name;
//                  account_transaction_history.transaction_id = trx.id();
//               });
            }

            for (const chain::Message& msg : trx.messages)
            {
               if (msg.code == config::EosContractName)
               {
                  if (msg.type == NEW_ACCOUNT)
                  {
                     const auto create = msg.as<types::newaccount>();
//                     add(db, create.owner.keys, create.name, OWNER);
//                     add(db, create.active.keys, create.name, ACTIVE);
//                     add(db, create.recovery.keys, create.name, RECOVERY);
//
//                     add(db, create.owner.accounts, create.name, OWNER);
//                     add(db, create.active.accounts, create.name, ACTIVE);
//                     add(db, create.recovery.accounts, create.name, RECOVERY);
                  }
                  else if (msg.type == UPDATE_AUTH)
                  {
                     const auto update = msg.as<types::updateauth>();
//                     remove<public_key_history_multi_index, by_account_permission>(db, update.account, update.permission);
//                     add(db, update.authority.keys, update.account, update.permission);
//
//                     remove<account_control_history_multi_index, by_controlled_authority>(db, update.account, update.permission);
//                     add(db, update.authority.accounts, update.account, update.permission);
                  }
                  else if (msg.type == DELETE_AUTH)
                  {
                     const auto del = msg.as<types::deleteauth>();
//                     remove<public_key_history_multi_index, by_account_permission>(db, del.account, del.permission);
//
//                     remove<account_control_history_multi_index, by_controlled_authority>(db, del.account, del.permission);
                  }
               }
            }
         }
      }
   }
}

void db_plugin_impl::add(chainbase::database& db, const vector<types::KeyPermissionWeight>& keys, const AccountName& account_name, const PermissionName& permission)
{
   for (auto pub_key_weight : keys )
   {
//      db.create<public_key_history_object>([&](public_key_history_object& obj) {
//         obj.public_key = pub_key_weight.key;
//         obj.account_name = account_name;
//         obj.permission = permission;
//      });
   }
}

void db_plugin_impl::add(chainbase::database& db, const vector<types::AccountPermissionWeight>& controlling_accounts, const AccountName& account_name, const PermissionName& permission)
{
   for (auto controlling_account : controlling_accounts )
   {
//      db.create<account_control_history_object>([&](account_control_history_object& obj) {
//         obj.controlled_account = account_name;
//         obj.controlled_permission = permission;
//         obj.controlling_account = controlling_account.permission.account;
//      });
   }
}

bool db_plugin_impl::is_scope_relevant(const eos::types::Vector<AccountName>& scope)
{
   for (const AccountName& account_name : scope)
      if (filter_on.count(account_name))
         return true;

   return false;
}


db_plugin::db_plugin()
:my(new db_plugin_impl())
{
}

db_plugin::~db_plugin()
{
}

void db_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("filter_on_accounts,f", bpo::value<vector<string>>()->composing(),
          "Track only transactions whose scopes involve the listed accounts. Default is to track all transactions.")
         ;
}

void db_plugin::wipe_database() {
   if (true)
      elog("TODO: db wipe_database");
   else
      elog("ERROR: db_plugin::wipe_database() called before configuration or after startup. Ignoring.");
}

void db_plugin::applied_irreversible_block(const signed_block& block) {
   my->applied_irreversible_block(block);
}


void db_plugin::plugin_initialize(const variables_map& options)
{
   ilog("initializing db plugin");
   if(options.count("filter_on_accounts")) {
      auto foa = options.at("filter_on_accounts").as<vector<string>>();
      for(auto filter_account : foa)
         my->filter_on.emplace(filter_account);
   }
}

void db_plugin::plugin_startup()
{
   ilog("starting db plugin");
   // TODO: during chain startup we want to pushback on apply so that chainbase has to wait for db.
   // TODO: assert that last irreversible in db is one less than received (on startup only?, every so often?)
   my->chain_plug = app().find_plugin<chain_plugin>();
//   auto& db = my->chain_plug->chain().get_mutable_database();
//   db.add_index<account_control_history_multi_index>();
//   db.add_index<account_transaction_history_multi_index>();
//   db.add_index<public_key_history_multi_index>();
//   db.add_index<transaction_history_multi_index>();

}

void db_plugin::plugin_shutdown()
{
}

} // namespace eos
