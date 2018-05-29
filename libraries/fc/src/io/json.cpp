#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
//#include <fc/io/fstream.hpp>
//#include <fc/io/sstream.hpp>
#include <fc/log/logger.hpp>
//#include <utfcpp/utf8.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/filesystem/fstream.hpp>

namespace fc
{
    // forward declarations of provided functions
    template<typename T, json::parse_type parser_type> variant variant_from_stream( T& in );
    template<typename T> char parseEscape( T& in );
    template<typename T> std::string stringFromStream( T& in );
    template<typename T> bool skip_white_space( T& in );
    template<typename T> std::string stringFromToken( T& in );
    template<typename T, json::parse_type parser_type> variant_object objectFromStream( T& in );
    template<typename T, json::parse_type parser_type> variants arrayFromStream( T& in );
    template<typename T, json::parse_type parser_type> variant number_from_stream( T& in );
    template<typename T> variant token_from_stream( T& in );
    void escape_string( const std::string& str, std::ostream& os );
    template<typename T> void to_stream( T& os, const variants& a, json::output_formatting format );
    template<typename T> void to_stream( T& os, const variant_object& o, json::output_formatting format );
    template<typename T> void to_stream( T& os, const variant& v, json::output_formatting format );
    std::string pretty_print( const std::string& v, uint8_t indent );
}

#include <fc/io/json_relaxed.hpp>

namespace fc
{
   template<typename T>
   char parseEscape( T& in )
   {
      if( in.peek() == '\\' )
      {
         try {
            in.get();
            switch( in.peek() )
            {
               case 't':
                  in.get();
                  return '\t';
               case 'n':
                  in.get();
                  return '\n';
               case 'r':
                  in.get();
                  return '\r';
               case '\\':
                  in.get();
                  return '\\';
               default:
                  return in.get();
            }
         } FC_RETHROW_EXCEPTIONS( info, "Stream ended with '\\'" );
      }
	    FC_THROW_EXCEPTION( parse_error_exception, "Expected '\\'"  );
   }

   template<typename T>
   bool skip_white_space( T& in )
   {
       bool skipped = false;
       while( true )
       {
          switch( in.peek() )
          {
             case ' ':
             case '\t':
             case '\n':
             case '\r':
                skipped = true;
                in.get();
                break;
             case '\0':
                FC_THROW_EXCEPTION( eof_exception, "unexpected end of file" );
                break;
             default:
                return skipped;
          }
       }
   }

   template<typename T>
   std::string stringFromStream( T& in )
   {
      std::stringstream token;
      try
      {
         char c = in.peek();

         if( c != '"' )
            FC_THROW_EXCEPTION( parse_error_exception,
                                            "Expected '\"' but read '${char}'",
                                            ("char", string(&c, (&c) + 1) ) );
         in.get();
         while( !in.eof() )
         {
            switch( c = in.peek() )
            {
               case '\\':
                  token << parseEscape( in );
                  break;
               case 0x04:
                  FC_THROW_EXCEPTION( parse_error_exception, "EOF before closing '\"' in string '${token}'",
                                                   ("token", token.str() ) );
               case '"':
                  in.get();
                  return token.str();
               default:
                  token << c;
                  in.get();
            }
         }
         FC_THROW_EXCEPTION( parse_error_exception, "EOF before closing '\"' in string '${token}'",
                                          ("token", token.str() ) );
       } FC_RETHROW_EXCEPTIONS( warn, "while parsing token '${token}'",
                                          ("token", token.str() ) );
   }
   template<typename T>
   std::string stringFromToken( T& in )
   {
      std::stringstream token;
      try
      {
         char c = in.peek();

         while( !in.eof() )
         {
            switch( c = in.peek() )
            {
               case '\\':
                  token << parseEscape( in );
                  break;
               case '\t':
               case ' ':
               case '\n':
                  in.get();
                  return token.str();
               case '\0':
                  FC_THROW_EXCEPTION( eof_exception, "unexpected end of file" );
               default:
                if( isalnum( c ) || c == '_' || c == '-' || c == '.' || c == ':' || c == '/' )
                {
                  token << c;
                  in.get();
                }
                else return token.str();
            }
         }
         return token.str();
      }
      catch( const fc::eof_exception& eof )
      {
         return token.str();
      }
      catch (const std::ios_base::failure&)
      {
         return token.str();
      }

      FC_RETHROW_EXCEPTIONS( warn, "while parsing token '${token}'",
                                          ("token", token.str() ) );
   }

