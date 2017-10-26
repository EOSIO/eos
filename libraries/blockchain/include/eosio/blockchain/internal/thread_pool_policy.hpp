#pragma once
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <fc/optional.hpp>

namespace eosio { namespace blockchain { namespace internal {
   using boost::asio::io_service;
   using boost::thread_group;
   using fc::optional;

   class thread_pool_policy {
      public:
         thread_pool_policy(int _num_threads)
            : num_threads(_num_threads)
         {}

         int num_threads;

         class dispatcher {
            public:
               dispatcher(thread_pool_policy& policy)
                  : ios()
                  , work(io_service::work(ios))
               {
                  auto thread_proc = boost::bind(&boost::asio::io_service::run, &ios);
                  for (int idx = 0; idx < policy.num_threads; idx++) {
                     thread_pool.create_thread(thread_proc);
                  }
               }

               ~dispatcher() {
                  work.reset();
                  thread_pool.join_all();
               }

               template<typename JobProc>
               void dispatch(JobProc &&proc) {
                  ios.post(proc);
               }

            private:
               io_service ios;
               optional<io_service::work> work;
               thread_group thread_pool;
         };
   };

   template<int NumThreads>
   class static_thread_pool_policy : public thread_pool_policy {
      public:
         static_thread_pool_policy()
            :thread_pool_policy(NumThreads)
         {}
   };

}}};