/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eos/chain/protocol.hpp>
#include <eos/utilities/exception_macros.hpp>

namespace eosio { namespace chain {

   FC_DECLARE_EXCEPTION( chain_exception, 3000000, "blockchain exception" )
   FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          eosio::chain::chain_exception, 3010000, "database query exception" )
   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          eosio::chain::chain_exception, 3020000, "block validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             eosio::chain::chain_exception, 3030000, "transaction validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_validate_exception,        eosio::chain::chain_exception, 3040000, "message validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_precondition_exception,    eosio::chain::chain_exception, 3050000, "message precondition exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_evaluate_exception,        eosio::chain::chain_exception, 3060000, "message evaluation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 eosio::chain::chain_exception, 3070000, "utility method exception" )
   FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           eosio::chain::chain_exception, 3080000, "undo database exception" )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        eosio::chain::chain_exception, 3090000, "unlinkable block" )
   FC_DECLARE_DERIVED_EXCEPTION( black_swan_exception,              eosio::chain::chain_exception, 3100000, "black swan" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,           eosio::chain::chain_exception, 3110000, "unknown block" )

   FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,         eosio::chain::block_validate_exception, 3020001, "transaction outputs in block do not match transaction outputs from applying block" )
   
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_auth,                   eosio::chain::transaction_exception, 3030001, "missing required authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_sigs,                   eosio::chain::transaction_exception, 3030002, "signatures do not satisfy declared authorizations" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_auth,                eosio::chain::transaction_exception, 3030003, "irrelevant authority included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                 eosio::chain::transaction_exception, 3030004, "irrelevant signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                  eosio::chain::transaction_exception, 3030005, "duplicate signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval,        eosio::chain::transaction_exception, 3030006, "committee account cannot directly approve transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  eosio::chain::transaction_exception, 3030007, "insufficient fee" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_scope,                  eosio::chain::transaction_exception, 3030008, "missing required scope" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_recipient,              eosio::chain::transaction_exception, 3030009, "missing required recipient" )
   FC_DECLARE_DERIVED_EXCEPTION( checktime_exceeded,                eosio::chain::transaction_exception, 3030010, "allocated processing time was exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                      eosio::chain::transaction_exception, 3030011, "duplicate transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,     eosio::chain::transaction_exception, 3030012, "unknown transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_scheduling_exception,           eosio::chain::transaction_exception, 3030013, "transaction failed during sheduling" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_unknown_argument,               eosio::chain::transaction_exception, 3030014, "transaction provided an unknown value to a system call" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_resource_exhausted,             eosio::chain::transaction_exception, 3030015, "transaction exhausted allowed resources" )
   FC_DECLARE_DERIVED_EXCEPTION( page_memory_error,                 eosio::chain::transaction_exception, 3030016, "error in WASM page memory" )
   FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_permission,            eosio::chain::transaction_exception, 3030017, "Unsatisfied permission" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_msgs_auth_exceeded,             eosio::chain::transaction_exception, 3030018, "Number of transaction messages per authorized account has been exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_msgs_code_exceeded,             eosio::chain::transaction_exception, 3030019, "Number of transaction messages per code account has been exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_code_db_limit_exceeded,         eosio::chain::transaction_exception, 3030020, "Database storage limit for code account has been exceeded in transaction message" )
   FC_DECLARE_DERIVED_EXCEPTION( msg_resource_exhausted,            eosio::chain::transaction_exception, 3030021, "message exhausted allowed resources" )
   FC_DECLARE_DERIVED_EXCEPTION( api_not_supported,                 eosio::chain::transaction_exception, 3030022, "API not currently supported" )

   FC_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception,     eosio::chain::message_precondition_exception, 3050001, "account name already exists" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_pts_address,               eosio::chain::utility_exception, 3060001, "invalid pts address" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_feeds,                eosio::chain::chain_exception, 37006, "insufficient feeds" )

   FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   eosio::chain::undo_database_exception, 3070001, "there are no blocks to pop" )

