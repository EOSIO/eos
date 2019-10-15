#pragma once
#include <set>
#include <string>
#include <vector>

#include <eosio/chain/chain_id_type.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <multibase/basic_codec.h>
#include <multibase/multibase.h>


std::vector<char> encodeOID(const std::string& str)
{
   std::vector<char> bytes;
   std::vector<uint64_t> values;
   std::istringstream stream(str);
   do {
      uint64_t v = 0;
      stream >> v;
      const auto ch = stream.get();
      EOS_ASSERT( ch == '.' || ch == decltype(stream)::traits_type::eof(), eosio::chain::invalid_oid_encoding, "Invalid OID encoding" );
      values.push_back(v);
   } while(stream);

   EOS_ASSERT( values.size() >= 1, eosio::chain::invalid_oid_encoding, "Invalid OID encoding" );
   bytes.emplace_back(values[0] * 40 + values[1]);
   for (size_t i = 2; i < values.size(); i++) {
      const auto value = values[i];
      if (value <= 127) {
         bytes.push_back(static_cast<unsigned char>(value));
      }
      else {
         std::vector<char> value7BitBytesReversed;
         //for every 7 bits of value create byte with 7th bit set
         constexpr size_t sevenBitChunks = sizeof(value) / 7 + 1;
         for (size_t j = 0; j < sevenBitChunks; j++) {
            value7BitBytesReversed.push_back(static_cast<unsigned char>(value >> (7 * j)) | 0b10000000); //set bit 7
         }
         const auto lastNonZeroIt = std::find_if(std::rbegin(value7BitBytesReversed), std::rend(value7BitBytesReversed),
                                                 [](const auto& v) { return v & 0b01111111; });
         const auto excessBytes = std::distance(std::rbegin(value7BitBytesReversed), lastNonZeroIt);
         value7BitBytesReversed.resize(value7BitBytesReversed.size() - excessBytes);
         value7BitBytesReversed.front() &= 0b01111111; //drop bit 7 for last byte
         std::copy(std::rbegin(value7BitBytesReversed), std::rend(value7BitBytesReversed), std::back_inserter<std::vector<char>>(bytes));
      }
   }
   return bytes;
}


std::string decodeOID(const std::vector<char>& bytes)
{
   if (bytes.empty()) {
      return "";
   }
   //7th bit of last byte must be 0
   EOS_ASSERT( !(bytes.back() & 0b10000000), eosio::chain::invalid_oid_encoding, "Invalid OID encoding" );

   std::vector<uint64_t> values;
   values.push_back(static_cast<uint64_t>(static_cast<unsigned char>(bytes.front())) / 40);
   values.push_back(static_cast<uint64_t>(static_cast<unsigned char>(bytes.front())) % 40);
   size_t i = 1;
   while (i < bytes.size()) {
      char byte = bytes[i];
      uint64_t value = static_cast<unsigned char>(byte) & 0b01111111;
      while (byte & 0b10000000) {
         i++;
         byte = bytes[i];
         value <<= 7;
         value |= byte & 0b01111111;
      }
      values.push_back(value);
      i++;
   }
   std::string result;
   std::for_each(std::begin(values), std::prev(std::end(values)), [&](const auto& v) { result += std::to_string(v) + '.'; });
   result += std::to_string(values.back());
   return result;
}


