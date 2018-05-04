#pragma once
#include <eosio/chain/block_header.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio { namespace chain {

   /**
    * When a transaction is referenced by a block it could imply one of several outcomes which
    * describe the state-transition undertaken by the block producer.
    */

   struct transaction_receipt_header {
      enum status_enum {
         executed  = 0, ///< succeed, no error handler executed
         soft_fail = 1, ///< objectively failed (not executed), error handler executed
         hard_fail = 2, ///< objectively failed and error handler objectively failed thus no state change
         delayed   = 3, ///< transaction delayed/deferred/scheduled for future execution
         expired   = 4  ///< transaction expired and storage space refuned to user
      };

      transaction_receipt_header():status(hard_fail){}
      transaction_receipt_header( status_enum s ):status(s){}

      fc::enum_type<uint8_t,status_enum>   status;
      fc::unsigned_int                     kcpu_usage; ///< total billed CPU usage
      fc::unsigned_int                     net_usage_words; ///<  total billed NET usage, so we can reconstruct resource state when skipping context free data... hard failures...
   };

   struct transaction_receipt : public transaction_receipt_header {

      transaction_receipt():transaction_receipt_header(){}
      transaction_receipt( transaction_id_type tid ):transaction_receipt_header(executed),trx(tid){}
      transaction_receipt( packed_transaction ptrx ):transaction_receipt_header(executed),trx(ptrx){}

      fc::static_variant<transaction_id_type, packed_transaction> trx;

      digest_type digest()const {
         digest_type::encoder enc;
         fc::raw::pack( enc, status );
         fc::raw::pack( enc, kcpu_usage );
         fc::raw::pack( enc, net_usage_words );
         if( trx.contains<transaction_id_type>() )
            fc::raw::pack( enc, trx.get<transaction_id_type>() );
         else
            fc::raw::pack( enc, trx.get<packed_transaction>().packed_digest() );
         return enc.result();
      }
   };


   /**
    */
   struct signed_block : public signed_block_header {
      using signed_block_header::signed_block_header;
      signed_block() = default;
      signed_block( const signed_block_header& h ):signed_block_header(h){}

      vector<transaction_receipt>   transactions; /// new or generated transactions
   };
   using signed_block_ptr = std::shared_ptr<signed_block>;

   struct producer_confirmation {
      block_id_type   block_id;
      digest_type     block_digest;
      account_name    producer;
      signature_type  sig;
   };

} } /// eosio::chain

FC_REFLECT_ENUM( eosio::chain::transaction_receipt::status_enum,
                 (executed)(soft_fail)(hard_fail)(delayed)(expired) )

FC_REFLECT(eosio::chain::transaction_receipt_header, (status)(kcpu_usage)(net_usage_words) )
FC_REFLECT_DERIVED(eosio::chain::transaction_receipt, (eosio::chain::transaction_receipt_header), (trx) )
FC_REFLECT_DERIVED(eosio::chain::signed_block, (eosio::chain::signed_block_header), (transactions))
