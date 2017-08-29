#include <eos/account_history_plugin/account_history_plugin.hpp>
#include <eos/account_history_plugin/account_control_history_object.hpp>
#include <eos/account_history_plugin/account_transaction_history_object.hpp>
#include <eos/account_history_plugin/public_key_history_object.hpp>
#include <eos/account_history_plugin/transaction_history_object.hpp>
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
using ordered_transaction_results = account_history_apis::read_only::ordered_transaction_results;
using get_transactions_results = account_history_apis::read_only::get_transactions_results;

class account_history_plugin_impl {
public:
   ProcessedTransaction get_transaction(const chain::transaction_id_type&  transaction_id) const;
   get_transactions_results get_transactions(const AccountName&  account_name, const optional<uint32_t>& skip_seq, const optional<uint32_t>& num_seq) const;
   vector<AccountName> get_key_accounts(const public_key_type& public_key) const;
   vector<AccountName> get_controlled_accounts(const AccountName& controlling_account) const;
   void applied_block(const signed_block&);

   chain_plugin* chain_plug;
   static const int64_t DEFAULT_TRANSACTION_TIME_LIMIT;
   int64_t transactions_time_limit = DEFAULT_TRANSACTION_TIME_LIMIT;
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

   optional<block_id_type> find_block_id(const chainbase::database& db, const transaction_id_type& transaction_id) const;
   ProcessedTransaction find_transaction(const chain::transaction_id_type&  transaction_id, const signed_block& block) const;
   bool is_scope_relevant(const eos::types::Vector<AccountName>& scope);
   get_transactions_results ordered_transactions(const block_transaction_id_map& block_transaction_ids, const fc::time_point& start_time, const uint32_t begin, const uint32_t end) const;
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
   bool time_exceeded(const fc::time_point& start_time) const;
   static const AccountName NEW_ACCOUNT;
   static const AccountName UPDATE_AUTH;
   static const AccountName DELETE_AUTH;
   static const PermissionName OWNER;
   static const PermissionName ACTIVE;
   static const PermissionName RECOVERY;
};
const int64_t account_history_plugin_impl::DEFAULT_TRANSACTION_TIME_LIMIT = 3;
const AccountName account_history_plugin_impl::NEW_ACCOUNT = "newaccount";
const AccountName account_history_plugin_impl::UPDATE_AUTH = "updateauth";
const AccountName account_history_plugin_impl::DELETE_AUTH = "deleteauth";
const PermissionName account_history_plugin_impl::OWNER = "owner";
const PermissionName account_history_plugin_impl::ACTIVE = "active";
const PermissionName account_history_plugin_impl::RECOVERY = "recovery";

optional<block_id_type> account_history_plugin_impl::find_block_id(const chainbase::database& db, const transaction_id_type& transaction_id) const
{
   optional<block_id_type> block_id;
   const auto& trx_idx = db.get_index<transaction_history_multi_index, by_trx_id>();
   auto transaction_history = trx_idx.find( transaction_id );
   if (transaction_history != trx_idx.end())
      block_id = transaction_history->block_id;

   return block_id;
}

ProcessedTransaction account_history_plugin_impl::find_transaction(const chain::transaction_id_type&  transaction_id, const chain::signed_block& block) const
{
   for (const auto& cycle : block.cycles)
      for (const auto& thread : cycle)
         for (const auto& trx : thread.user_input)
            if (trx.id() == transaction_id)
               return trx;

   // ERROR in indexing logic
   FC_THROW("Transaction with ID ${tid} was indexed as being in block ID ${bid}, but was not found in that block", ("tid", transaction_id)("bid", block.id()));
}

