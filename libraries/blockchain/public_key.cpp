#include <eosio/blockchain/public_key.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw.hpp>


namespace eosio { namespace blockchain {

    public_key::public_key():key_data(){};

    public_key::public_key(const fc::ecc::public_key_data& data)
        :key_data( data ) {};

    public_key::public_key(const fc::ecc::public_key& pubkey)
        :key_data( pubkey ) {};

    public_key::public_key(const std::string& base58str)
    {
      // TODO:  Refactor syntactic checks into static is_valid()
      //        to make public_key API more similar to address API
       std::string prefix( config::public_key_prefix );

       const size_t prefix_len = prefix.size();
       FC_ASSERT(base58str.size() > prefix_len);
       FC_ASSERT(base58str.substr(0, prefix_len) ==  prefix , "", ("base58str", base58str));
       auto bin = fc::from_base58(base58str.substr(prefix_len));
       auto bin_key = fc::raw::unpack<binary_key>(bin);
       key_data = bin_key.data;
       FC_ASSERT(fc::ripemd160::hash(key_data.data, key_data.size())._hash[0] == bin_key.check);
    }

    public_key::operator fc::ecc::public_key_data() const
    {
       return key_data;
    };

    public_key::operator fc::ecc::public_key() const
    {
       return fc::ecc::public_key(key_data);
    };

    public_key::operator std::string() const
    {
       binary_key k;
       k.data = key_data;
       k.check = fc::ripemd160::hash( k.data.data, k.data.size() )._hash[0];
       auto data = fc::raw::pack( k );
       return config::public_key_prefix + fc::to_base58( data.data(), data.size() );
    }

    bool operator == (const public_key& p1, const fc::ecc::public_key& p2)
    {
       return p1.key_data == p2.serialize();
    }

    bool operator == (const public_key& p1, const public_key& p2)
    {
       return p1.key_data == p2.key_data;
    }

    bool operator != (const public_key& p1, const public_key& p2)
    {
       return p1.key_data != p2.key_data;
    }
    
    bool operator <(const public_key& p1, const public_key& p2)
    {
        return p1.key_data < p2.key_data;
    };

    std::ostream& operator<<(std::ostream& s, const public_key& k) {
       s << "public_key(" << std::string(k) << ')';
       return s;
    }

} } // eosio::blockchain

namespace fc
{
    using namespace std;
    void to_variant(const eosio::blockchain::public_key& var, fc::variant& vo)
    {
        vo = std::string(var);
    }

    void from_variant(const fc::variant& var, eosio::blockchain::public_key& vo)
    {
        vo = eosio::blockchain::public_key(var.as_string());
    }
    
} // fc
