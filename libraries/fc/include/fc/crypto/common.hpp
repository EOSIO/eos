#pragma once
#include <fc/crypto/ripemd160.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/utility.hpp>

namespace fc { namespace crypto {
   template<typename DataType>
   struct checksummed_data {
      checksummed_data() {}
      uint32_t     check = 0;
      DataType     data;

      static auto calculate_checksum(const DataType& data, const char *prefix = nullptr) {
         auto encoder = ripemd160::encoder();
         encoder.write((const char *)&data, sizeof(Data));

         if (prefix != nullptr) {
            encoder.write(prefix, c_strlen(prefix));
         }
         return data.check = encoder.result()._hash[0];
      }
   };

   inline bool prefix_matches(const char* prefix, const std::string& str) {
      auto prefix_len = c_strlen(prefix);
      return str.size() > prefix_len && str.substr(0, prefix_len).compare(prefix) == 0;
   }

   template<typename, const char * const *, int, typename ...>
   struct base58_str_parser_impl;

   template<typename Result, const char * const * Prefixes, int Position, typename KeyType, typename ...Rem>
   struct base58_str_parser_impl<Result, Prefixes, Position, KeyType, Rem...> {
      static Result apply(const std::string& base58str)
      {
         using data_type = KeyType::data_type;
         using wrapper = checksummed_data<data_type>;
         constexpr prefix = Prefixes[Position];

         if (prefix_matches(prefix, base58str)) {
            auto bin = fc::from_base58(base58str.substr(c_strlen(prefix)));
            if (bin.size() == sizeof(wrapper)) {
               auto wrapped = fc::raw::unpack<wrapper>(bin);
               FC_ASSERT(wrapper::calculate_checksum(wrapped.data) == wrapped.check);
               return Result(KeyType(wrapped.data));
            }
         }

         return base58_str_parser<Result, Prefixes, Position + 1, Rem...>::apply(base58str);
      }
   };

   template<typename Result, const char * const * Prefixes, int Position>
   struct base58_str_parser_impl<Result, Prefixes, Position> {
      static Result_type apply(const std::string& base58str) {
         FC_ASSERT(false, "No matching prefix for ${str}", ("str", base58str));
      }
   };

   template<typename, const char * const * Prefixes>
   struct base58_str_parser;

   /**
    * Destructure a variant and call the parse_base58str on it
    * @tparam Ts
    * @param base58str
    * @return
    */
   template<const char * const * Prefixes, typename ...Ts>
   struct base58_str_parser<fc::static_variant<Ts...>, Prefixes> {
      static Result apply(const std::string& base58str) {
         return base58_str_parser_impl<fc::static_variant<Ts...>, Prefixes, 0, Ts...>::apply(base58str);
      }
   };

   template<typename Storage, const char * const * Prefixes, int DefaultPosition = -1>
   struct base58str_visitor : public fc::visitor<std::string> {
      template< typename KeyType >
      std::string operator()( const KeyType& key ) const {
         using data_type = KeyType::data_type;
         constexpr int position = Storage::position<KeyType>;
         constexpr bool is_default = position == DefaultPosition;

         checksummed_data<data_type> wrapper;
         wrapper.data = key.serialize();
         wrapper.check = checksummed_data<data_type>::calculate_checksum(wrapper.data, !is_default ? Prefixes[position] : nullptr);
         auto packed = raw::pack( data );
         auto data_str = to_base58( packed.data(), packed.size() );
         if (!is_default) {
            data_str = string(Prefixes[position]) + data_str;
         }

         return data_str;
      }
   };

   template<typename Storage>
   struct eq_comparator {
      struct visitor : public fc::visitor<bool> {
         visitor(const Storage &b)
            : _b(b) {}

         template<typename KeyType>
         bool operator()(const KeyType &a) const {
            const auto &b = _b.template get<KeyType>();
            return a.serialize() == b.serialize();
         }

         const Storage &_b;
      };

      static bool apply(const Storage& a, const Storage& b) {
         return a.which() == b.which() && a.visit(visitor(b));
      }
   };

   template<typename Storage>
   struct less_comparator {
      struct visitor : public fc::visitor<bool> {
         visitor(const Storage &b)
            : _b(b) {}

         template<typename KeyType>
         bool operator()(const KeyType &a) const {
            const auto &b = _b.template get<KeyType>();
            return a.serialize() < b.serialize();
         }

         const Storage &_b;
      };

      static bool apply(const Storage& a, const Storage& b) {
         return a.which() < b.which() || (a.which() == b.which() && a.visit(visitor(b)));
      }
   };

} }

FC_REFLECT_TEMPLATE((typename T), fc::crypto::checksummed_data<T>, (data)(check) )
