#pragma once
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/protocol_feature_activation.hpp>

#include <type_traits>

namespace eosio { namespace chain {

   namespace detail {
      struct extract_match {
         bool enforce_unique = false;
      };

      template<typename... Ts>
      struct decompose;

      template<>
      struct decompose<> {
         template<typename ResultVariant>
         static auto extract( uint16_t id, const vector<char>& data, ResultVariant& result )
         -> fc::optional<extract_match>
         {
            return {};
         }
      };

      template<typename T, typename... Rest>
      struct decompose<T, Rest...> {
         using head_t = T;
         using tail_t = decompose< Rest... >;

         template<typename ResultVariant>
         static auto extract( uint16_t id, const vector<char>& data, ResultVariant& result )
         -> fc::optional<extract_match>
         {
            if( id == head_t::extension_id() ) {
               result = fc::raw::unpack<head_t>( data );
               return { extract_match{ head_t::enforce_unique() } };
            }

            return tail_t::template extract<ResultVariant>( id, data, result );
         }
      };

      template<typename... Ts>
      struct block_header_extension_types {
         using block_header_extensions_t = fc::static_variant< Ts... >;
         using decompose_t = decompose< Ts... >;
      };
   }

   using block_header_extension_types = detail::block_header_extension_types<
      protocol_feature_activation
   >;

   using block_header_extensions = block_header_extension_types::block_header_extensions_t;

   struct block_header
   {
      block_timestamp_type             timestamp;
      account_name                     producer;

      /**
       *  By signing this block this producer is confirming blocks [block_num() - confirmed, blocknum())
       *  as being the best blocks for that range and that he has not signed any other
       *  statements that would contradict.
       *
       *  No producer should sign a block with overlapping ranges or it is proof of byzantine
       *  behavior. When producing a block a producer is always confirming at least the block he
       *  is building off of.  A producer cannot confirm "this" block, only prior blocks.
       */
      uint16_t                         confirmed = 1;

      block_id_type                    previous;

      checksum256_type                 transaction_mroot; /// mroot of cycles_summary
      checksum256_type                 action_mroot; /// mroot of all delivered action receipts


      /** The producer schedule version that should validate this block, this is used to
       * indicate that the prior block which included new_producers->version has been marked
       * irreversible and that it the new producer schedule takes effect this block.
       */
      uint32_t                          schedule_version = 0;
      optional_producer_schedule        new_producers;
      extensions_type                   header_extensions;


      block_header() = default;

      digest_type       digest()const;
      block_id_type     id() const;
      uint32_t          block_num() const { return num_from_id(previous) + 1; }
      static uint32_t   num_from_id(const block_id_type& id);

      vector<block_header_extensions> validate_and_extract_header_extensions()const;
   };


   struct signed_block_header : public block_header
   {
      signature_type    producer_signature;
   };

} } /// namespace eosio::chain

FC_REFLECT(eosio::chain::block_header,
           (timestamp)(producer)(confirmed)(previous)
           (transaction_mroot)(action_mroot)
           (schedule_version)(new_producers)(header_extensions))

FC_REFLECT_DERIVED(eosio::chain::signed_block_header, (eosio::chain::block_header), (producer_signature))
