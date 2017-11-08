#include <fc/crypto/private_key.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/io/raw.hpp>
#include <fc/utility.hpp>

namespace fc { namespace crypto {
   struct public_key_visitor : visitor<public_key> {
      template<typename KeyType>
      public_key apply(const KeyType& key)
      {
         return public_key(public_key::storage_type(key.get_public_key()));
      }
   };

   public_key private_key::get_public_key() const
   {
      return _storage.visit(public_key_visitor());
   }

   struct sign_visitor : visitor<signature> {
      sign_visitor( const sha256& digest, bool require_canonical )
      :_digest(digest)
      ,_require_canonical(require_canonical)
      {}

      template<typename KeyType>
      signature apply(const KeyType& key)
      {
         return signature(signature::storage_type(key.sign(_digest, _require_canonical)));
      }

      const sha256&  _digest;
      bool           _require_canonical;
   };

   signature private_key::sign( const sha256& digest, bool require_canonical ) const
   {
      return _storage.visit(sign_visitor(digest, require_canonical));
   }

   struct generate_shared_secret_visitor : visitor<sha512> {
      sign_visitor( const public_key& pub )
         :_pub(pub)
      {}

      template<typename KeyType>
      signature apply(const KeyType& key)
      {
         return key.sign(_digest, _require_canonical);
      }

      const public_key&  _pub;
   };

   sha512 private_key::generate_shared_secret( const public_key& pub ) const
   {
      return _storage.visit(generate_shared_secret_visitor(pub));
   }

   template<typename Data>
   string to_wif( const Data& secret )
   {
      const size_t size_of_data_to_hash = sizeof(secret) + 1;
      const size_t size_of_hash_bytes = 4;
      char data[size_of_data_to_hash + size_of_hash_bytes];
      data[0] = (char)0x80; // this is the Bitcoin MainNet code
      memcpy(&data[1], (char*)&secret, sizeof(secret));
      sha256 digest = sha256::hash(data, size_of_data_to_hash);
      digest = sha256::hash(digest);
      memcpy(data + size_of_data_to_hash, (char*)&digest, size_of_hash_bytes);
      return to_base58(data, sizeof(data));
   }

   template<typename Data>
   Data from_wif( const string& wif_key )
   {
      auto wif_bytes = from_base58(wif_key);
      FC_ASSERT(wif_bytes.size() >= 5);
      auto key_bytes = vector<char>(wif_bytes.begin() + 1, wif_bytes.end() - 4);
      fc::sha256 check = fc::sha256::hash(wif_bytes.data(), wif_bytes.size() - 4);
      fc::sha256 check2 = fc::sha256::hash(check);

      FC_ASSERT(memcmp( (char*)&check, wif_bytes.data() + wif_bytes.size() - 4, 4 ) == 0 ||
                memcmp( (char*)&check2, wif_bytes.data() + wif_bytes.size() - 4, 4 ) == 0 );

      return fc::variant(key_bytes).as<Data>();
   }

   static private_key::storage_type parse_base58(const string& base58str)
   {
      try {
         // wif import
         using default_type = storage_type::template type_at<0>;
         return storage_type(from_wif<default_type>(base58str));
      } catch (...) {
         // wif import failed
      }

      constexpr prefix = config::private_key_base_prefix;
      FC_ASSERT(prefix_matches(prefix, base58str), "Private Key has invalid prefix: ${str}", ("str", base58str));
      auto sub_str = base58str.substr(c_strlen(prefix));
      return base58_str_parser<private_key::storage_type, config::private_key_prefix>::apply(sub_str);
   }

   private_key::private_key(const std::string& base58str)
   :_storage(parse_base58(base58str))
   {}

   private_key::operator std::string() const
   {
      auto which = _storage.which();

      if (which == 0) {
         using default_type = storage_type::template type_at<0>;
         return to_wif(_storage.template get<default_type>());
      }

      auto data_str = _storage.visit(base58str_visitor<storage_type, config::private_key_prefix>());
      return std::string(config::private_key_base_prefix) + data_str;
   }

   std::ostream& operator<<(std::ostream& s, const private_key& k) {
      s << "private_key(" << std::string(k) << ')';
      return s;
   }

   bool operator == ( const private_key& p1, const private_key& p2) {
      return eq_comparator<private_key::storage_type>::apply(p1._storage, p2._storage);
   }

   bool operator < ( const private_key& p1, const private_key& p2)
   {
      return less_comparator<private_key::storage_type>::apply(p1._storage, p2._storage);
   }
} } // fc::crypto

namespace fc
{
   void to_variant(const fc::crypto::private_key& var, variant& vo)
   {
      vo = string(var);
   }

   void from_variant(const variant& var, fc::crypto::private_key& vo)
   {
      vo = fc::crypto::private_key(var.as_string());
   }

} // fc
