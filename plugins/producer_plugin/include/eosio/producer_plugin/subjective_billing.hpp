#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/config.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace eosio {

namespace bmi = boost::multi_index;
using chain::transaction_id_type;
using chain::account_name;
using chain::block_state_ptr;
using chain::packed_transaction;
namespace config = chain::config;

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

   using decaying_accumulator = chain::resource_limits::impl::exponential_decay_accumulator<>;

   struct subjective_billing_info {
      uint64_t              pending_cpu_us;        // tracked cpu us for transactions that may still succeed in a block
      decaying_accumulator  expired_accumulator;   // accumulator used to account for transactions that have expired

      bool empty(uint32_t block_num) {
         return pending_cpu_us == 0 && expired_accumulator.value_at(block_num, expired_accumulator_average_window) == 0;
      }
   };

   using account_subjective_bill_cache = std::map<account_name, subjective_billing_info>;
   using block_subjective_bill_cache = std::map<account_name, uint64_t>;

   bool                                      _disabled = false;
   trx_cache_index                           _trx_cache_index;
   account_subjective_bill_cache             _account_subjective_bill_cache;
   block_subjective_bill_cache               _block_subjective_bill_cache;

private:
   void remove_subjective_billing( const trx_cache_entry& entry, uint32_t block_num ) {
      auto aitr = _account_subjective_bill_cache.find( entry.account );
      if( aitr != _account_subjective_bill_cache.end() ) {
         aitr->second.pending_cpu_us -= entry.subjective_cpu_bill;
         EOS_ASSERT( aitr->second.pending_cpu_us >= 0, chain::tx_resource_exhaustion,
                     "Logic error in subjective account billing ${a}", ("a", entry.account) );
         if( aitr->second.empty(block_num) ) _account_subjective_bill_cache.erase( aitr );
      }
   }

   void transition_to_expired( const trx_cache_entry& entry, uint32_t block_num ) {
      auto aitr = _account_subjective_bill_cache.find( entry.account );
      if( aitr != _account_subjective_bill_cache.end() ) {
         aitr->second.pending_cpu_us -= entry.subjective_cpu_bill;
         aitr->second.expired_accumulator.add(entry.subjective_cpu_bill, block_num, expired_accumulator_average_window);
      }
   }

   void remove_subjective_billing( const block_state_ptr& bsp ) {
      if( !_trx_cache_index.empty() ) {
         for( const auto& receipt : bsp->block->transactions ) {
            if( std::holds_alternative<packed_transaction>(receipt.trx) ) {
               const auto& pt = std::get<packed_transaction>(receipt.trx);
               remove_subjective_billing( pt.id(), bsp->block_num );
            }
         }
      }
   }

public: // public for tests
   static constexpr uint32_t expired_accumulator_average_window = config::account_cpu_usage_average_window_ms / config::block_interval_ms;

   void remove_subjective_billing( const transaction_id_type& trx_id, uint32_t block_num ) {
      auto& idx = _trx_cache_index.get<by_id>();
      auto itr = idx.find( trx_id );
      if( itr != idx.end() ) {
         remove_subjective_billing( *itr, block_num );
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
            _account_subjective_bill_cache[first_auth].pending_cpu_us += bill;
            if( in_pending_block ) {
               _block_subjective_bill_cache[first_auth] += bill;
            }
         }
      }
   }

   void subjective_bill_failure( const account_name& first_auth, const fc::microseconds& elapsed, uint32_t block_num )
   {
      if( !_disabled ) {
         uint32_t bill = std::max<int64_t>( 0, elapsed.count() );
         _account_subjective_bill_cache[first_auth].expired_accumulator.add(bill, block_num, expired_accumulator_average_window);
      }
   }

   uint32_t get_subjective_bill( const account_name& first_auth, uint32_t block_num ) const {
      if( _disabled ) return 0;
      const subjective_billing_info* sub_bill_info = nullptr;
      auto aitr = _account_subjective_bill_cache.find( first_auth );
      if( aitr != _account_subjective_bill_cache.end() ) {
         sub_bill_info = &aitr->second;
      }
      uint64_t in_block_pending_cpu_us = 0;
      auto bitr = _block_subjective_bill_cache.find( first_auth );
      if( bitr != _block_subjective_bill_cache.end() ) {
         in_block_pending_cpu_us = bitr->second;
      }

      if (sub_bill_info) {
         EOS_ASSERT(sub_bill_info->pending_cpu_us >= in_block_pending_cpu_us, chain::tx_resource_exhaustion, "Logic error subjective billing ${a}", ("a", first_auth) );
         uint32_t sub_bill = sub_bill_info->pending_cpu_us - in_block_pending_cpu_us + sub_bill_info->expired_accumulator.value_at(block_num, expired_accumulator_average_window );
         return sub_bill;
      } else {
         return 0;
      }
   }

   void abort_block() {
      _block_subjective_bill_cache.clear();
   }

   void on_block( const block_state_ptr& bsp ) {
      if( bsp == nullptr ) return;
      remove_subjective_billing( bsp );
   }

   bool remove_expired( fc::logger& log, const fc::time_point& pending_block_time, uint32_t pending_block_num, const fc::time_point& deadline ) {
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
            transition_to_expired( *b, pending_block_num );
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
