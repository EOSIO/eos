#include <eos/types/TypeParser.hpp>
#include <fstream>
#include <iomanip>
#include <fc/io/json.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>

using std::string;
namespace eos { using types::SimpleSymbolTable; }


bool   isVector( const string& type ) {
   return type.back() == ']';
}

string getWrenType( const string& type ) {
   FC_ASSERT( type.size() );
   if( isVector(type) )
      return getWrenType( type.substr( 0, type.size() - 2 ) );
   if( type.substr(0,4) == "UInt" ) {
      if( type == "UInt8" ) return "UInt";
      if( type == "UInt16" ) return "UInt";
      if( type == "UInt32" ) return "UInt";
      if( type == "UInt64" ) return "UInt";
      if( type == "UInt128" ) return "UInt";
      if( type == "UInt256" ) return "UInt";
      return type;
   } else if ( type.substr( 0,3 ) == "Int" ) {
      if( type == "Int8" ) return "Int";
      if( type == "Int16" ) return "Int";
      if( type == "Int32" ) return "Int";
      if( type == "Int64" ) return "Int";
      if( type == "Int128" ) return "Int";
      if( type == "Int256" ) return "Int";
   }
   return type;
}

string call_unpack( const string& field, const string& type ) {
   auto wren_type = getWrenType(type);
   if( wren_type == "UInt" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(4) + " )";
   }
   else if( wren_type == "Int" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(3) + " )";
   }
   return  "_" + field + ".unpack( stream )";
}
string call_pack( const string& field, const string& type ) {
   auto wren_type = getWrenType(type);
   if( wren_type == "UInt" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(4) + " )";
   }
   else if( wren_type == "Int" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(3) + " )";
   }
   return  "_" + field + ".unpack( stream )";
}

string call_type_constructor( const string& type ) {
   FC_ASSERT( type.size() );
   if( isVector( type ) )
      return "Vector[" + getWrenType(type) + "]";
   return getWrenType(type) + ".new()";
}
string generate_wren( const eos::types::Struct& s, eos::types::SimpleSymbolTable& symbols ) {

   std::stringstream ss;
   ss << "class " << s.name;
   if( s.base.size() ) ss << " is " << s.base;
   ss << " {\n";
   ss << "    construct new() { \n";

   if( s.base.size() ) 
   ss << "        super()\n";

   for( const auto& field : s.fields ) {
      ss << "        _" << field.name << " = " << call_type_constructor( symbols.resolveTypedef(field.type) ) <<"\n";
   }
   ss << "    }\n";
   ss << "    unpack( stream ) { \n";
   if( s.base.size() ) 
   ss << "        super.unpack( stream )\n";
   for( const auto& field : s.fields ) {
      ss << "        " << call_unpack( field.name, symbols.resolveTypedef(field.type) ) <<"\n";
   }
   ss << "    }\n";
   ss << "    pack( stream ) { \n";
   if( s.base.size() ) 
   ss << "        super.pack( stream )\n";
   for( const auto& field : s.fields ) {
      ss << "        " << call_pack( field.name, symbols.resolveTypedef(field.type) ) <<"\n";
   }
   ss << "    }\n";
   for( const auto& field : s.fields ) {
      ss << "    " << field.name << " {_" << field.name <<"}\n";
   }

   ss << "}\n";

   return ss.str();
}

void generate_wren( eos::types::SimpleSymbolTable& ss, const char* outfile ) {
  //for( const auto& s : ss.order ) 
}

