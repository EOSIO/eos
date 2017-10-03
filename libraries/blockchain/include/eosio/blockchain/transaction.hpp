#pragma once
#include <eosio/blockchain/types.hpp>

namespace eosio { namespace blockchain {

   struct permission_level {
      account_name    account;
      permission_name level;
   };

   struct message {
      account_name               code;
      function_name              function;
      vector<permission_level>   permissions;
      vector<public_key_type>    key_permissions;
      bytes                      data;
   };

   struct transaction {
      uint16_t               ref_block_num = 0;
      uint32_t               ref_block_prefix = 0;
      time_point_sec         expiration;
      vector<account_name>   read_scope;
      vector<account_name>   write_scope;
      vector<message>        messages;
   };

   struct signed_transaction : public transaction {
      vector<signature_type> signatures;
   };
   typedef shared_ptr<signed_transaction> signed_transaction_ptr;


   struct processed_transaction;

   struct message_output {
      string log;
   };

   /**
    *  This data is tracked for data gathering purposes, but a processed transaction
    *  never needs to be part of the blockchain log. All the the implied transaction
    *  and message deliveries are part of the merkle calculation. This means this data
    *  can be maintained in a non-consensus database and merkle proofs can be generated
    *  over it.
    */
   struct processed_transaction : public signed_transaction {
      flat_set<public_key_type>       signedby;
      vector<message_output>          output;
      vector<processed_transaction>   inline_transaction;
      vector<signed_transaction>      deferred_transactions;
   };
   typedef shared_ptr<processed_transaction> processed_transaction_ptr;

   /**
    *  @brief defines meta data about a transaction so that it does not need to recomputed
    *
    *  This class is designed to be used as a shared pointer so that it can be easily moved 
    *  between threads, blocks, and pending queues. 
    */
   struct transaction_metadata : public std::enable_shared_from_this<transaction_metadata> {
      transaction_metadata( const processed_transaction& trx );
      transaction_metadata( const signed_transaction& trx );

      transaction_id_type          id;
      vector<char>                 packed;
      processed_transaction_ptr    trx;
      block_id_type                validated_in_block;
      std::exception_ptr           error;
   }; 

   typedef std::shared_ptr<transaction_metadata> transaction_metadata_ptr;

   struct  generated_transaction_id { transaction_id_type id; };
   struct  delivered_message_id     { message_id_type     id; };
   struct  executed_transaction_id  { transaction_id_type id; };
   typedef static_variant<generated_transaction_id,delivered_message_id,executed_transaction_id> merkle_id;


} } /// eosio::blockchain
