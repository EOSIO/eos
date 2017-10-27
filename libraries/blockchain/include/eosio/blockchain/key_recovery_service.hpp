#pragma once
#include <eosio/blockchain/types.hpp>

namespace eosio { namespace blockchain {

/**
 * Service that asynchronously recovers keys from signatures
 * @tparam DispatchPolicy - type of job dispatch policy to use
 */
template<typename DispatchPolicy>
class key_recovery_service {
   public:

      /**
       * Create a key recovery service and configure the dispatcher
       *
       * @param policy - the dispatch policy to use (defaults to trivial constructor if available)
       */
      key_recovery_service( DispatchPolicy&& policy = DispatchPolicy() )
         :_dispatcher(forward<decltype(policy)>(policy))
      {};

      /**
       * Asynchronously recover the keys from signatures over a given digest
       *
       * @param digest - the digest of the signed message
       * @param signatures - the signatures from which to recover keys
       * @param cb - callback to use for success or failure
       * @tparam Callback - callable type for void(vector<public_key_type>&&|std::exception_ptr)
       */
      template<typename Callback>
      void recover_keys( const digest_type& digest, const vector<signature_type>& signatures, Callback&& cb ) {
         _dispatcher.dispatch([=](){
            try {
               auto result = vector<public_key_type>();
               result.reserve(signatures.size());

               for( const auto& signature: signatures ) {
                  result.emplace_back(fc::ecc::public_key(signature, digest));
               }

               cb(move(result));
            } ESIO_CATCH_AND_THROW_TO_CALLBACK(cb);
         });
      }

   private:
      typename DispatchPolicy::dispatcher _dispatcher;
};

}} // eosio::blockchain