std::string decodeAttribute(const std::string& hex, int32_t type)
{
   if (hex.empty()) {
      return "";
   }

   std::vector<char> data(hex.size() / 2);
   fc::from_hex(hex, data.data(), data.size());
   fc::variant v;

   switch (type)
   {
      case 0:
         v = fc::raw::unpack<bool>(data);
         break;
      case 1:
         v = fc::raw::unpack<int32_t>(data);
         break;
      case 2:
         v = fc::raw::unpack<int64_t>(data);
         break;
      case 3:
         v = fc::raw::unpack<std::pair<fc::sha256, eosio::name>>(data);
         break;
      case 4:
         v = fc::raw::unpack<std::string>(data);
         break;
      case 5:
         v = fc::time_point(fc::seconds(fc::raw::unpack<int64_t>(data)));
         break;
      case 6:
         if (data.size() == 34 && data[0] == 18 && data[1] == 32) {
            auto codec = multibase::base_58_btc{};
            auto view = std::string_view(data.data(), data.size());
            auto encoded = codec.encode(view, false);
            v = encoded;
         }
         else {
            int64_t varint = 0;
            if (static_cast<unsigned char>(data[0]) <= 252) {
               varint = static_cast<unsigned char>(data[0]);
            }
            else {
               size_t size = static_cast<unsigned char>(data[0]) - 252;
               for (int i = 0; i < size; i++) {
                  varint <<= 8;
                  varint &= static_cast<unsigned char>(data[i + 1]);
               }

            }
            if (varint == 1) {
               auto codec = multibase::base_58_btc{};
               auto view = std::string_view(data.data(), data.size());
               auto encoded = codec.encode(view, true);
               v = encoded;
            }
         }
         break;
      case 7:
         v = decodeOID(data);
         break;
      case 8:
         v = data;
         break;
      case 9:
         v = fc::raw::unpack<std::set<std::pair<eosio::name, std::string>>>(data);
         break;
      default:
         EOS_ASSERT( false, eosio::chain::unknown_attribute_type, "Unknown attribute type" );
   }
   return fc::json::to_pretty_string(v);
}


std::vector<char> encodeAttribute(const std::string& json, int32_t type)
{
   const auto v = fc::json::from_string(json);
   std::vector<char> bytes;

   if (type == 0) {
      bool b;
      fc::from_variant(v, b);
      bytes = fc::raw::pack(b);
   }
   else if (type == 1) {
      int32_t i;
      fc::from_variant(v, i);
      bytes = fc::raw::pack(i);
   }
   else if (type == 2) {
      int64_t i;
      fc::from_variant(v, i);
      bytes = fc::raw::pack(i);
   }
   else if (type == 3) {
      std::pair<eosio::chain::chain_id_type, eosio::name> p(eosio::chain::chain_id_type(""), eosio::name(0));
      fc::from_variant(v, p);
      bytes = fc::raw::pack(p);
   }
   else if (type == 4) {
      std::string s;
      fc::from_variant(v, s);
      bytes = fc::raw::pack(s);
   }
   else if (type == 5) {
      int64_t time = 0;
      if (v.is_int64()) {
         time = v.as_int64();
      }
      else {
         std::string s;
         fc::from_variant(v, s);
         if (std::all_of(std::begin(s), std::end(s), [](const auto& ch) { return std::isdigit(ch); })) {
            time = v.as_int64();
         }
         else {
            time = fc::time_point_sec::from_iso_string(s).sec_since_epoch();
         }
      }
      bytes = fc::raw::pack(time);
   }
   else if (type == 6) {
      std::string s;
      fc::from_variant(v, s);
      if (s.size() == 46 && s.substr(0, 2) == "Qm") { //CiD v0 starts with 'Qm'
         auto codec = multibase::base_58_btc{};
         auto decoded = codec.decode(s);
         std::copy(std::begin(decoded), std::end(decoded), std::back_inserter<std::vector<char>>(bytes));
      }
      else {
         auto decoded = multibase::decode(s);
         EOS_ASSERT(static_cast<int>(decoded.front()) != 18, eosio::chain::invalid_cid_encoding, "Invalid CID encoding" );
         std::copy(std::begin(decoded), std::end(decoded), std::back_inserter<std::vector<char>>(bytes));
      }
   }
   else if (type == 7) {
      std::string s;
      fc::from_variant(v, s);
      bytes = encodeOID(s);
   }
   else if (type == 8) {
      std::vector<char> vec;
      fc::from_variant(v, vec);
      bytes = fc::raw::pack(vec);
   }
   else if (type == 9) {
      std::set<std::pair<eosio::name, std::string>> s;
      fc::from_variant(v, s);
      bytes = fc::raw::pack(s);
   }

   EOS_ASSERT( !bytes.empty(), eosio::chain::unknown_attribute_type, "Unknown attribute type" );
   return bytes;
}
