#pragma once
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/producer_schedule.hpp>


namespace eosio { namespace chain {

   struct block_header
   {
      block_id_type                    previous;
      block_timestamp_type             timestamp;

      checksum256_type                 transaction_mroot; /// mroot of cycles_summary
      checksum256_type                 action_mroot;
      checksum256_type                 block_mroot;

      account_name                     producer;

      /** The producer schedule version that should validate this block, this is used to
       * indicate that the prior block which included new_producers->version has been marked
       * irreversible and that it the new producer schedule takes effect this block.
       */
      uint32_t                          schedule_version = 0;
      optional<producer_schedule_type>  new_producers;


      digest_type       digest()const;
      uint32_t          block_num() const { return num_from_id(previous) + 1; }
      static uint32_t   num_from_id(const block_id_type& id);
   };


   struct signed_block_header : public block_header
   {
      block_id_type              id() const;
      public_key_type            signee( const digest_type& schedule_digest ) const;
      void                       sign(const private_key_type& signer, const digest_type& pending_producer_schedule_hash );
      bool                       validate_signee(const public_key_type& expected_signee, const digest_type& pending_producer_schedule_hash ) const;
      digest_type                signed_digest( const digest_type& pending_producer_schedule_hash )const;

      signature_type             producer_signature;
   };

} } /// namespace eosio::chain

FC_REFLECT(eosio::chain::block_header, (previous)(timestamp)
           (transaction_mroot)(action_mroot)(block_mroot)
           (producer)(schedule_version)(new_producers))

FC_REFLECT_DERIVED(eosio::chain::signed_block_header, (eosio::chain::block_header), (producer_signature))
