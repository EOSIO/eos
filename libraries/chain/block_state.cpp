#include <eosio/chain/block_state.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

   namespace {
      constexpr auto additional_sigs_eid = additional_block_signatures_extension::extension_id();

      /**
       * Given a complete signed block, extract the validated additional signatures if present;
       *
       * @param b complete signed block
       * @param pfs protocol feature set for digest access
       * @param pfa activated protocol feature set to determine if extensions are allowed
       * @return the list of additional signatures
       * @throws if additional signatures are present before being supported by protocol feature activations
       */
      vector<signature_type> extract_additional_signatures( const signed_block_ptr& b,
                                                            const protocol_feature_set& pfs,
                                                            const protocol_feature_activation_set_ptr& pfa )
      {
         auto exts = b->validate_and_extract_extensions();

         if ( exts.count(additional_sigs_eid) > 0 ) {
            auto& additional_sigs = std::get<additional_block_signatures_extension>(exts.lower_bound(additional_sigs_eid)->second);

            return std::move(additional_sigs.signatures);
         }

         return {};
      }
   }

   block_state::block_state( const block_header_state& prev,
                             signed_block_ptr b,
                             const protocol_feature_set& pfs,
                             const std::function<void( block_timestamp_type,
                                                       const flat_set<digest_type>&,
                                                       const vector<digest_type>& )>& validator,
                             bool skip_validate_signee
                           )
   :block_header_state( prev.next( *b, extract_additional_signatures(b, pfs, prev.activated_protocol_features), pfs, validator, skip_validate_signee ) )
   ,block( std::move(b) )
   {}

   block_state::block_state( pending_block_header_state&& cur,
                             signed_block_ptr&& b,
                             deque<transaction_metadata_ptr>&& trx_metas,
                             const protocol_feature_set& pfs,
                             const std::function<void( block_timestamp_type,
                                                       const flat_set<digest_type>&,
                                                       const vector<digest_type>& )>& validator
                           )
   :block_header_state(std::move(cur).finish_next(*b, pfs, validator))
   ,block( std::move(b) )
   ,_pub_keys_recovered( true ) // called by produce_block so signature recovery of trxs must have been done
   ,_cached_trxs( std::move(trx_metas) )
   {}

   void block_state::assign_signatures(std::vector<signature_type>&& sigs, bool wtmsig_enabled) {
      block_header_state::assign_signatures(std::move(sigs));  
      block->producer_signature = header.producer_signature;                                           
      if (!additional_signatures.empty()) {
         EOS_ASSERT(wtmsig_enabled, block_validate_exception,
                     "Block has multiple signatures before activation of WTMsig Block Signatures");

         // as an optimization we don't copy this out into the legitimate extension structure as it serializes
         // the same way as the vector of signatures
         static_assert(fc::reflector<additional_block_signatures_extension>::total_member_count == 1);
         static_assert(std::is_same_v<decltype(additional_block_signatures_extension::signatures), std::vector<signature_type>>);

         emplace_extension(block->block_extensions, additional_sigs_eid, fc::raw::pack( additional_signatures ));
      }
   }
} } /// eosio::chain
