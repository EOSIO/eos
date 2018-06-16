/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eosio/chain/protocol.hpp>
#include <boost/core/typeinfo.hpp>


#define EOS_ASSERT( expr, exc_type, FORMAT, ... )                \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END

#define EOS_THROW( exc_type, FORMAT, ... ) \
    throw exc_type( FC_LOG_MESSAGE( error, FORMAT, __VA_ARGS__ ) );

/**
 * Macro inspired from FC_RETHROW_EXCEPTIONS
 * The main difference here is that if the exception caught isn't of type "eosio::chain::chain_exception"
 * This macro will rethrow the exception as the specified "exception_type"
 */
#define EOS_RETHROW_EXCEPTIONS(exception_type, FORMAT, ... ) \
   catch (eosio::chain::chain_exception& e) { \
      FC_RETHROW_EXCEPTION( e, warn, FORMAT, __VA_ARGS__ ); \
   } catch (fc::exception& e) { \
      exception_type new_exception(FC_LOG_MESSAGE( warn, FORMAT, __VA_ARGS__ )); \
      for (const auto& log: e.get_log()) { \
         new_exception.append_log(log); \
      } \
      throw new_exception; \
   } catch( const std::exception& e ) {  \
      exception_type fce(FC_LOG_MESSAGE( warn, FORMAT" (${what})" ,__VA_ARGS__("what",e.what()))); \
      throw fce;\
   } catch( ... ) {  \
      throw fc::unhandled_exception( \
                FC_LOG_MESSAGE( warn, FORMAT,__VA_ARGS__), \
                std::current_exception() ); \
   }

/**
 * Macro inspired from FC_CAPTURE_AND_RETHROW
 * The main difference here is that if the exception caught isn't of type "eosio::chain::chain_exception"
 * This macro will rethrow the exception as the specified "exception_type"
 */
#define EOS_CAPTURE_AND_RETHROW( exception_type, ... ) \
   catch (eosio::chain::chain_exception& e) { \
      FC_RETHROW_EXCEPTION( e, warn, "", FC_FORMAT_ARG_PARAMS(__VA_ARGS__) ); \
   } catch (fc::exception& e) { \
      exception_type new_exception(e.get_log()); \
      throw new_exception; \
   } catch( const std::exception& e ) {  \
      exception_type fce( \
                FC_LOG_MESSAGE( warn, "${what}: ",FC_FORMAT_ARG_PARAMS(__VA_ARGS__)("what",e.what())), \
                fc::std_exception_code,\
                BOOST_CORE_TYPEID(decltype(e)).name(), \
                e.what() ) ; throw fce;\
   } catch( ... ) {  \
      throw fc::unhandled_exception( \
                FC_LOG_MESSAGE( warn, "",FC_FORMAT_ARG_PARAMS(__VA_ARGS__)), \
                std::current_exception() ); \
   }

#define EOS_RECODE_EXC( cause_type, effect_type ) \
   catch( const cause_type& e ) \
   { throw( effect_type( e.what(), e.get_log() ) ); }


namespace eosio { namespace chain {

   FC_DECLARE_EXCEPTION( chain_exception,
                         3000000, "blockchain exception" )
   /**
    *  chain_exception
    *   |- chain_type_exception
    *   |- fork_database_exception
    *   |- block_validate_exception
    *   |- transaction_exception
    *   |- action_validate_exception
    *   |- database_exception
    *   |- wasm_exception
    *   |- resource_exhausted_exception
    *   |- misc_exception
    *   |- missing_plugin_exception
    *   |- wallet_exception
    */

    FC_DECLARE_DERIVED_EXCEPTION( chain_type_exception, chain_exception,
                                  3010000, "chain type exception" )

      FC_DECLARE_DERIVED_EXCEPTION( name_type_exception,               chain_type_exception,
                                    3010001, "Invalid name" )
      FC_DECLARE_DERIVED_EXCEPTION( public_key_type_exception,         chain_type_exception,
                                    3010002, "Invalid public key" )
      FC_DECLARE_DERIVED_EXCEPTION( private_key_type_exception,        chain_type_exception,
                                    3010003, "Invalid private key" )
      FC_DECLARE_DERIVED_EXCEPTION( authority_type_exception,          chain_type_exception,
                                    3010004, "Invalid authority" )
      FC_DECLARE_DERIVED_EXCEPTION( action_type_exception,             chain_type_exception,
                                    3010005, "Invalid action" )
      FC_DECLARE_DERIVED_EXCEPTION( transaction_type_exception,        chain_type_exception,
                                    3010006, "Invalid transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( abi_type_exception,                chain_type_exception,
                                    3010007, "Invalid ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( abi_not_found_exception,           chain_type_exception,
                                    3010008, "No ABI found" )
      FC_DECLARE_DERIVED_EXCEPTION( block_id_type_exception,           chain_type_exception,
                                    3010009, "Invalid block ID" )
      FC_DECLARE_DERIVED_EXCEPTION( transaction_id_type_exception,     chain_type_exception,
                                    3010010, "Invalid transaction ID" )
      FC_DECLARE_DERIVED_EXCEPTION( packed_transaction_type_exception, chain_type_exception,
                                    3010011, "Invalid packed transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( asset_type_exception,              chain_type_exception,
                                    3010012, "Invalid asset" )


