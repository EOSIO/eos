#pragma once
namespace eosio { namespace blockchain { namespace internal {

   /*
    * construct a child dispatcher from a DispatchPolicy using the default
    * constructor when not provided a value then share a pointer to that
    * dispatcher to all the created dispatchers
    */
   template<typename DispatchPolicy>
   class shared_dispatch_policy {
      public:
         // dependent type of the child dispatcher
         typedef typename DispatchPolicy::dispatcher shared_dispatcher;

         // shared pointer to a dispatcher
         typedef shared_ptr<shared_dispatcher> shared_dispatcher_ptr;

         shared_dispatch_policy(DispatchPolicy policy = DispatchPolicy())
            :_shared_dispatcher_ptr(std::make_shared<shared_dispatcher>(policy))
         {};

         /*
          * Make a copy of the shared pointer from the policy and dispatch
          * indirectly through it.
          */
         class dispatcher {
            public:
               dispatcher(shared_dispatch_policy<DispatchPolicy>& policy )
                  : _shared_dispatcher_ptr(policy._shared_dispatcher_ptr)
               {}

               template<typename JobProc>
               void dispatch(JobProc &&proc) {
                  _shared_dispatcher_ptr->dispatch(proc);
               }

            private:
               shared_dispatcher_ptr _shared_dispatcher_ptr;
         };

      private:
         shared_dispatcher_ptr _shared_dispatcher_ptr;
   };
}}}