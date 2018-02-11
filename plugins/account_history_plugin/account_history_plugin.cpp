/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/account_history_plugin/account_history_plugin.hpp>
#include <eosio/account_history_plugin/account_control_history_object.hpp>
#include <eosio/account_history_plugin/account_transaction_history_object.hpp>
#include <eosio/account_history_plugin/public_key_history_object.hpp>
#include <eosio/account_history_plugin/transaction_history_object.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <eosio/chain/multi_index_includes.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

namespace fc { class variant; }

namespace eosio {

using chain::account_name;
using chain::block_id_type;
using chain::key_weight;
using chain::permission_level_weight;
using chain::permission_name;
using chain::packed_transaction;
using chain::signed_block;
using boost::multi_index_container;
using chain::transaction_id_type;
using namespace boost::multi_index;
using ordered_transaction_results = account_history_apis::read_only::ordered_transaction_results;
using get_transactions_results = account_history_apis::read_only::get_transactions_results;

class account_history_plugin_impl {
public:
   packed_transaction get_transaction(const chain::transaction_id_type&  transaction_id) const;
   get_transactions_results get_transactions(const account_name&  account_name, const optional<uint32_t>& skip_seq, const optional<uint32_t>& num_seq) const;
   vector<account_name> get_key_accounts(const public_key_type& public_key) const;
   vector<account_name> get_controlled_accounts(const account_name& controlling_account) const;
   void applied_block(const chain::block_trace&);
   fc::variant transaction_to_variant(const packed_transaction& pretty_input) const;

   chain_plugin* chain_plug;
   static const int64_t DEFAULT_TRANSACTION_TIME_LIMIT;
   int64_t transactions_time_limit = DEFAULT_TRANSACTION_TIME_LIMIT;
   std::set<account_name> filter_on;

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
   packed_transaction find_transaction(const chain::transaction_id_type&  transaction_id, const signed_block& block) const;
   bool is_scope_relevant(const vector<account_name>& scope);
   get_transactions_results ordered_transactions(const block_transaction_id_map& block_transaction_ids, const fc::time_point& start_time, const uint32_t begin, const uint32_t end) const;
   static void add(chainbase::database& db, const vector<key_weight>& keys, const account_name& account_name, const permission_name& permission);
   template<typename MultiIndex, typename LookupType>
   static void remove(chainbase::database& db, const account_name& account_name, const permission_name& permission)
   {
      const auto& idx = db.get_index<MultiIndex, LookupType>();
      auto& mutatable_idx = db.get_mutable_index<MultiIndex>();
      auto obj = idx.find( boost::make_tuple( account_name, permission ) );

      if (obj != idx.end())
      {
         mutatable_idx.remove(*obj);
      }
   }

   static void add(chainbase::database& db, const vector<permission_level_weight>& controlling_accounts, const account_name& account_name, const permission_name& permission);
   bool time_exceeded(const fc::time_point& start_time) const;

   static const account_name NEW_ACCOUNT;
   static const account_name UPDATE_AUTH;
   static const account_name DELETE_AUTH;
   static const permission_name OWNER;
   static const permission_name ACTIVE;
   static const permission_name RECOVERY;
};
const int64_t account_history_plugin_impl::DEFAULT_TRANSACTION_TIME_LIMIT = 3;
const account_name account_history_plugin_impl::NEW_ACCOUNT = "newaccount";
const account_name account_history_plugin_impl::UPDATE_AUTH = "updateauth";
const account_name account_history_plugin_impl::DELETE_AUTH = "deleteauth";
const permission_name account_history_plugin_impl::OWNER = "owner";
const permission_name account_history_plugin_impl::ACTIVE = "active";
const permission_name account_history_plugin_impl::RECOVERY = "recovery";

optional<block_id_type> account_history_plugin_impl::find_block_id(const chainbase::database& db, const transaction_id_type& transaction_id) const
{
   optional<block_id_type> block_id;
   const auto& trx_idx = db.get_index<transaction_history_multi_index, by_trx_id>();
   auto transaction_history = trx_idx.find( transaction_id );
   if (transaction_history != trx_idx.end())
      block_id = transaction_history->block_id;

   return block_id;
}

packed_transaction account_history_plugin_impl::find_transaction(const chain::transaction_id_type&  transaction_id, const chain::signed_block& block) const
{
   for (const packed_transaction& trx : block.input_transactions)
      if (trx.get_transaction().id() == transaction_id)
         return trx;

   // ERROR in indexing logic
   FC_THROW("Transaction with ID ${tid} was indexed as being in block ID ${bid}, but was not found in that block", ("tid", transaction_id)("bid", block.id()));
}

packed_transaction account_history_plugin_impl::get_transaction(const chain::transaction_id_type&  transaction_id) const
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

