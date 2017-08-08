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
   void applied_block(const signed_block&);
   chain_plugin* chain_plug;
private:

   optional<block_id_type> find_block_id(const transaction_id_type& transaction_id) const;
};

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

ProcessedTransaction account_history_plugin_impl::get_transaction(const chain::transaction_id_type&  transaction_id) const
{
   auto block_id = find_block_id(transaction_id);
   if( block_id.valid() )
   {
      auto block = chain_plug->chain().fetch_block_by_id(*block_id);
      if (block.valid())
      {
         for (const auto& cycle : block->cycles)
            for (const auto& thread : cycle)
               for (const auto& trx : thread.user_input)
                  if (trx.id() == transaction_id)
                     return trx;
      }

      // ERROR in indexing logic
      FC_ASSERT(block, "Transaction with ID ${tid} was indexed as being in block ID ${bid}, but no such block was found", ("tid", transaction_id)("bid", block_id));
      FC_THROW("Transaction with ID ${tid} was indexed as being in block ID ${bid}, but was not found in that block", ("tid", transaction_id)("bid", block_id));
   }

#warning TODO: lookup of recent transactions
   FC_THROW_EXCEPTION(chain::unknown_transaction_exception,
                      "Could not find transaction for: ${id}", ("id", transaction_id.str()));
}

void account_history_plugin_impl::applied_block(const signed_block& block)
{
   const auto block_id = block.id();
   auto& db = chain_plug->chain().get_mutable_database();
   for (const auto& cycle : block.cycles)
      for (const auto& thread : cycle)
         for (const auto& trx : thread.user_input) {
            db.create<transaction_history_object>([&block_id,&trx](transaction_history_object& transaction_history) {
               transaction_history.block_id = block_id;
               transaction_history.transaction_id = trx.id();
            });
         }
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
}

void account_history_plugin::plugin_initialize(const variables_map& options)
{
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
   return { account_history->chain_plug->chain().transaction_to_variant(trx) };
}

} // namespace account_history_apis
} // namespace eos
