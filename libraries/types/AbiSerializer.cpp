#include <eos/types/AbiSerializer.hpp>
#include <fc/io/raw.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace eos { namespace types {

   template <typename T>
   inline fc::variant variantFromStream(fc::datastream<const char*>& stream) {
      T temp;
      fc::raw::unpack( stream, temp );
      return fc::variant(temp);
   }

   AbiSerializer::AbiSerializer( const Abi& abi ) {
      configureTypes();
      setAbi(abi);
   }

   void AbiSerializer::configureTypes() {

      native_types.emplace("UInt8", std::make_pair( []( auto& stream ) -> auto  {
            return variantFromStream<uint8_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<uint8_t>() );
         }
      ));

      native_types.emplace("UInt16", std::make_pair( []( auto& stream ) -> auto  { 
            return variantFromStream<uint16_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<uint16_t>() );
         }
      ));

      native_types.emplace("UInt32", std::make_pair( []( auto& stream ) -> auto  { 
            return variantFromStream<uint32_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<uint32_t>() );
         }
      ));

      native_types.emplace("UInt64", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<uint64_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<uint64_t>() );
         }
      ));

      native_types.emplace("UInt128", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<UInt128>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<UInt128>() );
         }
      ));

      native_types.emplace("UInt256", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<UInt256>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<UInt256>() );
         }
      ));

      native_types.emplace("Int8", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<int8_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<int8_t>() );
         }
      ));

      native_types.emplace("Int16", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<int16_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<int16_t>() );
         }
      ));

      native_types.emplace("Int32", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<int32_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<int32_t>() );
         }
      ));

      native_types.emplace("Int64", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<int64_t>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<int64_t>() );
         }
      ));

      // native_types.emplace("Int128", std::make_pair(
      //    []( auto& stream ) -> auto  {
      //       return variantFromStream<__int128>(stream);
      //    },
      //    []( const fc::variant& var, fc::datastream<char*>& ds ){
      //       fc::raw::pack( ds, var.as<__int128>() );
      //    }
      // ));

      // native_types.emplace("Int256", std::make_pair(
      //    []( auto& stream ) -> auto  {
      //       return variantFromStream<Int256>(stream);
      //    },
      //    []( const fc::variant& var, fc::datastream<char*>& ds ){
      //       fc::raw::pack( ds, var.as<Int256>() );
      //    }
      // ));

      native_types.emplace("Name", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<Name>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<Name>() );
         }
      ));

      native_types.emplace("Time", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<Time>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<Time>() );
         }
      ));

      // native_types.emplace("String", std::make_pair(
      //    []( auto& stream ) -> auto  {
      //       return variantFromStream<String>(stream);
      //    },
      //    []( const fc::variant& var, fc::datastream<char*>& ds ){
      //       fc::raw::pack( ds, var.as<String>() );
      //    }
      // ));

      native_types.emplace("Checksum", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<Checksum>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<Checksum>() );
         }
      ));

      native_types.emplace("Signature", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<Signature>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<Signature>() );
         }
      ));

      native_types.emplace("FixedString32", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<FixedString32>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<FixedString32>() );
         }
      ));

      native_types.emplace("FixedString16", std::make_pair(
         []( auto& stream ) -> auto  {
            return variantFromStream<FixedString16>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds ){
            fc::raw::pack( ds, var.as<FixedString16>() );
         }
      ));
   }

   void AbiSerializer::setAbi( const Abi& abi ) {
      typedefs.clear();
      structs.clear();
      actions.clear();
      tables.clear();

      for( const auto& td : abi.types )
         typedefs[td.newTypeName] = td.type;

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

   bool AbiSerializer::isType( const TypeName& type )const {
      if( native_types.find(type) != native_types.end() ) return true;
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
         FC_ASSERT( isType( t.second ), "", ("type",t.second) );
      } FC_CAPTURE_AND_RETHROW( (t) ) }
      for( const auto& s : structs ) { try {
         if( s.second.base != TypeName() )
            FC_ASSERT( isType( s.second.base ) );
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
      auto native_type = native_types.find(rtype);
      if( native_type != native_types.end() ) {
         return native_type->second.first(stream);
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
   {
      auto rtype = resolveType(type);

      auto native_type = native_types.find(rtype);
      if( native_type != native_types.end() ) {
         native_type->second.second(var, ds);
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
   }

   Bytes AbiSerializer::variantToBinary(const TypeName& type, const fc::variant& var)const {
      if( !isType(type) )
         return var.as<Bytes>();

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