ProcessedTransaction account_history_plugin_impl::get_transaction(const chain::transaction_id_type&  transaction_id) const
{
   const auto& db = chain_plug->chain().get_database();
   optional<block_id_type> block_id;
   db.with_read_lock( [&]() {
      block_id = find_block_id(db, transaction_id);
   } );
   if( block_id.valid() )
   {
      auto block = chain_plug->chain().fetch_block_by_id(*block_id);
      FC_ASSERT(block, "Transaction with ID ${tid} was indexed as being in block ID ${bid}, but no such block was found", ("tid", transaction_id)("bid", block_id));
      return find_transaction(transaction_id, *block);
   }

#warning TODO: lookup of recent transactions
   FC_THROW_EXCEPTION(chain::unknown_transaction_exception,
                      "Could not find transaction for: ${id}", ("id", transaction_id.str()));
}

get_transactions_results account_history_plugin_impl::get_transactions(const AccountName&  account_name, const optional<uint32_t>& skip_seq, const optional<uint32_t>& num_seq) const
{
   fc::time_point start_time = fc::time_point::now();
   const auto& db = chain_plug->chain().get_database();

   block_transaction_id_map block_transaction_ids;
   db.with_read_lock( [&]() {
      const auto& account_idx = db.get_index<account_transaction_history_multi_index, by_account_name>();
      auto range = account_idx.equal_range( account_name );
      for (auto obj = range.first; obj != range.second; ++obj)
      {
         optional<block_id_type> block_id = find_block_id(db, obj->transaction_id);
         FC_ASSERT(block_id, "Transaction with ID ${tid} was tracked for ${account}, but no corresponding block id was found", ("tid", obj->transaction_id)("account", account_name));
         block_transaction_ids.emplace(std::make_pair(*block_id,  obj->transaction_id));
      }
   } );

   uint32_t begin, end;
   const auto size = block_transaction_ids.size();
   if (!skip_seq)
   {
      begin = 0;
      end = size;
   }
   else
   {
      begin = *skip_seq;

      if (!num_seq)
         end = size;
      else
      {
         end = begin + *num_seq;
         if (end > size)
            end = size;
      }

      if (begin > size - 1 || begin >= end)
         return get_transactions_results();
   }

   return ordered_transactions(block_transaction_ids, start_time, begin, end);
}

get_transactions_results account_history_plugin_impl::ordered_transactions(const block_transaction_id_map& block_transaction_ids, const fc::time_point& start_time, const uint32_t begin, const uint32_t end) const
{
   get_transactions_results results;
   results.transactions.reserve(end - begin);

   auto block_transaction = block_transaction_ids.cbegin();

   uint32_t current = 0;
   // keep iterating through each equal range
   while (block_transaction != block_transaction_ids.cend() && !time_exceeded(start_time))
   {
      optional<signed_block> block = chain_plug->chain().fetch_block_by_id(block_transaction->first);
      FC_ASSERT(block, "Transaction with ID ${tid} was indexed as being in block ID ${bid}, but no such block was found", ("tid", block_transaction->second)("bid", block_transaction->first));

      auto range = block_transaction_ids.equal_range(block_transaction->first);

      std::set<transaction_id_type> trans_ids_for_block;
      for (block_transaction = range.first; block_transaction != range.second; ++block_transaction)
      {
         trans_ids_for_block.insert(block_transaction->second);
      }

      const uint32_t trx_after_block = current + trans_ids_for_block.size();
      if (trx_after_block <= begin)
      {
         current = trx_after_block;
         continue;
      }
      for (auto cycle = block->cycles.crbegin(); cycle != block->cycles.crend() && current < trx_after_block; ++cycle)
      {
         for (auto thread = cycle->crbegin(); thread != cycle->crend() && current < trx_after_block; ++thread)
         {
            for (auto trx = thread->user_input.crbegin(); trx != thread->user_input.crend() && current < trx_after_block; ++trx)
            {
               if(trans_ids_for_block.count(trx->id()))
               {
                  if(++current > begin)
                  {
                     const auto pretty_trx = chain_plug->chain().transaction_to_variant(*trx);
                     results.transactions.emplace_back(ordered_transaction_results{(current - 1), trx->id(), pretty_trx});

                     if(current >= end)
                     {
                        return results;
                     }
                  }

                  // just check after finding transaction or transitioning to next block to avoid spending all our time checking
                  if (time_exceeded(start_time))
                  {
                     results.time_limit_exceeded_error = true;
                     return results;
                  }
               }
            }
         }
      }

      // just check after finding transaction or transitioning to next block to avoid spending all our time checking
      if (time_exceeded(start_time))
      {
         results.time_limit_exceeded_error = true;
         return results;
      }
   }

   return results;
}

