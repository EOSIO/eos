#pragma once
#include <map>
#include <string>
#include <vector>
#include <istream>

#include <fc/exception/exception.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

#include <eos/types/native.hpp>
#include <eos/types/exceptions.hpp>

namespace eos { namespace types {
using std::map;
using std::set;
using std::string;
using std::vector;

class AbstractSymbolTable {
public:
   virtual ~AbstractSymbolTable(){}
   virtual void addType(const Struct& s) =0;
   virtual void addTypeDef(const TypeName& from, const TypeName& to) = 0;
   virtual bool isKnownType(const TypeName& name) const = 0;
   virtual bool isValidTypeName(const TypeName& name, bool allowArray = false) const;
   virtual bool isValidFieldName(const FieldName& name) const;

   virtual TypeName resolveTypedef(TypeName name) const = 0;
   virtual Struct getType(TypeName name) const = 0;

   /**
    * Parses the input stream and populatse the symbol table, the table may be pre-populated
    */
   void parse(std::istream& in);

};


class SimpleSymbolTable : public AbstractSymbolTable {
public:
   SimpleSymbolTable():
      known({ "Field", "Struct", "Asset", "ShareType", "Name", "FixedString16", "FixedString32",
            "UInt8", "UInt16", "UInt32", "UInt64",
            "UInt128", "Checksum", "UInt256", "UInt512",
            "Int8", "Int16", "Int32", "Int64",
            "Int128", "Int224", "Int256", "Int512",
            "Bytes", "PublicKey", "Signature", "String", "Time" })
   {}

   virtual void addType(const Struct& s) override;
   virtual void addTypeDef(const TypeName& from, const TypeName& to) override;
   virtual bool isKnownType(const TypeName& n) const override;
   virtual Struct getType(TypeName name) const override;
   virtual TypeName resolveTypedef(TypeName name) const override;

   //      private:
   const set<TypeName>     known;
   vector<TypeName>        order;
   map<TypeName, TypeName> typedefs;
   map<TypeName, Struct>   structs;
};



}} // namespace eos::types

FC_REFLECT(eos::types::SimpleSymbolTable, (typedefs)(structs))
