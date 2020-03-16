#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::abort() {
      EOS_ASSERT( false, abort_called, "abort() called" );
   }

   void interface::eosio_assert( bool condition, null_terminated_ptr msg ) {
      if( BOOST_UNLIKELY( !condition ) ) {
         const size_t sz = strnlen( msg, max_assert_message );
         std::string message( msg, sz );
         EOS_THROW( eosio_assert_message_exception, "assertion failure with message: ${s}", ("s",message) );
      }
   }

   void eosio_assert_message( bool condition, legacy_array_ptr<const char> msg, uint32_t msg_len ) {
      if( BOOST_UNLIKELY( !condition ) ) {
         const size_t sz = msg_len > max_assert_message ? max_assert_message : msg_len;
         std::string message( msg, sz );
         EOS_THROW( eosio_assert_message_exception, "assertion failure with message: ${s}", ("s",message) );
      }
   }

   void eosio_assert_code( bool condition, uint64_t error_code ) {
      if( BOOST_UNLIKELY( !condition ) ) {
         if( error_code >= static_cast<uint64_t>(system_error_code::generic_system_error) ) {
            restricted_error_code_exception e( FC_LOG_MESSAGE(
                                                   error,
                                                   "eosio_assert_code called with reserved error code: ${error_code}",
                                                   ("error_code", error_code)
            ) );
            e.error_code = static_cast<uint64_t>(system_error_code::contract_restricted_error_code);
            throw e;
         } else {
            eosio_assert_code_exception e( FC_LOG_MESSAGE(
                                             error,
                                             "assertion failure with error code: ${error_code}",
                                             ("error_code", error_code)
            ) );
            e.error_code = error_code;
            throw e;
         }
      }
   }

   void interface::eosio_exit( int32_t code ) {
      ctx.control.get_wasm_interface().exit();
   }
}}} // ns eosio::chain::webassembly
