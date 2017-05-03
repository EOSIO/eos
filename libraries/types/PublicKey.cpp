#include <eos/types/PublicKey.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw.hpp>

#define KEY_PREFIX "EOS"

namespace eos { namespace types {

    PublicKey::PublicKey():key_data(){};

    PublicKey::PublicKey(const fc::ecc::public_key_data& data)
        :key_data( data ) {};

    PublicKey::PublicKey(const fc::ecc::public_key& pubkey)
        :key_data( pubkey ) {};

    PublicKey::PublicKey(const std::string& base58str)
    {
      // TODO:  Refactor syntactic checks into static is_valid()
      //        to make PublicKey API more similar to address API
       std::string prefix( KEY_PREFIX );

       const size_t prefix_len = prefix.size();
       FC_ASSERT(base58str.size() > prefix_len);
       FC_ASSERT(base58str.substr(0, prefix_len) ==  prefix , "", ("base58str", base58str));
       auto bin = fc::from_base58(base58str.substr(prefix_len));
       auto bin_key = fc::raw::unpack<BinaryKey>(bin);
       key_data = bin_key.data;
       FC_ASSERT(fc::ripemd160::hash(key_data.data, key_data.size())._hash[0] == bin_key.check);
    }

    PublicKey::operator fc::ecc::public_key_data() const
    {
       return key_data;
    };

    PublicKey::operator fc::ecc::public_key() const
    {
       return fc::ecc::public_key(key_data);
    };

    PublicKey::operator std::string() const
    {
       BinaryKey k;
       k.data = key_data;
       k.check = fc::ripemd160::hash( k.data.data, k.data.size() )._hash[0];
       auto data = fc::raw::pack( k );
       return KEY_PREFIX + fc::to_base58( data.data(), data.size() );
    }

    bool operator == (const PublicKey& p1, const fc::ecc::public_key& p2)
    {
       return p1.key_data == p2.serialize();
    }

    bool operator == (const PublicKey& p1, const PublicKey& p2)
    {
       return p1.key_data == p2.key_data;
    }

    bool operator != (const PublicKey& p1, const PublicKey& p2)
    {
       return p1.key_data != p2.key_data;
    }
    
    bool operator <(const PublicKey& p1, const PublicKey& p2)
    {
        return p1.key_data < p2.key_data;
    };

    std::ostream& operator<<(std::ostream& s, const PublicKey& k) {
       s << "PublicKey(" << std::string(k) << ')';
       return s;
    }

}} // eos::types

namespace fc
{
    using namespace std;
    void to_variant(const eos::types::PublicKey& var, fc::variant& vo)
    {
        vo = std::string(var);
    }

    void from_variant(const fc::variant& var, eos::types::PublicKey& vo)
    {
        vo = eos::types::PublicKey(var.as_string());
    }
    
} // fc
