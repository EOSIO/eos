#pragma once

#include <eosio/trace_api/common.hpp>
#include <eosio/trace_api/trace.hpp>
#include <eosio/trace_api/extract_util.hpp>
#include <exception>
#include <functional>
#include <map>

namespace eosio { namespace trace_api {

using chain::transaction_id_type;
using chain::packed_transaction;

template <typename StoreProvider>
class chain_extraction_impl_type {
public:
   /**
    * Chain Extractor for capturing transaction traces, action traces, and block info.
    * @param store provider of append & append_lib
    * @param except_handler called on exceptions, logging if any is left to the user
    */
   chain_extraction_impl_type( StoreProvider store, exception_handler except_handler )
   : store(std::move(store))
   , except_handler(std::move(except_handler))
   {}

   /// connect to chain controller applied_transaction signal
   void signal_applied_transaction( const chain::transaction_trace_ptr& trace, const chain::packed_transaction_ptr& ptrx ) {
      on_applied_transaction( trace, ptrx );
   }

   /// connect to chain controller accepted_block signal
   void signal_accepted_block( const chain::block_state_ptr& bsp ) {
      on_accepted_block( bsp );
   }

   /// connect to chain controller irreversible_block signal
   void signal_irreversible_block( const chain::block_state_ptr& bsp ) {
      on_irreversible_block( bsp );
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

   void on_applied_transaction(const chain::transaction_trace_ptr& trace, const chain::packed_transaction_ptr& t) {
      if( !trace->receipt ) return;
      // include only executed transactions; soft_fail included so that onerror (and any inlines via onerror) are included
      if((trace->receipt->status != chain::transaction_receipt_header::executed &&
          trace->receipt->status != chain::transaction_receipt_header::soft_fail)) {
         return;
      }
      if( is_onblock( trace )) {
         onblock_trace.emplace( trace );
      } else if( trace->failed_dtrx_trace ) {
         cached_traces[trace->failed_dtrx_trace->id] = trace;
      } else {
         cached_traces[trace->id] = trace;
      }
   }

   void on_accepted_block(const chain::block_state_ptr& block_state) {
      store_block_trace( block_state );
   }

   void on_irreversible_block( const chain::block_state_ptr& block_state ) {
      store_lib( block_state );
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
            if( it != cached_traces.end() ) {
               traces.emplace_back( to_transaction_trace_v0( it->second ));
            }
         }
         cached_traces.clear();
         onblock_trace.reset();

         store.append( std::move( bt ) );

      } catch( ... ) {
         except_handler( MAKE_EXCEPTION_WITH_CONTEXT( std::current_exception() ) );
      }
   }

   void store_lib( const chain::block_state_ptr& bsp ) {
      try {
         store.append_lib( bsp->block_num );
      } catch( ... ) {
         except_handler( MAKE_EXCEPTION_WITH_CONTEXT( std::current_exception() ) );
      }
   }

private:
   StoreProvider                                                store;
   exception_handler                                            except_handler;
   std::map<transaction_id_type, chain::transaction_trace_ptr>  cached_traces;
   fc::optional<chain::transaction_trace_ptr>                   onblock_trace;

};

}}