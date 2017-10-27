#pragma once
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <fc/optional.hpp>

namespace eosio { namespace blockchain { namespace internal {
   using boost::asio::io_service;
   using boost::thread_group;
   using fc::optional;

   /**
    * Dispatch policy that dispatches jobs to a thread pool
    */
   class thread_pool_policy {
      public:
         thread_pool_policy(int _num_threads)
         :num_threads(_num_threads)
         {}

         int num_threads;

         /**
          * dispatcher that instantiates a thread pool and dispatches jobs to it
          * destroying this dispatcher will wait for all pending jobs to complete
          */
         class dispatcher {
            public:
               dispatcher(thread_pool_policy&& policy)
               :_ios()
               ,_work(io_service::work(_ios))
               {
                  auto thread_proc = boost::bind(&boost::asio::io_service::run, &_ios);
                  for (int idx = 0; idx < policy.num_threads; idx++) {
                     _thread_pool.create_thread(thread_proc);
                  }
               }

               ~dispatcher() {
                  _work.reset();
                  _thread_pool.join_all();
               }

            /**
             * Dispatch a job to the thread pool
             *
             * @param proc - the procedure to dispatch
             * @tparam JobProc - callable type for void()
             */
            template<typename JobProc>
               void dispatch(JobProc&& proc) {
                  _ios.post(proc);
               }

            private:
               io_service                    _ios;
               optional<io_service::work>    _work;
               thread_group                  _thread_pool;
         };
   };

   /**
    * Dispatch policy that dispatches jobs to a thread pool whose size is
    * determined at compile time
    * @tparam NumThreads - the size of the thread pool
    */
   template<int NumThreads>
   class static_thread_pool_policy : public thread_pool_policy {
      public:
         static_thread_pool_policy()
         :thread_pool_policy(NumThreads)
         {}
   };

}}} // eosio::blockchain::internal