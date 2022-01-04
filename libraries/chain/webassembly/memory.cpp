#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void* interface::memcpy( memcpy_params args ) const {
      auto [dest, src, length] = args;
      EOS_ASSERT((size_t)(std::abs((ptrdiff_t)(char*)dest - (ptrdiff_t)(const char*)src)) >= length,
            overlapping_memory_error, "memcpy can only accept non-aliasing pointers");
      return (char *)std::memcpy((char*)dest, (const char*)src, length);
   }

   void* interface::memmove( memcpy_params args ) const {
      auto [dest, src, length] = args;
      return (char *)std::memmove((char*)dest, (const char*)src, length);
   }

   int32_t interface::memcmp( memcmp_params args ) const {
      auto [dest, src, length] = args;
      int32_t ret = std::memcmp((const char*)dest, (const char*)src, length);
      return ret < 0 ? -1 : ret > 0 ? 1 : 0;
   }

   void* interface::memset( memset_params args ) const {
      auto [dest, value, length] = args;
      return (char *)std::memset( (char*)dest, value, length );
   }

}}} // ns eosio::chain::webassembly
