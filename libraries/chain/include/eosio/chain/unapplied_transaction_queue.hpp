/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/block_state.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

namespace fc {
  inline std::size_t hash_value( const fc::sha256& v ) {
     return v._hash[3];
  }
}

namespace eosio { namespace chain {

using namespace boost::multi_index;

enum trx_enum_type {
   unknown = 0,
   persisted = 1,
   forked = 2,
   aborted = 3,
   subjective = 4
};

struct unapplied_transaction {
   const transaction_metadata_ptr trx_meta;
   const fc::time_point           expiry;
   const trx_enum_type            trx_type = unknown;

   const transaction_id_type& signed_id()const { return trx_meta->signed_id(); }
   const transaction_id_type& id()const { return trx_meta->id(); }
   bool is_persisted()const { return trx_type == persisted; }

   unapplied_transaction(const unapplied_transaction&) = delete;
   unapplied_transaction() = delete;
   unapplied_transaction& operator=(const unapplied_transaction&) = delete;
   unapplied_transaction(unapplied_transaction&&) = default;
};

/**
 * Track unapplied transactions for persisted, forked blocks, aborted blocks, and subjectively
 * failed transactions.
 */
class unapplied_transaction_queue {

   struct by_signed_id;
   struct by_type;
   struct by_expiry;

   typedef multi_index_container< unapplied_transaction,
      indexed_by<
         sequenced<>,
         hashed_unique< tag<by_signed_id>,
               const_mem_fun<unapplied_transaction, const transaction_id_type&, &unapplied_transaction::signed_id>
         >,
         ordered_non_unique< tag<by_type>, member<unapplied_transaction, const trx_enum_type, &unapplied_transaction::trx_type> >,
         ordered_non_unique< tag<by_expiry>, member<unapplied_transaction, const fc::time_point, &unapplied_transaction::expiry> >
      >
   > unapplied_trx_queue_type;

   unapplied_trx_queue_type queue;

   std::deque<chain::branch_type> forked_branches;
   size_t current_trx_in_block = 0;
   std::deque<std::vector<transaction_metadata_ptr>> aborted_block_trxs;
   size_t current_trx_in_trxs = 0;
   std::deque<chain::transaction_metadata_ptr> subjective_failed_trxs;
   size_t total_size = 0;
public:

   bool empty() const {
      return queue.empty();
   }

   size_t size() const {
      return queue.size();
   }

   void clear() {
      queue.clear();
   }

   bool contains_persisted()const {
      return queue.get<by_type>().find( persisted ) != queue.get<by_type>().end();
   }

   template <typename Func>
   bool clear_expired( const time_point& pending_block_time, const time_point& deadline, Func&& callback ) {
      auto& persisted_by_expiry = queue.get<by_expiry>();
      while(!persisted_by_expiry.empty() && persisted_by_expiry.begin()->expiry <= pending_block_time) {
         if (deadline <= fc::time_point::now()) {
            return false;
         }
         callback( persisted_by_expiry.begin()->id() );
         persisted_by_expiry.erase( persisted_by_expiry.begin() );
      }
      return true;
   }

   void add_forked( chain::branch_type forked_branch ) {
      // forked_branch is in reverse order
      for( auto ritr = forked_branch.rbegin(), rend = forked_branch.rend(); ritr != rend; ++ritr ) {
         for( auto itr = (*ritr)->trxs.begin(), end = (*ritr)->trxs.end(); itr != end; ++itr ) {
            const auto& trx = *itr;
            queue.push_back( {trx, trx->packed_trx()->expiration(), forked} );
         }
      }
   }

   void add_aborted( std::vector<chain::transaction_metadata_ptr> aborted_trxs ) {
      if( aborted_trxs.empty() ) return;
      for( const auto& trx : aborted_trxs ) {
         queue.push_back( { trx, trx->packed_trx()->expiration(), aborted } );
      }
   }

   void add_subjective_failure( chain::transaction_metadata_ptr trx ) {
      queue.push_back( { trx, trx->packed_trx()->expiration(), subjective } );
   }

   using iterator = unapplied_trx_queue_type::index<by_type>::type::iterator;

   iterator begin() { return queue.get<by_type>().begin(); }
   iterator end() { return queue.get<by_type>().end(); }
   iterator erase( iterator itr ) { return queue.get<by_type>().erase( itr ); }

};

} } //eosio::chain
