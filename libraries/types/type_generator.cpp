/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/types/type_parser.hpp>
#include <fstream>
#include <iomanip>
#include <fc/io/json.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>

using std::string;
namespace eosio { using types::simple_symbol_table; }


bool   is_vector(const string& type) {
   return type.back() == ']';
}

string get_wren_type(const string& type) {
   FC_ASSERT( type.size() );
   if(is_vector(type) )
      return get_wren_type(type.substr(0, type.size() - 2));
   if( type.substr(0,4) == "uint_t" ) {
      if( type == "uint8" ) return "uint_t";
      if( type == "uint16" ) return "uint_t";
      if( type == "uint32" ) return "uint_t";
      if( type == "uint64" ) return "uint_t";
      if( type == "uint128" ) return "uint_t";
      if( type == "uint256" ) return "uint_t";
      return type;
   } else if ( type.substr( 0,3 ) == "int_t" ) {
      if( type == "int8" ) return "int_t";
      if( type == "int16" ) return "int_t";
      if( type == "int32" ) return "int_t";
      if( type == "int64" ) return "int_t";
      if( type == "int128" ) return "int_t";
      if( type == "int256" ) return "int_t";
   }
   return type;
}

string call_unpack( const string& field, const string& type ) {
   auto wren_type = get_wren_type(type);
   if( wren_type == "uint_t" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(4) + " )";
   }
   else if( wren_type == "int_t" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(3) + " )";
   }
   return  "_" + field + ".unpack( stream )";
}
string call_pack( const string& field, const string& type ) {
   auto wren_type = get_wren_type(type);
   if( wren_type == "uint_t" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(4) + " )";
   }
   else if( wren_type == "int_t" ) {
      return "_" + field + ".unpack( stream, " +  type.substr(3) + " )";
   }
   return  "_" + field + ".unpack( stream )";
}

string call_type_constructor( const string& type ) {
   FC_ASSERT( type.size() );
   if(is_vector(type) )
      return "vector[" + get_wren_type(type) + "]";
   return get_wren_type(type) + ".new()";
}
string generate_wren( const eosio::types::struct_t& s, eosio::types::simple_symbol_table& symbols ) {

   std::stringstream ss;
   ss << "class " << s.name;
   if( s.base.size() ) ss << " is " << s.base;
   ss << " {\n";
   ss << "    construct new() { \n";

   if( s.base.size() ) 
   ss << "        super()\n";

   for( const auto& field : s.fields ) {
      ss << "        _" << field.name << " = " << call_type_constructor(symbols.resolve_type_def(field.type) ) <<"\n";
   }
   ss << "    }\n";
   ss << "    unpack( stream ) { \n";
   if( s.base.size() ) 
   ss << "        super.unpack( stream )\n";
   for( const auto& field : s.fields ) {
      ss << "        " << call_unpack( field.name, symbols.resolve_type_def(field.type) ) <<"\n";
   }
   ss << "    }\n";
   ss << "    pack( stream ) { \n";
   if( s.base.size() ) 
   ss << "        super.pack( stream )\n";
   for( const auto& field : s.fields ) {
      ss << "        " << call_pack( field.name, symbols.resolve_type_def(field.type) ) <<"\n";
   }
   ss << "    }\n";
   for( const auto& field : s.fields ) {
      ss << "    " << field.name << " {_" << field.name <<"}\n";
   }

   ss << "}\n";

   return ss.str();
}

void generate_wren( eosio::types::simple_symbol_table& ss, const char* outfile ) {
  //for( const auto& s : ss.order ) 
}

void generate_hpp( eosio::types::simple_symbol_table& ss, const char* outfile ) {
   struct FakeField { std::string type; std::string name; };
   auto arrays_to_vectors = [](eosio::types::field f) {
      if (boost::ends_with<std::string>(f.type, "[]")) {
         std::string type = f.type;
         type.resize(type.size() - 2);
         return FakeField{"vector<" + type + ">", f.name};
      }
      return FakeField{f.type, f.name};
   };

   wdump((outfile));
   std::ofstream out(outfile);
   out << "#pragma once\n";
   out << "namespace eosio { namespace types {\n";
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

      out << "    template<> struct get_struct<"<< s <<"> { \n";
      out << "        static const struct_t& type() { \n";
      out << "           static struct_t result = { \"" << s << "\", \"" << st.base <<"\", {\n";
      for( const auto& f : st.fields ) {
      out << "                {\"" << f.name <<"\", \"" << f.type <<"\"},\n";               
      }
      out << "              }\n";
      out << "           };\n";
      out << "           return result;\n";
      out << "         }\n";
      out << "    };\n\n";
   }


   out << "}} // namespace eosio::types\n";

   for( const auto& s : ss.order ) {
      if( ss.typedefs.find( s ) != ss.typedefs.end() ) {
         continue;
      }

      const auto& st = ss.structs[s];
      if( st.base.size() ) {
        out << "FC_REFLECT_DERIVED( eosio::types::" << s << ", (eosio::types::" << st.base <<"), ";
      } else  {
        out << "FC_REFLECT( eosio::types::" <<  std::setw(33) << s << ", ";
      }
      for( const auto& f : st.fields ) {
         out <<"("<<f.name<<")";
      }
      out <<" )\n";
   }

}

int  count_fields( eosio::types::simple_symbol_table& ss, const eosio::types::struct_t& st ) {
   if( st.base.size() )
      return st.fields.size() + count_fields( ss, ss.get_type(st.base) );
   return st.fields.size();
}

int main( int argc, char** argv ) {
  try {
     FC_ASSERT( argc > 2, "Usage: ${program} input path/to/out.hpp", ("program",string(argv[0]))  );
     std::ifstream in(argv[1]);

     eosio::types::simple_symbol_table ss;
     ss.parse(in);

     auto as_json = fc::json::to_pretty_string( ss );
     std:: cerr << as_json << "\n";

     generate_hpp( ss, argv[2] );
     /*
     generate_cpp( ss, argv[2], argv[3] );

     auto w = generate_wren( ss.getType( "message" ), ss );
     std::cerr << w <<"\n";
     w = generate_wren( ss.getType( "Transfer" ), ss );
     std::cerr << w <<"\n";
     w = generate_wren( ss.getType( "BlockHeader" ), ss );
     std::cerr << w <<"\n";
     w = generate_wren( ss.get_type( "Block" ), ss );
     std::cerr << w <<"\n";
     */

     return 0;
  } catch ( const fc::exception& e ) {
     elog( "${e}", ("e",e.to_detail_string()) );
  }
  return -1;
}