void generate_hpp( eos::types::SimpleSymbolTable& ss, const char* outfile ) {
   struct FakeField { std::string type; std::string name; };
   auto arrays_to_vectors = [](eos::types::Field f) {
      if (boost::ends_with<std::string>(f.type, "[]")) {
         std::string type = f.type;
         type.resize(type.size() - 2);
         return FakeField{"Vector<" + type + ">", f.name};
      }
      return FakeField{f.type, f.name};
   };

   wdump((outfile));
   std::ofstream out(outfile);
   out << "#pragma once\n";
   out << "namespace eos { namespace types {\n";
   for( const auto& s : ss.order ) {
      if( ss.typedefs.find( s ) != ss.typedefs.end() ) {
         const auto& td = ss.typedefs[s];
         out << "    typedef " << std::left << std::setw(32) << td << " " << s << ";\n";
         continue;
      }
      out << "    struct " << s ;
      const auto& st = ss.structs[s];
      if( st.base.size() )
         out << " : public " << st.base;
      out << " { \n";

      out << "        " << s << "() = default;\n        " << s << "(";
      {
         bool first = true;
         for (const auto& f : st.fields | boost::adaptors::transformed(arrays_to_vectors)) {
            if (first) first = false;
            else out << ", ";
            out << "const " << f.type << "& " << f.name;
         }
      }
      out << ")\n           : ";
      {
         bool first = true;
         for (const auto& f : st.fields | boost::adaptors::transformed(arrays_to_vectors)) {
            if (first) first = false;
            else out << ", ";
            out << f.name << "(" << f.name << ")";
         }
      }
      out << " {}\n\n";

      for(const auto& f : st.fields | boost::adaptors::transformed(arrays_to_vectors)) {
         out <<"        " << std::left << std::setw(32) << f.type << " " << f.name<<";\n";
      }
      out << "    };\n\n";

      out << "    template<> struct GetStruct<"<< s <<"> { \n";
      out << "        static const Struct& type() { \n";
      out << "           static Struct result = { \"" << s << "\", \"" << st.base <<"\", {\n";
      for( const auto& f : st.fields ) {
      out << "                {\"" << f.name <<"\", \"" << f.type <<"\"},\n";               
      }
      out << "              }\n";
      out << "           };\n";
      out << "           return result;\n";
      out << "         }\n";
      out << "    };\n\n";
   }


   out << "}} // namespace eos::types\n";

   for( const auto& s : ss.order ) {
      if( ss.typedefs.find( s ) != ss.typedefs.end() ) {
         continue;
      }

      const auto& st = ss.structs[s];
      if( st.base.size() ) {
        out << "FC_REFLECT_DERIVED( eos::types::" << s << ", (eos::types::" << st.base <<"), ";
      } else  {
        out << "FC_REFLECT( eos::types::" <<  std::setw(33) << s << ", ";
      }
      for( const auto& f : st.fields ) {
         out <<"("<<f.name<<")";
      }
      out <<" )\n";
   }

}

int  count_fields( eos::types::SimpleSymbolTable& ss, const eos::types::Struct& st ) {
   if( st.base.size() )
      return st.fields.size() + count_fields( ss, ss.getType( st.base ) );
   return st.fields.size();
}

int main( int argc, char** argv ) {
  try {
     FC_ASSERT( argc > 2, "Usage: ${program} input path/to/out.hpp", ("program",string(argv[0]))  );
     std::ifstream in(argv[1]);

     eos::types::SimpleSymbolTable ss;
     ss.parse(in);

     auto as_json = fc::json::to_pretty_string( ss );
     std:: cerr << as_json << "\n";

     generate_hpp( ss, argv[2] );
     /*
     generate_cpp( ss, argv[2], argv[3] );

     auto w = generate_wren( ss.getType( "Message" ), ss );
     std::cerr << w <<"\n";
     w = generate_wren( ss.getType( "Transfer" ), ss );
     std::cerr << w <<"\n";
     w = generate_wren( ss.getType( "BlockHeader" ), ss );
     std::cerr << w <<"\n";
     w = generate_wren( ss.getType( "Block" ), ss );
     std::cerr << w <<"\n";
     */

     return 0;
  } catch ( const fc::exception& e ) {
     elog( "${e}", ("e",e.to_detail_string()) );
  }
  return -1;
}
