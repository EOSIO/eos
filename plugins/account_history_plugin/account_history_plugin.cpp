#include <eos/account_history_plugin/account_history_plugin.hpp>
#include <eos/account_history_plugin/account_history_object.hpp>
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

namespace fc { class variant; }

namespace eos {

using chain::block_id_type;
using chain::ProcessedTransaction;
using chain::signed_block;
using boost::multi_index_container;
using chain::transaction_id_type;
using namespace boost::multi_index;

class account_history_plugin_impl {
public:
   ProcessedTransaction get_transaction(const chain::transaction_id_type&  transaction_id) const;
   vector<account_history_apis::read_only::get_transaction_results> get_transactions(const AccountName&  account_name, const optional<chain::transaction_id_type>& after_transaction_id) const;
   void applied_block(const signed_block&);

   chain_plugin* chain_plug;
   static const int DEFAULT_TRANSACTION_LIMIT;
   int transactions_limit = DEFAULT_TRANSACTION_LIMIT;
   std::set<AccountName> filter_on;
private:

   optional<block_id_type> find_block_id(const transaction_id_type& transaction_id) const;
   ProcessedTransaction find_transaction(const chain::transaction_id_type&  transaction_id, const block_id_type& block_id) const;
   bool scope_relevant(const eos::types::Vector<AccountName>& scope);
};
const int account_history_plugin_impl::DEFAULT_TRANSACTION_LIMIT = 100;

optional<block_id_type> account_history_plugin_impl::find_block_id(const transaction_id_type& transaction_id) const
{
   const auto& db = chain_plug->chain().get_database();
   optional<block_id_type> block_id;
   db.with_read_lock( [&]() {
      const auto& trx_idx = db.get_index<transaction_history_multi_index, by_trx_id>();
      auto transaction_history = trx_idx.find( transaction_id );
      if (transaction_history != trx_idx.end())
         block_id = transaction_history->block_id;
   } );
   return block_id;
}

ProcessedTransaction account_history_plugin_impl::find_transaction(const chain::transaction_id_type&  transaction_id, const chain::block_id_type& block_id) const
{
   auto block = chain_plug->chain().fetch_block_by_id(block_id);
   FC_ASSERT(block, "Transaction with ID ${tid} was indexed as being in block ID ${bid}, but no such block was found", ("tid", transaction_id)("bid", block_id));

   for (const auto& cycle : block->cycles)
      for (const auto& thread : cycle)
         for (const auto& trx : thread.user_input)
            if (trx.id() == transaction_id)
               return trx;

   // ERROR in indexing logic
   FC_THROW("Transaction with ID ${tid} was indexed as being in block ID ${bid}, but was not found in that block", ("tid", transaction_id)("bid", block_id));
}

ProcessedTransaction account_history_plugin_impl::get_transaction(const chain::transaction_id_type&  transaction_id) const
{
   auto block_id = find_block_id(transaction_id);
   if( block_id.valid() )
   {
      return find_transaction(transaction_id, *block_id);
   }

#warning TODO: lookup of recent transactions
   FC_THROW_EXCEPTION(chain::unknown_transaction_exception,
                      "Could not find transaction for: ${id}", ("id", transaction_id.str()));
}

vector<account_history_apis::read_only::get_transaction_results> account_history_plugin_impl::get_transactions(const AccountName&  account_name, const optional<chain::transaction_id_type>& after_transaction_id) const
{
   const auto& db = chain_plug->chain().get_database();
   typedef std::map<transaction_id_type, block_id_type> transaction_map;
   transaction_map transactions;

   db.with_read_lock( [&]() {
      const auto& account_idx = db.get_index<transaction_history_multi_index, by_account_name>();
      auto range = account_idx.equal_range( account_name );
      for (auto transaction_history = range.first; transaction_history != range.second; ++transaction_history)
      {
         transactions.emplace(std::make_pair(transaction_history->transaction_id, transaction_history->block_id));
      }
   } );
   vector<account_history_apis::read_only::get_transaction_results> results;
   transaction_map::const_iterator trans;
   if (after_transaction_id.valid())
   {
      trans = transactions.find(*after_transaction_id);
      if (trans != transactions.cend())
         ++trans;
   }
   else
      trans = transactions.cbegin();

   for(; trans != transactions.end() && results.size() < transactions_limit; ++trans)
   {
      const auto trx = find_transaction(trans->first, trans->second);
      const auto pretty_trx = chain_plug->chain().transaction_to_variant(trx);
      results.emplace_back(account_history_apis::read_only::get_transaction_results{trans->first, pretty_trx});
   }

   return results;
}

void account_history_plugin_impl::applied_block(const signed_block& block)
{
   const auto block_id = block.id();
   auto& db = chain_plug->chain().get_mutable_database();
   const bool check_relevance = filter_on.size();
   for (const auto& cycle : block.cycles)
      for (const auto& thread : cycle)
         for (const auto& trx : thread.user_input)
         {
            if (check_relevance && !scope_relevant(trx.scope))
               continue;

            for (const auto& account_name : trx.scope)
            {
               db.create<transaction_history_object>([&block_id,&trx,&account_name](transaction_history_object& transaction_history) {
                  transaction_history.block_id = block_id;
                  transaction_history.transaction_id = trx.id();
                  transaction_history.account_name = account_name;
               });
            }
         }
}

bool account_history_plugin_impl::scope_relevant(const eos::types::Vector<AccountName>& scope)
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
         ("get-transactions-limit", bpo::value<int>()->default_value(account_history_plugin_impl::DEFAULT_TRANSACTION_LIMIT),
          "Limits the number of transactions returned for get_transactions")
         ;
}

void account_history_plugin::plugin_initialize(const variables_map& options)
{
   my->transactions_limit = options.at("get-transactions-limit").as<int>();

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
   return { account_history->get_transactions(params.account_name, params.after_transaction_id) };
}

} // namespace account_history_apis
} // namespace eos
