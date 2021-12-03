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
      const auto& itr = tracked_blocks.find( trace->block_num );
      if( itr == tracked_blocks.end() )
         return;
      auto& tracked = itr->second;
      if( chain::is_onblock( *trace )) {
         tracked.onblock_trace.emplace( cache_trace{trace, t} );
      } else if( trace->failed_dtrx_trace ) {
         tracked.cached_traces[trace->failed_dtrx_trace->id] = {trace, t};
      } else {
         tracked.cached_traces[trace->id] = {trace, t};
      }
   }

   void on_accepted_block(const chain::block_state_ptr& block_state) {
      store_block_trace( block_state );
   }

   void on_irreversible_block( const chain::block_state_ptr& block_state ) {
      store_lib( block_state );
   }

   void on_block_start( uint32_t block_num ) {
      tracked_blocks[block_num] = block_tracking{};
   }

   void clear_caches( uint32_t block_num ) {
      tracked_blocks.erase( block_num );
   }

   void store_block_trace( const chain::block_state_ptr& block_state ) {
      try {
         using transaction_trace_t = transaction_trace_v2;

         auto bt = create_block_trace( block_state );
         const auto& itr = tracked_blocks.find( block_state->block_num );

         if( itr == tracked_blocks.end() )
            return;
         auto& tracked = itr->second;
         std::vector<transaction_trace_t>& traces = std::get<std::vector<transaction_trace_t>>(bt.transactions);
         traces.reserve( block_state->block->transactions.size() + 1 );
         if( tracked.onblock_trace )
            traces.emplace_back( to_transaction_trace<transaction_trace_t>( *(tracked.onblock_trace) ));
         for( const auto& r : block_state->block->transactions ) {
            transaction_id_type id;
            if( std::holds_alternative<transaction_id_type>(r.trx)) {
               id = std::get<transaction_id_type>(r.trx);
            } else {
               id = std::get<packed_transaction>(r.trx).id();
            }
            const auto it = tracked.cached_traces.find( id );
            if( it != tracked.cached_traces.end() ) {
               traces.emplace_back( to_transaction_trace<transaction_trace_t>( it->second ));
            }
         }
         clear_caches( block_state->block_num );

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
   struct block_tracking {
      std::map<transaction_id_type, cache_trace>                cached_traces;
      std::optional<cache_trace>                                onblock_trace;
   };
   std::map<uint32_t, block_tracking>                           tracked_blocks;
};

}}
