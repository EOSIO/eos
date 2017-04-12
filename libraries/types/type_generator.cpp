#include <eos/types/TypeParser.hpp>
#include <fstream>
#include <iomanip>
#include <fc/io/json.hpp>

using std::string;


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
string generate_wren( const EOS::Struct& s, EOS::SimpleSymbolTable& symbols ) {

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

void generate_wren( EOS::SimpleSymbolTable& ss, const char* outfile ) {
  //for( const auto& s : ss.order ) 
}

void generate_hpp( EOS::SimpleSymbolTable& ss, const char* outfile ) {
   wdump((outfile));
   std::ofstream out(outfile);
   out << "#pragma once\n";
   out << "#include <eos/types/native.hpp>\n";
   out << "namespace EOS { \n";
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

      for( const auto& f : st.fields ) {
         string type = f.type;
         if( type.back() == ']' ) 
            type = "vector<" + type.substr(0,type.size()-2) + ">";
         out <<"        " << std::left << std::setw(32) << type << " " << f.name<<";\n";
      }
      out << "    };\n\n";

      out << "    fc::variant toVariant( const " << s << "& t ); \n";
      out << "    void        fromVariant( " << s << "& t, const fc::variant& v ); \n";

      out << "    template<typename Stream>\n";
      out << "    void toBinary( Stream& stream, const " << s << "& t ) { \n";
      if( st.base.size() )
         out << "        EOS::toBinary( stream, static_cast<const " << st.base<<"&>(t) );\n"; 
      for( const auto& f : st.fields ) {
         out << "        EOS::toBinary( stream, t."<< f.name << " );\n";
      }
      out << "    }\n\n";
      out << "    template<typename Stream>\n";
      out << "    void fromBinary( Stream& stream, " << s << "& t ) { \n";
      if( st.base.size() )
         out << "        EOS::fromBinary( stream, static_cast<" << st.base<<"&>(t) );\n"; 
      for( const auto& f : st.fields ) {
         out << "        EOS::fromBinary( stream, t."<< f.name << " );\n";
      }
      out << "    }\n\n";
   }

   out << "} // namespace EOS  \n";

   for( const auto& s : ss.order ) {
      if( ss.typedefs.find( s ) != ss.typedefs.end() ) {
         continue;
      }

      const auto& st = ss.structs[s];
      if( st.base.size() ) {
        out << "FC_REFLECT_DERIVED( EOS::" <<  s << ", (EOS::" << st.base <<"), ";
      } else  {
        out << "FC_REFLECT( EOS::" <<  s << ", ";
      }
      for( const auto& f : st.fields ) {
         out <<"("<<f.name<<")";
      }
      out <<" )\n";
   }

}

int  count_fields( EOS::SimpleSymbolTable& ss, const EOS::Struct& st ) {
   if( st.base.size() )
      return st.fields.size() + count_fields( ss, ss.getType( st.base ) );
   return st.fields.size();
}
                  
void add_fields( EOS::SimpleSymbolTable& ss, std::ofstream& out, const EOS::Struct& st ) {
      if( st.base.size() ) {
        add_fields( ss, out, ss.getType( st.base ) );
      }
      for( const auto& f : st.fields ) {
         out << "        mvo[\""<<f.name<<"\"] = EOS::toVariant( t."<<f.name<<" );\n";
      }
}

void generate_cpp( EOS::SimpleSymbolTable& ss, const char* outfile, const char* hpp ) {
   edump((hpp)(outfile));
   std::ofstream out(outfile);
   out << "#include <eos/types/native.hpp>\n";
   out << "#include <eos/types/" << hpp <<">\n";
   out << "#include <eos/types/native_impl.hpp>\n";
   out << "namespace EOS { \n";
   for( const auto& s : ss.order ) {
      if( ss.typedefs.find( s ) != ss.typedefs.end() ) {
         continue;
      }
   }

   for( const auto& s : ss.order ) {
      if( ss.typedefs.find( s ) != ss.typedefs.end() ) { continue; }
      const auto& st = ss.structs[s];
      out << "    fc::variant toVariant( const " << s << "& t ) { \n";
      out << "        fc::mutable_variant_object mvo; \n";
      out << "        mvo.reserve( " << count_fields(ss, st) <<" ); \n";

      add_fields( ss, out, st );

      out << "       return fc::variant( std::move(mvo) );  \n";
      out << "    }\n\n";

      out << "    void fromVariant( " << s << "& t, const fc::variant& v ) { \n";
      out << "        const fc::variant_object& mvo = v.get_object(); \n";
      for( const auto& f : st.fields ) {
         out << "        { auto itr = mvo.find( \"" << f.name <<"\" );\n";
         out << "        if( itr != mvo.end() ) \n";
         out << "           EOS::fromVariant( t." << f.name << ", itr->value() );}\n";
      }
      out << "    }\n\n";
   }

   out << "} // namespace EOS \n";

   out.close();
}

int main( int argc, char** argv ) {
  try {
     FC_ASSERT( argc > 4, "Usage: ${program} input out.cpp out.hpp path/to/out.hpp", ("program",string(argv[0]))  );
     std::ifstream in(argv[1]);

     EOS::SimpleSymbolTable ss;
     ss.parse(in);

     auto as_json = fc::json::to_pretty_string( ss );
     std:: cerr << as_json << "\n";

     generate_hpp( ss, argv[4] );
     generate_cpp( ss, argv[2], argv[3] );

     /*
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
