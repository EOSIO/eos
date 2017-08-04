#include <eos/account_history_plugin/account_history_plugin.hpp>
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

namespace eos {

using chain::block_id_type;
using chain::ProcessedTransaction;
using chain::signed_block;
using boost::multi_index_container;
using chain::transaction_id_type;
using namespace boost::multi_index;

class transaction_history_object : public chainbase::object<chain::transaction_history_object_type, transaction_history_object> {
   OBJECT_CTOR(transaction_history_object)

   id_type             id;
   block_id_type       block_id;
   transaction_id_type transaction_id;
};

struct by_id;
struct by_trx_id;
using transaction_history_multi_index = chainbase::shared_multi_index_container<
   transaction_history_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(transaction_history_object, transaction_history_object::id_type, id)>,
      hashed_unique<tag<by_trx_id>, BOOST_MULTI_INDEX_MEMBER(transaction_history_object, transaction_id_type, transaction_id), std::hash<transaction_id_type>>
   >
>;

typedef chainbase::generic_index<transaction_history_multi_index> transaction_history_index;

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
      std::string msg = "transaction_id=" + transaction_id.str() + " indexed with block_id=" + block_id->str() + ", but ";
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

CHAINBASE_SET_INDEX_TYPE( eos::transaction_history_object, eos::transaction_history_multi_index )

FC_REFLECT( eos::transaction_history_object, (block_id)(transaction_id) )