//   EOS_DECLARE_OP_EVALUATE_EXCEPTION( from_account_not_whitelisted, transfer, 1, "owner mismatch" )
//   EOS_DECLARE_OP_EVALUATE_EXCEPTION( to_account_not_whitelisted, transfer, 2, "owner mismatch" )
//   EOS_DECLARE_OP_EVALUATE_EXCEPTION( restricted_transfer_asset, transfer, 3, "restricted transfer asset" )

   /*
   FC_DECLARE_DERIVED_EXCEPTION( addition_overflow,                 eosio::chain::chain_exception, 30002, "addition overflow" )
   FC_DECLARE_DERIVED_EXCEPTION( subtraction_overflow,              eosio::chain::chain_exception, 30003, "subtraction overflow" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_type_mismatch,               eosio::chain::chain_exception, 30004, "asset/price mismatch" )
   FC_DECLARE_DERIVED_EXCEPTION( unsupported_chain_operation,       eosio::chain::chain_exception, 30005, "unsupported chain operation" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction,               eosio::chain::chain_exception, 30006, "unknown transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( duplicate_transaction,             eosio::chain::chain_exception, 30007, "duplicate transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( zero_amount,                       eosio::chain::chain_exception, 30008, "zero amount" )
   FC_DECLARE_DERIVED_EXCEPTION( zero_price,                        eosio::chain::chain_exception, 30009, "zero price" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_divide_by_self,              eosio::chain::chain_exception, 30010, "asset divide by self" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_divide_by_zero,              eosio::chain::chain_exception, 30011, "asset divide by zero" )
   FC_DECLARE_DERIVED_EXCEPTION( new_database_version,              eosio::chain::chain_exception, 30012, "new database version" )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block,                  eosio::chain::chain_exception, 30013, "unlinkable block" )
   FC_DECLARE_DERIVED_EXCEPTION( price_out_of_range,                eosio::chain::chain_exception, 30014, "price out of range" )

   FC_DECLARE_DERIVED_EXCEPTION( block_numbers_not_sequential,      eosio::chain::chain_exception, 30015, "block numbers not sequential" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_previous_block_id,         eosio::chain::chain_exception, 30016, "invalid previous block" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_block_time,                eosio::chain::chain_exception, 30017, "invalid block time" )
   FC_DECLARE_DERIVED_EXCEPTION( time_in_past,                      eosio::chain::chain_exception, 30018, "time is in the past" )
   FC_DECLARE_DERIVED_EXCEPTION( time_in_future,                    eosio::chain::chain_exception, 30019, "time is in the future" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_block_digest,              eosio::chain::chain_exception, 30020, "invalid block digest" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_member_signee,   eosio::chain::chain_exception, 30021, "invalid committee_member signee" )
   FC_DECLARE_DERIVED_EXCEPTION( failed_checkpoint_verification,    eosio::chain::chain_exception, 30022, "failed checkpoint verification" )
   FC_DECLARE_DERIVED_EXCEPTION( wrong_chain_id,                    eosio::chain::chain_exception, 30023, "wrong chain id" )
   FC_DECLARE_DERIVED_EXCEPTION( block_older_than_undo_history,     eosio::chain::chain_exception, 30025, "block is older than our undo history allows us to process" )

   FC_DECLARE_EXCEPTION( evaluation_error, 31000, "Evaluation Error" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_deposit,                  eosio::chain::evaluation_error, 31001, "negative deposit" )
   FC_DECLARE_DERIVED_EXCEPTION( not_a_committee_member,            eosio::chain::evaluation_error, 31002, "not a committee_member" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_balance_record,            eosio::chain::evaluation_error, 31003, "unknown balance record" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_funds,                eosio::chain::evaluation_error, 31004, "insufficient funds" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_signature,                 eosio::chain::evaluation_error, 31005, "missing signature" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_claim_password,            eosio::chain::evaluation_error, 31006, "invalid claim password" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_withdraw_condition,        eosio::chain::evaluation_error, 31007, "invalid withdraw condition" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_withdraw,                 eosio::chain::evaluation_error, 31008, "negative withdraw" )
   FC_DECLARE_DERIVED_EXCEPTION( not_an_active_committee_member,    eosio::chain::evaluation_error, 31009, "not an active committee_member" )
   FC_DECLARE_DERIVED_EXCEPTION( expired_transaction,               eosio::chain::evaluation_error, 31010, "expired transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_transaction_expiration,    eosio::chain::evaluation_error, 31011, "invalid transaction expiration" )
   FC_DECLARE_DERIVED_EXCEPTION( oversized_transaction,             eosio::chain::evaluation_error, 31012, "transaction exceeded the maximum transaction size" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_account_name,              eosio::chain::evaluation_error, 32001, "invalid account name" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_account_id,                eosio::chain::evaluation_error, 32002, "unknown account id" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_account_name,              eosio::chain::evaluation_error, 32003, "unknown account name" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_parent_account_signature,  eosio::chain::evaluation_error, 32004, "missing parent account signature" )
   FC_DECLARE_DERIVED_EXCEPTION( parent_account_retracted,          eosio::chain::evaluation_error, 32005, "parent account retracted" )
   FC_DECLARE_DERIVED_EXCEPTION( account_expired,                   eosio::chain::evaluation_error, 32006, "account expired" )
   FC_DECLARE_DERIVED_EXCEPTION( account_already_registered,        eosio::chain::evaluation_error, 32007, "account already registered" )
   FC_DECLARE_DERIVED_EXCEPTION( account_key_in_use,                eosio::chain::evaluation_error, 32008, "account key already in use" )
   FC_DECLARE_DERIVED_EXCEPTION( account_retracted,                 eosio::chain::evaluation_error, 32009, "account retracted" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_parent_account_name,       eosio::chain::evaluation_error, 32010, "unknown parent account name" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_committee_member_slate,    eosio::chain::evaluation_error, 32011, "unknown committee_member slate" )
   FC_DECLARE_DERIVED_EXCEPTION( too_may_committee_members_in_slate,eosio::chain::evaluation_error, 32012, "too many committee_members in slate" )
   FC_DECLARE_DERIVED_EXCEPTION( pay_balance_remaining,             eosio::chain::evaluation_error, 32013, "pay balance remaining" )

   FC_DECLARE_DERIVED_EXCEPTION( not_a_committee_member_signature,  eosio::chain::evaluation_error, 33002, "not committee_members signature" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_precision,                 eosio::chain::evaluation_error, 35001, "invalid precision" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_asset_symbol,              eosio::chain::evaluation_error, 35002, "invalid asset symbol" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_asset_id,                  eosio::chain::evaluation_error, 35003, "unknown asset id" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_symbol_in_use,               eosio::chain::evaluation_error, 35004, "asset symbol in use" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_asset_amount,              eosio::chain::evaluation_error, 35005, "invalid asset amount" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_issue,                    eosio::chain::evaluation_error, 35006, "negative issue" )
   FC_DECLARE_DERIVED_EXCEPTION( over_issue,                        eosio::chain::evaluation_error, 35007, "over issue" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_asset_symbol,              eosio::chain::evaluation_error, 35008, "unknown asset symbol" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_id_in_use,                   eosio::chain::evaluation_error, 35009, "asset id in use" )
   FC_DECLARE_DERIVED_EXCEPTION( not_user_issued,                   eosio::chain::evaluation_error, 35010, "not user issued" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_asset_name,                eosio::chain::evaluation_error, 35011, "invalid asset name" )

   FC_DECLARE_DERIVED_EXCEPTION( committee_member_vote_limit,       eosio::chain::evaluation_error, 36001, "committee_member_vote_limit" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  eosio::chain::evaluation_error, 36002, "insufficient fee" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_fee,                      eosio::chain::evaluation_error, 36003, "negative fee" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_deposit,                   eosio::chain::evaluation_error, 36004, "missing deposit" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_relay_fee,            eosio::chain::evaluation_error, 36005, "insufficient relay fee" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_market,                    eosio::chain::evaluation_error, 37001, "invalid market" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_market_order,              eosio::chain::evaluation_error, 37002, "unknown market order" )
   FC_DECLARE_DERIVED_EXCEPTION( shorting_base_shares,              eosio::chain::evaluation_error, 37003, "shorting base shares" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_collateral,           eosio::chain::evaluation_error, 37004, "insufficient collateral" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_depth,                eosio::chain::evaluation_error, 37005, "insufficient depth" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_feeds,                eosio::chain::evaluation_error, 37006, "insufficient feeds" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_feed_price,                eosio::chain::evaluation_error, 37007, "invalid feed price" )

   FC_DECLARE_DERIVED_EXCEPTION( price_multiplication_overflow,     eosio::chain::evaluation_error, 38001, "price multiplication overflow" )
   FC_DECLARE_DERIVED_EXCEPTION( price_multiplication_underflow,    eosio::chain::evaluation_error, 38002, "price multiplication underflow" )
   FC_DECLARE_DERIVED_EXCEPTION( price_multiplication_undefined,    eosio::chain::evaluation_error, 38003, "price multiplication undefined product 0*inf" )
   */

   #define EOS_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // eosio::chain
