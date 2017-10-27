#pragma once
#include <fc/optional.hpp>

#include <boost/asio.hpp>
#include <thread>
#include <atomic>

#include <vector>
#include <deque>

namespace eosio { namespace blockchain {

   using std::function;
   using std::vector;
   using std::deque;



   /**
    *  @class scheduler
    *  @brief dispatches functions to a thread pool and that can be waited upon async
    *
    *  Once async_wait() is called no more tasks may be pushed to the scheduler and the
    *  specified callback will be excuted once all existing tasks have completed.
    *
    *  Tasks can be made order-dependent by creating strands.
    */
   class scheduler {
      public:

         /**
          *  A strand handle is provided to make the strand opaque from the
          *  perspective of the user of the scheduler. This forces all posting
          *  to be done via post() method and also ensures that the strand's
          *  life time is properly managed/owned by the scheduler.
          */
         struct strand_handle {
            private:
               friend class scheduler;
               strand_handle( uint32_t index, scheduler& s )
               :_scheduler(s),_index(index){};

               scheduler& _scheduler;
               uint32_t   _index     = 0;
         };

         typedef function<void()>    lambda_type;

         scheduler( uint32_t thread_count = 8 );
         ~scheduler();

         /**
          * Creates a new strand of execution
          */
         strand_handle create_strand();

         /**
          * You must call create_strand() before you can post.
          *
          * @param after - the strand which the lambda will execute in
          */
         strand_handle post( lambda_type exec, strand_handle after );

         void          async_wait( lambda_type on_done );

      private:
         typedef boost::asio::strand strand;

         boost::asio::io_service                     _ios;
         fc::optional<boost::asio::io_service::work> _work;

         std::vector<std::thread>           _threads;
         std::deque<strand>                 _current_strands;
         std::atomic<uint32_t>              _completed;
         bool                               _wait = false;
         lambda_type                        _on_done;
   };

} }  /// eosio::blockchain