   FC_THROW_EXCEPTION(chain::unknown_transaction_exception,
                      "Could not find transaction for: ${id}", ("id", transaction_id.str()));
}

get_transactions_results account_history_plugin_impl::get_transactions(const account_name&  account_name, const optional<uint32_t>& skip_seq, const optional<uint32_t>& num_seq) const
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
      for (auto trx = block->input_transactions.crbegin(); trx != block->input_transactions.crend() && current < trx_after_block; ++trx)
      {
         transaction_id_type trx_id = trx->get_transaction().id();
         if(trans_ids_for_block.count(trx_id))
         {
            if(++current > begin)
            {
               const auto pretty_trx = transaction_to_variant(*trx);
               results.transactions.emplace_back(ordered_transaction_results{(current - 1), trx_id, pretty_trx});

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

vector<account_name> account_history_plugin_impl::get_key_accounts(const public_key_type& public_key) const
{
   std::set<account_name> accounts;
   const auto& db = chain_plug->chain().get_database();
   db.with_read_lock( [&]() {
      const auto& pub_key_idx = db.get_index<public_key_history_multi_index, by_pub_key>();
      auto range = pub_key_idx.equal_range( public_key );
      for (auto obj = range.first; obj != range.second; ++obj)
      {
         accounts.insert(obj->name);
      }
   } );
   return vector<account_name>(accounts.begin(), accounts.end());
}

vector<account_name> account_history_plugin_impl::get_controlled_accounts(const account_name& controlling_account) const
{
   std::set<account_name> accounts;
   const auto& db = chain_plug->chain().get_database();
   db.with_read_lock( [&]() {
      const auto& account_control_idx = db.get_index<account_control_history_multi_index, by_controlling>();
      auto range = account_control_idx.equal_range( controlling_account );
      for (auto obj = range.first; obj != range.second; ++obj)
      {
         accounts.insert(obj->controlled_account);
      }
   } );
   return vector<account_name>(accounts.begin(), accounts.end());
}

static vector<account_name> generated_affected_accounts(const chain::transaction_trace& trx_trace) {
   vector<account_name> result;
   for (const auto& at: trx_trace.action_traces) {
      for (const auto& auth: at.act.authorization) {
         result.emplace_back(auth.actor);
      }

      result.emplace_back(at.receiver);
   }

   fc::deduplicate(result);
   return result;
}

void account_history_plugin_impl::applied_block(const chain::block_trace& trace)
{
   const auto& block = trace.block;
   const auto block_id = block.id();
   auto& db = chain_plug->chain().get_mutable_database();
   const bool check_relevance = filter_on.size();
   auto process_one = [&](const chain::transaction_trace& trx_trace )
   {
      auto affected_accounts = generated_affected_accounts(trx_trace);
      if (check_relevance && !is_scope_relevant(affected_accounts))
         return;

      auto trx_obj_ptr = db.find<transaction_history_object, by_trx_id>(trx_trace.id);
      if (trx_obj_ptr != nullptr)
         return; // on restart may already have block

      db.create<transaction_history_object>([&block_id,&trx_trace](transaction_history_object& transaction_history) {
         transaction_history.block_id = block_id;
         transaction_history.transaction_id = trx_trace.id;
         transaction_history.transaction_status = trx_trace.status;
      });

      auto create_ath_object = [&trx_trace,&db](const account_name& name) {
         db.create<account_transaction_history_object>([&trx_trace,&name](account_transaction_history_object& account_transaction_history) {
            account_transaction_history.name = name;
            account_transaction_history.transaction_id = trx_trace.id;
         });
      };

      for (const auto& account_name : affected_accounts)
         create_ath_object(account_name);

      for (const auto& act_trace : trx_trace.action_traces)
      {
         if (act_trace.receiver == chain::config::system_account_name)
         {
            if (act_trace.act.name == NEW_ACCOUNT)
            {
               const auto create = act_trace.act.as<chain::contracts::newaccount>();
               add(db, create.owner.keys, create.name, OWNER);
               add(db, create.active.keys, create.name, ACTIVE);
               add(db, create.recovery.keys, create.name, RECOVERY);

               add(db, create.owner.accounts, create.name, OWNER);
               add(db, create.active.accounts, create.name, ACTIVE);
               add(db, create.recovery.accounts, create.name, RECOVERY);
            }
            else if (act_trace.act.name == UPDATE_AUTH)
            {
               const auto update = act_trace.act.as<chain::contracts::updateauth>();
               remove<public_key_history_multi_index, by_account_permission>(db, update.account, update.permission);
               add(db, update.data.keys, update.account, update.permission);

               remove<account_control_history_multi_index, by_controlled_authority>(db, update.account, update.permission);
               add(db, update.data.accounts, update.account, update.permission);
            }
            else if (act_trace.act.name == DELETE_AUTH)
            {
               const auto del = act_trace.act.as<chain::contracts::deleteauth>();
               remove<public_key_history_multi_index, by_account_permission>(db, del.account, del.permission);

               remove<account_control_history_multi_index, by_controlled_authority>(db, del.account, del.permission);
            }
         }
      }
   };

   // go through all the transaction traces
   for (const auto& rt: trace.region_traces)
      for(const auto& ct: rt.cycle_traces)
         for(const auto& st: ct.shard_traces)
            for(const auto& trx_trace: st.transaction_traces)
               process_one(trx_trace);
}

void account_history_plugin_impl::add(chainbase::database& db, const vector<key_weight>& keys, const account_name& name, const permission_name& permission)
{
   for (auto pub_key_weight : keys )
   {
      db.create<public_key_history_object>([&](public_key_history_object& obj) {
         obj.public_key = pub_key_weight.key;
         obj.name = name;
         obj.permission = permission;
      });
   }
}

void account_history_plugin_impl::add(chainbase::database& db, const vector<permission_level_weight>& controlling_accounts, const account_name& account_name, const permission_name& permission)
{
   for (auto controlling_account : controlling_accounts )
   {
      db.create<account_control_history_object>([&](account_control_history_object& obj) {
         obj.controlled_account = account_name;
         obj.controlled_permission = permission;
         obj.controlling_account = controlling_account.permission.actor;
      });
   }
}

bool account_history_plugin_impl::is_scope_relevant(const vector<account_name>& scope)
{
   for (const account_name& account_name : scope)
      if (filter_on.count(account_name))
         return true;

   return false;
}

fc::variant account_history_plugin_impl::transaction_to_variant(const packed_transaction& ptrx) const
{
   const chainbase::database& database = chain_plug->chain().get_database();
   auto resolver = [&database]( const account_name& name ) -> optional<abi_serializer> {
      const auto* accnt = database.find<chain::account_object,chain::by_name>( name );
      if (accnt != nullptr) {
         abi_def abi;
         if (abi_serializer::to_abi(accnt->name, accnt->abi, abi)) {
            return abi_serializer(abi);
         }
      }

      return optional<abi_serializer>();
   };

   fc::variant pretty_output;
   abi_serializer::to_variant(ptrx, pretty_output, resolver);
   return pretty_output;
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

   my->chain_plug->chain().applied_block.connect ([&impl = my](const chain::block_trace& trace) {
      impl->applied_block(trace);
   });
}

void account_history_plugin::plugin_shutdown()
{
}

namespace account_history_apis {

read_only::get_transaction_results read_only::get_transaction(const read_only::get_transaction_params& params) const
{
   auto trx = account_history->get_transaction(params.transaction_id);
   return { params.transaction_id, account_history->transaction_to_variant(trx) };
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
} // namespace eosio
