/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/types/abi_serializer.hpp>
#include <fc/io/raw.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace boost;

namespace eosio { namespace types {

   using boost::algorithm::ends_with;
   using std::string;

   template <typename T>
   inline fc::variant variantFromStream(fc::datastream<const char*>& stream) {
      T temp;
      fc::raw::unpack( stream, temp );
      return fc::variant(temp);
   }

   template <typename T>
   auto packUnpack() {
      return std::make_pair<abi_serializer::unpack_function, abi_serializer::pack_function>(
         []( fc::datastream<const char*>& stream, bool is_array) -> fc::variant  {
            if( is_array )
               return variantFromStream<vector<T>>(stream);
            return variantFromStream<T>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds, bool is_array ){
            if( is_array )
               fc::raw::pack( ds, var.as<vector<T>>() );
            else
               fc::raw::pack( ds,  var.as<T>());
         }
      );
   }

   abi_serializer::abi_serializer( const abi& abi ) {
      configure_built_in_types();
      set_abi(abi);
   }

   void abi_serializer::configure_built_in_types() {
      //public_key.hpp
      built_in_types.emplace("public_key",     packUnpack<public_key>());

      //asset.hpp
      built_in_types.emplace("asset",         packUnpack<asset>());
      built_in_types.emplace("price",         packUnpack<price>());
     
      //native.hpp
      built_in_types.emplace("string",        packUnpack<string>());
      built_in_types.emplace("time",          packUnpack<time>());
      built_in_types.emplace("signature",     packUnpack<signature>());
      built_in_types.emplace("checksum",      packUnpack<checksum>());
      built_in_types.emplace("field_name",     packUnpack<field_name>());
      built_in_types.emplace("fixed_string32", packUnpack<fixed_string32>());
      built_in_types.emplace("fixed_string16", packUnpack<fixed_string16>());
      built_in_types.emplace("type_name",      packUnpack<type_name>());
      built_in_types.emplace("bytes",         packUnpack<bytes>());
      built_in_types.emplace("uint8",         packUnpack<uint8>());
      built_in_types.emplace("uint16",        packUnpack<uint16>());
      built_in_types.emplace("uint32",        packUnpack<uint32>());
      built_in_types.emplace("uint64",        packUnpack<uint64>());
      built_in_types.emplace("uint128",       packUnpack<uint128>());
      built_in_types.emplace("uint256",       packUnpack<uint256>());
      built_in_types.emplace("int8",          packUnpack<int8_t>());
      built_in_types.emplace("int16",         packUnpack<int16_t>());
      built_in_types.emplace("int32",         packUnpack<int32_t>());
      built_in_types.emplace("int64",         packUnpack<int64_t>());
      //built_in_types.emplace("int128",      packUnpack<int128>());
      //built_in_types.emplace("int256",      packUnpack<int256>());
      //built_in_types.emplace("uint128_t",   packUnpack<uint128_t>());
      built_in_types.emplace("name",          packUnpack<name>());
      built_in_types.emplace("field",         packUnpack<field>());
      built_in_types.emplace("struct_t",        packUnpack<struct_t>());
      built_in_types.emplace("fields",        packUnpack<fields>());
      
      //generated.hpp
      built_in_types.emplace("account_name",            packUnpack<account_name>());
      built_in_types.emplace("permission_name",          packUnpack<permission_name>());
      built_in_types.emplace("func_name",                packUnpack<func_name>());
      built_in_types.emplace("message_name",             packUnpack<message_name>());
      //built_in_types.emplace("type_name",              packUnpack<type_name>());
      built_in_types.emplace("account_permission",       packUnpack<account_permission>());
      built_in_types.emplace("message",                 packUnpack<message>());
      built_in_types.emplace("account_permission_weight", packUnpack<account_permission_weight>());
      built_in_types.emplace("transaction",             packUnpack<transaction>());
      built_in_types.emplace("signed_transaction",       packUnpack<signed_transaction>());
      built_in_types.emplace("key_permission_weight",     packUnpack<key_permission_weight>());
      built_in_types.emplace("authority",               packUnpack<authority>());
      built_in_types.emplace("blockchain_configuration", packUnpack<blockchain_configuration>());
      built_in_types.emplace("type_def",                 packUnpack<type_def>());
      built_in_types.emplace("action",                  packUnpack<action>());
      built_in_types.emplace("table",                   packUnpack<table>());
      built_in_types.emplace("abi",                     packUnpack<abi>());
   }

   void abi_serializer::set_abi(const abi& abi) {
      typedefs.clear();
      structs.clear();
      actions.clear();
      tables.clear();

      for( const auto& td : abi.types ) {
         FC_ASSERT(is_type(td.type), "invalid type", ("type",td.type));
         typedefs[td.new_type_name] = td.type;
      }

      for( const auto& st : abi.structs )
         structs[st.name] = st;
      
      for( const auto& a : abi.actions )
         actions[a.action_name] = a.type;

      for( const auto& t : abi.tables )
         tables[t.table_name] = t.type;

      /**
       *  The ABI vector may contain duplicates which would make it
       *  an invalid ABI
       */
      FC_ASSERT( typedefs.size() == abi.types.size() );
      FC_ASSERT( structs.size() == abi.structs.size() );
      FC_ASSERT( actions.size() == abi.actions.size() );
      FC_ASSERT( tables.size() == abi.tables.size() );
   }
   
   bool abi_serializer::is_array(const type_name& type)const {
      return ends_with(string(type), "[]");
   }

   type_name abi_serializer::array_type(const type_name& type)const {
      if( !is_array(type) ) return type;
      return type_name(string(type).substr(0, type.size()-2));
   }

