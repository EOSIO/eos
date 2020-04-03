#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/protocol_state_object.hpp>
#include <eosio/chain/transaction_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   template<class Encoder, class Context> auto encode(Context& context, char* data, uint32_t datalen) {
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

   void interface::assert_recover_key( legacy_ptr<const fc::sha256> digest,
                                       legacy_array_ptr<char> sig,
                                       legacy_array_ptr<char> pub ) const {
      fc::crypto::signature s;
      fc::crypto::public_key p;
      datastream<const char*> ds( sig.ref().data(), sig.ref().size() );
      datastream<const char*> pubds ( pub.ref().data(), pub.ref().size() );

      fc::raw::unpack( ds, s );
      fc::raw::unpack( pubds, p );

      EOS_ASSERT(s.which() < context.db.get<protocol_state_object>().num_supported_key_types, unactivated_signature_type,
        "Unactivated signature type used during assert_recover_key");
      EOS_ASSERT(p.which() < context.db.get<protocol_state_object>().num_supported_key_types, unactivated_key_type,
        "Unactivated key type used when creating assert_recover_key");

      if(context.control.is_producing_block())
         EOS_ASSERT(s.variable_size() <= context.control.configured_subjective_signature_length_limit(),
                    sig_variable_size_limit_exception, "signature variable length component size greater than subjective maximum");

      auto check = fc::crypto::public_key( s, digest.ref(), false );
      EOS_ASSERT( check == p, crypto_api_exception, "Error expected key different than recovered key" );
   }

   int32_t interface::recover_key( legacy_ptr<const fc::sha256> digest,
                                   legacy_array_ptr<char> sig,
                                   legacy_array_ptr<char> pub ) const {
      fc::crypto::signature s;
      datastream<const char*> ds( sig.ref().data(), sig.ref().size() );
      fc::raw::unpack(ds, s);

      EOS_ASSERT(s.which() < context.db.get<protocol_state_object>().num_supported_key_types, unactivated_signature_type,
                 "Unactivated signature type used during recover_key");

      if(context.control.is_producing_block())
         EOS_ASSERT(s.variable_size() <= context.control.configured_subjective_signature_length_limit(),
                    sig_variable_size_limit_exception, "signature variable length component size greater than subjective maximum");


      auto recovered = fc::crypto::public_key(s, digest.ref(), false);

      // the key types newer than the first 2 may be varible in length
      if (s.which() >= config::genesis_num_supported_key_types ) {
         EOS_ASSERT(pub.ref().size() >= 33, wasm_execution_error,
                    "destination buffer must at least be able to hold an ECC public key");
         auto packed_pubkey = fc::raw::pack(recovered);
         auto copy_size = std::min<size_t>(pub.ref().size(), packed_pubkey.size());
         std::memcpy(pub.ref().data(), packed_pubkey.data(), copy_size);
         return packed_pubkey.size();
      } else {
         // legacy behavior, key types 0 and 1 always pack to 33 bytes.
         // this will do one less copy for those keys while maintaining the rules of
         //    [0..33) dest sizes: assert (asserts in fc::raw::pack)
         //    [33..inf) dest sizes: return packed size (always 33)
         datastream<char*> out_ds( pub.ref().data(), pub.ref().size() );
         fc::raw::pack(out_ds, recovered);
         return out_ds.tellp();
      }
   }

   void interface::assert_sha256(legacy_array_ptr<char> data, legacy_ptr<const fc::sha256> hash_val) const {
      auto result = encode<fc::sha256::encoder>( context, data.ref().data(), data.ref().size() );
      EOS_ASSERT( result == hash_val.ref(), crypto_api_exception, "hash mismatch" );
   }

   void interface::assert_sha1(legacy_array_ptr<char> data, legacy_ptr<const fc::sha1> hash_val) const {
      auto result = encode<fc::sha1::encoder>( context, data.ref().data(), data.ref().size() );
      EOS_ASSERT( result == hash_val.ref(), crypto_api_exception, "hash mismatch" );
   }

   void interface::assert_sha512(legacy_array_ptr<char> data, legacy_ptr<const fc::sha512> hash_val) const {
      auto result = encode<fc::sha512::encoder>( context, data.ref().data(), data.ref().size() );
      EOS_ASSERT( result == hash_val.ref(), crypto_api_exception, "hash mismatch" );
   }

   void interface::assert_ripemd160(legacy_array_ptr<char> data, legacy_ptr<const fc::ripemd160> hash_val) const {
      auto result = encode<fc::ripemd160::encoder>( context, data.ref().data(), data.ref().size() );
      EOS_ASSERT( result == hash_val.ref(), crypto_api_exception, "hash mismatch" );
   }

   void interface::sha1(legacy_array_ptr<char> data, legacy_ptr<fc::sha1> hash_val) const {
      hash_val.ref() = encode<fc::sha1::encoder>( context, data.ref().data(), data.ref().size() );
   }

   void interface::sha256(legacy_array_ptr<char> data, legacy_ptr<fc::sha256> hash_val) const {
      hash_val.ref() = encode<fc::sha256::encoder>( context, data.ref().data(), data.ref().size() );
   }

   void interface::sha512(legacy_array_ptr<char> data, legacy_ptr<fc::sha512> hash_val) const {
      hash_val.ref() = encode<fc::sha512::encoder>( context, data.ref().data(), data.ref().size() );
   }

   void interface::ripemd160(legacy_array_ptr<char> data, legacy_ptr<fc::ripemd160> hash_val) const {
      hash_val.ref() = encode<fc::ripemd160::encoder>( context, data.ref().data(), data.ref().size() );
   }
}}} // ns eosio::chain::webassembly