   FC_DECLARE_DERIVED_EXCEPTION( fork_database_exception, chain_exception,
                                 3020000, "fork database exception" )

      FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception, chain_exception,
                                    3020001, "unlinkable block" )


   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception, chain_exception,
                                 3030000, "block exception" )

      FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,   block_validate_exception,
                                    3030001, "transaction outputs in block do not match transaction outputs from applying block" )
      FC_DECLARE_DERIVED_EXCEPTION( block_concurrency_exception, block_validate_exception,
                                    3030002, "block does not guarantee concurrent execution without conflicts" )
      FC_DECLARE_DERIVED_EXCEPTION( block_lock_exception,        block_validate_exception,
                                    3030003, "shard locks in block are incorrect or mal-formed" )
      FC_DECLARE_DERIVED_EXCEPTION( block_resource_exhausted,    block_validate_exception,
                                    3030004, "block exhausted allowed resources" )
      FC_DECLARE_DERIVED_EXCEPTION( block_too_old_exception,     block_validate_exception,
                                    3030005, "block is too old to push" )


   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             chain_exception,
                                 3040000, "transaction exception" )

      FC_DECLARE_DERIVED_EXCEPTION( tx_decompression_error,      transaction_exception,
                                    3040001, "Error decompressing transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_no_action,                transaction_exception,
                                    3040002, "transaction should have at least one normal action" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_no_auths,                 transaction_exception,
                                    3040003, "transaction should have at least one required authority" )
      FC_DECLARE_DERIVED_EXCEPTION( cfa_irrelevant_auth,         transaction_exception,
                                    3040004, "context-free action should have no required authority" )
      FC_DECLARE_DERIVED_EXCEPTION( expired_tx_exception,        transaction_exception,
                                    3040005, "Expired Transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_exp_too_far_exception,    transaction_exception,
                                    3040006, "Transaction Expiration Too Far" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_ref_block_exception, transaction_exception,
                                    3040007, "Invalid Reference Block" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                transaction_exception,
                                    3040008, "duplicate transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( deferred_tx_duplicate,       transaction_exception,
                                    3040009, "duplicate deferred transaction" )


   FC_DECLARE_DERIVED_EXCEPTION( action_validate_exception, chain_exception,
                                 3050000, "action exception" )

      FC_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception, action_validate_exception,
                                    3050001, "account name already exists" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_action_args_exception, action_validate_exception,
                                    3050002, "Invalid Action Arguments" )
      FC_DECLARE_DERIVED_EXCEPTION( eosio_assert_message_exception, action_validate_exception,
                                    3050003, "eosio_assert_message assertion failure" )
      FC_DECLARE_DERIVED_EXCEPTION( eosio_assert_code_exception, action_validate_exception,
                                    3050004, "eosio_assert_code assertion failure" )

   FC_DECLARE_DERIVED_EXCEPTION( database_exception, chain_exception,
                                 3060000, "database exception" )

      FC_DECLARE_DERIVED_EXCEPTION( permission_query_exception,     database_exception,
                                    3060001, "Permission Query Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( account_query_exception,        database_exception,
                                    3060002, "Account Query Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_table_query_exception, database_exception,
                                    3060003, "Contract Table Query Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_query_exception,       database_exception,
                                    3060004, "Contract Query Exception" )


   FC_DECLARE_DERIVED_EXCEPTION( wasm_exception, chain_exception,
                                 3070000, "WASM Exception" )

      FC_DECLARE_DERIVED_EXCEPTION( page_memory_error,        wasm_exception,
                                    3070001, "error in WASM page memory" )

      FC_DECLARE_DERIVED_EXCEPTION( wasm_execution_error,     wasm_exception,
                                    3070002, "Runtime Error Processing WASM" )

      FC_DECLARE_DERIVED_EXCEPTION( wasm_serialization_error, wasm_exception,
                                    3070003, "Serialization Error Processing WASM" )

      FC_DECLARE_DERIVED_EXCEPTION( overlapping_memory_error, wasm_exception,
                                    3070004, "memcpy with overlapping memory" )



   FC_DECLARE_DERIVED_EXCEPTION( resource_exhausted_exception, chain_exception,
                                 3080000, "resource exhausted exception" )

      FC_DECLARE_DERIVED_EXCEPTION( ram_usage_exceeded, resource_exhausted_exception,
                                    3080001, "account using more than allotted RAM usage" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_net_usage_exceeded, resource_exhausted_exception,
                                    3080002, "transaction exceeded the current network usage limit imposed on the transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( block_net_usage_exceeded, resource_exhausted_exception,
                                    3080003, "transaction network usage is too much for the remaining allowable usage of the current block" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_cpu_usage_exceeded, resource_exhausted_exception,
                                    3080004, "transaction exceeded the current CPU usage limit imposed on the transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( block_cpu_usage_exceeded, resource_exhausted_exception,
                                    3080005, "transaction CPU usage is too much for the remaining allowable usage of the current block" )
      FC_DECLARE_DERIVED_EXCEPTION( deadline_exception, resource_exhausted_exception,
                                    3080006, "transaction took too long" )
      FC_DECLARE_DERIVED_EXCEPTION( leeway_deadline_exception, deadline_exception,
                                    3081001, "transaction reached the deadline set due to leeway on account CPU limits" )

   FC_DECLARE_DERIVED_EXCEPTION( authorization_exception, chain_exception,
                                 3090000, "Authorization exception" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,             authorization_exception,
                                    3090001, "duplicate signature included" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,            authorization_exception,
                                    3090002, "irrelevant signature included" )
      FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_authorization,    authorization_exception,
                                    3090003, "provided keys, permissions, and delays do not satisfy declared authorizations" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_auth_exception,       authorization_exception,
                                    3090004, "missing required authority" )
      FC_DECLARE_DERIVED_EXCEPTION( irrelevant_auth_exception,    authorization_exception,
                                    3090005, "irrelevant authority included" )
      FC_DECLARE_DERIVED_EXCEPTION( insufficient_delay_exception, authorization_exception,
                                    3090006, "insufficient delay" )

   FC_DECLARE_DERIVED_EXCEPTION( misc_exception, chain_exception,
                                 3100000, "Miscellaneous exception" )

      FC_DECLARE_DERIVED_EXCEPTION( rate_limiting_state_inconsistent, misc_exception,
                                    3100001, "internal state is no longer consistent" )
      FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,          misc_exception,
                                    3100002, "unknown block" )
      FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,    misc_exception,
                                    3100003, "unknown transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( fixed_reversible_db_exception,    misc_exception,
                                    3100004, "corrupted reversible block database was fixed" )
      FC_DECLARE_DERIVED_EXCEPTION( extract_genesis_state_exception,    misc_exception,
                                    3100005, "extracted genesis state from blocks.log" )

   FC_DECLARE_DERIVED_EXCEPTION( missing_plugin_exception, chain_exception,
                                 3110000, "missing plugin exception" )

      FC_DECLARE_DERIVED_EXCEPTION( missing_chain_api_plugin_exception,           missing_plugin_exception,
                                    3110001, "Missing Chain API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_wallet_api_plugin_exception,          missing_plugin_exception,
                                    3110002, "Missing Wallet API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_history_api_plugin_exception, missing_plugin_exception,
                                    3110003, "Missing History API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_net_api_plugin_exception,             missing_plugin_exception,
                                    3110004, "Missing Net API Plugin" )


   FC_DECLARE_DERIVED_EXCEPTION( wallet_exception, chain_exception,
                                 3120000, "wallet exception" )

      FC_DECLARE_DERIVED_EXCEPTION( wallet_exist_exception,            wallet_exception,
                                    3120001, "Wallet already exists" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_nonexistent_exception,      wallet_exception,
                                    3120002, "Nonexistent wallet" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_locked_exception,           wallet_exception,
                                    3120003, "Locked wallet" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_missing_pub_key_exception,  wallet_exception,
                                    3120004, "Missing public key" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_invalid_password_exception, wallet_exception,
                                    3120005, "Invalid wallet password" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_not_available_exception,    wallet_exception,
                                    3120006, "No available wallet" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_unlocked_exception,         wallet_exception,
                                    3120007, "Already unlocked" )

   FC_DECLARE_DERIVED_EXCEPTION( whitelist_blacklist_exception,   chain_exception,
                                 3130000, "actor or contract whitelist/blacklist exception" )

      FC_DECLARE_DERIVED_EXCEPTION( actor_whitelist_exception,    chain_exception,
                                    3130001, "Authorizing actor of transaction is not on the whitelist" )
      FC_DECLARE_DERIVED_EXCEPTION( actor_blacklist_exception,    chain_exception,
                                    3130002, "Authorizing actor of transaction is on the blacklist" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_whitelist_exception, chain_exception,
                                    3130003, "Contract to execute is not on the whitelist" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_blacklist_exception, chain_exception,
                                    3130004, "Contract to execute is on the blacklist" )
      FC_DECLARE_DERIVED_EXCEPTION( action_blacklist_exception, chain_exception,
                                    3130005, "Action to execute is on the blacklist" )
      FC_DECLARE_DERIVED_EXCEPTION( key_blacklist_exception, chain_exception,
                                    3130006, "Public key in authority is on the blacklist" )

} } // eosio::chain
