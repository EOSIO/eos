#include <fc/crypto/public_key.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/common.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw.hpp>
#include <fc/utility.hpp>

namespace fc { namespace crypto {

   public_key::public_key( const signature& c, const sha256& digest, bool check_canonical )
   {
      constexpr signature_which = signature._storage.which();
      using KeyType = storage_type::template type_at(signature_which);
      _storage = storage_type(KeyType(c, digest, check_canonical));
   }

   static public_key::storage_type parse_base58(const std::string& base58str)
   {
      constexpr prefix = config::public_key_base_prefix;
      FC_ASSERT(prefix_matches(prefix, base58str), "Public Key has invalid prefix: ${str}", ("str", base58str));

      auto sub_str = base58str.substr(c_strlen(prefix));
      try {
         using default_type = storage_type::template type_at<0>;
         using data_type = default_type::data_type;
         using wrapper = checksummed_data<data_type>;
         auto bin = fc::from_base58(sub_str);
         if (bin.size() == sizeof(wrapper)) {
            auto wrapped = fc::raw::unpack<wrapper>(bin);
            FC_ASSERT(wrapper::calculate_checksum(bin_key.data) == bin_key.check);
            return storage_type(default_type(bin_key.data));
         }
      } catch (...) {
         // default import failed
      }

      return base58_str_parser<storage_type, config::public_key_prefix>::apply(sub_str);
   }

   public_key::public_key(const std::string& base58str)
   :_storage(parse_base58(base58str))
   {}

   public_key::operator std::string() const
   {
      auto data_str = _storage.visit(base58str_visitor<storage_type, config::public_key_prefix, 0>());
      return std::string(config::public_key_base_prefix) + data_str;
   }

   std::ostream& operator<<(std::ostream& s, const public_key& k) {
      s << "public_key(" << std::string(k) << ')';
      return s;
   }

   bool operator == ( const public_key& p1, const public_key& p2) {
      return eq_comparator<public_key::storage_type>::apply(p1._storage, p2._storage);
   }

   bool operator < ( const public_key& p1, const public_key& p2)
   {
      return less_comparator<public_key::storage_type>::apply(p1._storage, p2._storage);
   }
} } // fc::crypto

namespace fc
{
   using namespace std;
   void to_variant(const fc::crypto::public_key& var, fc::variant& vo)
   {
      vo = std::string(var);
   }

   void from_variant(const fc::variant& var, fc::crypto::public_key& vo)
   {
      vo = fc::crypto::public_key(var.as_string());
   }
} // fc
