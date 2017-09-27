#include <eos/types/AbiSerializer.hpp>
#include <fc/io/raw.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace boost;

namespace eos { namespace types {

   using boost::algorithm::ends_with;
   using std::vector;
   using std::string;

   template <typename T>
   inline fc::variant variantFromStream(fc::datastream<const char*>& stream) {
      T temp;
      fc::raw::unpack( stream, temp );
      return fc::variant(temp);
   }

   template <typename T>
   auto packUnpack() {
      return std::make_pair<AbiSerializer::unpack_function, AbiSerializer::pack_function>( 
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

   AbiSerializer::AbiSerializer( const Abi& abi ) {
      configureBuiltInTypes();
      setAbi(abi);
   }

   void AbiSerializer::configureBuiltInTypes() {
      //PublicKey.hpp
      built_in_types.emplace("PublicKey",     packUnpack<PublicKey>());

      //Asset.hpp
      built_in_types.emplace("Asset",         packUnpack<Asset>());
      built_in_types.emplace("Price",         packUnpack<Price>());
     
      //native.hpp
      built_in_types.emplace("String",        packUnpack<String>());
      built_in_types.emplace("Time",          packUnpack<Time>());
      built_in_types.emplace("Signature",     packUnpack<Signature>());
      built_in_types.emplace("Checksum",      packUnpack<Checksum>());
      built_in_types.emplace("FieldName",     packUnpack<FieldName>());
      built_in_types.emplace("FixedString32", packUnpack<FixedString32>());
      built_in_types.emplace("FixedString16", packUnpack<FixedString16>());
      built_in_types.emplace("TypeName",      packUnpack<TypeName>());
      built_in_types.emplace("Bytes",         packUnpack<Bytes>());
      built_in_types.emplace("UInt8",         packUnpack<UInt8>());
      built_in_types.emplace("UInt16",        packUnpack<UInt16>());
      built_in_types.emplace("UInt32",        packUnpack<UInt32>());
      built_in_types.emplace("UInt64",        packUnpack<UInt64>());
      built_in_types.emplace("UInt128",       packUnpack<UInt128>());
      built_in_types.emplace("UInt256",       packUnpack<UInt256>());
      built_in_types.emplace("Int8",          packUnpack<int8_t>());
      built_in_types.emplace("Int16",         packUnpack<int16_t>());
      built_in_types.emplace("Int32",         packUnpack<int32_t>());
      built_in_types.emplace("Int64",         packUnpack<int64_t>());
      //built_in_types.emplace("Int128",      packUnpack<Int128>());
      //built_in_types.emplace("Int256",      packUnpack<Int256>());
      //built_in_types.emplace("uint128_t",   packUnpack<uint128_t>());
      built_in_types.emplace("Name",          packUnpack<Name>());
      built_in_types.emplace("Field",         packUnpack<Field>());
      built_in_types.emplace("Struct",        packUnpack<Struct>());
      built_in_types.emplace("Fields",        packUnpack<Fields>());
      
      //generated.hpp
      built_in_types.emplace("AccountName",             packUnpack<AccountName>());
      built_in_types.emplace("PermissionName",          packUnpack<PermissionName>());
      built_in_types.emplace("FuncName",                packUnpack<FuncName>());
      built_in_types.emplace("MessageName",             packUnpack<MessageName>());
      //built_in_types.emplace("TypeName",              packUnpack<TypeName>());
      built_in_types.emplace("AccountPermission",       packUnpack<AccountPermission>());
      built_in_types.emplace("Message",                 packUnpack<Message>());
      built_in_types.emplace("AccountPermissionWeight", packUnpack<AccountPermissionWeight>());
      built_in_types.emplace("Transaction",             packUnpack<Transaction>());
      built_in_types.emplace("SignedTransaction",       packUnpack<SignedTransaction>());
      built_in_types.emplace("KeyPermissionWeight",     packUnpack<KeyPermissionWeight>());
      built_in_types.emplace("Authority",               packUnpack<Authority>());
      built_in_types.emplace("BlockchainConfiguration", packUnpack<BlockchainConfiguration>());
      built_in_types.emplace("TypeDef",                 packUnpack<TypeDef>());
      built_in_types.emplace("Action",                  packUnpack<Action>());
      built_in_types.emplace("Table",                   packUnpack<Table>());
      built_in_types.emplace("Abi",                     packUnpack<Abi>());
   }

   void AbiSerializer::setAbi( const Abi& abi ) {
      typedefs.clear();
      structs.clear();
      actions.clear();
      tables.clear();

      for( const auto& td : abi.types ) {
         FC_ASSERT( isType(td.type), "invalid type", ("type",td.type));
         typedefs[td.newTypeName] = td.type;
      }

      for( const auto& st : abi.structs )
         structs[st.name] = st;
      
      for( const auto& a : abi.actions )
         actions[a.action] = a.type;

      for( const auto& t : abi.tables )
         tables[t.table] = t.type;

      /**
       *  The ABI vector may contain duplicates which would make it
       *  an invalid ABI
       */
      FC_ASSERT( typedefs.size() == abi.types.size() );
      FC_ASSERT( structs.size() == abi.structs.size() );
      FC_ASSERT( actions.size() == abi.actions.size() );
      FC_ASSERT( tables.size() == abi.tables.size() );
   }
   
   bool AbiSerializer::isArray( const TypeName& type )const {
      return ends_with(string(type), "[]");
   }

   TypeName AbiSerializer::arrayType( const TypeName& type )const {
      if( !isArray(type) ) return type;
      return TypeName(string(type).substr(0, type.size()-2));
   }

   bool AbiSerializer::isType( const TypeName& rtype )const {
      auto type = arrayType(rtype);
      if( built_in_types.find(type) != built_in_types.end() ) return true;
      if( typedefs.find(type) != typedefs.end() ) return isType( typedefs.find(type)->second );
      if( structs.find(type) != structs.end() ) return true;
      return false;
   }

   const Struct& AbiSerializer::getStruct( const TypeName& type )const {
      auto itr = structs.find( resolveType(type) );
      FC_ASSERT( itr != structs.end(), "Unknown struct ${type}", ("type",type) );
      return itr->second;
   }

   void AbiSerializer::validate()const {
      for( const auto& t : typedefs ) { try {
         vector<TypeName> types_seen{t.first, t.second};
         auto itr = typedefs.find(t.second);
         while( itr != typedefs.end() ) {
            FC_ASSERT( find(types_seen.begin(), types_seen.end(), itr->second) == types_seen.end(), "Circular reference in type ${type}", ("type",t.first) );
            types_seen.emplace_back(itr->second);
            itr = typedefs.find(itr->second);
         }
      } FC_CAPTURE_AND_RETHROW( (t) ) }
      for( const auto& t : typedefs ) { try {
         FC_ASSERT( isType( t.second ), "", ("type",t.second) );
      } FC_CAPTURE_AND_RETHROW( (t) ) }
      for( const auto& s : structs ) { try {
         if( s.second.base != TypeName() ) {
            Struct current = s.second;
            vector<TypeName> types_seen{current.name};
            while( current.base != TypeName() ) {
               const auto& base = getStruct(current.base); //<-- force struct to inherit from another struct
               FC_ASSERT( find(types_seen.begin(), types_seen.end(), base.name) == types_seen.end(), "Circular reference in struct ${type}", ("type",s.second.name) );
               types_seen.emplace_back(base.name);
               current = base;
            }
         }
         for( const auto& field : s.second.fields ) { try {
            FC_ASSERT( isType( field.type ) );
         } FC_CAPTURE_AND_RETHROW( (field) ) }
      } FC_CAPTURE_AND_RETHROW( (s) ) }
      for( const auto& a : actions ) { try {
        FC_ASSERT( isType( a.second ), "", ("type",a.second) );
      } FC_CAPTURE_AND_RETHROW( (a)  ) }

      for( const auto& t : tables ) { try {
        FC_ASSERT( isType( t.second ), "", ("type",t.second) );
      } FC_CAPTURE_AND_RETHROW( (t)  ) }
   }

   TypeName AbiSerializer::resolveType( const TypeName& type )const  {
      auto itr = typedefs.find(type);
      if( itr != typedefs.end() )
         return resolveType(itr->second);
      return type;
   }

   void AbiSerializer::binaryToVariant(const TypeName& type, fc::datastream<const char*>& stream, fc::mutable_variant_object& obj )const {
      const auto& st = getStruct( type );
      if( st.base != TypeName() ) {
         binaryToVariant( resolveType(st.base), stream, obj );
      }
      for( const auto& field : st.fields ) {
         obj( field.name, binaryToVariant( resolveType(field.type), stream ) );
      }
   }

   fc::variant AbiSerializer::binaryToVariant(const TypeName& type, fc::datastream<const char*>& stream )const
   {
      TypeName rtype = resolveType( type );
      auto btype = built_in_types.find( arrayType(rtype) );
      if( btype != built_in_types.end() ) {
         return btype->second.first(stream, isArray(rtype));
      }
      
      fc::mutable_variant_object mvo;
      binaryToVariant( rtype, stream, mvo );
      return fc::variant( std::move(mvo) );
   }

   fc::variant AbiSerializer::binaryToVariant(const TypeName& type, const Bytes& binary)const{
      fc::datastream<const char*> ds( binary.data(), binary.size() );
      return binaryToVariant( type, ds );
   }

   void AbiSerializer::variantToBinary(const TypeName& type, const fc::variant& var, fc::datastream<char*>& ds )const
   { try {
      auto rtype = resolveType(type);

      auto btype = built_in_types.find(arrayType(rtype));
      if( btype != built_in_types.end() ) {
         btype->second.second(var, ds, isArray(rtype));
      } else {
         
         const auto& st = getStruct( rtype );
         const auto& vo = var.get_object();

         if( st.base != TypeName() ) {
            variantToBinary( resolveType( st.base ), var, ds );
         }
         for( const auto& field : st.fields ) {
            if( vo.contains( String(field.name).c_str() ) ) {
               variantToBinary( field.type, vo[field.name], ds );
            }
            else {
               /// TODO: default construct field and write it out
               FC_ASSERT( !"missing field in variant object", "Missing '${f}' in variant object", ("f",field.name) );
            }
         }
      }
   } FC_CAPTURE_AND_RETHROW( (type)(var) ) }

   Bytes AbiSerializer::variantToBinary(const TypeName& type, const fc::variant& var)const {
      if( !isType(type) ) {
         return var.as<Bytes>();
      }

      Bytes temp( 1024*1024 );
      fc::datastream<char*> ds(temp.data(), temp.size() );
      variantToBinary( type, var, ds );
      temp.resize(ds.tellp());
      return temp;
   }

   TypeName AbiSerializer::getActionType( Name action )const {
      auto itr = actions.find(action);
      if( itr != actions.end() ) return itr->second;
      return TypeName();
   }
   TypeName AbiSerializer::getTableType( Name action )const {
      auto itr = tables.find(action);
      if( itr != tables.end() ) return itr->second;
      return TypeName();
   }

} }
