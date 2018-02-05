#include <fc/crypto/public_key.hpp>
#include <fc/crypto/common.hpp>
#include <fc/exception/exception.hpp>

namespace fc { namespace crypto {

   struct recovery_visitor : fc::visitor<public_key::storage_type> {
      recovery_visitor(const sha256& digest, bool check_canonical)
      :_digest(digest)
      ,_check_canonical(check_canonical)
      {}

      template<typename SignatureType>
      public_key::storage_type operator()(const SignatureType& s) const {
         return public_key::storage_type(s.recover(_digest, _check_canonical));
      }

      const sha256& _digest;
      bool _check_canonical;
   };

   public_key::public_key( const signature& c, const sha256& digest, bool check_canonical )
   :_storage(c._storage.visit(recovery_visitor(digest, check_canonical)))
   {
   }

   static public_key::storage_type parse_base58(const std::string& base58str)
   {
      constexpr auto prefix = config::public_key_base_prefix;
      FC_ASSERT(prefix_matches(prefix, base58str), "Public Key has invalid prefix: ${str}", ("str", base58str));

      auto sub_str = base58str.substr(const_strlen(prefix));
      try {
         using default_type = typename public_key::storage_type::template type_at<0>;
         using data_type = default_type::data_type;
         using wrapper = checksummed_data<data_type>;
         auto bin = fc::from_base58(sub_str);
         if (bin.size() == sizeof(data_type) + sizeof(uint32_t)) {
            auto wrapped = fc::raw::unpack<wrapper>(bin);
            FC_ASSERT(wrapper::calculate_checksum(wrapped.data) == wrapped.check);
            return public_key::storage_type(default_type(wrapped.data));
         }
      } catch (...) {
         // default import failed
      }

      return base58_str_parser<public_key::storage_type, config::public_key_prefix>::apply(sub_str);
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

   bool operator != ( const public_key& p1, const public_key& p2) {
      return !(p1 == p2);
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
