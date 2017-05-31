#pragma once
#include <fc/crypto/openssl.hpp>
#include <vector>
#include <stdint.h>

namespace fc {

    struct diffie_hellman {
        diffie_hellman():valid(0),g(5){ fc::init_openssl(); }
        bool generate_params( int s, uint8_t g );
        bool generate_pub_key();
        bool compute_shared_key( const char* buf, uint32_t s );
        bool compute_shared_key( const std::vector<char>& pubk);
        bool validate();

        std::vector<char> p;
        std::vector<char> pub_key;
        std::vector<char> priv_key;
        std::vector<char> shared_key;
        bool             valid;
        uint8_t          g; 
    };

} // namespace fc


