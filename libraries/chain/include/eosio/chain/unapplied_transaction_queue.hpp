#pragma once

#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/exceptions.hpp>

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

enum class trx_enum_type {
   unknown = 0,
   persisted = 1,
   forked = 2,
   aborted = 3
};

struct unapplied_transaction {
   const transaction_metadata_ptr trx_meta;
   const fc::time_point           expiry;
   trx_enum_type                  trx_type = trx_enum_type::unknown;

   const transaction_id_type& id()const { return trx_meta->id(); }

   unapplied_transaction(const unapplied_transaction&) = delete;
   unapplied_transaction() = delete;
   unapplied_transaction& operator=(const unapplied_transaction&) = delete;
   unapplied_transaction(unapplied_transaction&&) = default;
};

/**
 * Track unapplied transactions for persisted, forked blocks, and aborted blocks.
 * Persisted are first so that they can be applied in each block until expired.
 */
class unapplied_transaction_queue {
public:
   enum class process_mode {
      non_speculative,           // HEAD, READ_ONLY, IRREVERSIBLE
      speculative_non_producer,  // will never produce
      speculative_producer       // can produce
   };

private:
   struct by_trx_id;
   struct by_type;
   struct by_expiry;

   typedef multi_index_container< unapplied_transaction,
      indexed_by<
         hashed_unique< tag<by_trx_id>,
               const_mem_fun<unapplied_transaction, const transaction_id_type&, &unapplied_transaction::id>
         >,
         ordered_non_unique< tag<by_type>, member<unapplied_transaction, trx_enum_type, &unapplied_transaction::trx_type> >,
         ordered_non_unique< tag<by_expiry>, member<unapplied_transaction, const fc::time_point, &unapplied_transaction::expiry> >
      >
   > unapplied_trx_queue_type;

   unapplied_trx_queue_type queue;
   process_mode mode = process_mode::speculative_producer;

public:

   void set_mode( process_mode new_mode ) {
      if( new_mode != mode ) {
         FC_ASSERT( empty(), "set_mode, queue required to be empty" );
      }
      mode = new_mode;
   }

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
      return queue.get<by_type>().find( trx_enum_type::persisted ) != queue.get<by_type>().end();
   }

   bool is_persisted(const transaction_metadata_ptr& trx)const {
      auto itr = queue.get<by_trx_id>().find( trx->id() );
      if( itr == queue.get<by_trx_id>().end() ) return false;
      return itr->trx_type == trx_enum_type::persisted;
   }

   transaction_metadata_ptr get_trx( const transaction_id_type& id ) const {
      auto itr = queue.get<by_trx_id>().find( id );
      if( itr == queue.get<by_trx_id>().end() ) return {};
      return itr->trx_meta;
   }

   template <typename Func>
   bool clear_expired( const time_point& pending_block_time, const time_point& deadline, Func&& callback ) {
      auto& persisted_by_expiry = queue.get<by_expiry>();
      while(!persisted_by_expiry.empty() && persisted_by_expiry.begin()->expiry <= pending_block_time) {
         if (deadline <= fc::time_point::now()) {
            return false;
         }
         callback( persisted_by_expiry.begin()->id(), persisted_by_expiry.begin()->trx_type );
         persisted_by_expiry.erase( persisted_by_expiry.begin() );
      }
      return true;
   }

   void clear_applied( const block_state_ptr& bs ) {
      if( empty() ) return;
      auto& idx = queue.get<by_trx_id>();
      for( const auto& receipt : bs->block->transactions ) {
         if( receipt.trx.contains<packed_transaction>() ) {
            const auto& pt = receipt.trx.get<packed_transaction>();
            auto itr = queue.get<by_trx_id>().find( pt.id() );
            if( itr != queue.get<by_trx_id>().end() ) {
               if( itr->trx_type != trx_enum_type::persisted ) {
                  idx.erase( pt.id() );
               }
            }
         }
      }
   }

   void add_forked( const branch_type& forked_branch ) {
      if( mode == process_mode::non_speculative || mode == process_mode::speculative_non_producer ) return;
      // forked_branch is in reverse order
      for( auto ritr = forked_branch.rbegin(), rend = forked_branch.rend(); ritr != rend; ++ritr ) {
         const block_state_ptr& bsptr = *ritr;
         for( auto itr = bsptr->trxs_metas().begin(), end = bsptr->trxs_metas().end(); itr != end; ++itr ) {
            const auto& trx = *itr;
            fc::time_point expiry = trx->packed_trx()->expiration();
            queue.insert( { trx, expiry, trx_enum_type::forked } );
         }
      }
   }

   void add_aborted( std::vector<transaction_metadata_ptr> aborted_trxs ) {
      if( mode == process_mode::non_speculative || mode == process_mode::speculative_non_producer ) return;
      for( auto& trx : aborted_trxs ) {
         fc::time_point expiry = trx->packed_trx()->expiration();
         queue.insert( { std::move( trx ), expiry, trx_enum_type::aborted } );
      }
   }

   void add_persisted( const transaction_metadata_ptr& trx ) {
      if( mode == process_mode::non_speculative ) return;
      auto itr = queue.get<by_trx_id>().find( trx->id() );
      if( itr == queue.get<by_trx_id>().end() ) {
         fc::time_point expiry = trx->packed_trx()->expiration();
         queue.insert( { trx, expiry, trx_enum_type::persisted } );
      } else if( itr->trx_type != trx_enum_type::persisted ) {
         queue.get<by_trx_id>().modify( itr, [](auto& un){
            un.trx_type = trx_enum_type::persisted;
         } );
      }
   }

   using iterator = unapplied_trx_queue_type::index<by_type>::type::iterator;

   iterator begin() { return queue.get<by_type>().begin(); }
   iterator end() { return queue.get<by_type>().end(); }

   iterator persisted_begin() { return queue.get<by_type>().lower_bound( trx_enum_type::persisted ); }
   iterator persisted_end() { return queue.get<by_type>().upper_bound( trx_enum_type::persisted ); }

   iterator erase( iterator itr ) { return queue.get<by_type>().erase( itr ); }

};

} } //eosio::chain
