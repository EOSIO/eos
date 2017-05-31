#pragma once
#include <fc/crypto/bigint.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/fwd.hpp>
#include <fc/array.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc {

  namespace ecc {
    namespace detail
    {
      class public_key_impl;
      class private_key_impl;
    }

    typedef fc::sha256                  blind_factor_type;
    typedef fc::array<char,33>          commitment_type;
    typedef fc::array<char,33>          public_key_data;
    typedef fc::sha256                  private_key_secret;
    typedef fc::array<char,65>          public_key_point_data; ///< the full non-compressed version of the ECC point
    typedef fc::array<char,72>          signature;
    typedef fc::array<unsigned char,65> compact_signature;
    typedef std::vector<char>           range_proof_type;
    typedef fc::array<char,78>          extended_key_data;
    typedef fc::sha256                  blinded_hash;
    typedef fc::sha256                  blind_signature;

    /**
     *  @class public_key
     *  @brief contains only the public point of an elliptic curve key.
     */
    class public_key
    {
        public:
           public_key();
           public_key(const public_key& k);
           ~public_key();
//           bool verify( const fc::sha256& digest, const signature& sig );
           public_key_data serialize()const;
           public_key_point_data serialize_ecc_point()const;

           operator public_key_data()const { return serialize(); }


           public_key( const public_key_data& v );
           public_key( const public_key_point_data& v );
           public_key( const compact_signature& c, const fc::sha256& digest, bool check_canonical = true );

           public_key child( const fc::sha256& offset )const;

           bool valid()const;
           /** Computes new pubkey = generator * offset + old pubkey ?! */
//           public_key mult( const fc::sha256& offset )const;
           /** Computes new pubkey = regenerate(offset).pubkey + old pubkey
            *                      = offset * G + 1 * old pubkey ?! */
           public_key add( const fc::sha256& offset )const;

           public_key( public_key&& pk );
           public_key& operator=( public_key&& pk );
           public_key& operator=( const public_key& pk );

           inline friend bool operator==( const public_key& a, const public_key& b )
           {
            return a.serialize() == b.serialize();
           }
           inline friend bool operator!=( const public_key& a, const public_key& b )
           {
            return a.serialize() != b.serialize();
           }

           /// Allows to convert current public key object into base58 number.
           std::string to_base58() const;
           static std::string to_base58( const public_key_data &key );
           static public_key from_base58( const std::string& b58 );

           unsigned int fingerprint() const;

        private:
          friend class private_key;
          static public_key from_key_data( const public_key_data& v );
          static bool is_canonical( const compact_signature& c );
          fc::fwd<detail::public_key_impl,33> my;
    };

    /**
     *  @class private_key
     *  @brief an elliptic curve private key.
     */
    class private_key
    {
        public:
           private_key();
           private_key( private_key&& pk );
           private_key( const private_key& pk );
           ~private_key();

           private_key& operator=( private_key&& pk );
           private_key& operator=( const private_key& pk );

           static private_key generate();
           static private_key regenerate( const fc::sha256& secret );

           private_key child( const fc::sha256& offset )const;

           /**
            *  This method of generation enables creating a new private key in a deterministic manner relative to
            *  an initial seed.   A public_key created from the seed can be multiplied by the offset to calculate
            *  the new public key without having to know the private key.
            */
           static private_key generate_from_seed( const fc::sha256& seed, const fc::sha256& offset = fc::sha256() );

           private_key_secret get_secret()const; // get the private key secret

           operator private_key_secret ()const { return get_secret(); }

           /**
            *  Given a public key, calculatse a 512 bit shared secret between that
            *  key and this private key.
            */
           fc::sha512 get_shared_secret( const public_key& pub )const;

//           signature         sign( const fc::sha256& digest )const;
           compact_signature sign_compact( const fc::sha256& digest, bool require_canonical = true )const;
//           bool              verify( const fc::sha256& digest, const signature& sig );

           public_key get_public_key()const;

           inline friend bool operator==( const private_key& a, const private_key& b )
           {
            return a.get_secret() == b.get_secret();
           }
           inline friend bool operator!=( const private_key& a, const private_key& b )
           {
            return a.get_secret() != b.get_secret();
           }
           inline friend bool operator<( const private_key& a, const private_key& b )
           {
            return a.get_secret() < b.get_secret();
           }

           unsigned int fingerprint() const { return get_public_key().fingerprint(); }

        private:
           private_key( EC_KEY* k );
           static fc::sha256 get_secret( const EC_KEY * const k );
           fc::fwd<detail::private_key_impl,32> my;
    };

    class extended_public_key : public public_key
    {
        public:
            extended_public_key( const public_key& k, const sha256& c,
                                 int child = 0, int parent_fp = 0, uint8_t depth = 0 );

            extended_public_key derive_child( int i ) const;
            extended_public_key derive_normal_child( int i ) const;

            extended_key_data serialize_extended() const;
            static extended_public_key deserialize( const extended_key_data& data );
            fc::string str() const;
            fc::string to_base58() const { return str(); }
            static extended_public_key from_base58( const fc::string& base58 );

            public_key generate_p( int i ) const;
            public_key generate_q( int i ) const;
        private:
            sha256 c;
            int child_num, parent_fp;
            uint8_t depth;
    };

    class extended_private_key : public private_key
    {
        public:
            extended_private_key( const private_key& k, const sha256& c,
                                  int child = 0, int parent_fp = 0, uint8_t depth = 0 );

            extended_public_key get_extended_public_key()const;

            extended_private_key derive_child( int i ) const;
            extended_private_key derive_normal_child( int i ) const;
            extended_private_key derive_hardened_child( int i ) const;

            extended_key_data serialize_extended() const;
            static extended_private_key deserialize( const extended_key_data& data );
            fc::string str() const;
            fc::string to_base58() const { return str(); }
            static extended_private_key from_base58( const fc::string& base58 );
            static extended_private_key generate_master( const fc::string& seed );
            static extended_private_key generate_master( const char* seed, uint32_t seed_len );

            // Oleg Andreev's blind signature scheme,
            // see http://blog.oleganza.com/post/77474860538/blind-signatures
            public_key blind_public_key( const extended_public_key& bob, int i ) const;
            blinded_hash blind_hash( const fc::sha256& hash, int i ) const;
            blind_signature blind_sign( const blinded_hash& hash, int i ) const;
            // WARNING! This may produce non-canonical signatures!
            compact_signature unblind_signature( const extended_public_key& bob,
                                                 const blind_signature& sig,
                                                 const fc::sha256& hash, int i ) const;

        private:
            extended_private_key private_derive_rest( const fc::sha512& hash,
                                                      int num ) const;
            private_key generate_a( int i ) const;
            private_key generate_b( int i ) const;
            private_key generate_c( int i ) const;
            private_key generate_d( int i ) const;
            private_key_secret compute_p( int i ) const;
            private_key_secret compute_q( int i, const private_key_secret& p ) const;
            sha256 c;
            int child_num, parent_fp;
            uint8_t depth;
    };

     struct range_proof_info
     {
         int          exp;
         int          mantissa;
         uint64_t     min_value;
         uint64_t     max_value;
     };

     commitment_type   blind( const blind_factor_type& blind, uint64_t value );
     blind_factor_type blind_sum( const std::vector<blind_factor_type>& blinds, uint32_t non_neg );
     /**  verifies taht commnits + neg_commits + excess == 0 */
     bool            verify_sum( const std::vector<commitment_type>& commits, const std::vector<commitment_type>& neg_commits, int64_t excess );
     bool            verify_range( uint64_t& min_val, uint64_t& max_val, const commitment_type& commit, const range_proof_type& proof );

     range_proof_type range_proof_sign( uint64_t min_value,
                                       const commitment_type& commit,
                                       const blind_factor_type& commit_blind,
                                       const blind_factor_type& nonce,
                                       int8_t base10_exp,
                                       uint8_t min_bits,
                                       uint64_t actual_value
                                     );

     bool            verify_range_proof_rewind( blind_factor_type& blind_out,
                                          uint64_t& value_out,
                                          string& message_out,
                                          const blind_factor_type& nonce,
                                          uint64_t& min_val,
                                          uint64_t& max_val,
                                          commitment_type commit,
                                          const range_proof_type& proof );
     range_proof_info range_get_info( const range_proof_type& proof );



  } // namespace ecc
  void to_variant( const ecc::private_key& var,  variant& vo );
  void from_variant( const variant& var,  ecc::private_key& vo );
  void to_variant( const ecc::public_key& var,  variant& vo );
  void from_variant( const variant& var,  ecc::public_key& vo );

  namespace raw
  {
      template<typename Stream>
      void unpack( Stream& s, fc::ecc::public_key& pk)
      {
          ecc::public_key_data ser;
          fc::raw::unpack(s,ser);
          pk = fc::ecc::public_key( ser );
      }

      template<typename Stream>
      void pack( Stream& s, const fc::ecc::public_key& pk)
      {
          fc::raw::pack( s, pk.serialize() );
      }

      template<typename Stream>
      void unpack( Stream& s, fc::ecc::private_key& pk)
      {
          fc::sha256 sec;
          unpack( s, sec );
          pk = ecc::private_key::regenerate(sec);
      }

      template<typename Stream>
      void pack( Stream& s, const fc::ecc::private_key& pk)
      {
          fc::raw::pack( s, pk.get_secret() );
      }

  } // namespace raw

} // namespace fc
#include <fc/reflect/reflect.hpp>

FC_REFLECT_TYPENAME( fc::ecc::private_key )
FC_REFLECT_TYPENAME( fc::ecc::public_key )
FC_REFLECT( fc::ecc::range_proof_info, (exp)(mantissa)(min_value)(max_value) )
