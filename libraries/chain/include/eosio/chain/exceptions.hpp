/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eosio/chain/protocol.hpp>
#include <eosio/utilities/exception_macros.hpp>

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
      FC_DECLARE_DERIVED_EXCEPTION( block_id_type_exception,           chain_type_exception,
                                    3010008, "Invalid block ID" )
      FC_DECLARE_DERIVED_EXCEPTION( transaction_id_type_exception,     chain_type_exception,
                                    3010009, "Invalid transaction ID" )
      FC_DECLARE_DERIVED_EXCEPTION( packed_transaction_type_exception, chain_type_exception,
                                    3010010, "Invalid packed transaction" )
      FC_DECLARE_DERIVED_EXCEPTION( asset_type_exception,              chain_type_exception,
                                    3010011, "Invalid asset" )


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
      FC_DECLARE_DERIVED_EXCEPTION( tx_missing_sigs,             transaction_exception,
                                    3040008, "signatures do not satisfy declared authorizations" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,           transaction_exception,
                                    3040009, "irrelevant signature included" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,            transaction_exception,
                                    3040010, "duplicate signature included" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                transaction_exception,
                                    3040011, "duplicate transaction" )


   FC_DECLARE_DERIVED_EXCEPTION( action_validate_exception, chain_exception,
                                 3050000, "action exception" )

      FC_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception, action_validate_exception,
                                    3050001, "account name already exists" )
      FC_DECLARE_DERIVED_EXCEPTION( invalid_action_args_exception, action_validate_exception,
                                    3050002, "Invalid Action Arguments" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_auth_exception,     action_validate_exception,
                                    3050003, "missing required authority" )
      FC_DECLARE_DERIVED_EXCEPTION( irrelevant_auth_exception,  action_validate_exception,
                                    3050004, "irrelevant authority included" )

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


   FC_DECLARE_DERIVED_EXCEPTION( wasm_exception, chain_exception, 3070000, "WASM Exception" )

      FC_DECLARE_DERIVED_EXCEPTION( page_memory_error,        wasm_exception,
                                    3070001, "error in WASM page memory" )

      FC_DECLARE_DERIVED_EXCEPTION( wasm_execution_error,     wasm_exception,
                                    3070002, "Runtime Error Processing WASM" )

      FC_DECLARE_DERIVED_EXCEPTION( wasm_serialization_error, wasm_exception,
                                    3070003, "Serialization Error Processing WASM" )


   FC_DECLARE_DERIVED_EXCEPTION( resource_exhausted_exception, chain_exception,
                                 3080000, "resource exhausted exception" )
      FC_DECLARE_DERIVED_EXCEPTION( ram_usage_exceeded, resource_exhausted_exception,
                                    3080002, "account using more than allotted RAM usage" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_cpu_resource_exhausted, resource_exhausted_exception,
                                    3080002, "transaction exceeded CPU usage limit" )
      FC_DECLARE_DERIVED_EXCEPTION( tx_net_resource_exhausted, resource_exhausted_exception,
                                    3080003, "transaction exceeded network usage limit" )
      FC_DECLARE_DERIVED_EXCEPTION( checktime_exceeded,    transaction_exception,
                                    3080004, "transaction exceeded allotted processing time" )


   FC_DECLARE_DERIVED_EXCEPTION( misc_exception, chain_exception,
                                 3090000, "Miscellaneous exception" )

      FC_DECLARE_DERIVED_EXCEPTION( rate_limiting_state_inconsistent, misc_exception,
                                    3090001, "internal state is no longer consistent" )
      FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,       misc_exception,
                                    3090002, "unknown block" )
      FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception, misc_exception,
                                    3090003, "unknown transaction" )


   FC_DECLARE_DERIVED_EXCEPTION( missing_plugin_exception, chain_exception,
                                 3100000, "missing plugin exception" )

      FC_DECLARE_DERIVED_EXCEPTION( missing_chain_api_plugin_exception,           missing_plugin_exception,
                                    3100001, "Missing Chain API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_wallet_api_plugin_exception,          missing_plugin_exception,
                                    3100002, "Missing Wallet API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_account_history_api_plugin_exception, missing_plugin_exception,
                                    3100003, "Missing Account History API Plugin" )
      FC_DECLARE_DERIVED_EXCEPTION( missing_net_api_plugin_exception,             missing_plugin_exception,
                                    3100004, "Missing Net API Plugin" )


   FC_DECLARE_DERIVED_EXCEPTION( wallet_exception, chain_exception,
                                 3110000, "wallet exception" )

      FC_DECLARE_DERIVED_EXCEPTION( wallet_exist_exception,            wallet_exception,
                                    3110001, "Wallet already exists" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_nonexistent_exception,      wallet_exception,
                                    3110002, "Nonexistent wallet" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_locked_exception,           wallet_exception,
                                    3110003, "Locked wallet" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_missing_pub_key_exception,  wallet_exception,
                                    3110004, "Missing public key" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_invalid_password_exception, wallet_exception,
                                    3110005, "Invalid wallet password" )
      FC_DECLARE_DERIVED_EXCEPTION( wallet_not_available_exception,    wallet_exception,
                                    3110006, "No available wallet" )



   #define EOS_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // eosio::chain