   template<typename T, json::parse_type parser_type>
   variant_object objectFromStream( T& in )
   {
      mutable_variant_object obj;
      try
      {
         char c = in.peek();
         if( c != '{' )
            FC_THROW_EXCEPTION( parse_error_exception,
                                     "Expected '{', but read '${char}'",
                                     ("char",string(&c, &c + 1)) );
         in.get();
         skip_white_space(in);
         while( in.peek() != '}' )
         {
            if( in.peek() == ',' )
            {
               in.get();
               continue;
            }
            if( skip_white_space(in) ) continue;
            string key = stringFromStream( in );
            skip_white_space(in);
            if( in.peek() != ':' )
            {
               FC_THROW_EXCEPTION( parse_error_exception, "Expected ':' after key \"${key}\"",
                                        ("key", key) );
            }
            in.get();
            auto val = variant_from_stream<T, parser_type>( in );

            obj(std::move(key),std::move(val));
            skip_white_space(in);
         }
         if( in.peek() == '}' )
         {
            in.get();
            return obj;
         }
         FC_THROW_EXCEPTION( parse_error_exception, "Expected '}' after ${variant}", ("variant", obj ) );
      }
      catch( const fc::eof_exception& e )
      {
         FC_THROW_EXCEPTION( parse_error_exception, "Unexpected EOF: ${e}", ("e", e.to_detail_string() ) );
      }
      catch( const std::ios_base::failure& e )
      {
         FC_THROW_EXCEPTION( parse_error_exception, "Unexpected EOF: ${e}", ("e", e.what() ) );
      } FC_RETHROW_EXCEPTIONS( warn, "Error parsing object" );
   }

   template<typename T, json::parse_type parser_type>
   variants arrayFromStream( T& in )
   {
      variants ar;
      try
      {
        if( in.peek() != '[' )
           FC_THROW_EXCEPTION( parse_error_exception, "Expected '['" );
        in.get();
        skip_white_space(in);

        while( in.peek() != ']' )
        {
           if( in.peek() == ',' )
           {
              in.get();
              continue;
           }
           if( skip_white_space(in) ) continue;
           ar.push_back( variant_from_stream<T, parser_type>(in) );
           skip_white_space(in);
        }
        if( in.peek() != ']' )
           FC_THROW_EXCEPTION( parse_error_exception, "Expected ']' after parsing ${variant}",
                                    ("variant", ar) );

        in.get();
      } FC_RETHROW_EXCEPTIONS( warn, "Attempting to parse array ${array}",
                                         ("array", ar ) );
      return ar;
   }

