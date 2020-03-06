#pragma once

#include <tuple>

namespace eosio::trace_api_plugin {

   using now_function = std::function<fc::time_point()>;

   /**
    * Exceptions
    */
   class deadline_exceeded : public std::runtime_error {
   public:
      explicit deadline_exceeded(const char* what_arg)
      :std::runtime_error(what_arg)
      {}
      explicit deadline_exceeded(const std::string& what_arg)
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