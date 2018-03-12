/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eosio/chain/protocol.hpp>
#include <eosio/utilities/exception_macros.hpp>

namespace eosio { namespace chain {

   FC_DECLARE_EXCEPTION( chain_exception, 3000000, "blockchain exception" )
   FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          eosio::chain::chain_exception, 3010000, "database query exception" )
   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          eosio::chain::chain_exception, 3020000, "block validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             eosio::chain::chain_exception, 3030000, "transaction validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( action_validate_exception,         eosio::chain::chain_exception, 3040000, "message validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 eosio::chain::chain_exception, 3070000, "utility method exception" )
   FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           eosio::chain::chain_exception, 3080000, "undo database exception" )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        eosio::chain::chain_exception, 3090000, "unlinkable block" )
   FC_DECLARE_DERIVED_EXCEPTION( black_swan_exception,              eosio::chain::chain_exception, 3100000, "black swan" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,           eosio::chain::chain_exception, 3110000, "unknown block" )
   FC_DECLARE_DERIVED_EXCEPTION( chain_type_exception,              eosio::chain::chain_exception, 3120000, "chain type exception" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_plugin_exception,          eosio::chain::chain_exception, 3130000, "missing plugin exception" )

   FC_DECLARE_DERIVED_EXCEPTION( permission_query_exception,        eosio::chain::database_query_exception, 3010001, "permission query exception" )

   FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,         eosio::chain::block_validate_exception, 3020001, "transaction outputs in block do not match transaction outputs from applying block" )
   FC_DECLARE_DERIVED_EXCEPTION( block_concurrency_exception,       eosio::chain::block_validate_exception, 3020002, "block does not guarantee concurrent exection without conflicts" )
   FC_DECLARE_DERIVED_EXCEPTION( block_lock_exception,              eosio::chain::block_validate_exception, 3020003, "shard locks in block are incorrect or mal-formed" )

   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_auth,                   eosio::chain::transaction_exception, 3030001, "missing required authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_sigs,                   eosio::chain::transaction_exception, 3030002, "signatures do not satisfy declared authorizations" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_auth,                eosio::chain::transaction_exception, 3030003, "irrelevant authority included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                 eosio::chain::transaction_exception, 3030004, "irrelevant signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                  eosio::chain::transaction_exception, 3030005, "duplicate signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval,        eosio::chain::transaction_exception, 3030006, "committee account cannot directly approve transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  eosio::chain::transaction_exception, 3030007, "insufficient fee" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_recipient,              eosio::chain::transaction_exception, 3030009, "missing required recipient" )
   FC_DECLARE_DERIVED_EXCEPTION( checktime_exceeded,                eosio::chain::transaction_exception, 3030010, "allotted processing time was exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                      eosio::chain::transaction_exception, 3030011, "duplicate transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,     eosio::chain::transaction_exception, 3030012, "unknown transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_scheduling_exception,           eosio::chain::transaction_exception, 3030013, "transaction failed during sheduling" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_unknown_argument,               eosio::chain::transaction_exception, 3030014, "transaction provided an unknown value to a system call" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_resource_exhausted,             eosio::chain::transaction_exception, 3030015, "transaction exhausted allowed resources" )
   FC_DECLARE_DERIVED_EXCEPTION( page_memory_error,                 eosio::chain::transaction_exception, 3030016, "error in WASM page memory" )
   FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_permission,            eosio::chain::transaction_exception, 3030017, "Unsatisfied permission" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_msgs_auth_exceeded,             eosio::chain::transaction_exception, 3030018, "Number of transaction messages per authorized account has been exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_msgs_code_exceeded,             eosio::chain::transaction_exception, 3030019, "Number of transaction messages per code account has been exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( wasm_execution_error,              eosio::chain::transaction_exception, 3030020, "Runtime Error Processing WASM" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_decompression_error,            eosio::chain::transaction_exception, 3030020, "Error decompressing transaction" )

   FC_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception,     eosio::chain::action_validate_exception, 3040001, "account name already exists" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_pts_address,               eosio::chain::utility_exception, 3060001, "invalid pts address" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_feeds,                eosio::chain::chain_exception, 37006, "insufficient feeds" )

   FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   eosio::chain::undo_database_exception, 3070001, "there are no blocks to pop" )

   FC_DECLARE_DERIVED_EXCEPTION( name_type_exception,               eosio::chain::chain_type_exception, 3120001, "Invalid name" )
   FC_DECLARE_DERIVED_EXCEPTION( public_key_type_exception,         eosio::chain::chain_type_exception, 3120002, "Invalid public key" )
   FC_DECLARE_DERIVED_EXCEPTION( authority_type_exception,          eosio::chain::chain_type_exception, 3120003, "Invalid authority" )
   FC_DECLARE_DERIVED_EXCEPTION( action_type_exception,             eosio::chain::chain_type_exception, 3120004, "Invalid action" )
   FC_DECLARE_DERIVED_EXCEPTION( transaction_type_exception,        eosio::chain::chain_type_exception, 3120005, "Invalid transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( abi_type_exception,                eosio::chain::chain_type_exception, 3120006, "Invalid ABI" )

   FC_DECLARE_DERIVED_EXCEPTION( missing_chain_api_plugin_exception,                 eosio::chain::missing_plugin_exception, 3130001, "Missing Chain API Plugin" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_wallet_api_plugin_exception,                eosio::chain::missing_plugin_exception, 3130002, "Missing Wallet API Plugin" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_account_history_api_plugin_exception,       eosio::chain::missing_plugin_exception, 3130003, "Missing Account History API Plugin" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_net_api_plugin_exception,                   eosio::chain::missing_plugin_exception, 3130003, "Missing Net API Plugin" )


   #define EOS_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // eosio::chain