   template<typename T, json::parse_type parser_type>
   variant number_from_stream( T& in )
   {
      std::stringstream ss;

      bool  dot = false;
      bool  neg = false;
      if( in.peek() == '-')
      {
        neg = true;
        ss.put( in.get() );
      }
      bool done = false;

      try
      {
        while( !done )
        {
          char c = in.peek();
          switch( c )
          {
              case '.':
                 if (dot)
                    FC_THROW_EXCEPTION(parse_error_exception, "Can't parse a number with two decimal places");
                 dot = true;
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                 ss.put( in.get() );
                 break;
              case '\0':
                 FC_THROW_EXCEPTION( eof_exception, "unexpected end of file" );
              default:
                 if( isalnum( c ) )
                 {
                    return ss.str() + stringFromToken( in );
                 }
                done = true;
                break;
          }
        }
      }
      catch (fc::eof_exception&)
      {
      }
      catch (const std::ios_base::failure&)
      {
      }
      std::string str = ss.str();
      if (str == "-." || str == ".") // check the obviously wrong things we could have encountered
        FC_THROW_EXCEPTION(parse_error_exception, "Can't parse token \"${token}\" as a JSON numeric constant", ("token", str));
      if( dot )
        return parser_type == json::legacy_parser_with_string_doubles ? variant(str) : variant(to_double(str));
      if( neg )
        return to_int64(str);
      return to_uint64(str);
   }
   template<typename T>
   variant token_from_stream( T& in )
   {
      std::stringstream ss;
      ss.exceptions( std::ifstream::badbit );
      bool received_eof = false;
      bool done = false;

      try
      {
        char c;
        while((c = in.peek()) && !done)
        {
           switch( c )
           {
              case 'n':
              case 'u':
              case 'l':
              case 't':
              case 'r':
              case 'e':
              case 'f':
              case 'a':
              case 's':
                 ss.put( in.get() );
                 break;
              default:
                 done = true;
                 break;
           }
        }
      }
      catch (fc::eof_exception&)
      {
        received_eof = true;
      }
      catch (const std::ios_base::failure&)
      {
        received_eof = true;
      }

      // we can get here either by processing a delimiter as in "null,"
      // an EOF like "null<EOF>", or an invalid token like "nullZ"
      std::string str = ss.str();
      if( str == "null" )
        return variant();
      if( str == "true" )
        return true;
      if( str == "false" )
        return false;
      else
      {
        if (received_eof)
        {
          if (str.empty())
            FC_THROW_EXCEPTION( parse_error_exception, "Unexpected EOF" );
          else
            return str;
        }
        else
        {
          // if we've reached this point, we've either seen a partial
          // token ("tru<EOF>") or something our simple parser couldn't
          // make out ("falfe")
          // A strict JSON parser would signal this as an error, but we
          // will just treat the malformed token as an un-quoted string.
          return str + stringFromToken(in);;
        }
      }
   }


   template<typename T, json::parse_type parser_type>
   variant variant_from_stream( T& in )
   {
      skip_white_space(in);
      variant var;
      while( 1 )
      {
         signed char c = in.peek();
         switch( c )
         {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
              in.get();
              continue;
            case '"':
              return stringFromStream( in );
            case '{':
              return objectFromStream<T, parser_type>( in );
            case '[':
              return arrayFromStream<T, parser_type>( in );
            case '-':
            case '.':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              return number_from_stream<T, parser_type>( in );
            // null, true, false, or 'warning' / string
            case 'n':
            case 't':
            case 'f':
              return token_from_stream( in );
            case 0x04: // ^D end of transmission
            case EOF:
            case '\0':
              FC_THROW_EXCEPTION( eof_exception, "unexpected end of file" );
            default:
              FC_THROW_EXCEPTION( parse_error_exception, "Unexpected char '${c}' in \"${s}\"",
                                 ("c", c)("s", stringFromToken(in)) );
         }
      }
	  return variant();
   }


   /** the purpose of this check is to verify that we will not get a stack overflow in the recursive descent parser */
   void check_string_depth( const string& utf8_str  )
   {
      int32_t open_object = 0;
      int32_t open_array  = 0;
      for( auto c : utf8_str )
      {
         switch( c )
         {
            case '{': open_object++; break;
            case '}': open_object--; break;
            case '[': open_array++; break;
            case ']': open_array--; break;
            default: break;
         }
         FC_ASSERT( open_object < 100 && open_array < 100, "object graph too deep", ("object depth",open_object)("array depth", open_array) );
      }
   }

   variant json::from_string( const std::string& utf8_str, parse_type ptype )
   { try {
      check_string_depth( utf8_str );

      std::stringstream in( utf8_str );
      //in.exceptions( std::ifstream::eofbit );
      switch( ptype )
      {
          case legacy_parser:
              return variant_from_stream<std::stringstream, legacy_parser>( in );
          case legacy_parser_with_string_doubles:
              return variant_from_stream<std::stringstream, legacy_parser_with_string_doubles>( in );
          case strict_parser:
              return json_relaxed::variant_from_stream<std::stringstream, true>( in );
          case relaxed_parser:
              return json_relaxed::variant_from_stream<std::stringstream, false>( in );
          default:
              FC_ASSERT( false, "Unknown JSON parser type {ptype}", ("ptype", ptype) );
      }
   } FC_RETHROW_EXCEPTIONS( warn, "", ("str",utf8_str) ) }

