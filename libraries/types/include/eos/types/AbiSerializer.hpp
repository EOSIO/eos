/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/types/types.hpp>

namespace eos { namespace types {

using std::map;
using std::string;
using std::function;
using std::pair;
using std::vector;

/**
 *  Describes the binary representation message and table contents so that it can
 *  be converted to and from JSON.
 */
struct AbiSerializer {
   AbiSerializer(){ configureBuiltInTypes(); }
   AbiSerializer( const Abi& abi );
   void setAbi( const Abi& abi );

   map<TypeName, TypeName> typedefs;
   map<TypeName, Struct>   structs;
   map<Name,TypeName>      actions;
   map<Name,TypeName>      tables;

   typedef std::function<fc::variant(fc::datastream<const char*>&, bool)>  unpack_function;
   typedef std::function<void(const fc::variant&, fc::datastream<char*>&, bool)>  pack_function;
   
   map<TypeName, pair<unpack_function, pack_function>> built_in_types;
   void configureBuiltInTypes();

   bool hasCycle( const vector<pair<string,string>>& sedges )const;
   void validate()const;

   TypeName resolveType( const TypeName& t )const;
   bool isArray( const TypeName& type )const;
   bool isType( const TypeName& type )const;
   bool isStruct( const TypeName& type )const;

   TypeName arrayType( const TypeName& type )const;

   const Struct& getStruct( const TypeName& type )const;

   TypeName getActionType( Name action )const;
   TypeName getTableType( Name action )const;

   fc::variant binaryToVariant(const TypeName& type, const Bytes& binary)const;
   Bytes       variantToBinary(const TypeName& type, const fc::variant& var)const;

   fc::variant binaryToVariant(const TypeName& type, fc::datastream<const char*>& binary )const;
   void        variantToBinary(const TypeName& type, const fc::variant& var, fc::datastream<char*>& ds )const;

   template<typename Vec>
   static bool is_empty_abi(const Vec& abi_vec)
   {
      return abi_vec.size() <= 4;
   }

   template<typename Vec>
   static bool to_abi(const Vec& abi_vec, Abi& abi)
   {
      if( !is_empty_abi(abi_vec) ) { /// 4 == packsize of empty Abi
         fc::datastream<const char*> ds( abi_vec.data(), abi_vec.size() );
         fc::raw::unpack( ds, abi );
         return true;
      }
      return false;
   }

   private:
   void binaryToVariant(const TypeName& type, fc::datastream<const char*>& stream, fc::mutable_variant_object& obj )const;
};

} } // eos::types
