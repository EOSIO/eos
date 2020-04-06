#pragma once

#include <fc/utility.hpp>
#include <tuple>

namespace eosio::trace_api {
   /**
    * A function used to separate cooperative or external concerns from long running tasks
    * calling code should expect that this can throw yield_exception and gracefully unwind if it does
    * @throws yield_exception if the provided yield needs to terminate the long running process for any reason
    */
   using yield_function = fc::optional_delegate<void()>;

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
   using exception_handler = fc::optional_delegate<void(const exception_with_context&)>;

   using log_handler = fc::optional_delegate<void(const std::string&)>;

   struct block_trace_v0;
   // optional block trace and irreversibility paired data
   using get_block_t = std::optional<std::tuple<block_trace_v0, bool>>;
   /**
    * Normal use case: exception_handler except_handler;
    *   except_handler( MAKE_EXCEPTION_WITH_CONTEXT( std::current_exception() ) );
    */
#define MAKE_EXCEPTION_WITH_CONTEXT(eptr) \
   (eosio::trace_api::exception_with_context((eptr),  __FILE__, __LINE__, __func__))


}