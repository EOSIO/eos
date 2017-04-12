#include <eos/types/native.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/io/raw.hpp>

namespace EOS {

    PublicKey::PublicKey():key_data(){};

    PublicKey::PublicKey( const fc::ecc::public_key_data& data )
        :key_data( data ) {};

    PublicKey::PublicKey( const fc::ecc::public_key& pubkey )
        :key_data( pubkey ) {};

    PublicKey::PublicKey( const std::string& base58str )
    {
      // TODO:  Refactor syntactic checks into static is_valid()
      //        to make PublicKey API more similar to address API
       std::string prefix( "EOS" );

       const size_t prefix_len = prefix.size();
       FC_ASSERT( base58str.size() > prefix_len );
       FC_ASSERT( base58str.substr( 0, prefix_len ) ==  prefix , "", ("base58str", base58str) );
       auto bin = fc::from_base58( base58str.substr( prefix_len ) );
       auto bin_key = fc::raw::unpack<BinaryKey>(bin);
       key_data = bin_key.data;
       FC_ASSERT( fc::ripemd160::hash( key_data.data, key_data.size() )._hash[0] == bin_key.check );
    };


    PublicKey::operator fc::ecc::public_key_data() const
    {
       return key_data;
    };

    PublicKey::operator fc::ecc::public_key() const
    {
       return fc::ecc::public_key( key_data );
    };

    PublicKey::operator std::string() const
    {
       BinaryKey k;
       k.data = key_data;
       k.check = fc::ripemd160::hash( k.data.data, k.data.size() )._hash[0];
       auto data = fc::raw::pack( k );
       return "EOS" + fc::to_base58( data.data(), data.size() );
    }

    bool operator == ( const PublicKey& p1, const fc::ecc::public_key& p2)
    {
       return p1.key_data == p2.serialize();
    }

    bool operator == ( const PublicKey& p1, const PublicKey& p2)
    {
       return p1.key_data == p2.key_data;
    }

    bool operator != ( const PublicKey& p1, const PublicKey& p2)
    {
       return p1.key_data != p2.key_data;
    }

}
namespace fc {
  void to_variant( const std::vector<EOS::Field>& c, fc::variant& v ) {
     fc::mutable_variant_object mvo; mvo.reserve( c.size() );
     for( const auto& f : c ) {
        mvo.set( f.name, f.type );
     }
     v = std::move(mvo);
  }
  void from_variant( const fc::variant& v, std::vector<EOS::Field>& fields ) {
     const auto& obj = v.get_object();
     fields.reserve( obj.size() );
     for( const auto& f : obj  )
        fields.emplace_back( EOS::Field{ f.key(), f.value().get_string() } );
  }
  void to_variant( const std::map<std::string,EOS::Struct>& c, fc::variant& v ) 
  {
     fc::mutable_variant_object mvo; mvo.reserve( c.size() );
     for( const auto& item : c ) {
        if( item.second.base.size() )
           mvo.set( item.first, fc::mutable_variant_object("base",item.second.base)("fields",item.second.fields) );
        else
           mvo.set( item.first, fc::mutable_variant_object("fields",item.second.fields) );
     }
     v = std::move(mvo);
  }
  void from_variant( const fc::variant& v, std::map<std::string,EOS::Struct>& structs ) {
     const auto& obj = v.get_object();
     structs.clear();
     for( const auto& f : obj ) {
        auto& stru = structs[f.key()];
        if( f.value().get_object().contains( "fields" ) )
           fc::from_variant( f.value().get_object()["fields"], stru.fields );
        if( f.value().get_object().contains( "base" ) )
           fc::from_variant( f.value().get_object()["base"], stru.base );
        stru.name = f.key();
     }
  }
}
