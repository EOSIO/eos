#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {
   void additional_block_signatures_extension::reflector_init() {
      static_assert( fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                     "additional_block_signatures_extension expects FC to support reflector_init" );

      EOS_ASSERT( signatures.size() > 0, ill_formed_additional_block_signatures_extension,
                  "Additional block signatures extension must contain at least one signature",
      );

      set<signature_type> unique_sigs;

      for( const auto& s : signatures ) {
         auto res = unique_sigs.insert( s );
         EOS_ASSERT( res.second, ill_formed_additional_block_signatures_extension,
                     "Signature ${s} was repeated in the additional block signatures extension",
                     ("s", s)
         );
      }
   }

   static fc::static_variant<transaction_id_type, pruned_transaction> translate_transaction_receipt(const transaction_id_type& tid) {
      return tid;
   }
   static fc::static_variant<transaction_id_type, pruned_transaction> translate_transaction_receipt(const packed_transaction& ptrx) {
      return pruned_transaction(ptrx);
   }

   pruned_transaction_receipt::pruned_transaction_receipt(const transaction_receipt& other)
     : transaction_receipt_header(static_cast<const transaction_receipt_header&>(other)),
       trx(other.trx.visit([&](const auto& obj) { return translate_transaction_receipt(obj); }))
   {}

   flat_multimap<uint16_t, block_extension> signed_block::validate_and_extract_extensions()const {
      using decompose_t = block_extension_types::decompose_t;

      flat_multimap<uint16_t, block_extension> results;

      uint16_t id_type_lower_bound = 0;

      for( size_t i = 0; i < block_extensions.size(); ++i ) {
         const auto& e = block_extensions[i];
         auto id = e.first;

         EOS_ASSERT( id >= id_type_lower_bound, invalid_block_extension,
                     "Block extensions are not in the correct order (ascending id types required)"
         );

         auto iter = results.emplace(std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple()
         );

         auto match = decompose_t::extract<block_extension>( id, e.second, iter->second );
         EOS_ASSERT( match, invalid_block_extension,
                     "Block extension with id type ${id} is not supported",
                     ("id", id)
         );

         if( match->enforce_unique ) {
            EOS_ASSERT( i == 0 || id > id_type_lower_bound, invalid_block_header_extension,
                        "Block extension with id type ${id} is not allowed to repeat",
                        ("id", id)
            );
         }


         id_type_lower_bound = id;
      }

      return results;

   }

   pruned_block::pruned_block( const signed_block& other )
     : signed_block_header(static_cast<const signed_block_header&>(other)),
       prune_state(prune_state_type::complete_legacy),
       transactions(other.transactions.begin(), other.transactions.end()),
       block_extensions(other.block_extensions)
   {}

   static std::size_t pruned_trx_receipt_packed_size(const pruned_transaction& obj, pruned_transaction::compression_type segment_compression) {
      return obj.maximum_pruned_pack_size(segment_compression);
   }
   static std::size_t pruned_trx_receipt_packed_size(const transaction_id_type& obj, pruned_transaction::compression_type) {
      return fc::raw::pack_size(obj);
   }

   std::size_t pruned_transaction_receipt::maximum_pruned_pack_size( pruned_transaction::compression_type segment_compression ) const {
      return fc::raw::pack_size(*static_cast<const transaction_receipt_header*>(this)) + 1 +
         trx.visit([&](const auto& obj){ return pruned_trx_receipt_packed_size(obj, segment_compression); });
   }

   std::size_t pruned_block::maximum_pruned_pack_size( pruned_transaction::compression_type segment_compression ) const {
      std::size_t result = fc::raw::pack_size(fc::unsigned_int(transactions.size()));
      for(const pruned_transaction_receipt& r: transactions) {
         result += r.maximum_pruned_pack_size( segment_compression );
      }
      return fc::raw::pack_size(*static_cast<const signed_block_header*>(this)) + fc::raw::pack_size(prune_state) + result + fc::raw::pack_size(block_extensions);
   }

} } /// namespace eosio::chain