   variants json::variants_from_string( const std::string& utf8_str, parse_type ptype )
   { try {
      check_string_depth( utf8_str );
      variants result;
      std::stringstream in( utf8_str );
      //in.exceptions( std::ifstream::eofbit );
      try {
         while( true )
         {
           // result.push_back( variant_from_stream( in ));
           result.push_back(json_relaxed::variant_from_stream<std::stringstream, false>( in ));
         }
      } catch ( const fc::eof_exception& ){}
      return result;
   } FC_RETHROW_EXCEPTIONS( warn, "", ("str",utf8_str) ) }
   /*
   void toUTF8( const char str, std::ostream& os )
   {
      // validate str == valid utf8
      utf8::replace_invalid( &str, &str + 1, std::ostream_iterator<char>(os) );
   }

   void toUTF8( const wchar_t c, std::ostream& os )
   {
      utf8::utf16to8( &c, (&c)+1, std::ostream_iterator<char>(os) );
   }
   */

   /**
    *  Convert '\t', '\a', '\n', '\\' and '"'  to "\t\a\n\\\""
    *
    *  All other characters are printed as UTF8.
    */
   void escape_string( const string& str, std::ostream& os )
   {
      os << '"';
      for( auto itr = str.begin(); itr != str.end(); ++itr )
      {
         switch( *itr )
         {
            case '\b':        // \x08
               os << "\\b";
               break;
            case '\f':        // \x0c
               os << "\\f";
               break;
            case '\n':        // \x0a
               os << "\\n";
               break;
            case '\r':        // \x0d
               os << "\\r";
               break;
            case '\t':        // \x09
               os << "\\t";
               break;
            case '\\':
               os << "\\\\";
               break;
            case '\"':
               os << "\\\"";
               break;
            case '\x00': os << "\\u0000"; break;
            case '\x01': os << "\\u0001"; break;
            case '\x02': os << "\\u0002"; break;
            case '\x03': os << "\\u0003"; break;
            case '\x04': os << "\\u0004"; break;
            case '\x05': os << "\\u0005"; break;
            case '\x06': os << "\\u0006"; break;
            case '\x07': os << "\\u0007"; break; // \a is not valid JSON
         // case '\x08': os << "\\u0008"; break; // \b
         // case '\x09': os << "\\u0009"; break; // \t
         // case '\x0a': os << "\\u000a"; break; // \n
            case '\x0b': os << "\\u000b"; break;
         // case '\x0c': os << "\\u000c"; break; // \f
         // case '\x0d': os << "\\u000d"; break; // \r
            case '\x0e': os << "\\u000e"; break;
            case '\x0f': os << "\\u000f"; break;

            case '\x10': os << "\\u0010"; break;
            case '\x11': os << "\\u0011"; break;
            case '\x12': os << "\\u0012"; break;
            case '\x13': os << "\\u0013"; break;
            case '\x14': os << "\\u0014"; break;
            case '\x15': os << "\\u0015"; break;
            case '\x16': os << "\\u0016"; break;
            case '\x17': os << "\\u0017"; break;
            case '\x18': os << "\\u0018"; break;
            case '\x19': os << "\\u0019"; break;
            case '\x1a': os << "\\u001a"; break;
            case '\x1b': os << "\\u001b"; break;
            case '\x1c': os << "\\u001c"; break;
            case '\x1d': os << "\\u001d"; break;
            case '\x1e': os << "\\u001e"; break;
            case '\x1f': os << "\\u001f"; break;

            default:
               os << *itr;
               //toUTF8( *itr, os );
         }
      }
      os << '"';
   }
   std::ostream& json::to_stream( std::ostream& out, const std::string& str )
   {
        escape_string( str, out );
        return out;
   }

