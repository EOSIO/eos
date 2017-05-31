#include <fc/crypto/dh.hpp>
#include <openssl/dh.h>

namespace fc {
    SSL_TYPE(ssl_dh, DH, DH_free)

    static bool validate( const ssl_dh& dh, bool& valid ) {
        int check;
        DH_check(dh,&check);
        return valid = !(check /*& DH_CHECK_P_NOT_SAFE_PRIME*/);
    }

   bool diffie_hellman::generate_params( int s, uint8_t g )
   {
        ssl_dh dh = DH_generate_parameters( s, g, NULL, NULL );
        p.resize( BN_num_bytes( dh->p ) );
        if( p.size() )
            BN_bn2bin( dh->p, (unsigned char*)&p.front()  );
        this->g = g;
        return fc::validate( dh, valid );
   }

   bool diffie_hellman::validate()
   {
        if( !p.size() ) 
            return valid = false;
        ssl_dh dh = DH_new();
        dh->p = BN_bin2bn( (unsigned char*)&p.front(), p.size(), NULL );
        dh->g = BN_bin2bn( (unsigned char*)&g, 1, NULL );
        return fc::validate( dh, valid );
   }

   bool diffie_hellman::generate_pub_key()
   {
        if( !p.size() ) 
            return valid = false;
        ssl_dh dh = DH_new();
        dh->p = BN_bin2bn( (unsigned char*)&p.front(), p.size(), NULL );
        dh->g = BN_bin2bn( (unsigned char*)&g, 1, NULL );

        if( !fc::validate( dh, valid ) )
        {
            return false;
        }
        DH_generate_key(dh);

        pub_key.resize( BN_num_bytes( dh->pub_key ) );
        priv_key.resize( BN_num_bytes( dh->priv_key ) );
        if( pub_key.size() )
            BN_bn2bin( dh->pub_key, (unsigned char*)&pub_key.front()  );
        if( priv_key.size() )
            BN_bn2bin( dh->priv_key, (unsigned char*)&priv_key.front()  );

        return true;
   }
   bool diffie_hellman::compute_shared_key( const char* buf, uint32_t s ) {
        ssl_dh dh = DH_new();
        dh->p = BN_bin2bn( (unsigned char*)&p.front(), p.size(), NULL );
        dh->pub_key = BN_bin2bn( (unsigned char*)&pub_key.front(), pub_key.size(), NULL );
        dh->priv_key = BN_bin2bn( (unsigned char*)&priv_key.front(), priv_key.size(), NULL );
        dh->g = BN_bin2bn( (unsigned char*)&g, 1, NULL );

        int check;
        DH_check(dh,&check);
        if( !fc::validate( dh, valid ) )
        {
            return false;
        }

        ssl_bignum pk;
        BN_bin2bn( (unsigned char*)buf, s, pk );
        shared_key.resize( DH_size(dh) ); 
        DH_compute_key( (unsigned char*)&shared_key.front(), pk, dh );

        return true;
   }
   bool diffie_hellman::compute_shared_key( const std::vector<char>& pubk ) {
      return compute_shared_key( &pubk.front(), pubk.size() );
   }
}
