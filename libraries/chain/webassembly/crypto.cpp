#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   template<class Encoder> auto encode(char* data, uint32_t datalen) {
      Encoder e;
      const size_t bs = eosio::chain::config::hashing_checktime_block_size;
      while ( datalen > bs ) {
         e.write( data, bs );
         data += bs;
         datalen -= bs;
         context.trx_context.checktime();
      }
      e.write( data, datalen );
      return e.result();
   }

   void interface::assert_recover_key( const fc::sha256& digest,
                                       legacy_array_ptr<char> sig, uint32_t siglen,
                                       legacy_array_ptr<char> pub, uint32_t publen ) {
      fc::crypto::signature s;
      fc::crypto::public_key p;
      datastream<const char*> ds( sig, siglen );
      datastream<const char*> pubds ( pub, publen );

      fc::raw::unpack( ds, s );
      fc::raw::unpack( pubds, p );

      EOS_ASSERT(s.which() < ctx.db.get<protocol_state_object>().num_supported_key_types, unactivated_signature_type,
        "Unactivated signature type used during assert_recover_key");
      EOS_ASSERT(p.which() < ctx.db.get<protocol_state_object>().num_supported_key_types, unactivated_key_type,
        "Unactivated key type used when creating assert_recover_key");

      if(ctx.control.is_producing_block())
         EOS_ASSERT(s.variable_size() <= ctx.control.configured_subjective_signature_length_limit(),
                    sig_variable_size_limit_exception, "signature variable length component size greater than subjective maximum");

      auto check = fc::crypto::public_key( s, digest, false );
      EOS_ASSERT( check == p, crypto_api_exception, "Error expected key different than recovered key" );
   }

   int32_t interface::recover_key( const fc::sha256& digest,
                                   legacy_array_ptr<char> sig, uint32_t siglen,
                                   legacy_array_ptr<char> pub, uint32_t publen ) {
      fc::crypto::signature s;
      datastream<const char*> ds( sig, siglen );
      fc::raw::unpack(ds, s);

      EOS_ASSERT(s.which() < ctx.db.get<protocol_state_object>().num_supported_key_types, unactivated_signature_type,
                 "Unactivated signature type used during recover_key");

      if(ctx.control.is_producing_block())
         EOS_ASSERT(s.variable_size() <= ctx.control.configured_subjective_signature_length_limit(),
                    sig_variable_size_limit_exception, "signature variable length component size greater than subjective maximum");


      auto recovered = fc::crypto::public_key(s, digest, false);

      // the key types newer than the first 2 may be varible in length
      if (s.which() >= config::genesis_num_supported_key_types ) {
         EOS_ASSERT(publen >= 33, wasm_execution_error,
                    "destination buffer must at least be able to hold an ECC public key");
         auto packed_pubkey = fc::raw::pack(recovered);
         auto copy_size = std::min<size_t>(publen, packed_pubkey.size());
         memcpy(pub, packed_pubkey.data(), copy_size);
         return packed_pubkey.size();
      } else {
         // legacy behavior, key types 0 and 1 always pack to 33 bytes.
         // this will do one less copy for those keys while maintaining the rules of
         //    [0..33) dest sizes: assert (asserts in fc::raw::pack)
         //    [33..inf) dest sizes: return packed size (always 33)
         datastream<char*> out_ds( pub, publen );
         fc::raw::pack(out_ds, recovered);
         return out_ds.tellp();
      }
   }

   void interface::assert_sha256(legacy_array_ptr<char> data, uint32_t datalen, const fc::sha256& hash_val) {
      auto result = encode<fc::sha256::encoder>( data, datalen );
      EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
   }

   void interface::assert_sha1(legacy_array_ptr<char> data, uint32_t datalen, const fc::sha1& hash_val) {
      auto result = encode<fc::sha1::encoder>( data, datalen );
      EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
   }

   void interface::assert_sha512(legacy_array_ptr<char> data, uint32_t datalen, const fc::sha512& hash_val) {
      auto result = encode<fc::sha512::encoder>( data, datalen );
      EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
   }

   void interface::assert_ripemd160(legacy_array_ptr<char> data, uint32_t datalen, const fc::ripemd160& hash_val) {
      auto result = encode<fc::ripemd160::encoder>( data, datalen );
      EOS_ASSERT( result == hash_val, crypto_api_exception, "hash mismatch" );
   }

   void interface::sha1(legacy_array_ptr<char> data, uint32_t datalen, fc::sha1& hash_val) {
      hash_val = encode<fc::sha1::encoder>( data, datalen );
   }

   void interface::sha256(legacy_array_ptr<char> data, uint32_t datalen, fc::sha256& hash_val) {
      hash_val = encode<fc::sha256::encoder>( data, datalen );
   }

   void interface::sha512(legacy_array_ptr<char> data, uint32_t datalen, fc::sha512& hash_val) {
      hash_val = encode<fc::sha512::encoder>( data, datalen );
   }

   void interface::ripemd160(legacy_array_ptr<char> data, uint32_t datalen, fc::ripemd160& hash_val) {
      hash_val = encode<fc::ripemd160::encoder>( data, datalen );
   }
}}} // ns eosio::chain::webassembly