   bool abi_serializer::is_type(const type_name& rtype)const {
      auto type = array_type(rtype);
      if( built_in_types.find(type) != built_in_types.end() ) return true;
      if( typedefs.find(type) != typedefs.end() ) return is_type(typedefs.find(type)->second);
      if( structs.find(type) != structs.end() ) return true;
      return false;
   }

   const struct_t& abi_serializer::get_struct(const type_name& type)const {
      auto itr = structs.find(resolve_type(type) );
      FC_ASSERT( itr != structs.end(), "Unknown struct ${type}", ("type",type) );
      return itr->second;
   }

   void abi_serializer::validate()const {
      for( const auto& t : typedefs ) { try {
         vector<type_name> types_seen{t.first, t.second};
         auto itr = typedefs.find(t.second);
         while( itr != typedefs.end() ) {
            FC_ASSERT( find(types_seen.begin(), types_seen.end(), itr->second) == types_seen.end(), "Circular reference in type ${type}", ("type",t.first) );
            types_seen.emplace_back(itr->second);
            itr = typedefs.find(itr->second);
         }
      } FC_CAPTURE_AND_RETHROW( (t) ) }
      for( const auto& t : typedefs ) { try {
         FC_ASSERT(is_type(t.second), "", ("type",t.second) );
      } FC_CAPTURE_AND_RETHROW( (t) ) }
      for( const auto& s : structs ) { try {
         if( s.second.base != type_name() ) {
            struct_t current = s.second;
            vector<type_name> types_seen{current.name};
            while( current.base != type_name() ) {
               const auto& base = get_struct(current.base); //<-- force struct to inherit from another struct
               FC_ASSERT( find(types_seen.begin(), types_seen.end(), base.name) == types_seen.end(), "Circular reference in struct ${type}", ("type",s.second.name) );
               types_seen.emplace_back(base.name);
               current = base;
            }
         }
         for( const auto& field : s.second.fields ) { try {
            FC_ASSERT(is_type(field.type) );
         } FC_CAPTURE_AND_RETHROW( (field) ) }
      } FC_CAPTURE_AND_RETHROW( (s) ) }
      for( const auto& a : actions ) { try {
        FC_ASSERT(is_type(a.second), "", ("type",a.second) );
      } FC_CAPTURE_AND_RETHROW( (a)  ) }

      for( const auto& t : tables ) { try {
        FC_ASSERT(is_type(t.second), "", ("type",t.second) );
      } FC_CAPTURE_AND_RETHROW( (t)  ) }
   }

   type_name abi_serializer::resolve_type(const type_name& type)const  {
      auto itr = typedefs.find(type);
      if( itr != typedefs.end() )
         return resolve_type(itr->second);
      return type;
   }

   void abi_serializer::binary_to_variant(const type_name& type, fc::datastream<const char *>& stream,
                                          fc::mutable_variant_object& obj)const {
      const auto& st = get_struct(type);
      if( st.base != type_name() ) {
         binary_to_variant(resolve_type(st.base), stream, obj);
      }
      for( const auto& field : st.fields ) {
         obj( field.name, binary_to_variant(resolve_type(field.type), stream) );
      }
   }

   fc::variant abi_serializer::binary_to_variant(const type_name& type, fc::datastream<const char *>& stream)const
   {
      type_name rtype = resolve_type(type);
      auto btype = built_in_types.find(array_type(rtype) );
      if( btype != built_in_types.end() ) {
         return btype->second.first(stream, is_array(rtype));
      }
      
      fc::mutable_variant_object mvo;
      binary_to_variant(rtype, stream, mvo);
      return fc::variant( std::move(mvo) );
   }

   fc::variant abi_serializer::binary_to_variant(const type_name& type, const bytes& binary)const{
      fc::datastream<const char*> ds( binary.data(), binary.size() );
      return binary_to_variant(type, ds);
   }

   void abi_serializer::variant_to_binary(const type_name& type, const fc::variant& var, fc::datastream<char *>& ds)const
   { try {
      auto rtype = resolve_type(type);

      auto btype = built_in_types.find(array_type(rtype));
      if( btype != built_in_types.end() ) {
         btype->second.second(var, ds, is_array(rtype));
      } else {
         
         const auto& st = get_struct(rtype);
         const auto& vo = var.get_object();

         if( st.base != type_name() ) {
            variant_to_binary(resolve_type(st.base), var, ds);
         }
         for( const auto& field : st.fields ) {
            if( vo.contains( string(field.name).c_str() ) ) {
               variant_to_binary(field.type, vo[field.name], ds);
            }
            else {
               /// TODO: default construct field and write it out
               FC_ASSERT( !"missing field in variant object", "Missing '${f}' in variant object", ("f",field.name) );
            }
         }
      }
   } FC_CAPTURE_AND_RETHROW( (type)(var) ) }

   bytes abi_serializer::variant_to_binary(const type_name& type, const fc::variant& var)const {
      if( !is_type(type) ) {
         return var.as<bytes>();
      }

      bytes temp( 1024*1024 );
      fc::datastream<char*> ds(temp.data(), temp.size() );
      variant_to_binary(type, var, ds);
      temp.resize(ds.tellp());
      return temp;
   }

   type_name abi_serializer::get_action_type(name action)const {
      auto itr = actions.find(action);
      if( itr != actions.end() ) return itr->second;
      return type_name();
   }
   type_name abi_serializer::get_table_type(name action)const {
      auto itr = tables.find(action);
      if( itr != tables.end() ) return itr->second;
      return type_name();
   }

} }
