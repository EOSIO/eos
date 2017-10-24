/*
 * Copyright (c) 2017, Respective Authors.
 *
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eos/utilities/exception_macros.hpp>

namespace eosio { namespace blockchain {

   FC_DECLARE_EXCEPTION( chain_exception, 3000000, "blockchain exception" )
   FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          eosio::blockchain::chain_exception, 3010000, "database query exception" )
   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          eosio::blockchain::chain_exception, 3020000, "block validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             eosio::blockchain::chain_exception, 3030000, "transaction validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_validate_exception,        eosio::blockchain::chain_exception, 3040000, "message validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_precondition_exception,    eosio::blockchain::chain_exception, 3050000, "message precondition exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_evaluate_exception,        eosio::blockchain::chain_exception, 3060000, "message evaluation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 eosio::blockchain::chain_exception, 3070000, "utility method exception" )
   FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           eosio::blockchain::chain_exception, 3080000, "undo database exception" )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        eosio::blockchain::chain_exception, 3090000, "unlinkable block" )
   FC_DECLARE_DERIVED_EXCEPTION( black_swan_exception,              eosio::blockchain::chain_exception, 3100000, "black swan" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,           eosio::blockchain::chain_exception, 3110000, "unknown block" )

   FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,         eosio::blockchain::block_validate_exception, 3020001, "transaction outputs in block do not match transaction outputs from applying block" )
   
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_auth,                   eosio::blockchain::transaction_exception, 3030001, "missing required authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_sigs,                   eosio::blockchain::transaction_exception, 3030002, "signatures do not satisfy declared authorizations" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_auth,                eosio::blockchain::transaction_exception, 3030003, "irrelevant authority included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                 eosio::blockchain::transaction_exception, 3030004, "irrelevant signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                  eosio::blockchain::transaction_exception, 3030005, "duplicate signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval,        eosio::blockchain::transaction_exception, 3030006, "committee account cannot directly approve transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  eosio::blockchain::transaction_exception, 3030007, "insufficient fee" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_scope,                  eosio::blockchain::transaction_exception, 3030008, "missing required scope" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_recipient,              eosio::blockchain::transaction_exception, 3030009, "missing required recipient" )
   FC_DECLARE_DERIVED_EXCEPTION( checktime_exceeded,                eosio::blockchain::transaction_exception, 3030010, "allotted processing time was exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                      eosio::blockchain::transaction_exception, 3030011, "duplicate transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,     eosio::blockchain::transaction_exception, 3030012, "unknown transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_scheduling_exception,           eosio::blockchain::transaction_exception, 3030013, "transaction failed during sheduling" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_unknown_argument,               eosio::blockchain::transaction_exception, 3030014, "transaction provided an unknown value to a system call" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_resource_exhausted,             eosio::blockchain::transaction_exception, 3030015, "transaction exhausted allowed resources" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_pts_address,               eosio::blockchain::utility_exception, 3060001, "invalid pts address" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_feeds,                eosio::blockchain::chain_exception, 37006, "insufficient feeds" )

   FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   eosio::blockchain::undo_database_exception, 3070001, "there are no blocks to pop" )


   #define EOS_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // eosio::blockchain
