#pragma once

#include <tuple>

namespace eosio::trace_api_plugin {

   /**
    * A function used to separate cooperative or external concerns from long running tasks
    * calling code should expect that this can throw yield_exception and gracefully unwind if it does
    * @throws yield_exception if the provided yield needs to terminate the long running process for any reason
    */
   using yield_function = std::function<void()>;

   template<typename F, typename ...Args>
   void call_if_set(F&& f, Args&&  ...args ) {
      if (static_cast<bool>(f))
         std::forward<F>(f)(std::forward<Args>(args)...);
   }

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

#define MAKE_EXCEPTION_WITH_CONTEXT(eptr) \
   (eosio::trace_api_plugin::exception_with_context((eptr),  __FILE__, __LINE__, __func__))


}