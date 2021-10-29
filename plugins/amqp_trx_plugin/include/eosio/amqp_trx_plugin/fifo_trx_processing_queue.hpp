#pragma once

#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/log/logger_config.hpp>

namespace eosio {

namespace detail {

/**
 * Thread safe blocking queue which supports pausing.
 * push will block once max_depth_ is reached.
 * pop blocks until item is available in queue and unpaused.
 */
template <typename T>
class blocking_queue {
public:

   /// @param max_depth allowed entries until push is blocked
   explicit blocking_queue( size_t max_depth )
         : max_depth_( max_depth ) {}

   /// blocks thread if queue is full
   bool push(T&& t) {
      {
         std::unique_lock<std::mutex> lk(mtx_);
         full_cv_.wait(lk, [this]() {
            return (queue_.size() < max_depth_) || stopped_;
         });
         if( stopped_ ) return false;
         queue_.emplace_back(std::move(t));
      }
      empty_cv_.notify_one();
      return true;
   }

   /// non-blocking and does not respect max_depth_
   void push_front(T&& t) {
      {
         std::unique_lock<std::mutex> lk(mtx_);
         queue_.emplace_front(std::move(t));
      }
      empty_cv_.notify_one();
   }

   /// blocks thread until item is available and queue is unpaused.
   /// pauses queue so nothing else can pull one off until unpaused.
   /// @return false if queue is stopped and t is not modified
   bool pop_and_pause(T& t) {
      {
         std::unique_lock<std::mutex> lk(mtx_);
         empty_cv_.wait(lk, [this]() {
            return (!queue_.empty() && !paused_) || stopped_;
         });
         if( stopped_ ) return false;
         t = std::move(queue_.front());
         queue_.pop_front();
         ++paused_;
      }
      full_cv_.notify_one();
      return true;
   }

   /// pause pop of queue. Does not pause/block push.
   void pause() {
      std::scoped_lock<std::mutex> lk(mtx_);
      ++paused_;
   }

   /// unpause pop of queue.
   void unpause() {
      {
         std::scoped_lock<std::mutex> lk( mtx_ );
         --paused_;
         if( paused_ < 0 ) {
            throw std::logic_error("blocking_queue unpaused when not paused");
         }
      }
      empty_cv_.notify_all();
   }

   /// cause pop to unblock
   void stop() {
      mtx_.lock();
      stopped_ = true;
      mtx_.unlock();
      empty_cv_.notify_all();
      full_cv_.notify_all();
   }

   /// also checks paused flag because a paused queue indicates processing is on-going or explicitly paused
   bool empty() const {
      std::scoped_lock<std::mutex> lk(mtx_);
      return queue_.empty() && !paused_;
   }

   template<typename Lamba>
   void for_each(Lamba cb, bool clear) {
      {
         std::scoped_lock<std::mutex> lk( mtx_ );
         for( const auto& i : queue_ ) {
            cb(i);
         }
         if( clear ) queue_.clear();
      }
      if( clear ) full_cv_.notify_one();
   }

private:
   mutable std::mutex mtx_;
   bool stopped_ = false;
   int32_t paused_ = 0; // 0 is unpaused
   std::condition_variable full_cv_;
   std::condition_variable empty_cv_;
   std::deque<T> queue_;
   size_t max_depth_ = 0;
};

} // detail

/**
 * FIFO queue for starting signature recovery. Enforces limit (max_depth) on queued transactions to be processed.
 * Only one transaction at a time is submitted to the main application thread so that order is preserved.
 */
template<typename ProducerPlugin>
class fifo_trx_processing_queue : public std::enable_shared_from_this<fifo_trx_processing_queue<ProducerPlugin>> {

private:
   struct q_item {
      uint64_t delivery_tag = 0;
      chain::packed_transaction_ptr trx;
      chain::recover_keys_future fut;
      producer_plugin::next_function<chain::transaction_trace_ptr> next;
   };
   eosio::detail::blocking_queue<q_item> queue_;
   std::thread thread_;
   std::atomic_bool running_ = true;
   bool started_ = false;
   const chain::chain_id_type chain_id_;
   const uint32_t configured_subjective_signature_length_limit_ = 0;
   const bool allow_speculative_execution = false;
   boost::asio::io_context& sig_thread_pool_;
   ProducerPlugin* prod_plugin_ = nullptr;

public:

   /**
    * @param chain_id for signature recovery
    * @param configured_subjective_signature_length_limit for signature recovery
    * @param sig_thread_pool for signature recovery
    * @param prod_plugin producer_plugin
    * @param max_depth the queue depth for number of trxs to start signature recovery on
    */
   fifo_trx_processing_queue( const chain::chain_id_type& chain_id,
                              uint32_t configured_subjective_signature_length_limit,
                              bool allow_speculative_execution,
                              boost::asio::io_context& sig_thread_pool,
                              ProducerPlugin* prod_plugin,
                              size_t max_depth )
   : queue_(max_depth)
   , chain_id_(chain_id)
   , configured_subjective_signature_length_limit_(configured_subjective_signature_length_limit)
   , allow_speculative_execution(allow_speculative_execution)
   , sig_thread_pool_(sig_thread_pool)
   , prod_plugin_(prod_plugin)
   {
   }

   /// separate run() because of shared_from_this
   void run() {
      queue_.pause(); // start paused, on_block_start will unpause
      thread_ = std::thread([self=this->shared_from_this()]() {
         fc::set_os_thread_name( "trxq" );
         while( self->running_ ) {
            try {
               q_item i;
               if( self->queue_.pop_and_pause(i) ) {
                  auto exception_handler = [&i, &prod_plugin=self->prod_plugin_](fc::exception_ptr ex) {
                     prod_plugin->log_failed_transaction(i.trx->id(), i.trx, ex->what());
                     i.next(ex);
                  };
                  chain::transaction_metadata_ptr trx_meta;
                  try {
                     trx_meta = i.fut.get();
                  } CATCH_AND_CALL(exception_handler);

                  if( trx_meta ) {
                     dlog( "posting trx: ${id}", ("id", trx_meta->id()) );
                     app().post( priority::low, [self, trx{std::move( trx_meta )}, next{std::move( i.next )}]() {
                        self->prod_plugin_->execute_incoming_transaction( trx, next );
                        self->queue_.unpause();
                     } );
                  } else {
                     self->queue_.unpause();
                  }
               }
               continue;
            }
            FC_LOG_AND_DROP();
            // something completely unexpected
            elog( "Unexpected error, exiting. See above errors." );
            app().quit();
            self->queue_.stop();
            self->running_ = false;
         }
      });
   }

   /// shutdown queue processing
   void stop() {
      running_ = false;
      queue_.stop();
      if( thread_.joinable() ) {
         thread_.join();
      }
   }

   /// Should be called on each start block from app() thread
   void on_block_start() {
      started_ = true;
      if( allow_speculative_execution || prod_plugin_->is_producing_block() ) {
         queue_.unpause();
      }
   }

   /// Should be called on each block finalize from app() thread
   void on_block_stop() {
      if( started_ && ( allow_speculative_execution || prod_plugin_->is_producing_block() ) ) {
         queue_.pause();
      }
   }

   template<typename Lambda>
   void for_each_delivery_tag(Lambda cb, bool clear) {
      queue_.for_each(
            [&cb](const q_item& i){
               cb(i.delivery_tag);
         }, clear);
   }

   /// thread-safe
   /// queue a trx to be processed. transactions are processed in the order pushed
   /// next function is called from the app() thread
   void push(chain::packed_transaction_ptr trx, uint64_t delivery_tag, producer_plugin::next_function<chain::transaction_trace_ptr> next) {
      fc::microseconds max_trx_cpu_usage = prod_plugin_->get_max_transaction_time();
      auto future = chain::transaction_metadata::start_recover_keys( trx, sig_thread_pool_, chain_id_, max_trx_cpu_usage,
                                                                     transaction_metadata::trx_type::input,
                                                                     configured_subjective_signature_length_limit_ );
      q_item i{ .delivery_tag = delivery_tag, .trx = std::move(trx), .fut = std::move(future), .next = std::move(next) };
      if( !queue_.push( std::move( i ) ) ) {
         ilog( "Queue stopped, unable to process transaction ${id}, not ack'ed to AMQP", ("id", trx->id()) );
      }
   }

   bool empty() const {
      return queue_.empty();
   }
};

} //eosio
