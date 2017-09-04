/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <fc/exception/exception.hpp>
#include <eos/chain/protocol.hpp>
#include <eos/utilities/exception_macros.hpp>

namespace eos { namespace chain {

   FC_DECLARE_EXCEPTION( chain_exception, 3000000, "blockchain exception" )
   FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          eos::chain::chain_exception, 3010000, "database query exception" )
   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          eos::chain::chain_exception, 3020000, "block validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             eos::chain::chain_exception, 3030000, "transaction validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_validate_exception,        eos::chain::chain_exception, 3040000, "message validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_precondition_exception,    eos::chain::chain_exception, 3050000, "message precondition exception" )
   FC_DECLARE_DERIVED_EXCEPTION( message_evaluate_exception,        eos::chain::chain_exception, 3060000, "message evaluation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 eos::chain::chain_exception, 3070000, "utility method exception" )
   FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           eos::chain::chain_exception, 3080000, "undo database exception" )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        eos::chain::chain_exception, 3090000, "unlinkable block" )
   FC_DECLARE_DERIVED_EXCEPTION( black_swan_exception,              eos::chain::chain_exception, 3100000, "black swan" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,           eos::chain::chain_exception, 3110000, "unknown block" )

   FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,         eos::chain::block_validate_exception, 3020001, "transaction outputs in block do not match transaction outputs from applying block" )
   
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_auth,                   eos::chain::transaction_exception, 3030001, "missing required authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_sigs,                   eos::chain::transaction_exception, 3030002, "signatures do not satisfy declared authorizations" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_auth,                eos::chain::transaction_exception, 3030003, "irrelevant authority included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                 eos::chain::transaction_exception, 3030004, "irrelevant signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                  eos::chain::transaction_exception, 3030005, "duplicate signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval,        eos::chain::transaction_exception, 3030006, "committee account cannot directly approve transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  eos::chain::transaction_exception, 3030007, "insufficient fee" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_scope,                  eos::chain::transaction_exception, 3030008, "missing required scope" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_recipient,              eos::chain::transaction_exception, 3030009, "missing required recipient" )
   FC_DECLARE_DERIVED_EXCEPTION( checktime_exceeded,                eos::chain::transaction_exception, 3030010, "allotted processing time was exceeded" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                      eos::chain::transaction_exception, 3030011, "duplicate transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,     eos::chain::transaction_exception, 3030012, "unknown transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_scheduling_exception,           eos::chain::transaction_exception, 3030013, "transaction failed during sheduling" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_unknown_argument,               eos::chain::transaction_exception, 3030014, "transaction provided an unknown value to a system call" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_resource_exhausted,             eos::chain::transaction_exception, 3030015, "transaction exhausted allowed resources" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_pts_address,               eos::chain::utility_exception, 3060001, "invalid pts address" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_feeds,                eos::chain::chain_exception, 37006, "insufficient feeds" )

   FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   eos::chain::undo_database_exception, 3070001, "there are no blocks to pop" )

