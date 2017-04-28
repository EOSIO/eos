#pragma once
#include <map>
#include <string>
#include <vector>
#include <istream>

#include <fc/exception/exception.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

#include <eos/types/native.hpp>

namespace EOS {
   using std::map;
   using std::set;
   using std::string;
   using std::vector;

   class AbstractSymbolTable {
      public:
         virtual ~AbstractSymbolTable(){};
         virtual void addType( const Struct& s ) =0;
         virtual void addTypeDef( const TypeName& from, const TypeName& to ) = 0;
         virtual bool isKnownType( const TypeName& name ) = 0;

         virtual TypeName resolveTypedef( TypeName name )const = 0;
         virtual Struct getType( TypeName name )const = 0;

        /**
         *  Parses the input stream and populatse the symbol table, the table may be pre-populated
         */
        void   parse( std::istream& in ); 

        string binaryToJson( const TypeName& type, const Bytes& binary );
        Bytes  jsonToBinary( const TypeName& type, const string& json );
   };


   class SimpleSymbolTable : public AbstractSymbolTable {
      public:
         SimpleSymbolTable():
         known( { "FixedString16", "FixedString32",
                  "UInt8", "UInt16", "UInt32", "UInt64",
                  "UInt128", "Checksum", "UInt256", "UInt512",
                  "Int8", "Int16", "Int32", "Int64",
                  "Int128", "Int224", "Int256", "Int512",
                  "Bytes", "PublicKey", "Signature", "String", "Time" } ) 
         {
         }

         virtual void addType( const Struct& s ) override { try {
            FC_ASSERT( !isKnownType( s.name ) );
            for( const auto& f : s.fields ) {
               FC_ASSERT( isKnownType( f.type ), "", ("type",f.type) );
            }
            structs[s.name] = s;
            order.push_back(s.name);
          //  wdump((s.name)(s.base)(s.fields));
         } FC_CAPTURE_AND_RETHROW( (s) ) }

         virtual void addTypeDef( const TypeName& from, const TypeName& to ) override { try {
            FC_ASSERT( !isKnownType( to ) );
            FC_ASSERT( isKnownType( from ) );
            typedefs[to] = from;
            order.push_back(to);
         } FC_CAPTURE_AND_RETHROW( (from)(to) ) }

         virtual bool isKnownType( const TypeName& name ) override { try {
            FC_ASSERT( name.size() > 0 );
            if( name.size() > 2 && name.back() == ']' ) {
               FC_ASSERT( name[name.size()-2] == '[' );
               return isKnownType( name.substr( 0, name.size()-2 ) );
            }
            return known.find(name) != known.end() ||
                   typedefs.find(name) != typedefs.end() ||
                   structs.find(name) != structs.end();
         } FC_CAPTURE_AND_RETHROW( (name) ) }

         virtual Struct getType( TypeName name )const override {
            name = resolveTypedef( name );

            auto itr = structs.find( name );
            if( itr != structs.end() ) 
               return itr->second;

            if( known.find(name) != known.end() )
               FC_ASSERT( !"type is a built in type" );
            FC_ASSERT( !"unknown type", "", ("n", name) );
         }
         virtual TypeName resolveTypedef( TypeName name )const override {
            auto tdef = typedefs.find( name );
            while( tdef != typedefs.end() ) {
               name = tdef->second;
               tdef = typedefs.find( name );
            }
            return name;
         }


//      private:
         set<TypeName>           known;
         vector<TypeName>        order;
         map<TypeName, TypeName> typedefs;
         map<TypeName, Struct>   structs;
   };


} // namespace EOS

FC_REFLECT( EOS::Field, (name)(type) )
FC_REFLECT( EOS::Struct, (name)(base)(fields) )
FC_REFLECT( EOS::SimpleSymbolTable, (typedefs)(structs) )
