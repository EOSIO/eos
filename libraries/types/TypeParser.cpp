#include <eos/types/TypeParser.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace EOS {
   void AbstractSymbolTable::parse( std::istream& in ) {
      vector<string> line_tokens;

      bool   in_struct = false;
      Struct current;

      auto finishStruct = [&](){
        FC_ASSERT( current.fields.size() > 0, "A struct must specify at least one field" );
        this->addType( current );
        current.fields.clear();
        current.base.clear();
        in_struct = false;
      };

      int linenum = -1;
      for( string line; std::getline(in, line); ) {
         ++linenum;
         if( !line.size() ) continue;
         line = line.substr( 0, line.find( '#' ) );
         line = fc::trim(line);
         if( !line.size() ) continue;
         std::cerr << line << "\n";

         line_tokens.clear();
         split( line_tokens, line, boost::is_any_of( " \t" ), boost::token_compress_on );

         if( !line_tokens.size() ) continue;
         if( line_tokens[0] == "struct" ) {
            if( in_struct ) finishStruct();
            in_struct = true;
            FC_ASSERT( line_tokens.size() >= 2, "Expected a struct name" );
            current.name = line_tokens[1];
            if( line_tokens.size() > 2 ) {
               FC_ASSERT( line_tokens.size() == 4, "Expected a struct name" );
               FC_ASSERT( line_tokens[2] == "inherits" );
               current.base = line_tokens[3];
            }
         } else if( line_tokens[0] == "typedef" ) {
            if( in_struct ) finishStruct();
            FC_ASSERT( line_tokens.size() == 3, "Expected a struct name" );
            this->addTypeDef( line_tokens[1], line_tokens[2] );
         } else if( in_struct ) { // parse field
            idump((line_tokens));
            FC_ASSERT( line_tokens.size() == 2, "a field must be two tokens long" );
            const auto& name = line_tokens[0];
            const auto& type = line_tokens[1];
            FC_ASSERT( name.size() > 0 );
            current.fields.emplace_back( Field{name,type} );
         }
         else
           FC_ASSERT( false, "Invalid token ${t} on line ${n}", ("t",line_tokens[0])("n",linenum) );
      }
   }

} // namespace EOS
