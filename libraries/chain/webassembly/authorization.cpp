#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/apply_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::require_auth( account_name account ) const {
      context.require_authorization( account );
   }

   bool interface::has_auth( account_name account ) const {
      return context.has_authorization( account );
   }

   void interface::require_auth2( account_name account,
                       permission_name permission ) const {
      context.require_authorization( account, permission );
   }

   void interface::require_recipient( account_name recipient ) {
      context.require_recipient( recipient );
   }

   bool interface::is_account( account_name account ) const {
      return context.is_account( account );
   }

   struct get_code_hash_result {
      unsigned_int struct_version;
      uint64_t code_sequence;
      fc::sha256 code_hash;
      uint8_t vm_type;
      uint8_t vm_version;
   };

   uint32_t interface::get_code_hash(
      account_name account,
      uint32_t struct_version,
      vm::span<char> packed_result
   ) const {
      struct_version = std::min(uint32_t(0), struct_version);
      get_code_hash_result result = {struct_version};
      context.get_code_hash(account, result.code_sequence, result.code_hash, result.vm_type, result.vm_version);

      auto s = fc::raw::pack_size(result);
      if (s <= packed_result.size()) {
         datastream<char*> ds(packed_result.data(), s);
         fc::raw::pack(ds, result);
      }
      return s;
   }
}}} // ns eosio::chain::webassembly

FC_REFLECT(eosio::chain::webassembly::get_code_hash_result, (struct_version)(code_sequence)(code_hash)(vm_type)(vm_version))
