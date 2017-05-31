#include <fc/io/sstream.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <sstream>

namespace fc {
  class stringstream::impl {
    public:
    impl( fc::string&s )
    :ss( s )
    { ss.exceptions( std::stringstream::badbit ); }

    impl( const fc::string&s )
    :ss( s )
    { ss.exceptions( std::stringstream::badbit ); }

    impl(){ss.exceptions( std::stringstream::badbit ); }
    
    std::stringstream ss;
  };

  stringstream::stringstream( fc::string& s )
  :my(s) {
  }
  stringstream::stringstream( const fc::string& s )
  :my(s) {
  }
  stringstream::stringstream(){}
  stringstream::~stringstream(){}


  fc::string stringstream::str(){
    return my->ss.str();//.c_str();//*reinterpret_cast<fc::string*>(&st);
  }

  void stringstream::str(const fc::string& s) {
    my->ss.str(s);
  }

  void stringstream::clear() {
    my->ss.clear();
  }


  bool     stringstream::eof()const {
    return my->ss.eof();
  }
  size_t stringstream::writesome( const char* buf, size_t len ) {
    my->ss.write(buf,len);
    if( my->ss.eof() )
    {
       FC_THROW_EXCEPTION( eof_exception, "stringstream" );
    }
    return len;
  }
  size_t stringstream::writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
  {
    return writesome(buf.get() + offset, len);
  }

  size_t   stringstream::readsome( char* buf, size_t len ) {
    size_t r = static_cast<size_t>(my->ss.readsome(buf,len));
    if( my->ss.eof() || r == 0 )
    {
       FC_THROW_EXCEPTION( eof_exception, "stringstream" );
    }
    return r;
  }
  size_t   stringstream::readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset )
  {
    return readsome(buf.get() + offset, len);
  }


  void     stringstream::close(){ my->ss.flush(); };
  void     stringstream::flush(){ my->ss.flush(); };

  /*
  istream&   stringstream::read( char* buf, size_t len ) {
    my->ss.read(buf,len);
    return *this;
  }
  istream& stringstream::read( int64_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint64_t&    v ) { my->ss >> v; return *this; }
  istream& stringstream::read( int32_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint32_t&    v ) { my->ss >> v; return *this; }
  istream& stringstream::read( int16_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint16_t&    v ) { my->ss >> v; return *this; }
  istream& stringstream::read( int8_t&      v ) { my->ss >> v; return *this; }
  istream& stringstream::read( uint8_t&     v ) { my->ss >> v; return *this; }
  istream& stringstream::read( float&       v ) { my->ss >> v; return *this; }
  istream& stringstream::read( double&      v ) { my->ss >> v; return *this; }
  istream& stringstream::read( bool&        v ) { my->ss >> v; return *this; }
  istream& stringstream::read( char&        v ) { my->ss >> v; return *this; }
  istream& stringstream::read( fc::string&  v ) { my->ss >> *reinterpret_cast<std::string*>(&v); return *this; }

  ostream& stringstream::write( const fc::string& s) {
    my->ss.write( s.c_str(), s.size() );
    return *this;
  }
  */

  char     stringstream::peek() 
  { 
    char c = my->ss.peek(); 
    if( my->ss.eof() )
    {
       FC_THROW_EXCEPTION( eof_exception, "stringstream" );
    }
    return c;
  }
} 


