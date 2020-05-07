#include <fc/string.hpp>
#include <fc/utility.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <sstream>
#include <iomanip>
#include <locale>
#include <limits>

/**
 *  Implemented with std::string for now.
 */

namespace fc  {
   class comma_numpunct : public std::numpunct<char>
   {
      protected:
         virtual char do_thousands_sep() const { return ','; }
         virtual std::string do_grouping() const { return "\03"; }
   };

  std::string to_pretty_string( int64_t value )
  {
     std::stringstream ss;
     ss.imbue( {std::locale(), new comma_numpunct} );
     ss << std::fixed << value;
     return ss.str();
  }

#ifdef USE_FC_STRING
  string::string(const char* s, int l) :my(s,l){ }
  string::string(){}
  string::string( const fc::string& c ):my(*c.my) { }
  string::string( string&& m ):my(fc::move(*m.my)) {}
  string::string( const char* c ):my(c){}
  string::string( const_iterator b, const_iterator e ):my(b,e){}
  string::string( const std::string& s ):my(s) {}
  string::string( std::string&& s ):my(fc::move(s)) {}
  string::~string() { }
  string::operator std::string&() { return *my; }
  string::operator const std::string&()const { return *my; }
  char* string::data() { return &my->front(); }

  string::iterator string::begin()            { return &(*this)[0]; }
  string::iterator string::end()              { return &(*this)[size()]; }
  string::const_iterator string::begin()const { return my->c_str(); }
  string::const_iterator string::end()const   { return my->c_str() + my->size(); }

  char&       string::operator[](size_t idx)      { return (*my)[idx]; }
  const char& string::operator[](size_t idx)const { return (*my)[idx]; }

  void       string::reserve(size_t r)           { my->reserve(r); }
  size_t   string::size()const                   { return my->size(); }
  size_t   string::find(char c, size_t p)const { return my->find(c,p); }
  size_t   string::find(const fc::string& str, size_t pos /* = 0 */) const { return my->find(str, pos); }
  size_t   string::find(const char* s, size_t pos /* = 0 */) const { return my->find(s,pos); }
  size_t   string::rfind(char c, size_t p)const { return my->rfind(c,p); }
  size_t   string::rfind(const char* c, size_t p)const { return my->rfind(c,p); }
  size_t   string::rfind(const fc::string& c, size_t p)const { return my->rfind(c,p); }
  size_t   string::find_first_of(const fc::string& str, size_t pos /* = 0 */) const { return my->find_first_of(str, pos); }
  size_t   string::find_first_of(const char* s, size_t pos /* = 0 */) const { return my->find_first_of(s, pos); }

  fc::string& string::replace(size_t pos,  size_t len,  const string& str) { my->replace(pos, len, str); return *this; }
  fc::string& string::replace(size_t pos,  size_t len,  const char* s) { my->replace(pos, len, s); return *this; }

  void       string::clear()                       { my->clear(); }
  void       string::resize( size_t s )          { my->resize(s); }
                                            
  fc::string string::substr( size_t start, size_t len )const { return my->substr(start,len); }
  const char* string::c_str()const                        { return my->c_str(); }

  bool    string::operator == ( const char* s )const   { return *my == s; }
  bool    string::operator == ( const string& s )const { return *my == *s.my; }
  bool    string::operator != ( const string& s )const { return *my != *s.my; }

  string& string::operator =( const string& c )          { *my = *c.my; return *this; }
  string& string::operator =( string&& c )               { *my = fc::move( *c.my ); return *this; }

  string& string::operator+=( const string& s )          { *my += *s.my; return *this; }
  string& string::operator+=( char c )                   { *my += c; return *this; }

  bool operator < ( const string& a, const string& b )   { return *a.my < *b.my; } 
  string operator + ( const string& s, const string& c ) { return string(s) += c; }
  string operator + ( const string& s, char c ) 	 { return string(s) += c; }
#endif // USE_FC_STRING


  int64_t    to_int64( const fc::string& i )
  {
    try
    {
      return boost::lexical_cast<int64_t>(i.c_str(), i.size());
    }
    catch( const boost::bad_lexical_cast& e )
    {
      FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse int64_t" );
    }
    FC_RETHROW_EXCEPTIONS( warn, "${i} => int64_t", ("i",i) )
  }

  uint64_t   to_uint64( const fc::string& i )
  { try {
    try
    {
      return boost::lexical_cast<uint64_t>(i.c_str(), i.size());
    }
    catch( const boost::bad_lexical_cast& e )
    {
      FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse uint64_t" );
    }
    FC_RETHROW_EXCEPTIONS( warn, "${i} => uint64_t", ("i",i) )
  } FC_CAPTURE_AND_RETHROW( (i) ) }

  double     to_double( const fc::string& i)
  {
    try
    {
      return boost::lexical_cast<double>(i.c_str(), i.size());
    }
    catch( const boost::bad_lexical_cast& e )
    {
      FC_THROW_EXCEPTION( parse_error_exception, "Couldn't parse double" );
    }
    FC_RETHROW_EXCEPTIONS( warn, "${i} => double", ("i",i) )
  }

  fc::string to_string(double d)
  {
    // +2 is required to ensure that the double is rounded correctly when read back in.  http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::digits10 + 2) << std::fixed << d;
    return ss.str();
  }

  fc::string to_string( uint64_t d)
  {
    return boost::lexical_cast<std::string>(d);
  }

  fc::string to_string( int64_t d)
  {
    return boost::lexical_cast<std::string>(d);
  }
  fc::string to_string( uint16_t d)
  {
    return boost::lexical_cast<std::string>(d);
  }
  std::string trim( const std::string& s )
  {
      return boost::algorithm::trim_copy(s);
      /*
      std::string cpy(s);
      boost::algorithm::trim(cpy);
      return cpy;
      */
  }
  std::string to_lower( const std::string& s )
  {
     auto tmp = s;
     boost::algorithm::to_lower(tmp);
     return tmp;
  }
  string trim_and_normalize_spaces( const string& s )
  {
     string result = boost::algorithm::trim_copy( s );
     while( result.find( "  " ) != result.npos )
       boost::algorithm::replace_all( result, "  ", " " );
     return result;
  }

} // namespace fc


