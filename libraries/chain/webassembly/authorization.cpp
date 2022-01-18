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

   bool interface::get_code_hash(
      account_name account,
      vm::argument_proxy<uint64_t*> code_sequence,
      vm::argument_proxy<fc::sha256*> code_hash,
      vm::argument_proxy<uint8_t*> vm_type,
      vm::argument_proxy<uint8_t*> vm_version
   ) const {
      return context.get_code_hash( account, code_sequence, code_hash, vm_type, vm_version );
   }
}}} // ns eosio::chain::webassembly
