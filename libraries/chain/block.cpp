/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
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

} } /// namespace eosio::chain