/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <eos/chain/config.hpp>
#include <eos/chain/protocol/types.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw.hpp>

namespace eos { namespace chain {

    public_key_type::public_key_type():key_data(){};

    public_key_type::public_key_type(const fc::ecc::public_key_data& data)
        :key_data( data ) {};

    public_key_type::public_key_type(const fc::ecc::public_key& pubkey)
        :key_data( pubkey ) {};

    public_key_type::public_key_type(const std::string& base58str)
    {
      // TODO:  Refactor syntactic checks into static is_valid()
      //        to make public_key_type API more similar to address API
       std::string prefix( EOS_KEY_PREFIX );

       const size_t prefix_len = prefix.size();
       FC_ASSERT(base58str.size() > prefix_len);
       FC_ASSERT(base58str.substr(0, prefix_len) ==  prefix , "", ("base58str", base58str));
       auto bin = fc::from_base58(base58str.substr(prefix_len));
       auto bin_key = fc::raw::unpack<binary_key>(bin);
       key_data = bin_key.data;
       FC_ASSERT(fc::ripemd160::hash(key_data.data, key_data.size())._hash[0] == bin_key.check);
    }

    public_key_type::operator fc::ecc::public_key_data() const
    {
       return key_data;
    };

    public_key_type::operator fc::ecc::public_key() const
    {
       return fc::ecc::public_key(key_data);
    };

    public_key_type::operator std::string() const
    {
       binary_key k;
       k.data = key_data;
       k.check = fc::ripemd160::hash( k.data.data, k.data.size() )._hash[0];
       auto data = fc::raw::pack( k );
       return EOS_KEY_PREFIX + fc::to_base58( data.data(), data.size() );
    }

    bool operator == (const public_key_type& p1, const fc::ecc::public_key& p2)
    {
       return p1.key_data == p2.serialize();
    }

    bool operator == (const public_key_type& p1, const public_key_type& p2)
    {
       return p1.key_data == p2.key_data;
    }

    bool operator != (const public_key_type& p1, const public_key_type& p2)
    {
       return p1.key_data != p2.key_data;
    }
    
    bool operator <(const public_key_type& p1, const public_key_type& p2)
    {
        return p1.key_data < p2.key_data;
    };


} } // eos::chain

namespace fc
{
    using namespace std;
    void to_variant(const eos::chain::public_key_type& var,  fc::variant& vo)
    {
        vo = std::string(var);
    }

    void from_variant(const fc::variant& var,  eos::chain::public_key_type& vo)
    {
        vo = eos::chain::public_key_type(var.as_string());
    }
    
} // fc
