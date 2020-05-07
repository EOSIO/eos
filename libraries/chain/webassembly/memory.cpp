#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void* interface::memcpy( unvalidated_ptr<char> dest, unvalidated_ptr<const char> src, wasm_size_t length) const {
      volatile auto check_src = *((const char*)src + length - 1);
      volatile auto check_dest = *((const char*)dest + length - 1);
      volatile auto check_result = *(const char*)dest;
      vm::ignore_unused_variable_warning(check_src, check_dest, check_result);
      EOS_ASSERT((size_t)(std::abs((ptrdiff_t)(char*)dest - (ptrdiff_t)(const char*)src)) >= length,
            overlapping_memory_error, "memcpy can only accept non-aliasing pointers");
      return (char *)std::memcpy((char*)dest, (const char*)src, length);
   }

   void* interface::memmove( unvalidated_ptr<char> dest, unvalidated_ptr<const char> src, wasm_size_t length) const {
      volatile auto check_src = *((const char*)src + length - 1);
      volatile auto check_dest = *((const char*)dest + length - 1);
      volatile auto check_result = *(const char*)dest;
      vm::ignore_unused_variable_warning(check_src, check_dest, check_result);
      return (char *)std::memmove((char*)dest, (const char*)src, length);
   }

   int32_t interface::memcmp( unvalidated_ptr<const char> dest, unvalidated_ptr<const char> src, wasm_size_t length) const {
      volatile auto check_src = *((const char*)src + length - 1);
      volatile auto check_dest = *((const char*)dest + length - 1);
      vm::ignore_unused_variable_warning(check_src, check_dest);
      int32_t ret = std::memcmp((const char*)dest, (const char*)src, length);
      return ret < 0 ? -1 : ret > 0 ? 1 : 0;
   }

   void* interface::memset( unvalidated_ptr<char> dest, int32_t value, wasm_size_t length ) const {
      volatile auto check_dest = *((const char*)dest + length - 1);
      volatile auto check_result = *(const char*)dest;
      vm::ignore_unused_variable_warning(check_dest, check_result);
      return (char *)std::memset( (char*)dest, value, length );
   }

}}} // ns eosio::chain::webassembly
