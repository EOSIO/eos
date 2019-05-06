/**
 *   @file
 *   @copyright defined in eos/LICENSE
 */
#pragma once

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/string/to_string.hpp>
#include <bsoncxx/types.hpp>

#include <fc/variant.hpp>

#include <bson/bson.h>

namespace eosio {
void to_bson(const fc::variant_object& o, bsoncxx::builder::core& c);
void to_bson(const fc::variants& v, bsoncxx::builder::core& c);
void to_bson(const fc::variant& v, bsoncxx::builder::core& c);
bsoncxx::document::value to_bson(const fc::variant& v);

void from_bson(const bsoncxx::document::view& view, fc::mutable_variant_object& o);
void from_bson(const bsoncxx::array::view& bson_array, fc::variants& a);
template <typename T> void from_bson(const T& ele, fc::variant& v);
fc::variant from_bson(const bsoncxx::document::view& view);
} // namespace eosio

namespace eosio {

void to_bson(const fc::variant_object& o, bsoncxx::builder::core& c)
{
   auto itr = o.begin();
   while (itr != o.end()) {
      c.key_owned(itr->key());
      to_bson(itr->value(), c);
      ++itr;
   }
}

void to_bson(const fc::variants& a, bsoncxx::builder::core& c)
{
   auto itr = a.begin();
   while (itr != a.end()) {
      to_bson(*itr, c);
      ++itr;
   }
}

void to_bson(const fc::variant& v, bsoncxx::builder::core& c)
{
   switch (v.get_type()) {
      case fc::variant::null_type: {
         c.append(bsoncxx::types::b_null{});
         return;
      }
      case fc::variant::int64_type:
      case fc::variant::uint64_type: {
         c.append(v.as_int64());
         return;
      }
      case fc::variant::double_type:
         c.append(v.as_double());
         return;
      case fc::variant::bool_type:
         c.append(v.as_bool());
         return;
      case fc::variant::string_type: {
         c.append(v.as_string());
         return;
      }
      case fc::variant::blob_type: {
         bsoncxx::types::b_binary bin;
         bin.sub_type = bsoncxx::binary_sub_type::k_binary;
         bin.size = v.as_blob().data.size();
         bin.bytes = reinterpret_cast<uint8_t*>(&(*v.as_blob().data.begin()));
         c.append(bin);
         return;
      }
      case fc::variant::array_type: {
         const fc::variants& a = v.get_array();
         bsoncxx::builder::core sub(true);
         to_bson(a, sub);
         c.append(sub.extract_array());
         return;
      }
      case fc::variant::object_type: {
         const fc::variant_object& o = v.get_object();
         if (o.size() == 1) {
            const auto value = o.begin()->value();
            if (o.begin()->key().compare("$oid") == 0) {
               if (value.get_type() == fc::variant::string_type
                  && bson_oid_is_valid(value.as_string().data(), value.as_string().size())) {
                  bsoncxx::oid oid(value.as_string());
                  c.append(oid);
                  break;
               }
            }
            else if (o.begin()->key().compare("$date") == 0) {
               if (value.get_type() == fc::variant::int64_type) {
                  bsoncxx::types::b_date date(std::chrono::milliseconds(value.as_int64()));
                  c.append(date);
                  break;
               }
               else if (value.get_type() == fc::variant::object_type) {
                  const fc::variant_object& obj = value.get_object();
                  if (obj.size() == 1) {
                     auto number = obj.begin();
                     if (number->key().compare("$numberLong") == 0) {
                        bsoncxx::types::b_date date(std::chrono::milliseconds(number->value().as_int64()));
                        c.append(date);
                        break;
                     }
                  }
               }
            }
            else if (o.begin()->key().compare("$timestamp") == 0) {
               if (value.get_type() == fc::variant::object_type) {
                  const fc::variant_object& obj = value.get_object();
                  if (obj.size() == 2) {
                     auto t = obj.begin();
                     auto i = t;
                     ++i;
                     if (t->key().compare("t") == 0 && i->key().compare("i") == 0) {
                        bsoncxx::types::b_timestamp ts;
                        ts.timestamp = static_cast<uint32_t>(t->value().as_uint64());
                        ts.increment = static_cast<uint32_t>(i->value().as_uint64());
                        c.append(ts);
                        break;
                     }
                  }
               }
            }
         }
         bsoncxx::builder::core sub(false);
         to_bson(o, sub);
         c.append(sub.extract_document());
         return;
      }
      default:
         FC_THROW_EXCEPTION(
            fc::invalid_arg_exception,
            "Unsupported fc::variant type: " + std::to_string(v.get_type()));
   }
}

bsoncxx::document::value to_bson(const fc::variant& v)
{
   bsoncxx::builder::core doc(false);
   if (v.get_type() == fc::variant::object_type) {
      const fc::variant_object& o = v.get_object();
      to_bson(o, doc);
   }
   else if (v.get_type() != fc::variant::null_type) {
      FC_THROW_EXCEPTION(
         fc::invalid_arg_exception,
         "Unsupported root fc::variant type: " + std::to_string(v.get_type()));
   }
   return doc.extract_document();
}

void from_bson(const bsoncxx::document::view& view, fc::mutable_variant_object& o)
{
   for (bsoncxx::document::element ele : view) {
      fc::variant v;
      from_bson(ele, v);
      o(ele.key().data(), v);
   }
}

void from_bson(const bsoncxx::array::view& bson_array, fc::variants& a)
{
   a.reserve(std::distance(bson_array.cbegin(), bson_array.cend()));
   for (bsoncxx::array::element ele : bson_array) {
      fc::variant v;
      from_bson(ele, v);
      a.push_back(v);
   }
}

template <typename T>
void from_bson(const T& ele, fc::variant& v)
{
   switch (ele.type()) {
      case bsoncxx::type::k_double:
         v = ele.get_double().value;
         return;
      case bsoncxx::type::k_utf8:
         v = bsoncxx::string::to_string(ele.get_utf8().value);
         return;
      case bsoncxx::type::k_document: {
         fc::mutable_variant_object o;
         from_bson(ele.get_document().value, o);
         v = o;
         return;
      }
      case bsoncxx::type::k_array: {
         bsoncxx::array::view sub_array{ele.get_array().value};
         fc::variants a;
         from_bson(sub_array, a);
         v = a;
         return;
      }
      case bsoncxx::type::k_binary: {
         fc::blob blob;
         blob.data.resize(ele.get_binary().size);
         std::copy(ele.get_binary().bytes, ele.get_binary().bytes + ele.get_binary().size, blob.data.begin());
         v = blob;
         return;
      }
      case bsoncxx::type::k_undefined:
      case bsoncxx::type::k_null:
         v = fc::variant();
         return;
      case bsoncxx::type::k_oid:
         v = fc::variant_object("$oid", ele.get_oid().value.to_string());
         return;
      case bsoncxx::type::k_bool:
         v = ele.get_bool().value;
         return;
      case bsoncxx::type::k_date:
         v = fc::variant_object("$date", ele.get_date().to_int64());
         return;
      case bsoncxx::type::k_int32:
         v = ele.get_int32().value;
         return;
      case bsoncxx::type::k_timestamp:
         v = fc::variant_object("$timestamp", fc::mutable_variant_object("t", ele.get_timestamp().timestamp)("i", ele.get_timestamp().increment));
         return;
      case bsoncxx::type::k_int64:
         v = ele.get_int64().value;
         return;
      default:
         FC_THROW_EXCEPTION(
            fc::invalid_arg_exception,
            "Unsupported BSON type: " + bsoncxx::to_string(ele.type()));
   }
}

fc::variant from_bson(const bsoncxx::document::view& view)
{
   fc::mutable_variant_object o;
   from_bson(view, o);
   return o;
}

}   // namespace eosio

