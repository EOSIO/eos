#pragma once

//clashes with BOOST PP and Some Applications
#pragma push_macro("N")
#undef N

#include <boost/signals2.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace appbase {

   using erased_method_ptr = std::unique_ptr<void, void(*)(void*)>;

   /**
    * Basic DispatchPolicy that will try providers sequentially until one succeeds
    * without throwing an exception.  that result becomes the result of the method
    */
   template<typename FunctionSig>
   struct first_success_policy;

   template<typename Ret, typename ... Args>
   struct first_success_policy<Ret(Args...)> {
      using result_type = Ret;

      /**
       * Iterate through the providers, calling (dereferencing) each
       * if the provider throws, then store then try the next provider
       * if none succeed throw an error with the aggregated error descriptions
       *
       * @tparam InputIterator
       * @param first
       * @param last
       * @return
       */
      template<typename InputIterator>
      Ret operator()(InputIterator first, InputIterator last) {
         std::string err;
         while (first != last) {
            try {
               return *first; // de-referencing the iterator causes the provider to run
            } catch (...) {
               if (!err.empty()) {
                  err += "\",\"";
               }

               err += boost::current_exception_diagnostic_information();
            }

            ++first;
         }

         throw std::length_error(std::string("No Result Available, All providers returned exceptions[") + err + "]");
      }
   };

   template<typename ... Args>
   struct first_success_policy<void(Args...)> {
      using result_type = void;

      /**
       * Iterate through the providers, calling (dereferencing) each
       * if the provider throws, then store then try the next provider
       * if none succeed throw an error with the aggregated error descriptions
       *
       * @tparam InputIterator
       * @param first
       * @param last
       * @return
       */
      template<typename InputIterator>
      void operator()(InputIterator first, InputIterator last) {
         std::string err;

         while (first != last) {
            try {
               *first; // de-referencing the iterator causes the provider to run
            } catch (...) {
               if (!err.empty()) {
                  err += "\",\"";
               }

               err += boost::current_exception_diagnostic_information();
            }

            ++first;
         }

         throw std::length_error(std::string("No Result Available, All providers returned exceptions[") + err + "]");
      }
   };


   /**
    * Basic DispatchPolicy that will only call the first provider throwing or returning that providers results
    */
   template<typename FunctionSig>
   struct first_provider_policy;

   template<typename Ret, typename ... Args>
   struct first_provider_policy<Ret(Args...)> {
      using result_type = Ret;

      /**
       * Call the first provider as ordered by registered priority, return its result
       * throw its exceptions
       *
       * @tparam InputIterator
       * @param first
       * @param
       * @return
       */
      template<typename InputIterator>
      Ret operator()(InputIterator first, InputIterator) {
         return *first;
      }
   };

   template<typename ... Args>
   struct first_provider_policy<void(Args...)> {
      using result_type = void;

      /**
       * Call the first provider as ordered by registered priority, return its result
       * throw its exceptions
       *
       * @tparam InputIterator
       * @param first
       * @param
       * @return
       */
      template<typename InputIterator>
      void operator()(InputIterator first, InputIterator) {
         *first;
      }
   };

   namespace impl {
      template<typename FunctionSig, typename DispatchPolicy>
      class method_caller;

      template<typename Ret, typename ...Args, typename DispatchPolicy>
      class method_caller<Ret(Args...), DispatchPolicy> {
         public:
            using signal_type = boost::signals2::signal<Ret(Args...), DispatchPolicy>;
            using result_type = Ret;

            method_caller()
            {}

            /**
             * call operator from boost::signals2
             *
             * @throws exception depending on the DispatchPolicy
             */
            Ret operator()(Args&&... args)
            {
               return _signal(std::forward<Args>(args)...);
            }

            signal_type _signal;
      };

      template<typename ...Args, typename DispatchPolicy>
      class method_caller<void(Args...), DispatchPolicy> {
         public:
            using signal_type = boost::signals2::signal<void(Args...), DispatchPolicy>;
            using result_type = void;

            method_caller()
            {}

            /**
             * call operator from boost::signals2
             *
             * @throws exception depending on the DispatchPolicy
             */
            void operator()(Args&&... args)
            {
               _signal(std::forward<Args>(args)...);
            }

            signal_type _signal;
      };
   }

   /**
    * A method is a loosely linked application level function.
    * Callers can grab a method and call it
    * Providers can grab a method and register themselves
    *
    * This removes the need to tightly couple different plugins in the application.
    *
    * @tparam FunctionSig - the signature of the method (eg void(int, int))
    * @tparam DispatchPolicy - the policy for dispatching this method
    */
   template<typename FunctionSig, typename DispatchPolicy>
   class method final : public impl::method_caller<FunctionSig,DispatchPolicy> {
      public:
         /**
          * Type that represents a registered provider for a method allowing
          * for ownership via RAII and also explicit unregistered actions
          */
         class handle {
            public:
               ~handle() {
                  unregister();
               }

               /**
                * Explicitly unregister a provider for this channel
                * of this object expires
                */
               void unregister() {
                  if (_handle.connected()) {
                     _handle.disconnect();
                  }
               }

               // This handle can be constructed and moved
               handle() = default;
               handle(handle&&) = default;
               handle& operator= (handle&& rhs) = default;

               // dont allow copying since this protects the resource
               handle(const handle& ) = delete;
               handle& operator= (const handle& ) = delete;

            private:
               using handle_type = boost::signals2::connection;
               handle_type _handle;

               /**
                * Construct a handle from an internal represenation of a handle
                * In this case a boost::signals2::connection
                *
                * @param _handle - the boost::signals2::connection to wrap
                */
               handle(handle_type&& _handle)
               :_handle(std::move(_handle))
               {}

               friend class method;
         };

         /**
          * Register a provider of this method
          *
          * @tparam T - the type of the provider (functor, lambda)
          * @param provider - the provider
          * @param priority - the priority of this provider, lower is called before higher
          */
         template<typename T>
         handle register_provider(T provider, int priority = 0) {
            return handle(this->_signal.connect(priority, provider));
         }

      protected:
         method() = default;
         virtual ~method() = default;

         /**
          * Proper deleter for type-erased method
          * note: no type checking is performed at this level
          *
          * @param erased_method_ptr
          */
         static void deleter(void* erased_method_ptr) {
            auto ptr = reinterpret_cast<method*>(erased_method_ptr);
            delete ptr;
         }

         /**
          * get the method* back from an erased pointer
          *
          * @param ptr - the type-erased method pointer
          * @return - the type safe method pointer
          */
         static method* get_method(erased_method_ptr& ptr) {
            return reinterpret_cast<method*>(ptr.get());
         }

         /**
          * Construct a unique_ptr for the type erased method poiner
          * @return
          */
         static erased_method_ptr make_unique() {
            return erased_method_ptr(new method(), &deleter);
         }

         friend class appbase::application;
   };


   /**
    *
    * @tparam Tag - API specific discriminator used to distinguish between otherwise identical method signatures
    * @tparam FunctionSig - the signature of the method
    * @tparam DispatchPolicy - dispatch policy that dictates how providers for a method are accessed defaults to @ref first_success_policy
    */
   template< typename Tag, typename FunctionSig, template <typename> class DispatchPolicy = first_success_policy>
   struct method_decl {
      using method_type = method<FunctionSig, DispatchPolicy<FunctionSig>>;
      using tag_type = Tag;
   };

   template <typename Tag, typename FunctionSig, template <typename> class DispatchPolicy>
   std::true_type is_method_decl_impl(const method_decl<Tag, FunctionSig, DispatchPolicy>*);

   std::false_type is_method_decl_impl(...);

   template <typename T>
   using is_method_decl = decltype(is_method_decl_impl(std::declval<T*>()));


}

#pragma pop_macro("N")