   template<typename T>
   void to_stream( T& os, const variants& a, json::output_formatting format )
   {
      os << '[';
      auto itr = a.begin();

      while( itr != a.end() )
      {
         to_stream( os, *itr, format );
         ++itr;
         if( itr != a.end() )
            os << ',';
      }
      os << ']';
   }
   template<typename T>
   void to_stream( T& os, const variant_object& o, json::output_formatting format )
   {
       os << '{';
       auto itr = o.begin();

       while( itr != o.end() )
       {
          escape_string( itr->key(), os );
          os << ':';
          to_stream( os, itr->value(), format );
          ++itr;
          if( itr != o.end() )
             os << ',';
       }
       os << '}';
   }

   template<typename T>
   void to_stream( T& os, const variant& v, json::output_formatting format )
   {
      switch( v.get_type() )
      {
         case variant::null_type:
              os << "null";
              return;
         case variant::int64_type:
         {
              int64_t i = v.as_int64();
              if( format == json::stringify_large_ints_and_doubles &&
                  i > 0xffffffff )
                 os << '"'<<v.as_string()<<'"';
              else
                 os << i;

              return;
         }
         case variant::uint64_type:
         {
              uint64_t i = v.as_uint64();
              if( format == json::stringify_large_ints_and_doubles &&
                  i > 0xffffffff )
                 os << '"'<<v.as_string()<<'"';
              else
                 os << i;

              return;
         }
         case variant::double_type:
              if (format == json::stringify_large_ints_and_doubles)
                 os << '"'<<v.as_string()<<'"';
              else
                 os << v.as_string();
              return;
         case variant::bool_type:
              os << v.as_string();
              return;
         case variant::string_type:
              escape_string( v.get_string(), os );
              return;
         case variant::blob_type:
              escape_string( v.as_string(), os );
              return;
         case variant::array_type:
           {
              const variants&  a = v.get_array();
              to_stream( os, a, format );
              return;
           }
         case variant::object_type:
           {
              const variant_object& o =  v.get_object();
              to_stream(os, o, format );
              return;
           }
      }
   }

   std::string   json::to_string( const variant& v, output_formatting format /* = stringify_large_ints_and_doubles */ )
   {
      std::stringstream ss;
      fc::to_stream( ss, v, format );
      return ss.str();
   }


    std::string pretty_print( const std::string& v, uint8_t indent ) {
      int level = 0;
      std::stringstream ss;
      bool first = false;
      bool quote = false;
      bool escape = false;
      for( uint32_t i = 0; i < v.size(); ++i ) {
         switch( v[i] ) {
            case '\\':
              if( !escape ) {
                if( quote )
                  escape = true;
              } else { escape = false; }
              ss<<v[i];
              break;
            case ':':
              if( !quote ) {
                ss<<": ";
              } else {
                ss<<':';
              }
              break;
            case '"':
              if( first ) {
                 ss<<'\n';
                 for( int i = 0; i < level*indent; ++i ) ss<<' ';
                 first = false;
              }
              if( !escape ) {
                quote = !quote;
              }
              escape = false;
              ss<<'"';
              break;
            case '{':
            case '[':
              ss<<v[i];
              if( !quote ) {
                ++level;
                first = true;
              }else {
                escape = false;
              }
              break;
            case '}':
            case ']':
              if( !quote ) {
                if( v[i-1] != '[' && v[i-1] != '{' ) {
                  ss<<'\n';
                }
                --level;
                if( !first ) {
                  for( int i = 0; i < level*indent; ++i ) ss<<' ';
                }
                first = false;
                ss<<v[i];
                break;
              } else {
                escape = false;
                ss<<v[i];
              }
              break;
            case ',':
              if( !quote ) {
                ss<<',';
                first = true;
              } else {
                escape = false;
                ss<<',';
              }
              break;
            case 'n':
              //If we're in quotes and see a \n, just print it literally but unset the escape flag.
              if( quote && escape )
                escape = false;
              //No break; fall through to default case
            default:
              if( first ) {
                 ss<<'\n';
                 for( int i = 0; i < level*indent; ++i ) ss<<' ';
                 first = false;
              }
              ss << v[i];
         }
      }
      return ss.str();
    }



