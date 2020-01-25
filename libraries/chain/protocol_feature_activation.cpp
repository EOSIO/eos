#include <eosio/chain/protocol_feature_activation.hpp>
#include <eosio/chain/exceptions.hpp>

#include <algorithm>

namespace eosio { namespace chain {

   void protocol_feature_activation::reflector_init() {
      static_assert( fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                     "protocol_feature_activation expects FC to support reflector_init" );


      EOS_ASSERT( protocol_features.size() > 0, ill_formed_protocol_feature_activation,
                  "Protocol feature activation extension must have at least one protocol feature digest",
      );

      set<digest_type> s;

      for( const auto& d : protocol_features ) {
         auto res = s.insert( d );
         EOS_ASSERT( res.second, ill_formed_protocol_feature_activation,
                     "Protocol feature digest ${d} was repeated in the protocol feature activation extension",
                     ("d", d)
         );
      }
   }

   protocol_feature_activation_set::protocol_feature_activation_set(
                                       const protocol_feature_activation_set& orig_pfa_set,
                                       vector<digest_type> additional_features,
                                       bool enforce_disjoint
   )
   {
      std::sort( additional_features.begin(), additional_features.end() );

      const auto& s1 = orig_pfa_set.protocol_features;
      const auto& s2 = additional_features;

      auto expected_size = s1.size() + s2.size();
      protocol_features.reserve( expected_size );

      std::set_union( s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(), end_inserter( protocol_features ) );

      EOS_ASSERT( !enforce_disjoint || protocol_features.size() == expected_size,
                  invalid_block_header_extension,
                  "duplication of protocol feature digests"
      );
   }

} }  // eosio::chain
