#include "version_impl.hpp"

namespace eosio { namespace version {

   const std::string& version_client() {
      static const std::string version{_version_client()};
      return version;
   }

   const std::string& version_full() {
      static const std::string version{_version_full()};
      return version;
   }

} }
