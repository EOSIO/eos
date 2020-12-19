#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/transaction_metadata.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace eosio {

namespace bmi = boost::multi_index;
using chain::transaction_id_type;
using chain::account_name;
using chain::block_state_ptr;
using chain::transaction_metadata_ptr;
using chain::packed_transaction;

class subjective_billing {
private:

   struct trx_cache_entry {
      transaction_id_type     trx_id;
      account_name            account;
      uint32_t                subjective_cpu_bill;
      fc::time_point          expiry;
   };
   struct by_id;
   struct by_expiry;

   using trx_cache_index = bmi::multi_index_container<
         trx_cache_entry,
         indexed_by<
               bmi::hashed_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER( trx_cache_entry, transaction_id_type, trx_id ) >,
               ordered_non_unique<tag<by_expiry>, BOOST_MULTI_INDEX_MEMBER( trx_cache_entry, fc::time_point, expiry ) >
         >
   >;

   using account_subjective_bill_cache = std::map<account_name, int32_t>;
   using block_subjective_bill_cache = std::map<account_name, int32_t>;

   bool                                      _disabled = false;
   trx_cache_index                           _trx_cache_index;
   account_subjective_bill_cache             _account_subjective_bill_cache;
   block_subjective_bill_cache               _block_subjective_bill_cache;

private:
   void remove_subjective_billing( const trx_cache_entry& entry ) {
      auto aitr = _account_subjective_bill_cache.find( entry.account );
      if( aitr != _account_subjective_bill_cache.end() ) {
         aitr->second -= entry.subjective_cpu_bill;
         EOS_ASSERT( aitr->second >= 0, chain::tx_resource_exhaustion,
                     "Logic error in subjective account billing ${a}", ("a", entry.account) );
         if( aitr->second == 0 ) _account_subjective_bill_cache.erase( aitr );
      }
   }

   void remove_subjective_billing( const block_state_ptr& bsp ) {
      if( !_trx_cache_index.empty() ) {
         for( const auto& receipt : bsp->block->transactions ) {
            if( receipt.trx.contains<packed_transaction>() ) {
               const auto& pt = receipt.trx.get<packed_transaction>();
               remove_subjective_billing( pt.id() );
            }
         }
      }
   }

   void remove_subjective_billing( const std::vector<transaction_metadata_ptr>& trxs ) {
      if( !_trx_cache_index.empty() ) {
         for( const auto& trx : trxs ) {
            remove_subjective_billing( trx->id() );
         }
      }
   }

public: // public for tests

   void remove_subjective_billing( const transaction_id_type& trx_id ) {
      auto& idx = _trx_cache_index.get<by_id>();
      auto itr = idx.find( trx_id );
      if( itr != idx.end() ) {
         remove_subjective_billing( *itr );
         idx.erase( itr );
      }
   }

public:
   void disable() { _disabled = true; }

   /// @param in_pending_block pass true if pt's bill time is accounted for in the pending block
   void subjective_bill( const transaction_id_type& id, const fc::time_point& expire, const account_name& first_auth,
                         const fc::microseconds& elapsed, bool in_pending_block )
   {
      if( !_disabled ) {
         uint32_t bill = std::max<int64_t>( 0, elapsed.count() );
         auto p = _trx_cache_index.emplace(
               trx_cache_entry{id,
                               first_auth,
                               bill,
                               expire} );
         if( p.second ) {
            _account_subjective_bill_cache[first_auth] += bill;
            if( in_pending_block ) {
               // if in_pending_block then we have billed user until block is aborted/applied
               // keep track of this double bill amount so we can subtract it out in get_subjective_bill
               _block_subjective_bill_cache[first_auth] += bill;
            }
         }
      }
   }

   uint32_t get_subjective_bill( const account_name& first_auth ) const {
      if( _disabled ) return 0;
      int32_t sub_bill = 0;
      auto aitr = _account_subjective_bill_cache.find( first_auth );
      if( aitr != _account_subjective_bill_cache.end() ) {
         sub_bill = aitr->second;
         auto bitr = _block_subjective_bill_cache.find( first_auth );
         if( bitr != _block_subjective_bill_cache.end() ) {
            sub_bill -= bitr->second;
         }
      }
      EOS_ASSERT( sub_bill >= 0, chain::tx_resource_exhaustion, "Logic error subjective billing ${a}", ("a", first_auth) );
      return sub_bill;
   }

   void abort_block() {
      _block_subjective_bill_cache.clear();
   }

   void on_block( const block_state_ptr& bsp ) {
      if( bsp == nullptr ) return;
      remove_subjective_billing( bsp );
   }

   bool remove_expired( fc::logger& log, const fc::time_point& pending_block_time, const fc::time_point& deadline ) {
      bool exhausted = false;
      auto& idx = _trx_cache_index.get<by_expiry>();
      if( !idx.empty() ) {
         const auto orig_count = _trx_cache_index.size();
         uint32_t num_expired = 0;

         while( !idx.empty() ) {
            if( deadline <= fc::time_point::now() ) {
               exhausted = true;
               break;
            }
            auto b = idx.begin();
            if( b->expiry > pending_block_time ) break;
            remove_subjective_billing( *b );
            idx.erase( b );
            num_expired++;
         }

         fc_dlog( log, "Processed ${n} subjective billed transactions, Expired ${expired}",
                  ("n", orig_count)( "expired", num_expired ) );
      }
      return !exhausted;
   }
};

} //eosio
