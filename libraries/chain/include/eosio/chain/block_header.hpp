#pragma once
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/producer_schedule.hpp>

namespace eosio { namespace chain {

   struct block_header
   {
      block_id_type                    previous;
      block_timestamp_type             timestamp;

      checksum256_type                 transaction_mroot; /// mroot of cycles_summary
      checksum256_type                 action_mroot; /// mroot of all delivered action receipts

      account_name                     producer;

      /** The producer schedule version that should validate this block, this is used to
       * indicate that the prior block which included new_producers->version has been marked
       * irreversible and that it the new producer schedule takes effect this block.
       */
      uint32_t                          schedule_version = 0;
      optional<producer_schedule_type>  new_producers;


      digest_type       digest()const;
      block_id_type     id() const;
      uint32_t          block_num() const { return num_from_id(previous) + 1; }
      static uint32_t   num_from_id(const block_id_type& id);
   };


   struct signed_block_header : public block_header
   {
      signature_type    producer_signature;
   };

   struct header_confirmation {
      block_id_type   block_id;
      account_name    producer;
      signature_type  producer_signature;
   };

} } /// namespace eosio::chain

FC_REFLECT(eosio::chain::block_header, (previous)(timestamp)
           (transaction_mroot)(action_mroot)
           (producer)(schedule_version)(new_producers))

FC_REFLECT_DERIVED(eosio::chain::signed_block_header, (eosio::chain::block_header), (producer_signature))
FC_REFLECT(eosio::chain::header_confirmation,  (block_id)(producer)(producer_signature) )
