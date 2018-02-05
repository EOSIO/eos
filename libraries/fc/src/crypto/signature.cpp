#include <fc/crypto/signature.hpp>
#include <fc/crypto/common.hpp>
#include <fc/exception/exception.hpp>

namespace fc { namespace crypto {
   struct hash_visitor : public fc::visitor<size_t> {
      template<typename SigType>
      size_t operator()(const SigType& sig) const {
         static_assert(sizeof(sig._data.data) == 65, "sig size is expected to be 65");
         //signatures are two bignums: r & s. Just add up least significant digits of the two
         return *(size_t*)&sig._data.data[32-sizeof(size_t)] + *(size_t*)&sig._data.data[64-sizeof(size_t)];
      }
   };

   static signature::storage_type parse_base58(const std::string& base58str)
   {
      constexpr auto prefix = config::signature_base_prefix;
      FC_ASSERT(prefix_matches(prefix, base58str), "Signature has invalid prefix: ${str}", ("str", base58str));

      auto sub_str = base58str.substr(const_strlen(prefix));
      try {
         using default_type = signature::storage_type::template type_at<0>;
         using data_type = default_type::data_type;
         using wrapper = checksummed_data<data_type>;
         auto bin = fc::from_base58(sub_str);
         if (bin.size() == sizeof(data_type) + sizeof(uint32_t)) {
            auto wrapped = fc::raw::unpack<wrapper>(bin);
            FC_ASSERT(wrapper::calculate_checksum(wrapped.data) == wrapped.check);
            return signature::storage_type(default_type(wrapped.data));
         }
      } catch (...) {
         // default import failed
      }

      return base58_str_parser<signature::storage_type, config::signature_prefix>::apply(sub_str);
   }

   signature::signature(const std::string& base58str)
      :_storage(parse_base58(base58str))
   {}

   signature::operator std::string() const
   {
      auto data_str = _storage.visit(base58str_visitor<storage_type, config::signature_prefix, 0>());
      return std::string(config::signature_base_prefix) + data_str;
   }

   std::ostream& operator<<(std::ostream& s, const signature& k) {
      s << "signature(" << std::string(k) << ')';
      return s;
   }

   bool operator == ( const signature& p1, const signature& p2) {
      return eq_comparator<signature::storage_type>::apply(p1._storage, p2._storage);
   }

   bool operator != ( const signature& p1, const signature& p2) {
      return !eq_comparator<signature::storage_type>::apply(p1._storage, p2._storage);
   }

   bool operator < ( const signature& p1, const signature& p2)
   {
      return less_comparator<signature::storage_type>::apply(p1._storage, p2._storage);
   }

   size_t hash_value(const signature& b) {
       return b._storage.visit(hash_visitor());
   }
} } // eosio::blockchain

namespace fc
{
   void to_variant(const fc::crypto::signature& var, fc::variant& vo)
   {
      vo = string(var);
   }

   void from_variant(const fc::variant& var, fc::crypto::signature& vo)
   {
      vo = fc::crypto::signature(var.as_string());
   }
} // fc
