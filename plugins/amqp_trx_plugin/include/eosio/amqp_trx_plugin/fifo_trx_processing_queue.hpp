#pragma once

#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/log/logger_config.hpp>

namespace eosio {

namespace detail {

template <typename T>
class blocking_queue {
public:
   explicit blocking_queue(size_t max_depth)
         : max_depth_(max_depth) {}

   void push(T&& t) {
      {
         std::unique_lock<std::mutex> lk(mtx_);
         while( queue_.size() > max_depth_ ) {
            full_cv_.wait(lk);
         }
         queue_.emplace_back(std::move(t));
      }
      empty_cv_.notify_one();
   }

   /// non-blocking and does not respect max_depth_
   void push_front(T&& t) {
      {
         std::unique_lock<std::mutex> lk(mtx_);
         queue_.emplace_front(std::move(t));
      }
      empty_cv_.notify_one();
   }

   bool pop(T& t) {
      {
         std::unique_lock<std::mutex> lk(mtx_);
         while( (queue_.empty() || paused_) && !stopped_ ) {
            dlog( "empty: ${e}, paused: ${p}", ("e", queue_.empty())("p", paused_.load()));
            empty_cv_.wait(lk);
         }
         dlog( "empty: ${e}, paused: ${p}", ("e", queue_.empty())("p", paused_.load()));
         if( stopped_ ) return false;
         t = std::move(queue_.front());
         queue_.pop_front();
      }
      full_cv_.notify_one();
      return true;
   }

   void pause() {
      paused_ = true;
      empty_cv_.notify_one();
   }

   void unpause() {
      paused_ = false;
      empty_cv_.notify_one();
   }

   void stop() {
      stopped_ = true;
      empty_cv_.notify_one();
   }

private:
   std::mutex mtx_;
   std::atomic_bool stopped_ = false;
   std::atomic_bool paused_ = false;
   std::condition_variable full_cv_;
   std::condition_variable empty_cv_;
   std::deque<T> queue_;
   size_t max_depth_ = 0;
};

} // detail

/**
 * FIFO queue for starting signature recovery. Enforces limit (max_depth) on queued transactions to be processed.
 */
class fifo_trx_processing_queue : public std::enable_shared_from_this<fifo_trx_processing_queue> {

private:
   struct q_item {
      chain::recover_keys_future fut;
      producer_plugin::next_function<chain::transaction_trace_ptr> next;
   };
   detail::blocking_queue<q_item> queue_;
   std::thread thread_;
   std::atomic_bool running_ = true;
   chain::chain_id_type chain_id_;
   uint32_t configured_subjective_signature_length_limit_ = 0;
   boost::asio::io_context& sig_thread_pool_;
   producer_plugin* prod_plugin_ = nullptr;

public:

   fifo_trx_processing_queue( const chain::chain_id_type& chain_id,
                              uint32_t configured_subjective_signature_length_limit,
                              boost::asio::io_context& sig_thread_pool,
                              producer_plugin* prod_plugin,
                              size_t max_depth )
   : queue_(max_depth)
   , chain_id_(chain_id)
   , configured_subjective_signature_length_limit_(configured_subjective_signature_length_limit)
   , sig_thread_pool_(sig_thread_pool)
   , prod_plugin_(prod_plugin)
   {
   }

   void run() {
      thread_ = std::thread([self=shared_from_this()]() {
         fc::set_os_thread_name( "trxq" );
         while( self->running_ ) {
            try {
               q_item i;
               if( self->queue_.pop(i) ) {
                  auto trx_meta = i.fut.get();
                  self->queue_.pause();
                  dlog("posting trx: ${id}", ("id", trx_meta->id()));
                  app().post(priority::low, [self, trx{std::move(trx_meta)}, next{std::move(i.next)}](){
                     auto retry_later = [self](const chain::transaction_metadata_ptr& trx, producer_plugin::next_function<chain::transaction_trace_ptr>& next) {
                        std::promise<chain::transaction_metadata_ptr> p;
                        q_item i;
                        i.fut = p.get_future();
                        p.set_value(trx);
                        i.next = next;
                        self->queue_.push_front( std::move( i ) );
                     };
                     self->prod_plugin_->execute_incoming_transaction(trx, next, retry_later);
                     dlog("unpausing for: ${id}", ("id", trx->id()));
                     self->queue_.unpause();
                  });
               }
            }
            FC_LOG_AND_DROP();
         }
      });
   }

   ~fifo_trx_processing_queue() {
      queue_.stop();
      running_ = false;
      if( thread_.joinable() ) {
         thread_.join();
      }
   }

   /// thread-safe
   /// queue a trx to be processed. transactions are processed in the order pushed
   void push(const chain::packed_transaction_ptr& trx, producer_plugin::next_function<chain::transaction_trace_ptr> next) {
      fc::microseconds max_trx_cpu_usage = prod_plugin_->get_max_transaction_time();
      auto future = chain::transaction_metadata::start_recover_keys( trx, sig_thread_pool_, chain_id_, max_trx_cpu_usage,
                                                                     configured_subjective_signature_length_limit_ );
      q_item i{ std::move(future), std::move(next)};
      queue_.push( std::move( i ) );
   }

};

} //eosio
