#pragma once
#include <eosio/blockchain/types.hpp>

namespace eosio { namespace blockchain { namespace internal {

   /**
    * Dispatch policy that uses shared pointers to share a single dispatcher
    * instance with multiple services
    *
    * NOTE: this makes no additional guarantees of thread safety
    *
    * @tparam DispatchPolicy - type of job dispatch policy to share
    */
   template<typename DispatchPolicy>
   class shared_dispatch_policy {
      public:
         // dependent type of the child dispatcher
         typedef typename DispatchPolicy::dispatcher shared_dispatcher;

         // shared pointer to a dispatcher
         typedef shared_ptr<shared_dispatcher> shared_dispatcher_ptr;

         shared_dispatch_policy(DispatchPolicy&& policy = DispatchPolicy())
         :_shared_dispatcher_ptr(make_shared<shared_dispatcher>(forward<decltype(policy)>(policy)))
         {};

         /*
          * Dispatcher that makes a copy of the policy's dispatcher pointer
          * and dispatches through it indirectly
          */
         class dispatcher {
            public:
               dispatcher(shared_dispatch_policy<DispatchPolicy>& policy )
               :_shared_dispatcher_ptr(policy._shared_dispatcher_ptr)
               {}

               /**
                * Dispatch a job indirectly to a shared dispatcher
                *
                * @param proc - the procedure to dispatch
                * @tparam JobProc - callable type for void()
                */
               template<typename JobProc>
               void dispatch(JobProc&& proc) {
                  _shared_dispatcher_ptr->dispatch(proc);
               }

            private:
               shared_dispatcher_ptr _shared_dispatcher_ptr;
         };

      private:
         shared_dispatcher_ptr _shared_dispatcher_ptr;
   };
}}} // eosio::blockchain::internal