bool account_history_plugin_impl::time_exceeded(const fc::time_point& start_time) const
{
   return (fc::time_point::now() - start_time).count() > transactions_time_limit;
}

vector<AccountName> account_history_plugin_impl::get_key_accounts(const public_key_type& public_key) const
{
   std::set<AccountName> accounts;
   const auto& db = chain_plug->chain().get_database();
   db.with_read_lock( [&]() {
      const auto& pub_key_idx = db.get_index<public_key_history_multi_index, by_pub_key>();
      auto range = pub_key_idx.equal_range( public_key );
      for (auto obj = range.first; obj != range.second; ++obj)
      {
         accounts.insert(obj->account_name);
      }
   } );
   return vector<AccountName>(accounts.begin(), accounts.end());
}

vector<AccountName> account_history_plugin_impl::get_controlled_accounts(const AccountName& controlling_account) const
{
   std::set<AccountName> accounts;
   const auto& db = chain_plug->chain().get_database();
   db.with_read_lock( [&]() {
      const auto& account_control_idx = db.get_index<account_control_history_multi_index, by_controlling>();
      auto range = account_control_idx.equal_range( controlling_account );
      for (auto obj = range.first; obj != range.second; ++obj)
      {
         accounts.insert(obj->controlled_account);
      }
   } );
   return vector<AccountName>(accounts.begin(), accounts.end());
}

void account_history_plugin_impl::applied_block(const signed_block& block)
{
   const auto block_id = block.id();
   auto& db = chain_plug->chain().get_mutable_database();
   const bool check_relevance = filter_on.size();
   for (const auto& cycle : block.cycles)
   {
      for (const auto& thread : cycle)
      {
         for (const auto& trx : thread.user_input)
         {
            if (check_relevance && !is_scope_relevant(trx.scope))
               continue;

            db.create<transaction_history_object>([&block_id,&trx](transaction_history_object& transaction_history) {
               transaction_history.block_id = block_id;
               transaction_history.transaction_id = trx.id();
            });

            for (const auto& account_name : trx.scope)
            {
               db.create<account_transaction_history_object>([&trx,&account_name](account_transaction_history_object& account_transaction_history) {
                  account_transaction_history.account_name = account_name;
                  account_transaction_history.transaction_id = trx.id();
               });
            }

            for (const chain::Message& msg : trx.messages)
            {
               if (msg.code == config::EosContractName)
               {
                  if (msg.type == NEW_ACCOUNT)
                  {
                     const auto create = msg.as<types::newaccount>();
                     add(db, create.owner.keys, create.name, OWNER);
                     add(db, create.active.keys, create.name, ACTIVE);
                     add(db, create.recovery.keys, create.name, RECOVERY);

                     add(db, create.owner.accounts, create.name, OWNER);
                     add(db, create.active.accounts, create.name, ACTIVE);
                     add(db, create.recovery.accounts, create.name, RECOVERY);
                  }
                  else if (msg.type == UPDATE_AUTH)
                  {
                     const auto update = msg.as<types::updateauth>();
                     remove<public_key_history_multi_index, by_account_permission>(db, update.account, update.permission);
                     add(db, update.authority.keys, update.account, update.permission);

                     remove<account_control_history_multi_index, by_controlled_authority>(db, update.account, update.permission);
                     add(db, update.authority.accounts, update.account, update.permission);
                  }
                  else if (msg.type == DELETE_AUTH)
                  {
                     const auto del = msg.as<types::deleteauth>();
                     remove<public_key_history_multi_index, by_account_permission>(db, del.account, del.permission);

                     remove<account_control_history_multi_index, by_controlled_authority>(db, del.account, del.permission);
}
               }
            }
         }
      }
   }
}

