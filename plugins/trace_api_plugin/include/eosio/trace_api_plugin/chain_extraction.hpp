#pragma once

#include <eosio/trace_api_plugin/trace.hpp>
#include <map>

namespace eosio { namespace trace_api_plugin {

using chain::transaction_id_type;
using chain::packed_transaction;

template <typename StoreProvider>
class chain_extraction_impl_type {
public:
   explicit chain_extraction_impl_type( StoreProvider& store )
   : store(store) {}

   void signal_applied_transaction( const chain::transaction_trace_ptr& trace, const chain::signed_transaction& strx ) {
      on_applied_transaction( trace, strx );
   }

   void signal_accepted_block( const chain::block_state_ptr& bsp ) {
      on_accepted_block( bsp );
   }

private:
   static bool is_onblock(const chain::transaction_trace_ptr& p) {
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

   void on_applied_transaction(const chain::transaction_trace_ptr& p, const chain::signed_transaction& t) {
      if (p->receipt) {
         if( is_onblock( p )) {
            onblock_trace.emplace( p );
         } else if (p->failed_dtrx_trace) {
            cached_traces[p->failed_dtrx_trace->id] = p;
         } else {
            cached_traces[p->id] = p;
         }
      }
   }

   void on_accepted_block(const chain::block_state_ptr& block_state) {
      store_block_trace( block_state );
   }

   void store_block_trace( const chain::block_state_ptr& block_state ) {
      try {
         block_trace_v0 bt = create_block_trace_v0( block_state );

         std::vector<transaction_trace_v0>& traces = bt.transactions;
         traces.reserve( block_state->block->transactions.size() + 1 );
         if( onblock_trace )
            traces.emplace_back( to_transaction_trace_v0( *onblock_trace ));
         for( const auto& r : block_state->block->transactions ) {
            transaction_id_type id;
            if( r.trx.contains<transaction_id_type>()) {
               id = r.trx.get<transaction_id_type>();
            } else {
               id = r.trx.get<packed_transaction>().id();
            }
            const auto it = cached_traces.find( id );
            // could assert, go with best effort
            if( it != cached_traces.end() ) {
               traces.emplace_back( to_transaction_trace_v0( it->second ));
            } else {
               traces.emplace_back( transaction_trace_v0{ .id = id } );
            }
         }
         cached_traces.clear();
         onblock_trace.reset();

         uint64_t offset = store.append_data_log( std::move( bt ));

         store.append_metadata_log( {.id = block_state->id, .number = block_state->block_num, .offset = offset} );
      } catch( const fc::exception& e ) {
         // todo log
      } catch( const std::exception& e ) {
         // todo log
      } catch( ... ) {
         //todo shutdown, log
      }
   }

private:
   StoreProvider&                                               store;
   std::map<transaction_id_type, chain::transaction_trace_ptr>  cached_traces;
   fc::optional<chain::transaction_trace_ptr>                   onblock_trace;

};

}}