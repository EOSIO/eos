/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/block_state.hpp>

#include <queue>

namespace eosio { namespace chain {

/**
 * Track unapplied transactions for forked blocks, aborted blocks, and subjectively
 * failed transactions.
 */
class unapplied_transaction_queue {
   std::deque<chain::branch_type> forked_branches;
   size_t current_trx_in_block = 0;
   std::deque<std::vector<chain::transaction_metadata_ptr>> aborted_block_trxs;
   size_t current_trx_in_trxs = 0;
   std::deque<chain::transaction_metadata_ptr> subjective_failed_trxs;
   size_t total_size;
public:

   bool empty() const {
      return forked_branches.empty() && aborted_block_trxs.empty() && subjective_failed_trxs.empty();
   }

   size_t size() const {
      return total_size;
   }

   void clear() {
      forked_branches.clear();
      current_trx_in_block = 0;
      aborted_block_trxs.clear();
      current_trx_in_trxs = 0;
      subjective_failed_trxs.clear();
      total_size = 0;
   }

   void add_forked( chain::branch_type forked_branch ) {
      if( forked_branch.empty() ) return;
      // forked_branch is in reverse order
      // fifo queue for branches so pop back
      size_t size = 0;
      for( const auto& bptr : forked_branch ) {
         size += bptr->trxs.size();
      }
      forked_branches.emplace_front( std::move( forked_branch ) );
      total_size += size;
   }

   void add_aborted( std::vector<chain::transaction_metadata_ptr> aborted_trxs ) {
      if( aborted_trxs.empty() ) return;
      // fifo queue so pop back
      size_t size = aborted_trxs.size();
      aborted_block_trxs.emplace_front( std::move( aborted_trxs ) );
      total_size += size;
   }

   void add_subjective_failure( chain::transaction_metadata_ptr trx ) {
      // fifo queue so pop back
      subjective_failed_trxs.emplace_front( std::move( trx ) );
      ++total_size;
   }

   /// Order returned: forked block transactions, aborted block transactions, subjectively failed
   /// FIFO within each category
   /// @return nullptr if no next
   chain::transaction_metadata_ptr next() {
      while( !forked_branches.empty() ) {
         chain::branch_type& branch = forked_branches.back();
         if( branch.empty() ) {
            current_trx_in_block = 0;
            forked_branches.pop_back();
            continue;
         }
         chain::block_state_ptr& bs = branch.back();
         if( current_trx_in_block >= bs->trxs.size() ) {
            current_trx_in_block = 0;
            branch.resize( branch.size() - 1 );
            if( branch.empty() ) {
               forked_branches.pop_back();
            }
            continue;
         } else {
            chain::transaction_metadata_ptr n = bs->trxs[current_trx_in_block];
            ++current_trx_in_block;
            if( current_trx_in_block >= bs->trxs.size() ) {
               current_trx_in_block = 0;
               branch.resize( branch.size() - 1 );
               if( branch.empty() ) {
                  forked_branches.pop_back();
               }
            }
            --total_size;
            return n;
         }
      }
      if( !aborted_block_trxs.empty() ) {
         std::vector<chain::transaction_metadata_ptr>& p = aborted_block_trxs.back();
         chain::transaction_metadata_ptr n = p[current_trx_in_trxs];
         ++current_trx_in_trxs;
         if( current_trx_in_trxs >= p.size() ) {
            current_trx_in_trxs = 0;
            aborted_block_trxs.pop_back();
         }
         --total_size;
         return n;
      }
      if( !subjective_failed_trxs.empty() ) {
         chain::transaction_metadata_ptr n = subjective_failed_trxs.back();
         subjective_failed_trxs.pop_back();
         --total_size;
         return n;
      }
      return {};
   }

};

} } //eosio::chain