   std::string json::to_pretty_string( const variant& v, output_formatting format /* = stringify_large_ints_and_doubles */ )
   {
	   return pretty_print(to_string(v, format), 2);
   }

   void json::save_to_file( const variant& v, const fc::path& fi, bool pretty, output_formatting format /* = stringify_large_ints_and_doubles */ )
   {
      if( pretty )
      {
        auto str = json::to_pretty_string( v, format );
        std::ofstream o(fi.generic_string().c_str());
        o.write( str.c_str(), str.size() );
      }
      else
      {
       std::ofstream o(fi.generic_string().c_str());
       fc::to_stream( o, v, format );
      }
   }
   variant json::from_file( const fc::path& p, parse_type ptype )
   {
      //auto tmp = std::make_shared<fc::ifstream>( p, ifstream::binary );
      //auto tmp = std::make_shared<std::ifstream>( p.generic_string().c_str(), std::ios::binary );
      //buffered_istream bi( tmp );
      boost::filesystem::ifstream bi( p, std::ios::binary );
      switch( ptype )
      {
          case legacy_parser:
              return variant_from_stream<boost::filesystem::ifstream, legacy_parser>( bi );
          case legacy_parser_with_string_doubles:
              return variant_from_stream<boost::filesystem::ifstream, legacy_parser_with_string_doubles>( bi );
          case strict_parser:
              return json_relaxed::variant_from_stream<boost::filesystem::ifstream, true>( bi );
          case relaxed_parser:
              return json_relaxed::variant_from_stream<boost::filesystem::ifstream, false>( bi );
          default:
              FC_ASSERT( false, "Unknown JSON parser type {ptype}", ("ptype", ptype) );
      }
   }
   /*
   variant json::from_stream( buffered_istream& in, parse_type ptype )
   {
      switch( ptype )
      {
          case legacy_parser:
              return variant_from_stream<fc::buffered_istream, legacy_parser>( in );
          case legacy_parser_with_string_doubles:
              return variant_from_stream<fc::buffered_istream, legacy_parser_with_string_doubles>( in );
          case strict_parser:
              return json_relaxed::variant_from_stream<buffered_istream, true>( in );
          case relaxed_parser:
              return json_relaxed::variant_from_stream<buffered_istream, false>( in );
          default:
              FC_ASSERT( false, "Unknown JSON parser type {ptype}", ("ptype", ptype) );
      }
   }
   */

   std::ostream& json::to_stream( std::ostream& out, const variant& v, output_formatting format /* = stringify_large_ints_and_doubles */ )
   {
      fc::to_stream( out, v, format );
      return out;
   }
   std::ostream& json::to_stream( std::ostream& out, const variants& v, output_formatting format /* = stringify_large_ints_and_doubles */ )
   {
      fc::to_stream( out, v, format );
      return out;
   }
   std::ostream& json::to_stream( std::ostream& out, const variant_object& v, output_formatting format /* = stringify_large_ints_and_doubles */ )
   {
      fc::to_stream( out, v, format );
      return out;
   }

   bool json::is_valid( const std::string& utf8_str, parse_type ptype )
   {
      if( utf8_str.size() == 0 ) return false;
      std::stringstream in( utf8_str );
      switch( ptype )
      {
          case legacy_parser:
              variant_from_stream<std::stringstream, legacy_parser>( in );
              break;
          case legacy_parser_with_string_doubles:
              variant_from_stream<std::stringstream, legacy_parser_with_string_doubles>( in );
              break;
          case strict_parser:
              json_relaxed::variant_from_stream<std::stringstream, true>( in );
              break;
          case relaxed_parser:
              json_relaxed::variant_from_stream<std::stringstream, false>( in );
              break;
          default:
              FC_ASSERT( false, "Unknown JSON parser type {ptype}", ("ptype", ptype) );
      }
      try { in.peek(); } catch ( const eof_exception& e ) { return true; }
      return false;
   }

} // fc