//   EOS_DECLARE_OP_EVALUATE_EXCEPTION( from_account_not_whitelisted, transfer, 1, "owner mismatch" )
//   EOS_DECLARE_OP_EVALUATE_EXCEPTION( to_account_not_whitelisted, transfer, 2, "owner mismatch" )
//   EOS_DECLARE_OP_EVALUATE_EXCEPTION( restricted_transfer_asset, transfer, 3, "restricted transfer asset" )

   /*
   FC_DECLARE_DERIVED_EXCEPTION( addition_overflow,                 eos::chain::chain_exception, 30002, "addition overflow" )
   FC_DECLARE_DERIVED_EXCEPTION( subtraction_overflow,              eos::chain::chain_exception, 30003, "subtraction overflow" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_type_mismatch,               eos::chain::chain_exception, 30004, "asset/price mismatch" )
   FC_DECLARE_DERIVED_EXCEPTION( unsupported_chain_operation,       eos::chain::chain_exception, 30005, "unsupported chain operation" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction,               eos::chain::chain_exception, 30006, "unknown transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( duplicate_transaction,             eos::chain::chain_exception, 30007, "duplicate transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( zero_amount,                       eos::chain::chain_exception, 30008, "zero amount" )
   FC_DECLARE_DERIVED_EXCEPTION( zero_price,                        eos::chain::chain_exception, 30009, "zero price" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_divide_by_self,              eos::chain::chain_exception, 30010, "asset divide by self" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_divide_by_zero,              eos::chain::chain_exception, 30011, "asset divide by zero" )
   FC_DECLARE_DERIVED_EXCEPTION( new_database_version,              eos::chain::chain_exception, 30012, "new database version" )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block,                  eos::chain::chain_exception, 30013, "unlinkable block" )
   FC_DECLARE_DERIVED_EXCEPTION( price_out_of_range,                eos::chain::chain_exception, 30014, "price out of range" )

   FC_DECLARE_DERIVED_EXCEPTION( block_numbers_not_sequential,      eos::chain::chain_exception, 30015, "block numbers not sequential" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_previous_block_id,         eos::chain::chain_exception, 30016, "invalid previous block" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_block_time,                eos::chain::chain_exception, 30017, "invalid block time" )
   FC_DECLARE_DERIVED_EXCEPTION( time_in_past,                      eos::chain::chain_exception, 30018, "time is in the past" )
   FC_DECLARE_DERIVED_EXCEPTION( time_in_future,                    eos::chain::chain_exception, 30019, "time is in the future" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_block_digest,              eos::chain::chain_exception, 30020, "invalid block digest" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_member_signee,           eos::chain::chain_exception, 30021, "invalid committee_member signee" )
   FC_DECLARE_DERIVED_EXCEPTION( failed_checkpoint_verification,    eos::chain::chain_exception, 30022, "failed checkpoint verification" )
   FC_DECLARE_DERIVED_EXCEPTION( wrong_chain_id,                    eos::chain::chain_exception, 30023, "wrong chain id" )
   FC_DECLARE_DERIVED_EXCEPTION( block_older_than_undo_history,     eos::chain::chain_exception, 30025, "block is older than our undo history allows us to process" )

   FC_DECLARE_EXCEPTION( evaluation_error, 31000, "Evaluation Error" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_deposit,                  eos::chain::evaluation_error, 31001, "negative deposit" )
   FC_DECLARE_DERIVED_EXCEPTION( not_a_committee_member,                    eos::chain::evaluation_error, 31002, "not a committee_member" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_balance_record,            eos::chain::evaluation_error, 31003, "unknown balance record" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_funds,                eos::chain::evaluation_error, 31004, "insufficient funds" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_signature,                 eos::chain::evaluation_error, 31005, "missing signature" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_claim_password,            eos::chain::evaluation_error, 31006, "invalid claim password" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_withdraw_condition,        eos::chain::evaluation_error, 31007, "invalid withdraw condition" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_withdraw,                 eos::chain::evaluation_error, 31008, "negative withdraw" )
   FC_DECLARE_DERIVED_EXCEPTION( not_an_active_committee_member,            eos::chain::evaluation_error, 31009, "not an active committee_member" )
   FC_DECLARE_DERIVED_EXCEPTION( expired_transaction,               eos::chain::evaluation_error, 31010, "expired transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_transaction_expiration,    eos::chain::evaluation_error, 31011, "invalid transaction expiration" )
   FC_DECLARE_DERIVED_EXCEPTION( oversized_transaction,             eos::chain::evaluation_error, 31012, "transaction exceeded the maximum transaction size" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_account_name,              eos::chain::evaluation_error, 32001, "invalid account name" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_account_id,                eos::chain::evaluation_error, 32002, "unknown account id" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_account_name,              eos::chain::evaluation_error, 32003, "unknown account name" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_parent_account_signature,  eos::chain::evaluation_error, 32004, "missing parent account signature" )
   FC_DECLARE_DERIVED_EXCEPTION( parent_account_retracted,          eos::chain::evaluation_error, 32005, "parent account retracted" )
   FC_DECLARE_DERIVED_EXCEPTION( account_expired,                   eos::chain::evaluation_error, 32006, "account expired" )
   FC_DECLARE_DERIVED_EXCEPTION( account_already_registered,        eos::chain::evaluation_error, 32007, "account already registered" )
   FC_DECLARE_DERIVED_EXCEPTION( account_key_in_use,                eos::chain::evaluation_error, 32008, "account key already in use" )
   FC_DECLARE_DERIVED_EXCEPTION( account_retracted,                 eos::chain::evaluation_error, 32009, "account retracted" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_parent_account_name,       eos::chain::evaluation_error, 32010, "unknown parent account name" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_committee_member_slate,            eos::chain::evaluation_error, 32011, "unknown committee_member slate" )
   FC_DECLARE_DERIVED_EXCEPTION( too_may_committee_members_in_slate,        eos::chain::evaluation_error, 32012, "too many committee_members in slate" )
   FC_DECLARE_DERIVED_EXCEPTION( pay_balance_remaining,             eos::chain::evaluation_error, 32013, "pay balance remaining" )

   FC_DECLARE_DERIVED_EXCEPTION( not_a_committee_member_signature,          eos::chain::evaluation_error, 33002, "not committee_members signature" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_precision,                 eos::chain::evaluation_error, 35001, "invalid precision" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_asset_symbol,              eos::chain::evaluation_error, 35002, "invalid asset symbol" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_asset_id,                  eos::chain::evaluation_error, 35003, "unknown asset id" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_symbol_in_use,               eos::chain::evaluation_error, 35004, "asset symbol in use" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_asset_amount,              eos::chain::evaluation_error, 35005, "invalid asset amount" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_issue,                    eos::chain::evaluation_error, 35006, "negative issue" )
   FC_DECLARE_DERIVED_EXCEPTION( over_issue,                        eos::chain::evaluation_error, 35007, "over issue" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_asset_symbol,              eos::chain::evaluation_error, 35008, "unknown asset symbol" )
   FC_DECLARE_DERIVED_EXCEPTION( asset_id_in_use,                   eos::chain::evaluation_error, 35009, "asset id in use" )
   FC_DECLARE_DERIVED_EXCEPTION( not_user_issued,                   eos::chain::evaluation_error, 35010, "not user issued" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_asset_name,                eos::chain::evaluation_error, 35011, "invalid asset name" )

   FC_DECLARE_DERIVED_EXCEPTION( committee_member_vote_limit,               eos::chain::evaluation_error, 36001, "committee_member_vote_limit" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  eos::chain::evaluation_error, 36002, "insufficient fee" )
   FC_DECLARE_DERIVED_EXCEPTION( negative_fee,                      eos::chain::evaluation_error, 36003, "negative fee" )
   FC_DECLARE_DERIVED_EXCEPTION( missing_deposit,                   eos::chain::evaluation_error, 36004, "missing deposit" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_relay_fee,            eos::chain::evaluation_error, 36005, "insufficient relay fee" )

   FC_DECLARE_DERIVED_EXCEPTION( invalid_market,                    eos::chain::evaluation_error, 37001, "invalid market" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_market_order,              eos::chain::evaluation_error, 37002, "unknown market order" )
   FC_DECLARE_DERIVED_EXCEPTION( shorting_base_shares,              eos::chain::evaluation_error, 37003, "shorting base shares" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_collateral,           eos::chain::evaluation_error, 37004, "insufficient collateral" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_depth,                eos::chain::evaluation_error, 37005, "insufficient depth" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_feeds,                eos::chain::evaluation_error, 37006, "insufficient feeds" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_feed_price,                eos::chain::evaluation_error, 37007, "invalid feed price" )

   FC_DECLARE_DERIVED_EXCEPTION( price_multiplication_overflow,     eos::chain::evaluation_error, 38001, "price multiplication overflow" )
   FC_DECLARE_DERIVED_EXCEPTION( price_multiplication_underflow,    eos::chain::evaluation_error, 38002, "price multiplication underflow" )
   FC_DECLARE_DERIVED_EXCEPTION( price_multiplication_undefined,    eos::chain::evaluation_error, 38003, "price multiplication undefined product 0*inf" )
   */

   #define EOS_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // eos::chain
