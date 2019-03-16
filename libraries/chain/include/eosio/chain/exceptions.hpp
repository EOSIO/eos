/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <fc/exception/exception.hpp>
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
    *   |- plugin_exception
    *   |- wallet_exception
    *   |- whitelist_blacklist_exception
    *   |- controller_emit_signal_exception
    *   |- abi_exception
    *   |- contract_exception
    *   |- producer_exception
    *   |- reversible_blocks_exception
    *   |- block_log_exception
    *   |- resource_limit_exception
    *   |- mongo_db_exception
    *   |- contract_api_exception
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
      FC_DECLARE_DERIVED_EXCEPTION( block_id_type_exception,           chain_type_exception,
                                    3010008, "Invalid block ID" )
      FC_DECLARE_DERIVED_EXCEPTION( transaction_id_type_exception,     chain_type_exception,
                                    3010009, "Invalid transaction ID" )
      FC_DECLARE_DERIVED_EXCEPTION( packed_transaction_type_exception, chain_type_exception,
                                    3010010, "Invalid packed transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( asset_type_exception,              chain_type_exception,
                                    3010011, "Invalid asset" )
      FC_DECLARE_DERIVED_EXCEPTION( chain_id_type_exception,           chain_type_exception,
                                    3010012, "Invalid chain ID" )
      FC_DECLARE_DERIVED_EXCEPTION( fixed_key_type_exception,           chain_type_exception,
                                    3010013, "Invalid fixed key" )
      FC_DECLARE_DERIVED_EXCEPTION( symbol_type_exception,           chain_type_exception,
                                    3010014, "Invalid symbol" )


   FC_DECLARE_DERIVED_EXCEPTION( fork_database_exception, chain_exception,
                                 3020000, "Fork database exception" )

      FC_DECLARE_DERIVED_EXCEPTION( fork_db_block_not_found, fork_database_exception,
                                    3020001, "Block can not be found" )


   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception, chain_exception,
                                 3030000, "Block exception" )

      FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception, block_validate_exception,
                                    3030001, "Unlinkable block" )
      FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,   block_validate_exception,
                                    3030002, "Transaction outputs in block do not match transaction outputs from applying block" )
      FC_DECLARE_DERIVED_EXCEPTION( block_concurrency_exception, block_validate_exception,
                                    3030003, "Block does not guarantee concurrent execution without conflicts" )
      FC_DECLARE_DERIVED_EXCEPTION( block_lock_exception,        block_validate_exception,
                                    3030004, "Shard locks in block are incorrect or mal-formed" )
      FC_DECLARE_DERIVED_EXCEPTION( block_resource_exhausted,    block_validate_exception,
                                    3030005, "Block exhausted allowed resources" )
      FC_DECLARE_DERIVED_EXCEPTION( block_too_old_exception,     block_validate_exception,
                                    3030006, "Block is too old to push" )
      FC_DECLARE_DERIVED_EXCEPTION( block_from_the_future,       block_validate_exception,
                                    3030007, "Block is from the future" )
      FC_DECLARE_DERIVED_EXCEPTION( wrong_signing_key,           block_validate_exception,
                                    3030008, "Block is not signed with expected key" )
      FC_DECLARE_DERIVED_EXCEPTION( wrong_producer,              block_validate_exception,
                                    3030009, "Block is not signed by expected producer" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_block_header_extension, block_validate_exception,
                                    3030010, "Invalid block header extension" )
      FC_DECLARE_DERIVED_EXCEPTION( ill_formed_protocol_feature_activation, block_validate_exception,
                                    3030011, "Block includes an ill-formed protocol feature activation extension" )



   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             chain_exception,
                                 3040000, "Transaction exception" )

      FC_DECLARE_DERIVED_EXCEPTION( tx_decompression_error,      transaction_exception,
                                    3040001, "Error decompressing transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_no_action,                transaction_exception,
                                    3040002, "Transaction should have at least one normal action" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_no_auths,                 transaction_exception,
                                    3040003, "Transaction should have at least one required authority" )
      FC_DECLARE_DERIVED_EXCEPTION( cfa_irrelevant_auth,         transaction_exception,
                                    3040004, "Context-free action should have no required authority" )
      FC_DECLARE_DERIVED_EXCEPTION( expired_tx_exception,        transaction_exception,
                                    3040005, "Expired Transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_exp_too_far_exception,    transaction_exception,
                                    3040006, "Transaction Expiration Too Far" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_ref_block_exception, transaction_exception,
                                    3040007, "Invalid Reference Block" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                transaction_exception,
                                    3040008, "Duplicate transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( deferred_tx_duplicate,       transaction_exception,
                                    3040009, "Duplicate deferred transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( cfa_inside_generated_tx,     transaction_exception,
                                    3040010, "Context free action is not allowed inside generated transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_not_found,     transaction_exception,
                                    3040011, "The transaction can not be found" )
      FC_DECLARE_DERIVED_EXCEPTION( too_many_tx_at_once,          transaction_exception,
                                    3040012, "Pushing too many transactions at once" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_too_big,                   transaction_exception,
                                    3040013, "Transaction is too big" )
      FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_compression, transaction_exception,
                                    3040014, "Unknown transaction compression" )


   FC_DECLARE_DERIVED_EXCEPTION( action_validate_exception, chain_exception,
                                 3050000, "Action validate exception" )

      FC_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception, action_validate_exception,
                                    3050001, "Account name already exists" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_action_args_exception, action_validate_exception,
                                    3050002, "Invalid Action Arguments" )
      FC_DECLARE_DERIVED_EXCEPTION( eosio_assert_message_exception, action_validate_exception,
                                    3050003, "eosio_assert_message assertion failure" )
      FC_DECLARE_DERIVED_EXCEPTION( eosio_assert_code_exception, action_validate_exception,
                                    3050004, "eosio_assert_code assertion failure" )
      FC_DECLARE_DERIVED_EXCEPTION( action_not_found_exception, action_validate_exception,
                                    3050005, "Action can not be found" )
      FC_DECLARE_DERIVED_EXCEPTION( action_data_and_struct_mismatch, action_validate_exception,
                                    3050006, "Mismatch between action data and its struct" )
      FC_DECLARE_DERIVED_EXCEPTION( unaccessible_api, action_validate_exception,
                                    3050007, "Attempt to use unaccessible API" )
      FC_DECLARE_DERIVED_EXCEPTION( abort_called, action_validate_exception,
                                    3050008, "Abort Called" )
      FC_DECLARE_DERIVED_EXCEPTION( inline_action_too_big, action_validate_exception,
                                    3050009, "Inline Action exceeds maximum size limit" )

   FC_DECLARE_DERIVED_EXCEPTION( database_exception, chain_exception,
                                 3060000, "Database exception" )

      FC_DECLARE_DERIVED_EXCEPTION( permission_query_exception,     database_exception,
                                    3060001, "Permission Query Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( account_query_exception,        database_exception,
                                    3060002, "Account Query Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_table_query_exception, database_exception,
                                    3060003, "Contract Table Query Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_query_exception,       database_exception,
                                    3060004, "Contract Query Exception" )

   FC_DECLARE_DERIVED_EXCEPTION( guard_exception, database_exception,
                                 3060100, "Guard Exception" )

      FC_DECLARE_DERIVED_EXCEPTION( database_guard_exception, guard_exception,
                                    3060101, "Database usage is at unsafe levels" )
      FC_DECLARE_DERIVED_EXCEPTION( reversible_guard_exception, guard_exception,
                                    3060102, "Reversible block log usage is at unsafe levels" )

   FC_DECLARE_DERIVED_EXCEPTION( wasm_exception, chain_exception,
                                 3070000, "WASM Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( page_memory_error,        wasm_exception,
                                    3070001, "Error in WASM page memory" )
      FC_DECLARE_DERIVED_EXCEPTION( wasm_execution_error,     wasm_exception,
                                    3070002, "Runtime Error Processing WASM" )
      FC_DECLARE_DERIVED_EXCEPTION( wasm_serialization_error, wasm_exception,
                                    3070003, "Serialization Error Processing WASM" )
      FC_DECLARE_DERIVED_EXCEPTION( overlapping_memory_error, wasm_exception,
                                    3070004, "memcpy with overlapping memory" )
      FC_DECLARE_DERIVED_EXCEPTION( binaryen_exception, wasm_exception,
                                    3070005, "binaryen exception" )


   FC_DECLARE_DERIVED_EXCEPTION( resource_exhausted_exception, chain_exception,
                                 3080000, "Resource exhausted exception" )

      FC_DECLARE_DERIVED_EXCEPTION( ram_usage_exceeded, resource_exhausted_exception,
                                    3080001, "Account using more than allotted RAM usage" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_net_usage_exceeded, resource_exhausted_exception,
                                    3080002, "Transaction exceeded the current network usage limit imposed on the transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( block_net_usage_exceeded, resource_exhausted_exception,
                                    3080003, "Transaction network usage is too much for the remaining allowable usage of the current block" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_cpu_usage_exceeded, resource_exhausted_exception,
                                    3080004, "Transaction exceeded the current CPU usage limit imposed on the transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( block_cpu_usage_exceeded, resource_exhausted_exception,
                                    3080005, "Transaction CPU usage is too much for the remaining allowable usage of the current block" )
      FC_DECLARE_DERIVED_EXCEPTION( deadline_exception, resource_exhausted_exception,
                                    3080006, "Transaction took too long" )
      FC_DECLARE_DERIVED_EXCEPTION( greylist_net_usage_exceeded, resource_exhausted_exception,
                                    3080007, "Transaction exceeded the current greylisted account network usage limit" )
      FC_DECLARE_DERIVED_EXCEPTION( greylist_cpu_usage_exceeded, resource_exhausted_exception,
                                    3080008, "Transaction exceeded the current greylisted account CPU usage limit" )
      FC_DECLARE_DERIVED_EXCEPTION( leeway_deadline_exception, deadline_exception,
                                    3081001, "Transaction reached the deadline set due to leeway on account CPU limits" )

   FC_DECLARE_DERIVED_EXCEPTION( authorization_exception, chain_exception,
                                 3090000, "Authorization exception" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,             authorization_exception,
                                    3090001, "Duplicate signature included" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,            authorization_exception,
                                    3090002, "Irrelevant signature included" )
      FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_authorization,    authorization_exception,
                                    3090003, "Provided keys, permissions, and delays do not satisfy declared authorizations" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_auth_exception,       authorization_exception,
                                    3090004, "Missing required authority" )
      FC_DECLARE_DERIVED_EXCEPTION( irrelevant_auth_exception,    authorization_exception,
                                    3090005, "Irrelevant authority included" )
      FC_DECLARE_DERIVED_EXCEPTION( insufficient_delay_exception, authorization_exception,
                                    3090006, "Insufficient delay" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_permission,           authorization_exception,
                                    3090007, "Invalid Permission" )
      FC_DECLARE_DERIVED_EXCEPTION( unlinkable_min_permission_action, authorization_exception,
                                    3090008, "The action is not allowed to be linked with minimum permission" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_parent_permission,           authorization_exception,
                                    3090009, "The parent permission is invalid" )

   FC_DECLARE_DERIVED_EXCEPTION( misc_exception, chain_exception,
                                 3100000, "Miscellaneous exception" )

      FC_DECLARE_DERIVED_EXCEPTION( rate_limiting_state_inconsistent,       misc_exception,
                                    3100001, "Internal state is no longer consistent" )
      FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,                misc_exception,
                                    3100002, "Unknown block" )
      FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,          misc_exception,
                                    3100003, "Unknown transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( fixed_reversible_db_exception,          misc_exception,
                                    3100004, "Corrupted reversible block database was fixed" )
      FC_DECLARE_DERIVED_EXCEPTION( extract_genesis_state_exception,        misc_exception,
                                    3100005, "Extracted genesis state from blocks.log" )
      FC_DECLARE_DERIVED_EXCEPTION( subjective_block_production_exception,  misc_exception,
                                    3100006, "Subjective exception thrown during block production" )
      FC_DECLARE_DERIVED_EXCEPTION( multiple_voter_info,                    misc_exception,
                                    3100007, "Multiple voter info detected" )
      FC_DECLARE_DERIVED_EXCEPTION( unsupported_feature,                    misc_exception,
                                    3100008, "Feature is currently unsupported" )
      FC_DECLARE_DERIVED_EXCEPTION( node_management_success,                misc_exception,
                                    3100009, "Node management operation successfully executed" )



   FC_DECLARE_DERIVED_EXCEPTION( plugin_exception, chain_exception,
                                 3110000, "Plugin exception" )

      FC_DECLARE_DERIVED_EXCEPTION( missing_chain_api_plugin_exception,           plugin_exception,
                                    3110001, "Missing Chain API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_wallet_api_plugin_exception,          plugin_exception,
                                    3110002, "Missing Wallet API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_history_api_plugin_exception,         plugin_exception,
                                    3110003, "Missing History API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_net_api_plugin_exception,             plugin_exception,
                                    3110004, "Missing Net API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_chain_plugin_exception,               plugin_exception,
                                    3110005, "Missing Chain Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( plugin_config_exception,               plugin_exception,
                                    3110006, "Incorrect plugin configuration" )


   FC_DECLARE_DERIVED_EXCEPTION( wallet_exception, chain_exception,
                                 3120000, "Wallet exception" )

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
      FC_DECLARE_DERIVED_EXCEPTION( key_exist_exception,               wallet_exception,
                                    3120008, "Key already exists" )
      FC_DECLARE_DERIVED_EXCEPTION( key_nonexistent_exception,         wallet_exception,
                                    3120009, "Nonexistent key" )
      FC_DECLARE_DERIVED_EXCEPTION( unsupported_key_type_exception,    wallet_exception,
                                    3120010, "Unsupported key type" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_lock_timeout_exception,    wallet_exception,
                                    3120011, "Wallet lock timeout is invalid" )
      FC_DECLARE_DERIVED_EXCEPTION( secure_enclave_exception,          wallet_exception,
                                    3120012, "Secure Enclave Exception" )


   FC_DECLARE_DERIVED_EXCEPTION( whitelist_blacklist_exception,   chain_exception,
                                 3130000, "Actor or contract whitelist/blacklist exception" )

      FC_DECLARE_DERIVED_EXCEPTION( actor_whitelist_exception,    whitelist_blacklist_exception,
                                    3130001, "Authorizing actor of transaction is not on the whitelist" )
      FC_DECLARE_DERIVED_EXCEPTION( actor_blacklist_exception,    whitelist_blacklist_exception,
                                    3130002, "Authorizing actor of transaction is on the blacklist" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_whitelist_exception, whitelist_blacklist_exception,
                                    3130003, "Contract to execute is not on the whitelist" )
      FC_DECLARE_DERIVED_EXCEPTION( contract_blacklist_exception, whitelist_blacklist_exception,
                                    3130004, "Contract to execute is on the blacklist" )
      FC_DECLARE_DERIVED_EXCEPTION( action_blacklist_exception,   whitelist_blacklist_exception,
                                    3130005, "Action to execute is on the blacklist" )
      FC_DECLARE_DERIVED_EXCEPTION( key_blacklist_exception,      whitelist_blacklist_exception,
                                    3130006, "Public key in authority is on the blacklist" )

   FC_DECLARE_DERIVED_EXCEPTION( controller_emit_signal_exception, chain_exception,
                                 3140000, "Exceptions that are allowed to bubble out of emit calls in controller" )
      FC_DECLARE_DERIVED_EXCEPTION( checkpoint_exception,          controller_emit_signal_exception,
                                   3140001, "Block does not match checkpoint" )


   FC_DECLARE_DERIVED_EXCEPTION( abi_exception,                           chain_exception,
                                 3015000, "ABI exception" )
      FC_DECLARE_DERIVED_EXCEPTION( abi_not_found_exception,              abi_exception,
                                    3015001, "No ABI found" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_ricardian_clause_exception,   abi_exception,
                                    3015002, "Invalid Ricardian Clause" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_ricardian_action_exception,   abi_exception,
                                    3015003, "Invalid Ricardian Action" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_type_inside_abi,           abi_exception,
                                    3015004, "The type defined in the ABI is invalid" ) // Not to be confused with abi_type_exception
      FC_DECLARE_DERIVED_EXCEPTION( duplicate_abi_type_def_exception,     abi_exception,
                                    3015005, "Duplicate type definition in the ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( duplicate_abi_struct_def_exception,   abi_exception,
                                    3015006, "Duplicate struct definition in the ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( duplicate_abi_action_def_exception,   abi_exception,
                                    3015007, "Duplicate action definition in the ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( duplicate_abi_table_def_exception,    abi_exception,
                                    3015008, "Duplicate table definition in the ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( duplicate_abi_err_msg_def_exception,  abi_exception,
                                    3015009, "Duplicate error message definition in the ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( abi_serialization_deadline_exception, abi_exception,
                                    3015010, "ABI serialization time has exceeded the deadline" )
      FC_DECLARE_DERIVED_EXCEPTION( abi_recursion_depth_exception,        abi_exception,
                                    3015011, "ABI recursive definition has exceeded the max recursion depth" )
      FC_DECLARE_DERIVED_EXCEPTION( abi_circular_def_exception,           abi_exception,
                                    3015012, "Circular definition is detected in the ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( unpack_exception,                     abi_exception,
                                    3015013, "Unpack data exception" )
      FC_DECLARE_DERIVED_EXCEPTION( pack_exception,                     abi_exception,
                                    3015014, "Pack data exception" )
      FC_DECLARE_DERIVED_EXCEPTION( duplicate_abi_variant_def_exception,  abi_exception,
                                    3015015, "Duplicate variant definition in the ABI" )
      FC_DECLARE_DERIVED_EXCEPTION( unsupported_abi_version_exception,  abi_exception,
                                    3015016, "ABI has an unsupported version" )

   FC_DECLARE_DERIVED_EXCEPTION( contract_exception,           chain_exception,
                                 3160000, "Contract exception" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_table_payer,             contract_exception,
                                    3160001, "The payer of the table data is invalid" )
      FC_DECLARE_DERIVED_EXCEPTION( table_access_violation,          contract_exception,
                                    3160002, "Table access violation" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_table_iterator,          contract_exception,
                                    3160003, "Invalid table iterator" )
      FC_DECLARE_DERIVED_EXCEPTION( table_not_in_cache,          contract_exception,
                                    3160004, "Table can not be found inside the cache" )
      FC_DECLARE_DERIVED_EXCEPTION( table_operation_not_permitted,          contract_exception,
                                    3160005, "The table operation is not allowed" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_contract_vm_type,          contract_exception,
                                    3160006, "Invalid contract vm type" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_contract_vm_version,          contract_exception,
                                    3160007, "Invalid contract vm version" )
      FC_DECLARE_DERIVED_EXCEPTION( set_exact_code,          contract_exception,
                                    3160008, "Contract is already running this version of code" )
      FC_DECLARE_DERIVED_EXCEPTION( wasm_file_not_found,          contract_exception,
                                    3160009, "No wasm file found" )
      FC_DECLARE_DERIVED_EXCEPTION( abi_file_not_found,          contract_exception,
                                    3160010, "No abi file found" )

   FC_DECLARE_DERIVED_EXCEPTION( producer_exception,           chain_exception,
                                 3170000, "Producer exception" )
      FC_DECLARE_DERIVED_EXCEPTION( producer_priv_key_not_found,   producer_exception,
                                    3170001, "Producer private key is not available" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_pending_block_state,   producer_exception,
                                    3170002, "Pending block state is missing" )
      FC_DECLARE_DERIVED_EXCEPTION( producer_double_confirm,       producer_exception,
                                    3170003, "Producer is double confirming known range" )
      FC_DECLARE_DERIVED_EXCEPTION( producer_schedule_exception,   producer_exception,
                                    3170004, "Producer schedule exception" )
      FC_DECLARE_DERIVED_EXCEPTION( producer_not_in_schedule,      producer_exception,
                                    3170006, "The producer is not part of current schedule" )
      FC_DECLARE_DERIVED_EXCEPTION( snapshot_directory_not_found_exception,  producer_exception,
                                    3170007, "The configured snapshot directory does not exist" )
      FC_DECLARE_DERIVED_EXCEPTION( snapshot_exists_exception,  producer_exception,
                                    3170008, "The requested snapshot already exists" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_protocol_features_to_activate,  producer_exception,
                                    3170009, "The protocol features to be activated were not valid" )

   FC_DECLARE_DERIVED_EXCEPTION( reversible_blocks_exception,           chain_exception,
                                 3180000, "Reversible Blocks exception" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_reversible_blocks_dir,             reversible_blocks_exception,
                                    3180001, "Invalid reversible blocks directory" )
      FC_DECLARE_DERIVED_EXCEPTION( reversible_blocks_backup_dir_exist,          reversible_blocks_exception,
                                    3180002, "Backup directory for reversible blocks already existg" )
      FC_DECLARE_DERIVED_EXCEPTION( gap_in_reversible_blocks_db,          reversible_blocks_exception,
                                    3180003, "Gap in the reversible blocks database" )

   FC_DECLARE_DERIVED_EXCEPTION( block_log_exception, chain_exception,
                                 3190000, "Block log exception" )
      FC_DECLARE_DERIVED_EXCEPTION( block_log_unsupported_version, block_log_exception,
                                    3190001, "unsupported version of block log" )
      FC_DECLARE_DERIVED_EXCEPTION( block_log_append_fail, block_log_exception,
                                    3190002, "fail to append block to the block log" )
      FC_DECLARE_DERIVED_EXCEPTION( block_log_not_found, block_log_exception,
                                    3190003, "block log can not be found" )
      FC_DECLARE_DERIVED_EXCEPTION( block_log_backup_dir_exist, block_log_exception,
                                    3190004, "block log backup dir already exists" )

   FC_DECLARE_DERIVED_EXCEPTION( http_exception, chain_exception,
                                 3200000, "http exception" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_http_client_root_cert,    http_exception,
                                    3200001, "invalid http client root certificate" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_http_response, http_exception,
                                    3200002, "invalid http response" )
      FC_DECLARE_DERIVED_EXCEPTION( resolved_to_multiple_ports, http_exception,
                                    3200003, "service resolved to multiple ports" )
      FC_DECLARE_DERIVED_EXCEPTION( fail_to_resolve_host, http_exception,
                                    3200004, "fail to resolve host" )
      FC_DECLARE_DERIVED_EXCEPTION( http_request_fail, http_exception,
                                    3200005, "http request fail" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_http_request, http_exception,
                                    3200006, "invalid http request" )

   FC_DECLARE_DERIVED_EXCEPTION( resource_limit_exception, chain_exception,
                                 3210000, "Resource limit exception" )

   FC_DECLARE_DERIVED_EXCEPTION( mongo_db_exception, chain_exception,
                                 3220000, "Mongo DB exception" )
      FC_DECLARE_DERIVED_EXCEPTION( mongo_db_insert_fail, mongo_db_exception,
                                 3220001, "Fail to insert new data to Mongo DB" )
      FC_DECLARE_DERIVED_EXCEPTION( mongo_db_update_fail, mongo_db_exception,
                                 3220002, "Fail to update existing data in Mongo DB" )

   FC_DECLARE_DERIVED_EXCEPTION( contract_api_exception,    chain_exception,
                                 3230000, "Contract API exception" )
      FC_DECLARE_DERIVED_EXCEPTION( crypto_api_exception,   contract_api_exception,
                                    3230001, "Crypto API Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( db_api_exception,       contract_api_exception,
                                    3230002, "Database API Exception" )
      FC_DECLARE_DERIVED_EXCEPTION( arithmetic_exception,   contract_api_exception,
                                    3230003, "Arithmetic Exception" )

   FC_DECLARE_DERIVED_EXCEPTION( snapshot_exception,    chain_exception,
                                 3240000, "Snapshot exception" )
      FC_DECLARE_DERIVED_EXCEPTION( snapshot_validation_exception,   snapshot_exception,
                                    3240001, "Snapshot Validation Exception" )

   FC_DECLARE_DERIVED_EXCEPTION( protocol_feature_exception,    chain_exception,
                                 3250000, "Protocol feature exception" )
      FC_DECLARE_DERIVED_EXCEPTION( protocol_feature_validation_exception, protocol_feature_exception,
                                    3250001, "Protocol feature validation exception" )
      FC_DECLARE_DERIVED_EXCEPTION( protocol_feature_bad_block_exception, protocol_feature_exception,
                                    3250002, "Protocol feature exception (invalid block)" )
      FC_DECLARE_DERIVED_EXCEPTION( protocol_feature_iterator_exception, protocol_feature_exception,
                                    3250003, "Protocol feature iterator exception" )
} } // eosio::chain
