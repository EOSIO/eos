#pragma once
#include <fc/crypto/bigint.hpp>
#include <fc/crypto/common.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/fwd.hpp>
#include <fc/array.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc {

  namespace crypto { namespace r1 {
    namespace detail
    {
      class public_key_impl;
      class private_key_impl;
    }

    typedef fc::array<char,33>          public_key_data;
    typedef fc::sha256                  private_key_secret;
    typedef fc::array<char,65>          public_key_point_data; ///< the full non-compressed version of the ECC point
    typedef fc::array<char,72>          signature;
    typedef fc::array<unsigned char,65> compact_signature;

    int ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig, const unsigned char *msg, int msglen, int recid, int check);

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
           bool verify( const fc::sha256& digest, const signature& sig );
           public_key_data serialize()const;
           public_key_point_data serialize_ecc_point()const;

           operator public_key_data()const { return serialize(); }


           public_key( const public_key_data& v );
           public_key( const public_key_point_data& v );
           public_key( const compact_signature& c, const fc::sha256& digest, bool check_canonical = true );

           bool valid()const;
           public_key mult( const fc::sha256& offset );
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
           static public_key from_base58( const std::string& b58 );

        private:
          friend class private_key;
          friend compact_signature signature_from_ecdsa(const EC_KEY* key, const public_key_data& pub_data, fc::ecdsa_sig& sig, const fc::sha256& d);
          fc::fwd<detail::public_key_impl,8> my;
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

           signature         sign( const fc::sha256& digest )const;
           compact_signature sign_compact( const fc::sha256& digest )const;
           bool              verify( const fc::sha256& digest, const signature& sig );

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

        private:
           fc::fwd<detail::private_key_impl,8> my;
    };

     /**
       * Shims
       */
     struct public_key_shim : public crypto::shim<public_key_data> {
        using crypto::shim<public_key_data>::shim;

        bool valid()const {
           return public_key(_data).valid();
        }
     };

     struct signature_shim : public crypto::shim<compact_signature> {
        using public_key_type = public_key_shim;
        using crypto::shim<compact_signature>::shim;

        public_key_type recover(const sha256& digest, bool check_canonical) const {
           return public_key_type(public_key(_data, digest, check_canonical).serialize());
        }
     };

     struct private_key_shim : public crypto::shim<private_key_secret> {
        using crypto::shim<private_key_secret>::shim;
        using signature_type = signature_shim;
        using public_key_type = public_key_shim;

        signature_type sign( const sha256& digest, bool require_canonical = true ) const
        {
           return signature_type(private_key::regenerate(_data).sign_compact(digest));
        }

        public_key_type get_public_key( ) const
        {
           return public_key_type(private_key::regenerate(_data).get_public_key().serialize());
        }

        sha512 generate_shared_secret( const public_key_type &pub_key ) const
        {
           return private_key::regenerate(_data).get_shared_secret(public_key(pub_key.serialize()));
        }

        static private_key_shim generate()
        {
           return private_key_shim(private_key::generate().get_secret());
        }
     };

     //key here is just an optimization for getting the curve's parameters from an already constructed curve
     compact_signature signature_from_ecdsa(const EC_KEY* key, const public_key_data& pub, fc::ecdsa_sig& sig, const fc::sha256& d);

  } // namespace r1
  } // namespace crypto
  void to_variant( const crypto::r1::private_key& var,  variant& vo );
  void from_variant( const variant& var,  crypto::r1::private_key& vo );
  void to_variant( const crypto::r1::public_key& var,  variant& vo );
  void from_variant( const variant& var,  crypto::r1::public_key& vo );

  namespace raw
  {
      template<typename Stream>
      void unpack( Stream& s, fc::crypto::r1::public_key& pk)
      {
          crypto::r1::public_key_data ser;
          fc::raw::unpack(s,ser);
          pk = fc::crypto::r1::public_key( ser );
      }

      template<typename Stream>
      void pack( Stream& s, const fc::crypto::r1::public_key& pk)
      {
          fc::raw::pack( s, pk.serialize() );
      }

      template<typename Stream>
      void unpack( Stream& s, fc::crypto::r1::private_key& pk)
      {
          fc::sha256 sec;
          unpack( s, sec );
          pk = crypto::r1::private_key::regenerate(sec);
      }

      template<typename Stream>
      void pack( Stream& s, const fc::crypto::r1::private_key& pk)
      {
          fc::raw::pack( s, pk.get_secret() );
      }

  } // namespace raw

} // namespace fc
#include <fc/reflect/reflect.hpp>

FC_REFLECT_TYPENAME( fc::crypto::r1::private_key )
FC_REFLECT_TYPENAME( fc::crypto::r1::public_key )
FC_REFLECT_DERIVED( fc::crypto::r1::public_key_shim, (fc::crypto::shim<fc::crypto::r1::public_key_data>), BOOST_PP_SEQ_NIL )
FC_REFLECT_DERIVED( fc::crypto::r1::signature_shim, (fc::crypto::shim<fc::crypto::r1::compact_signature>), BOOST_PP_SEQ_NIL )
FC_REFLECT_DERIVED( fc::crypto::r1::private_key_shim, (fc::crypto::shim<fc::crypto::r1::private_key_secret>), BOOST_PP_SEQ_NIL )
