#include <eos/types/TypeParser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range.hpp>

#include <algorithm>
#include <iostream>

namespace eos { namespace types {
namespace alg = boost::algorithm;

bool AbstractSymbolTable::isValidTypeName(const TypeName& name, bool allowArray) const {
   std::string mutable_name = name;
   if (mutable_name.empty() ) return false;

   // If appropriate, remove trailing []
   if (allowArray && alg::ends_with(mutable_name, "[]"))
      mutable_name.resize(mutable_name.size() - 2);

   return alg::all_of(mutable_name, alg::is_alnum());
}

bool AbstractSymbolTable::isValidFieldName(const FieldName& name) const {
   std::string mutable_name = name;
   if (mutable_name.empty() || !std::islower(mutable_name[0])) return false;

   return alg::all_of(mutable_name, alg::is_alnum());
}

void AbstractSymbolTable::parse(std::istream& in) {
   vector<string> line_tokens;

   bool   in_struct = false;
   Struct current;

   auto finishStruct = [&](){
      FC_ASSERT(current.fields.size() > 0, "A struct must specify at least one field");
      this->addType(current);
      current.fields.clear();
      current.base = TypeName();
      in_struct = false;
   };

   int linenum = -1;
   for(string line; std::getline(in, line);) {
      ++linenum;
      if(!line.size()) continue;
      line = line.substr(0, line.find('#'));
      line = fc::trim(line);
      if(!line.size()) continue;
      //std::cerr << line << "\n";

      line_tokens.clear();
      split(line_tokens, line, boost::is_any_of(" \t"), boost::token_compress_on);

      if(!line_tokens.size()) continue;
      if(line_tokens[0] == "struct") {
         if(in_struct) finishStruct();
         in_struct = true;
         FC_ASSERT(line_tokens.size() >= 2, "Expected a struct name");
         current.name = line_tokens[1];
         if(line_tokens.size() > 2) {
            FC_ASSERT(line_tokens.size() == 4, "Expected a struct name");
            FC_ASSERT(line_tokens[2] == "inherits");
            current.base = line_tokens[3];
         }
      } else if(line_tokens[0] == "typedef") {
         if(in_struct) finishStruct();
         FC_ASSERT(line_tokens.size() == 3, "Expected a struct name");
         this->addTypeDef(line_tokens[1], line_tokens[2]);
      } else if(in_struct) { // parse field
         //idump((line_tokens));
         FC_ASSERT(line_tokens.size() == 2, "a field must be two tokens long");
         const auto& name = line_tokens[0];
         const auto& type = line_tokens[1];
         FC_ASSERT(name.size() > 0);
         current.fields.emplace_back(Field{name,type});
      }
      else
         FC_ASSERT(false, "Invalid token ${t} on line ${n}", ("t",line_tokens[0])("n",linenum));
   }

   if (in_struct)
      finishStruct();
}

void SimpleSymbolTable::addType(const Struct& s) { try {
      EOS_ASSERT(isValidTypeName(s.name), invalid_type_name_exception,
                 "Invalid type name: ${name}", ("name", s.name));
      EOS_ASSERT(!isKnownType(s.name), duplicate_type_exception,
                 "Duplicate type name: ${name}", ("name", s.name));
      EOS_ASSERT(s.base.size() == 0 || isKnownType(s.base), unknown_type_exception,
                 "Unknown base type name: ${name}", ("name", s.base));
      for(const auto& f : s.fields) {
         EOS_ASSERT(isKnownType(f.type), unknown_type_exception,
                    "No such type found: ${type}", ("type",f.type));
         EOS_ASSERT(isValidFieldName(f.name), invalid_field_name_exception,
                    "Invalid field name: ${name}", ("name", f.name));
      }
      structs[s.name] = s;
      order.push_back(s.name);
} FC_CAPTURE_AND_RETHROW((s)) }

void SimpleSymbolTable::addTypeDef(const TypeName& from, const TypeName& to) { try {
      EOS_ASSERT(isValidTypeName(to), invalid_type_name_exception,
                 "Invalid type name: ${name}", ("name", to));
      EOS_ASSERT(!isKnownType(to), duplicate_type_exception,
                 "Duplicate type name: ${name}", ("name", to));
      EOS_ASSERT(isKnownType(from), unknown_type_exception,
                 "No such type found: ${type}", ("type", from));
      EOS_ASSERT(!boost::ends_with(std::string(from), "[]"), type_exception,
                 "Cannot create typedef of an array");
      typedefs[to] = from;
      order.push_back(to);
} FC_CAPTURE_AND_RETHROW((from)(to)) }

bool SimpleSymbolTable::isKnownType(const TypeName& n) const { try {
      String name(n);
      EOS_ASSERT(!name.empty(), invalid_type_name_exception, "Type name cannot be empty");
      if(name.size() > 2 && name.back() == ']') {
         FC_ASSERT(name[name.size()-2] == '[');
         return isKnownType(name.substr(0, name.size()-2));
      }
      return known.find(n) != known.end() ||
                              typedefs.find(n) != typedefs.end() ||
                                                  structs.find(n) != structs.end();
                                                         } FC_CAPTURE_AND_RETHROW((n)) }

Struct eos::types::SimpleSymbolTable::getType(TypeName name) const {
   name = resolveTypedef(name);

   auto itr = structs.find(name);
   if(itr != structs.end())
      return itr->second;

   if(known.find(name) != known.end())
      FC_THROW_EXCEPTION(type_exception, "Cannot get description of built-in type ${type}", ("type", name));
   FC_THROW_EXCEPTION(unknown_type_exception, "Unknown type ${type}", ("type", name));
}

TypeName SimpleSymbolTable::resolveTypedef(TypeName name) const {
   auto tdef = typedefs.find(name);
   while(tdef != typedefs.end()) {
      name = tdef->second;
      tdef = typedefs.find(name);
   }
   return name;
}

}} // namespace eos::types
