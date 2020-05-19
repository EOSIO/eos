#include <fc/crypto/pke.hpp>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <fc/crypto/base64.hpp>
#include <fc/io/sstream.hpp>
#include <fc/io/stdio.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>

namespace fc {

    namespace detail {
        class pke_impl
        {
           public:
              pke_impl():rsa(nullptr){}
              ~pke_impl()
              {
                 if( rsa != nullptr )
                    RSA_free(rsa);
              }
              RSA* rsa;
        };
    } // detail

    public_key::operator bool()const { return !!my; }
    private_key::operator bool()const { return !!my; }

    public_key::public_key()
    {}

    public_key::public_key( const bytes& d )
    :my( std::make_shared<detail::pke_impl>() )
    {
       string pem = "-----BEGIN RSA PUBLIC KEY-----\n";
       auto b64 = fc::base64_encode( (const unsigned char*)d.data(), d.size() );
       for( size_t i = 0; i < b64.size(); i += 64 )
           pem += b64.substr( i, 64 ) + "\n";
       pem += "-----END RSA PUBLIC KEY-----\n";
    //   fc::cerr<<pem;

       BIO* mem = (BIO*)BIO_new_mem_buf( (void*)pem.c_str(), pem.size() );
       my->rsa = PEM_read_bio_RSAPublicKey(mem, NULL, NULL, NULL );
       BIO_free(mem);
    }
    public_key::public_key( const public_key& k )
    :my(k.my)
    {
    }

    public_key::public_key( public_key&& k )
    :my(std::move(k.my))
    {
    }

    public_key::~public_key() { }

    public_key& public_key::operator=(const public_key&  p )
    {
       my = p.my; return *this;
    }
    public_key& public_key::operator=( public_key&& p )
    {
       my = std::move(p.my); return *this;
    }
    bool public_key::verify( const sha1& digest, const array<char,2048/8>& sig )const
    {
       return 0 != RSA_verify( NID_sha1, (const uint8_t*)&digest, 20,
                               (uint8_t*)&sig, 2048/8, my->rsa );
    }

