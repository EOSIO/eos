#include <eosio/chain/block.hpp>
#include <eosio/chain/types.hpp>

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

   struct transaction_receipt_translator {
      bool legacy = true;
      fc::static_variant<transaction_id_type, packed_transaction> operator()(const transaction_id_type& tid) const {
         return tid;
      }
      fc::static_variant<transaction_id_type, packed_transaction> operator()(const packed_transaction_v0& ptrx) const {
         return packed_transaction(ptrx, legacy);
      }
      fc::static_variant<transaction_id_type, packed_transaction> operator()(packed_transaction_v0&& ptrx) const {
         return packed_transaction(std::move(ptrx), legacy);
      }
   };

   transaction_receipt::transaction_receipt(const transaction_receipt_v0& other, bool legacy)
     : transaction_receipt_header(static_cast<const transaction_receipt_header&>(other)),
       trx( std::move(other.trx).visit(transaction_receipt_translator{legacy}))
   {}

   transaction_receipt::transaction_receipt(transaction_receipt_v0&& other, bool legacy)
     : transaction_receipt_header(std::move(static_cast<transaction_receipt_header&>(other))),
       trx( std::move(other.trx).visit(transaction_receipt_translator{legacy}))
   {}

   static flat_multimap<uint16_t, block_extension> validate_and_extract_block_extensions(const extensions_type& block_extensions) {
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

   flat_multimap<uint16_t, block_extension> signed_block_v0::validate_and_extract_extensions()const {
      return validate_and_extract_block_extensions( block_extensions );
   }

   signed_block::signed_block( const signed_block_v0& other, bool legacy )
     : signed_block_header(static_cast<const signed_block_header&>(other)),
       prune_state(legacy ? prune_state_type::complete_legacy : prune_state_type::complete),
       block_extensions(other.block_extensions)
   {
      for(const auto& trx : other.transactions) {
         transactions.emplace_back(trx, legacy);
      }
   }

   signed_block::signed_block( signed_block_v0&& other, bool legacy )
     : signed_block_header(std::move(static_cast<signed_block_header&>(other))),
       prune_state(legacy ? prune_state_type::complete_legacy : prune_state_type::complete),
       block_extensions(std::move(other.block_extensions))
   {
      for(auto& trx : other.transactions) {
         transactions.emplace_back(std::move(trx), legacy);
      }
   }

   static std::size_t pruned_trx_receipt_packed_size(const packed_transaction& obj, packed_transaction::cf_compression_type segment_compression) {
      return obj.maximum_pruned_pack_size(segment_compression);
   }
   static std::size_t pruned_trx_receipt_packed_size(const transaction_id_type& obj, packed_transaction::cf_compression_type) {
      return fc::raw::pack_size(obj);
   }

   std::size_t transaction_receipt::maximum_pruned_pack_size( packed_transaction::cf_compression_type segment_compression ) const {
      return fc::raw::pack_size(*static_cast<const transaction_receipt_header*>(this)) + 1 +
         trx.visit([&](const auto& obj){ return pruned_trx_receipt_packed_size(obj, segment_compression); });
   }

   std::size_t signed_block::maximum_pruned_pack_size( packed_transaction::cf_compression_type segment_compression ) const {
      std::size_t result = fc::raw::pack_size(fc::unsigned_int(transactions.size()));
      for(const transaction_receipt& r: transactions) {
         result += r.maximum_pruned_pack_size( segment_compression );
      }
      return fc::raw::pack_size(*static_cast<const signed_block_header*>(this)) + fc::raw::pack_size(prune_state) + result + fc::raw::pack_size(block_extensions);
   }

   flat_multimap<uint16_t, block_extension> signed_block::validate_and_extract_extensions()const {
      return validate_and_extract_block_extensions( block_extensions );
   }

   signed_block_v0_ptr signed_block::to_signed_block_v0() const {
      if (prune_state != prune_state_type::complete_legacy)
         return signed_block_v0_ptr{};

      auto result = std::make_shared<signed_block_v0>(*static_cast<const signed_block_header*>(this));
      result->block_extensions = this->block_extensions;

      auto visitor = overloaded{
          [](const transaction_id_type &id) -> transaction_receipt_v0::trx_type { return id; },
          [](const packed_transaction &trx) -> transaction_receipt_v0::trx_type {
             const auto& legacy = trx.get_prunable_data().prunable_data.get<prunable_transaction_data::full_legacy>();
             return packed_transaction_v0(trx.get_packed_transaction(), legacy.signatures, legacy.packed_context_free_data, trx.get_compression());
          }};

      for (const transaction_receipt &r : transactions){
         result->transactions.emplace_back(transaction_receipt_v0{r, r.trx.visit(visitor)});
      }
      return result;
   }
} } /// namespace eosio::chain
