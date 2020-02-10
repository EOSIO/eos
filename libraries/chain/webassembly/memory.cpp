#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   char* interface::memcpy( legacy_array_ptr<char> dest, legacy_array_ptr<const char> src, uint32_t length) const {
      EOS_ASSERT((size_t)(std::abs((ptrdiff_t)dest.value - (ptrdiff_t)src.value)) >= length,
            overlapping_memory_error, "memcpy can only accept non-aliasing pointers");
      return (char *)std::memcpy(dest, src, length);
   }

   char* interface::memmove( legacy_array_ptr<char> dest, legacy_array_ptr<const char> src, uint32_t length) const {
      return (char *)std::memmove(dest, src, length);
   }

   int interface::memcmp( legacy_array_ptr<const char> dest, legacy_array_ptr<const char> src, uint32_t length) const {
      int ret = std::memcmp(dest, src, length);
      return ret < 0 ? -1 : ret > 0 ? 1 : 0;
   }

   char* interface::memset( legacy_array_ptr<char> dest, int32_t value, uint32_t length ) const {
      return (char *)std::memset( dest, value, length );
   }

}}} // ns eosio::chain::webassembly
