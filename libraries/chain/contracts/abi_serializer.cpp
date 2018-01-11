/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/transaction.hpp>
#include <fc/io/raw.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <fc/io/varint.hpp>

using namespace boost;

namespace eosio { namespace chain { namespace contracts {

   using boost::algorithm::ends_with;
   using std::string;

   template <typename T>
   inline fc::variant variant_from_stream(fc::datastream<const char*>& stream) {
      T temp;
      fc::raw::unpack( stream, temp );
      return fc::variant(temp);
   }

   template <typename T>
   auto pack_unpack() {
      return std::make_pair<abi_serializer::unpack_function, abi_serializer::pack_function>(
         []( fc::datastream<const char*>& stream, bool is_array) -> fc::variant  {
            if( is_array )
               return variant_from_stream<vector<T>>(stream);
            return variant_from_stream<T>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds, bool is_array ){
            if( is_array )
               fc::raw::pack( ds, var.as<vector<T>>() );
            else
               fc::raw::pack( ds,  var.as<T>());
         }
      );
   }

   abi_serializer::abi_serializer( const abi_def& abi ) {
      configure_built_in_types();
      set_abi(abi);
   }

   void abi_serializer::configure_built_in_types() {
      //public_key.hpp
      built_in_types.emplace("public_key",                pack_unpack<public_key_type>());

      //asset.hpp
      built_in_types.emplace("asset",                     pack_unpack<asset>());
      built_in_types.emplace("price",                     pack_unpack<price>());

      //native.hpp
      built_in_types.emplace("string",                    pack_unpack<string>());
      built_in_types.emplace("time",                      pack_unpack<fc::time_point_sec>());
      built_in_types.emplace("signature",                 pack_unpack<signature_type>());
      built_in_types.emplace("checksum",                  pack_unpack<checksum_type>());
      built_in_types.emplace("field_name",                pack_unpack<field_name>());
      built_in_types.emplace("fixed_string32",            pack_unpack<fixed_string32>());
      built_in_types.emplace("fixed_string16",            pack_unpack<fixed_string16>());
      built_in_types.emplace("type_name",                 pack_unpack<type_name>());
      built_in_types.emplace("bytes",                     pack_unpack<bytes>());
      built_in_types.emplace("uint8",                     pack_unpack<uint8>());
      built_in_types.emplace("uint16",                    pack_unpack<uint16>());
      built_in_types.emplace("uint32",                    pack_unpack<uint32>());
      built_in_types.emplace("uint64",                    pack_unpack<uint64>());
      built_in_types.emplace("uint128",                   pack_unpack<boost::multiprecision::uint128_t>());
      built_in_types.emplace("uint256",                   pack_unpack<boost::multiprecision::uint256_t>());
      built_in_types.emplace("int8",                      pack_unpack<int8_t>());
      built_in_types.emplace("int16",                     pack_unpack<int16_t>());
      built_in_types.emplace("int32",                     pack_unpack<int32_t>());
      built_in_types.emplace("int64",                     pack_unpack<int64_t>());
      built_in_types.emplace("name",                      pack_unpack<name>());
      built_in_types.emplace("field",                     pack_unpack<field_def>());
      built_in_types.emplace("struct_def",                pack_unpack<struct_def>());
      built_in_types.emplace("fields",                    pack_unpack<vector<field_def>>());
      built_in_types.emplace("account_name",              pack_unpack<account_name>());
      built_in_types.emplace("permission_name",           pack_unpack<permission_name>());
      built_in_types.emplace("action_name",               pack_unpack<action_name>());
      built_in_types.emplace("scope_name",                pack_unpack<scope_name>());
      built_in_types.emplace("permission_level",          pack_unpack<permission_level>());
      built_in_types.emplace("action",                    pack_unpack<action>());
      built_in_types.emplace("permission_level_weight",   pack_unpack<permission_level_weight>());
      built_in_types.emplace("transaction",               pack_unpack<transaction>());
      built_in_types.emplace("signed_transaction",        pack_unpack<signed_transaction>());
      built_in_types.emplace("key_weight",                pack_unpack<key_weight>());
      built_in_types.emplace("authority",                 pack_unpack<authority>());
      built_in_types.emplace("chain_config",              pack_unpack<chain_config>());
      built_in_types.emplace("type_def",                  pack_unpack<type_def>());
      built_in_types.emplace("action_def",                pack_unpack<action_def>());
      built_in_types.emplace("table_def",                 pack_unpack<table_def>());
      built_in_types.emplace("abi_def",                   pack_unpack<abi_def>());
      built_in_types.emplace("nonce",                     pack_unpack<nonce>());
   }

   void abi_serializer::set_abi(const abi_def& abi) {
      typedefs.clear();
      structs.clear();
      actions.clear();
      tables.clear();

      for( const auto& st : abi.structs )
         structs[st.name] = st;
      
      for( const auto& td : abi.types ) {
         FC_ASSERT(is_type(td.type), "invalid type", ("type",td.type));
         typedefs[td.new_type_name] = td.type;
      }

      for( const auto& a : abi.actions )
         actions[a.name] = a.type;

      for( const auto& t : abi.tables )
         tables[t.name] = t.type;

      /**
       *  The ABI vector may contain duplicates which would make it
       *  an invalid ABI
       */
      FC_ASSERT( typedefs.size() == abi.types.size() );
      FC_ASSERT( structs.size() == abi.structs.size() );
      FC_ASSERT( actions.size() == abi.actions.size() );
      FC_ASSERT( tables.size() == abi.tables.size() );
   }
   
   bool abi_serializer::is_builtin_type(const type_name& type)const {
      return built_in_types.find(type) != built_in_types.end();
   }

   bool abi_serializer::is_integer(const type_name& type) const {
      string stype = type;
      return boost::starts_with(stype, "uint") || boost::starts_with(stype, "int");
   }

   int abi_serializer::get_integer_size(const type_name& type) const {
      string stype = type;
      FC_ASSERT( is_integer(type), "${stype} is not an integer type", ("stype",stype));
      if( boost::starts_with(stype, "uint") ) {
         return boost::lexical_cast<int>(stype.substr(4));
      } else {
         return boost::lexical_cast<int>(stype.substr(3));
      }      
   }
   
   bool abi_serializer::is_struct(const type_name& type)const {
      return structs.find(resolve_type(type)) != structs.end();
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

   const struct_def& abi_serializer::get_struct(const type_name& type)const {
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
            struct_def current = s.second;
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
      if ( is_array(rtype) ) {
        fc::unsigned_int size;
        fc::raw::unpack(stream, size);
        vector<fc::variant> vars;
        vars.resize(size);
        for (auto& var : vars) {
           var = binary_to_variant(array_type(rtype), stream);
        }
        return fc::variant( std::move(vars) );
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
      } else if ( is_array(rtype) ) {
         vector<fc::variant> vars = var.get_array();
         fc::raw::pack(ds, (fc::unsigned_int)vars.size());
         for (const auto& var : vars) {
           variant_to_binary(array_type(rtype), var, ds);
         }
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

} } } 
