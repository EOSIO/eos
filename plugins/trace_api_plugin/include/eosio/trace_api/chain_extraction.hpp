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

   /// connect to chain controller block_start signal
   void signal_block_start( uint32_t block_num ) {
      on_block_start( block_num );
   }

private:
   void on_applied_transaction(const chain::transaction_trace_ptr& trace, const chain::packed_transaction_ptr& t) {
      if( !trace->receipt ) return;
      // include only executed transactions; soft_fail included so that onerror (and any inlines via onerror) are included
      if((trace->receipt->status != chain::transaction_receipt_header::executed &&
          trace->receipt->status != chain::transaction_receipt_header::soft_fail)) {
         return;
      }
      if( chain::is_onblock( *trace )) {
         onblock_trace.emplace( cache_trace{trace, t} );
      } else if( trace->failed_dtrx_trace ) {
         cached_traces[trace->failed_dtrx_trace->id] = {trace, t};
      } else {
         cached_traces[trace->id] = {trace, t};
      }
   }

   void on_accepted_block(const chain::block_state_ptr& block_state) {
      store_block_trace( block_state );
   }

   void on_irreversible_block( const chain::block_state_ptr& block_state ) {
      store_lib( block_state );
   }

   void on_block_start( uint32_t block_num ) {
      clear_caches();
   }

   void clear_caches() {
      cached_traces.clear();
      onblock_trace.reset();
   }

   void store_block_trace( const chain::block_state_ptr& block_state ) {
      try {
         block_trace_v1 bt = create_block_trace_v1( block_state );

         std::vector<transaction_trace_v1>& traces = bt.transactions_v1;
         traces.reserve( block_state->block->transactions.size() + 1 );
         if( onblock_trace )
            traces.emplace_back( to_transaction_trace_v1( *onblock_trace ));
         for( const auto& r : block_state->block->transactions ) {
            transaction_id_type id;
            if( std::holds_alternative<transaction_id_type>(r.trx)) {
               id = std::get<transaction_id_type>(r.trx);
            } else {
               id = std::get<packed_transaction>(r.trx).id();
            }
            const auto it = cached_traces.find( id );
            if( it != cached_traces.end() ) {
               traces.emplace_back( to_transaction_trace_v1( it->second ));
            }
         }
         clear_caches();

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
   std::map<transaction_id_type, cache_trace>                   cached_traces;
   std::optional<cache_trace>                                   onblock_trace;

};

}}