    bool public_key::verify( const sha1& digest, const signature& sig )const
    {
       static_assert( sig.size() == 2048/8, "" );
       return 0 != RSA_verify( NID_sha1, (const uint8_t*)&digest, 20,
                               (uint8_t*)sig.data(), 2048/8, my->rsa );
    }
    bool public_key::verify( const sha256& digest, const signature& sig )const
    {
       static_assert( sig.size() == 2048/8, "" );
       return 0 != RSA_verify( NID_sha256, (const uint8_t*)&digest, 32,
                               (uint8_t*)sig.data(), 2048/8, my->rsa );
    }
    bytes public_key::encrypt( const char* b, size_t l )const
    {
       FC_ASSERT( my && my->rsa );
       bytes out( RSA_size(my->rsa) ); //, char(0) );
       int rtn = RSA_public_encrypt( l,
                                      (unsigned char*)b,
                                      (unsigned char*)out.data(),
                                      my->rsa, RSA_PKCS1_OAEP_PADDING );
       if( rtn >= 0 ) {
          out.resize(rtn);
          return out;
       }
       FC_THROW_EXCEPTION( exception, "openssl: ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
    }

    bytes public_key::encrypt( const bytes& in )const
    {
       FC_ASSERT( my && my->rsa );
       bytes out( RSA_size(my->rsa) ); //, char(0) );
       int rtn = RSA_public_encrypt( in.size(),
                                      (unsigned char*)in.data(),
                                      (unsigned char*)out.data(),
                                      my->rsa, RSA_PKCS1_OAEP_PADDING );
       fc::cerr<<"rtn: "<<rtn<<"\n";
       if( rtn >= 0 ) {
          out.resize(rtn);
          return out;
       }
       FC_THROW_EXCEPTION( exception, "openssl: ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
    }
    bytes public_key::decrypt( const bytes& in )const
    {
       FC_ASSERT( my && my->rsa );
       bytes out( RSA_size(my->rsa) );//, char(0) );
       int rtn = RSA_public_decrypt( in.size(),
                                      (unsigned char*)in.data(),
                                      (unsigned char*)out.data(),
                                      my->rsa, RSA_PKCS1_OAEP_PADDING );
       if( rtn >= 0 ) {
          out.resize(rtn);
          return out;
       }
       FC_THROW_EXCEPTION( exception, "openssl: ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
    }

    bytes public_key::serialize()const
    {
       bytes ba;
       if( !my ) { return ba; }

       BIO *mem = BIO_new(BIO_s_mem());
       int e = PEM_write_bio_RSAPublicKey( mem, my->rsa );
       if( e != 1 )
       {
           BIO_free(mem);
           FC_THROW_EXCEPTION( exception, "openssl: ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
       }
       char* dat;
       uint32_t l = BIO_get_mem_data( mem, &dat );

       fc::stringstream ss( string( dat, l ) );
       fc::stringstream key;
       fc::string tmp;
       fc::getline( ss, tmp );
       fc::getline( ss, tmp );
       while( tmp.size() && tmp[0] != '-' )
       {
         key << tmp; 
         fc::getline( ss, tmp );
       }
       auto str = key.str();
       str = fc::base64_decode( str );
       ba = bytes( str.begin(), str.end() );

       BIO_free(mem);
       return ba;
    }

    private_key::private_key()
    {
    }
    private_key::private_key( const bytes& d )
    :my( std::make_shared<detail::pke_impl>() )
    {
      
       string pem = "-----BEGIN RSA PRIVATE KEY-----\n";
       auto b64 = fc::base64_encode( (const unsigned char*)d.data(), d.size() );
       for( size_t i = 0; i < b64.size(); i += 64 )
           pem += b64.substr( i, 64 ) + "\n";
       pem += "-----END RSA PRIVATE KEY-----\n";
   //    fc::cerr<<pem;

       BIO* mem = (BIO*)BIO_new_mem_buf( (void*)pem.c_str(), pem.size() );
       my->rsa = PEM_read_bio_RSAPrivateKey(mem, NULL, NULL, NULL );
       BIO_free(mem);

       FC_ASSERT( my->rsa, "read private key" );
    }

    private_key::private_key( const private_key& k )
    :my(k.my)
    {
    }
    private_key::private_key( private_key&& k )
    :my(std::move(k.my) )
    {
    }
    private_key::~private_key() { }

    private_key& private_key::operator=(const private_key&  p )
    {
       my = p.my; return *this;
    }
    private_key& private_key::operator=(private_key&& p )
    {
       my = std::move(p.my); return *this;
    }

    void private_key::sign( const sha1& digest, array<char,2048/8>& sig )const
    {
       FC_ASSERT( (size_t(RSA_size(my->rsa)) <= sizeof(sig)), "Invalid RSA size" ); 
       uint32_t slen = 0;
       if( 1 != RSA_sign( NID_sha1, (uint8_t*)&digest,
                          20, (unsigned char*)&sig, &slen, my->rsa ) )
       {
           FC_THROW_EXCEPTION( exception, "rsa sign failed with ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
       }
    }

    signature  private_key::sign( const sha1& digest )const
    {
       if( !my ) FC_THROW_EXCEPTION( assert_exception, "!null" );
       signature sig;
       sig.resize( RSA_size(my->rsa) );

       uint32_t slen = 0;
       if( 1 != RSA_sign( NID_sha1, (uint8_t*)digest.data(),
                          20, (unsigned char*)sig.data(), &slen, my->rsa ) )
       {
           FC_THROW_EXCEPTION( exception, "rsa sign failed with ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
       }
       return sig;
    }
    signature  private_key::sign( const sha256& digest )const
    {
       if( !my ) FC_THROW_EXCEPTION( assert_exception, "!null" );
       signature sig;
       sig.resize( RSA_size(my->rsa) );

       uint32_t slen = 0;
       if( 1 != RSA_sign( NID_sha256, (uint8_t*)digest.data(),
                          32, (unsigned char*)sig.data(), &slen, my->rsa ) )
       {
          FC_THROW_EXCEPTION( exception, "rsa sign failed with ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
       }
       return sig;
    }


    bytes private_key::encrypt( const bytes& in )const
    {
       if( !my ) FC_THROW_EXCEPTION( assert_exception, "!null" );
       bytes out;
       out.resize( RSA_size(my->rsa) );
       int rtn = RSA_private_encrypt( in.size(),
                                      (unsigned char*)in.data(),
                                      (unsigned char*)out.data(),
                                      my->rsa, RSA_PKCS1_OAEP_PADDING );
       if( rtn >= 0 ) {
          out.resize(rtn);
          return out;
       }

       FC_THROW_EXCEPTION( exception, "encrypt failed" );
    }

    bytes private_key::decrypt( const char* in, size_t l )const
    {
       if( !my ) FC_THROW_EXCEPTION( assert_exception, "!null" );
       bytes out;
       out.resize( RSA_size(my->rsa) );
       int rtn = RSA_private_decrypt( l,
                                      (unsigned char*)in,
                                      (unsigned char*)out.data(),
                                      my->rsa, RSA_PKCS1_OAEP_PADDING );
       if( rtn >= 0 ) {
          out.resize(rtn);
          return out;
       }
       FC_THROW_EXCEPTION( exception, "decrypt failed" );
    }
    bytes private_key::decrypt( const bytes& in )const
    {
       if( !my ) FC_THROW_EXCEPTION( assert_exception, "!null" );
       bytes out;
       out.resize( RSA_size(my->rsa) );
       int rtn = RSA_private_decrypt( in.size(),
                                      (unsigned char*)in.data(),
                                      (unsigned char*)out.data(),
                                      my->rsa, RSA_PKCS1_OAEP_PADDING );
       if( rtn >= 0 ) {
          out.resize(rtn);
          return out;
       }
       FC_THROW_EXCEPTION( exception, "decrypt failed" );
    }

    bytes private_key::serialize()const
    {
       bytes ba;
       if( !my ) { return ba; }

       BIO *mem = BIO_new(BIO_s_mem());
       int e = PEM_write_bio_RSAPrivateKey( mem, my->rsa, NULL, NULL, 0, NULL, NULL );
       if( e != 1 )
       {
           BIO_free(mem);
           FC_THROW_EXCEPTION( exception, "Error writing private key, ${message}", ("message",fc::string(ERR_error_string( ERR_get_error(),NULL))) );
       }
       char* dat;
       uint32_t l = BIO_get_mem_data( mem, &dat );
    //   return bytes( dat, dat + l );

       stringstream ss( string( dat, l ) );
       stringstream key;
       string tmp;
       fc::getline( ss, tmp );
       fc::getline( ss, tmp );

       while( tmp.size() && tmp[0] != '-' )
       {
         key << tmp; 
         fc::getline( ss, tmp );
       }
       auto str = key.str();
       str = fc::base64_decode( str );
       ba = bytes( str.begin(), str.end() );
    //   ba = bytes( dat, dat + l );
       BIO_free(mem);
       return ba;
    }

    void generate_key_pair( public_key& pub, private_key& priv )
    {
       static bool init = true;
       if( init ) { ERR_load_crypto_strings(); init = false; }

       pub.my = std::make_shared<detail::pke_impl>();
       priv.my = pub.my;
       pub.my->rsa = RSA_generate_key( 2048, 65537, NULL, NULL );
    }

  /** encodes the big int as base64 string, or a number */
  void to_variant( const public_key& bi, variant& v )
  {
    v = bi.serialize();
  }

  /** decodes the big int as base64 string, or a number */
  void from_variant( const variant& v, public_key& bi )
  {
    bi = public_key( v.as<std::vector<char> >() ); 
  }


  /** encodes the big int as base64 string, or a number */
  void to_variant( const private_key& bi, variant& v )
  {
    v = bi.serialize();
  }

  /** decodes the big int as base64 string, or a number */
  void from_variant( const variant& v, private_key& bi )
  {
    bi = private_key( v.as<std::vector<char> >() ); 
  }

} // fc
