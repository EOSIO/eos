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

      /**
       * Given a pending block header state, wrap the promotion to a block header state such that additional signatures
       * can be allowed based on activations *prior* to the promoted block and properly injected into the signed block
       * that is previously constructed and mutated by the promotion
       *
       * This cleans up lifetime issues involved with accessing activated protocol features and moving from the
       * pending block header state
       *
       * @param cur the pending block header state to promote
       * @param b the signed block that will receive signatures during this process
       * @param pfs protocol feature set for digest access
       * @param extras all the remaining parameters that pass through
       * @return the block header state
       * @throws if the block was signed with multiple signatures before the extension is allowed
       */

      template<typename ...Extras>
      block_header_state inject_additional_signatures( pending_block_header_state&& cur,
                                                       signed_block& b,
                                                       const protocol_feature_set& pfs,
                                                       Extras&& ... extras )
      {
         return std::move(cur).finish_next(b, pfs, std::forward<Extras>(extras)...);
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
   :block_header_state( inject_additional_signatures( std::move(cur), *b, pfs, validator ) )
   ,block( std::move(b) )
   ,_pub_keys_recovered( true ) // called by produce_block so signature recovery of trxs must have been done
   ,_cached_trxs( std::move(trx_metas) )
   {}

} } /// eosio::chain