void account_history_plugin_impl::add(chainbase::database& db, const vector<types::KeyPermissionWeight>& keys, const AccountName& account_name, const PermissionName& permission)
{
   for (auto pub_key_weight : keys )
   {
      db.create<public_key_history_object>([&](public_key_history_object& obj) {
         obj.public_key = pub_key_weight.key;
         obj.account_name = account_name;
         obj.permission = permission;
      });
   }
}

void account_history_plugin_impl::add(chainbase::database& db, const vector<types::AccountPermissionWeight>& controlling_accounts, const AccountName& account_name, const PermissionName& permission)
{
   for (auto controlling_account : controlling_accounts )
   {
      db.create<account_control_history_object>([&](account_control_history_object& obj) {
         obj.controlled_account = account_name;
         obj.controlled_permission = permission;
         obj.controlling_account = controlling_account.permission.account;
      });
   }
}

bool account_history_plugin_impl::is_scope_relevant(const eos::types::Vector<AccountName>& scope)
{
   for (const AccountName& account_name : scope)
      if (filter_on.count(account_name))
         return true;

   return false;
}


account_history_plugin::account_history_plugin()
:my(new account_history_plugin_impl())
{
}

account_history_plugin::~account_history_plugin()
{
}

void account_history_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("filter_on_accounts,f", bpo::value<vector<string>>()->composing(),
          "Track only transactions whose scopes involve the listed accounts. Default is to track all transactions.")
         ("get-transactions-time-limit", bpo::value<int>()->default_value(account_history_plugin_impl::DEFAULT_TRANSACTION_TIME_LIMIT),
          "Limits the maximum time (in milliseconds) processing a single get_transactions call.")
         ;
}

void account_history_plugin::plugin_initialize(const variables_map& options)
{
   my->transactions_time_limit = options.at("get-transactions-time-limit").as<int>() * 1000;

   if(options.count("filter_on_accounts"))
   {
      auto foa = options.at("filter_on_accounts").as<vector<string>>();
      for(auto filter_account : foa)
         my->filter_on.emplace(filter_account);
   }
}

void account_history_plugin::plugin_startup()
{
   my->chain_plug = app().find_plugin<chain_plugin>();
   auto& db = my->chain_plug->chain().get_mutable_database();
   db.add_index<account_control_history_multi_index>();
   db.add_index<account_transaction_history_multi_index>();
   db.add_index<public_key_history_multi_index>();
   db.add_index<transaction_history_multi_index>();

   my->chain_plug->chain().applied_block.connect ([&impl = my](const signed_block& block) {
      impl->applied_block(block);
   });
}

void account_history_plugin::plugin_shutdown()
{
}

namespace account_history_apis {

read_only::get_transaction_results read_only::get_transaction(const read_only::get_transaction_params& params) const
{
   auto trx = account_history->get_transaction(params.transaction_id);
   return { params.transaction_id, account_history->chain_plug->chain().transaction_to_variant(trx) };
}

read_only::get_transactions_results read_only::get_transactions(const read_only::get_transactions_params& params) const
{
   return account_history->get_transactions(params.account_name, params.skip_seq, params.num_seq);
}

read_only::get_key_accounts_results read_only::get_key_accounts(const get_key_accounts_params& params) const
{
   return { account_history->get_key_accounts(params.public_key) };
}

read_only::get_controlled_accounts_results read_only::get_controlled_accounts(const get_controlled_accounts_params& params) const
{
   return { account_history->get_controlled_accounts(params.controlling_account) };
}

} // namespace account_history_apis
} // namespace eos
