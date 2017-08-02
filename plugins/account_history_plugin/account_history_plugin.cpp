#include <eos/account_history_plugin/account_history_plugin.hpp>
#include <eos/chain/fork_database.hpp>
#include <eos/chain/block_log.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <fc/log/logger.hpp>

namespace eos {

using namespace eos;
using chain::block_id_type;
using chain::ProcessedTransaction;
using chain::signed_block;
using boost::multi_index_container;
using namespace boost::multi_index;

struct block_data {
   block_id_type block_id;
   transaction_id_type transaction_id;
};

class account_history_plugin_impl {
public:
   ProcessedTransaction get_transaction(const chain::transaction_id_type&  transaction_id) const;
   void applied_block(const signed_block&);
   chain_plugin* chain_plug;
private:

   struct trx_id;
   typedef multi_index_container<
      block_data,
      indexed_by<
         hashed_unique<tag<trx_id>, member<block_data, transaction_id_type, &block_data::transaction_id>, std::hash<transaction_id_type>>
      >
   > block_multi_index_type;

   block_multi_index_type _block_index;
};

ProcessedTransaction account_history_plugin_impl::get_transaction(const chain::transaction_id_type&  transaction_id) const
{
   const auto& by_trx_idx = _block_index.get<trx_id>();
   auto itr = by_trx_idx.find( transaction_id );
   if( itr != by_trx_idx.end() )
   {
      auto block = chain_plug->chain().fetch_block_by_id_backwards(itr->block_id);
      if (block.valid())
      {
         for (const auto& cycle : block->cycles)
            for (const auto& thread : cycle)
               for (const auto& trx : thread.user_input)
                  if (trx.id() == transaction_id)
                     return trx;
      }

      // ERROR in indexing logic
      std::string msg = "transaction_id=" + transaction_id.str() + " indexed with block_id=" + itr->block_id.str() + ", but ";
      if (!block)
         msg += "block was not found";
      else
         msg += "transaction was not found in the block";
      BOOST_THROW_EXCEPTION( std::runtime_error( msg ) );
   }

#warning TODO: lookup of recent transactions
   FC_THROW_EXCEPTION(chain::unknown_transaction_exception,
                      "Could not find transaction for: ${id}", ("id", transaction_id.str()));
}

void account_history_plugin_impl::applied_block(const signed_block& block)
{
   auto block_id = block.id();
   for (const auto& cycle : block.cycles)
      for (const auto& thread : cycle)
         for (const auto& trx : thread.user_input)
            _block_index.insert({block_id, trx.id()});
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
