#pragma once

#include <tuple>

namespace eosio::trace_api {
   template<typename R, typename ...Args>
   class optional_delegate : private std::function<R(Args...)> {
   public:
      using std::function<R(Args...)>::function;

      /**
       * overloaded call operator to ignore unset functions
       */
      template<typename U = R>
      auto operator()( Args... args ) const -> std::enable_if_t<!std::is_void_v<U>, R> {
         if (static_cast<bool>(*this)) {
            return std::function<R(Args...)>::operator()(std::move(args)...);
         } else {
            return {};
         }
      }

      template<typename U = R>
      auto operator()( Args... args ) const -> std::enable_if_t<std::is_void_v<U>> {
         if (static_cast<bool>(*this)) {
            std::function<R(Args...)>::operator()(std::move(args)...);
         }
      }
   };

   /**
    * A function used to separate cooperative or external concerns from long running tasks
    * calling code should expect that this can throw yield_exception and gracefully unwind if it does
    * @throws yield_exception if the provided yield needs to terminate the long running process for any reason
    */
   using yield_function = optional_delegate<void>;

   /**
    * Exceptions
    */
   class yield_exception : public std::runtime_error {
      public:
      explicit yield_exception(const char* what_arg)
      :std::runtime_error(what_arg)
      {}

      explicit yield_exception(const std::string& what_arg)
      :std::runtime_error(what_arg)
      {}
   };

   class bad_data_exception : public std::runtime_error {
   public:
      explicit bad_data_exception(const char* what_arg)
      :std::runtime_error(what_arg)
      {}

      explicit bad_data_exception(const std::string& what_arg)
      :std::runtime_error(what_arg)
      {}
   };

   using exception_with_context = std::tuple<const std::exception_ptr&, char const *, uint64_t, char const *>;
   using exception_handler = optional_delegate<void, const exception_with_context&>;

   /**
    * Normal use case: exception_handler except_handler;
    *   except_handler( MAKE_EXCEPTION_WITH_CONTEXT( std::current_exception() ) );
    */
#define MAKE_EXCEPTION_WITH_CONTEXT(eptr) \
   (eosio::trace_api::exception_with_context((eptr),  __FILE__, __LINE__, __func__))


}