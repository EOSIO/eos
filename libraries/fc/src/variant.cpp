#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <fc/exception/exception.hpp>
#include <string.h>
#include <fc/crypto/base64.hpp>
#include <fc/crypto/hex.hpp>
#include <boost/scoped_array.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>
#include <algorithm>

namespace fc
{
/**
 *  The TypeID is stored in the 'last byte' of the variant.
 */
void set_variant_type( variant* v, variant::type_id t)
{
   char* data = reinterpret_cast<char*>(v);
   data[ sizeof(variant) -1 ] = t;
}

variant::variant()
{
   set_variant_type( this, null_type );
}

variant::variant( fc::nullptr_t )
{
   set_variant_type( this, null_type );
}

variant::variant( uint8_t val )
{
   *reinterpret_cast<uint64_t*>(this)  = val;
   set_variant_type( this, uint64_type );
}

variant::variant( int8_t val )
{
   *reinterpret_cast<int64_t*>(this)  = val;
   set_variant_type( this, int64_type );
}

variant::variant( uint16_t val )
{
   *reinterpret_cast<uint64_t*>(this)  = val;
   set_variant_type( this, uint64_type );
}

variant::variant( int16_t val )
{
   *reinterpret_cast<int64_t*>(this)  = val;
   set_variant_type( this, int64_type );
}

variant::variant( uint32_t val )
{
   *reinterpret_cast<uint64_t*>(this)  = val;
   set_variant_type( this, uint64_type );
}

variant::variant( int32_t val )
{
   *reinterpret_cast<int64_t*>(this)  = val;
   set_variant_type( this, int64_type );
}

variant::variant( uint64_t val )
{
   *reinterpret_cast<uint64_t*>(this)  = val;
   set_variant_type( this, uint64_type );
}

variant::variant( int64_t val )
{
   *reinterpret_cast<int64_t*>(this)  = val;
   set_variant_type( this, int64_type );
}

variant::variant( float val )
{
   *reinterpret_cast<double*>(this)  = val;
   set_variant_type( this, double_type );
}

variant::variant( double val )
{
   *reinterpret_cast<double*>(this)  = val;
   set_variant_type( this, double_type );
}

variant::variant( bool val )
{
   *reinterpret_cast<bool*>(this)  = val;
   set_variant_type( this, bool_type );
}

variant::variant( char* str )
{
   *reinterpret_cast<string**>(this)  = new string( str );
   set_variant_type( this, string_type );
}

variant::variant( const char* str )
{
   *reinterpret_cast<string**>(this)  = new string( str );
   set_variant_type( this, string_type );
}

// TODO: do a proper conversion to utf8
variant::variant( wchar_t* str )
{
   size_t len = wcslen(str);
   boost::scoped_array<char> buffer(new char[len]);
   for (unsigned i = 0; i < len; ++i)
     buffer[i] = (char)str[i];
   *reinterpret_cast<string**>(this)  = new string(buffer.get(), len);
   set_variant_type( this, string_type );
}

// TODO: do a proper conversion to utf8
variant::variant( const wchar_t* str )
{
   size_t len = wcslen(str);
   boost::scoped_array<char> buffer(new char[len]);
   for (unsigned i = 0; i < len; ++i)
     buffer[i] = (char)str[i];
   *reinterpret_cast<string**>(this)  = new string(buffer.get(), len);
   set_variant_type( this, string_type );
}

variant::variant( fc::string val )
{
   *reinterpret_cast<string**>(this)  = new string( fc::move(val) );
   set_variant_type( this, string_type );
}
variant::variant( blob val )
{
   *reinterpret_cast<blob**>(this)  = new blob( fc::move(val) );
   set_variant_type( this, blob_type );
}

variant::variant( variant_object obj)
{
   *reinterpret_cast<variant_object**>(this)  = new variant_object(fc::move(obj));
   set_variant_type(this,  object_type );
}
variant::variant( mutable_variant_object obj)
{
   *reinterpret_cast<variant_object**>(this)  = new variant_object(fc::move(obj));
   set_variant_type(this,  object_type );
}

variant::variant( variants arr )
{
   *reinterpret_cast<variants**>(this)  = new variants(fc::move(arr));
   set_variant_type(this,  array_type );
}


typedef const variant_object* const_variant_object_ptr; 
typedef const variants* const_variants_ptr; 
typedef const blob*   const_blob_ptr; 
typedef const string* const_string_ptr;

void variant::clear()
{
   switch( get_type() )
   {
     case object_type:
        delete *reinterpret_cast<variant_object**>(this);
        break;
     case array_type:
        delete *reinterpret_cast<variants**>(this);
        break;
     case string_type:
        delete *reinterpret_cast<string**>(this);
        break;
     default:
        break;
   }
   set_variant_type( this, null_type );
}

variant::variant( const variant& v )
{
   switch( v.get_type() )
   {
       case object_type:
          *reinterpret_cast<variant_object**>(this)  = 
             new variant_object(**reinterpret_cast<const const_variant_object_ptr*>(&v));
          set_variant_type( this, object_type );
          return;
       case array_type:
          *reinterpret_cast<variants**>(this)  = 
             new variants(**reinterpret_cast<const const_variants_ptr*>(&v));
          set_variant_type( this,  array_type );
          return;
       case string_type:
          *reinterpret_cast<string**>(this)  = 
             new string(**reinterpret_cast<const const_string_ptr*>(&v) );
          set_variant_type( this, string_type );
          return;
       default:
          memcpy( this, &v, sizeof(v) );
   }
}

variant::variant( variant&& v )
{
   memcpy( this, &v, sizeof(v) );
   set_variant_type( &v, null_type );
}

variant::~variant()
{
   clear();
}

variant& variant::operator=( variant&& v )
{
   if( this == &v ) return *this;
   clear();
   memcpy( (char*)this, (char*)&v, sizeof(v) );
   set_variant_type( &v, null_type ); 
   return *this;
}

variant& variant::operator=( const variant& v )
{
   if( this == &v ) 
      return *this;

   clear();
   switch( v.get_type() )
   {
      case object_type:
         *reinterpret_cast<variant_object**>(this)  = 
            new variant_object((**reinterpret_cast<const const_variant_object_ptr*>(&v)));
         break;
      case array_type:
         *reinterpret_cast<variants**>(this)  = 
            new variants((**reinterpret_cast<const const_variants_ptr*>(&v)));
         break;
      case string_type:
         *reinterpret_cast<string**>(this)  = new string((**reinterpret_cast<const const_string_ptr*>(&v)) );
         break;

      default:
         memcpy( this, &v, sizeof(v) );
   }
   set_variant_type( this, v.get_type() );
   return *this;
}

void  variant::visit( const visitor& v )const
{
   switch( get_type() )
   {
      case null_type:
         v.handle();
         return;
      case int64_type:
         v.handle( *reinterpret_cast<const int64_t*>(this) );
         return;
      case uint64_type:
         v.handle( *reinterpret_cast<const uint64_t*>(this) );
         return;
      case double_type:
         v.handle( *reinterpret_cast<const double*>(this) );
         return;
      case bool_type:
         v.handle( *reinterpret_cast<const bool*>(this) );
         return;
      case string_type:
         v.handle( **reinterpret_cast<const const_string_ptr*>(this) );
         return;
      case array_type:
         v.handle( **reinterpret_cast<const const_variants_ptr*>(this) );
         return;
      case object_type:
         v.handle( **reinterpret_cast<const const_variant_object_ptr*>(this) );
         return;
      default:
         FC_THROW_EXCEPTION( assert_exception, "Invalid Type / Corrupted Memory" );
   }
}

variant::type_id variant::get_type()const
{
   return (type_id)reinterpret_cast<const char*>(this)[sizeof(*this)-1];
}

bool variant::is_null()const
{
   return get_type() == null_type;
}

bool variant::is_string()const
{
   return get_type() == string_type;
}
bool variant::is_bool()const
{
   return get_type() == bool_type;
}
bool variant::is_double()const
{
   return get_type() == double_type;
}
bool variant::is_uint64()const
{
   return get_type() == uint64_type;
}
bool variant::is_int64()const
{
   return get_type() == int64_type;
}

bool variant::is_integer()const
{
   switch( get_type() )
   {
      case int64_type:
      case uint64_type:
      case bool_type:
         return true;
      default:
         return false;
   }
   return false;
}
bool variant::is_numeric()const
{
   switch( get_type() )
   {
      case int64_type:
      case uint64_type:
      case double_type:
      case bool_type:
         return true;
      default:
         return false;
   }
   return false;
}

bool variant::is_object()const
{
   return get_type() == object_type;
}

bool variant::is_array()const
{
   return get_type() == array_type;
}
bool variant::is_blob()const
{
   return get_type() == blob_type;
}

int64_t variant::as_int64()const
{
   switch( get_type() )
   {
      case string_type:
          return to_int64(**reinterpret_cast<const const_string_ptr*>(this)); 
      case double_type:
          return int64_t(*reinterpret_cast<const double*>(this));
      case int64_type:
          return *reinterpret_cast<const int64_t*>(this);
      case uint64_type:
          return int64_t(*reinterpret_cast<const uint64_t*>(this));
      case bool_type:
          return *reinterpret_cast<const bool*>(this);
      case null_type:
          return 0;
      default:
         FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to int64", ("type", get_type()) );
   }
}

uint64_t variant::as_uint64()const
{ try {
   switch( get_type() )
   {
      case string_type:
          return to_uint64(**reinterpret_cast<const const_string_ptr*>(this)); 
      case double_type:
          return static_cast<uint64_t>(*reinterpret_cast<const double*>(this));
      case int64_type:
          return static_cast<uint64_t>(*reinterpret_cast<const int64_t*>(this));
      case uint64_type:
          return *reinterpret_cast<const uint64_t*>(this);
      case bool_type:
          return static_cast<uint64_t>(*reinterpret_cast<const bool*>(this));
      case null_type:
          return 0;
      default:
         FC_THROW_EXCEPTION( bad_cast_exception,"Invalid cast from ${type} to uint64", ("type",get_type()));
   }
} FC_CAPTURE_AND_RETHROW( (*this) ) }


double  variant::as_double()const
{
   switch( get_type() )
   {
      case string_type:
          return to_double(**reinterpret_cast<const const_string_ptr*>(this)); 
      case double_type:
          return *reinterpret_cast<const double*>(this);
      case int64_type:
          return static_cast<double>(*reinterpret_cast<const int64_t*>(this));
      case uint64_type:
          return static_cast<double>(*reinterpret_cast<const uint64_t*>(this));
      case bool_type:
          return *reinterpret_cast<const bool*>(this);
      case null_type:
          return 0;
      default:
         FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to double", ("type",get_type()) );
   }
}

bool  variant::as_bool()const
{
   switch( get_type() )
   {
      case string_type:
      {
          const string& s = **reinterpret_cast<const const_string_ptr*>(this);
          if( s == "true" )
             return true;
          if( s == "false" )
             return false;
          FC_THROW_EXCEPTION( bad_cast_exception, "Cannot convert string to bool (only \"true\" or \"false\" can be converted)" );
      }
      case double_type:
          return *reinterpret_cast<const double*>(this) != 0.0;
      case int64_type:
          return *reinterpret_cast<const int64_t*>(this) != 0;
      case uint64_type:
          return *reinterpret_cast<const uint64_t*>(this) != 0;
      case bool_type:
          return *reinterpret_cast<const bool*>(this);
      case null_type:
          return false;
      default:
         FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to bool" , ("type",get_type()));
   }
}

string    variant::as_string()const
{
   switch( get_type() )
   {
      case string_type:
          return **reinterpret_cast<const const_string_ptr*>(this); 
      case double_type:
          return to_string(*reinterpret_cast<const double*>(this)); 
      case int64_type:
          return to_string(*reinterpret_cast<const int64_t*>(this)); 
      case uint64_type:
          return to_string(*reinterpret_cast<const uint64_t*>(this)); 
      case bool_type:
          return *reinterpret_cast<const bool*>(this) ? "true" : "false";
      case blob_type:
          if( get_blob().data.size() )
             return base64_encode( get_blob().data.data(), get_blob().data.size() ) + "=";
          return string();
      case null_type:
          return string();
      default:
      FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to string", ("type", get_type() ) );
   }
}

                            
/// @throw if get_type() != array_type | null_type
variants&         variant::get_array()
{
  if( get_type() == array_type )
     return **reinterpret_cast<variants**>(this);
   
  FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to Array", ("type",get_type()) );
}
blob&         variant::get_blob()
{
  if( get_type() == blob_type )
     return **reinterpret_cast<blob**>(this);
   
  FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to Blob", ("type",get_type()) );
}
const blob&         variant::get_blob()const
{
  if( get_type() == blob_type )
     return **reinterpret_cast<const const_blob_ptr*>(this);
   
  FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to Blob", ("type",get_type()) );
}

blob variant::as_blob()const
{
   switch( get_type() )
   {
      case null_type: return blob();
      case blob_type: return get_blob();
      case string_type:
      {
         const string& str = get_string();
         if( str.size() == 0 ) return blob();
         if( str.back() == '=' )
         {
            std::string b64 = base64_decode( get_string() );
            return blob( { std::vector<char>( b64.begin(), b64.end() ) } );
         }
         return blob( { std::vector<char>( str.begin(), str.end() ) } );
      }
      case object_type:
      case array_type:
         FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to Blob", ("type",get_type()) );
      default:
         return blob( { std::vector<char>( (char*)&_data, (char*)&_data + sizeof(_data) ) } );
   }
}


/// @throw if get_type() != array_type 
const variants&       variant::get_array()const
{
  if( get_type() == array_type )
     return **reinterpret_cast<const const_variants_ptr*>(this);
  FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to Array", ("type",get_type()) );
}


/// @throw if get_type() != object_type | null_type
variant_object&        variant::get_object()
{
  if( get_type() == object_type )
     return **reinterpret_cast<variant_object**>(this);
  FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from ${type} to Object", ("type",get_type()) );
}

const variant& variant::operator[]( const char* key )const
{
    return get_object()[key];
}
const variant&    variant::operator[]( size_t pos )const
{
    return get_array()[pos];
}
        /// @pre is_array()
size_t            variant::size()const
{
    return get_array().size();
}

const string&        variant::get_string()const
{
  if( get_type() == string_type )
     return **reinterpret_cast<const const_string_ptr*>(this);
  FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from type '${type}' to string", ("type",get_type()) );
}

/// @throw if get_type() != object_type 
const variant_object&  variant::get_object()const
{
  if( get_type() == object_type )
     return **reinterpret_cast<const const_variant_object_ptr*>(this);
  FC_THROW_EXCEPTION( bad_cast_exception, "Invalid cast from type '${type}' to Object", ("type",get_type()) );
}

void from_variant( const variant& var,  variants& vo )
{
   vo = var.get_array();
}

//void from_variant( const variant& var,  variant_object& vo )
//{
//   vo  = var.get_object();
//}

void from_variant( const variant& var,  variant& vo ) { vo = var; }

void to_variant( const uint8_t& var,  variant& vo )  { vo = uint64_t(var); }
// TODO: warn on overflow?
void from_variant( const variant& var,  uint8_t& vo ){ vo = static_cast<uint8_t>(var.as_uint64()); }

void to_variant( const int8_t& var,  variant& vo )  { vo = int64_t(var); }
// TODO: warn on overflow?
void from_variant( const variant& var,  int8_t& vo ){ vo = static_cast<int8_t>(var.as_int64()); }

void to_variant( const uint16_t& var,  variant& vo )  { vo = uint64_t(var); }
// TODO: warn on overflow?
void from_variant( const variant& var,  uint16_t& vo ){ vo = static_cast<uint16_t>(var.as_uint64()); }

void to_variant( const int16_t& var,  variant& vo )  { vo = int64_t(var); }
// TODO: warn on overflow?
void from_variant( const variant& var,  int16_t& vo ){ vo = static_cast<int16_t>(var.as_int64()); }

void to_variant( const uint32_t& var,  variant& vo )  { vo = uint64_t(var); }
void from_variant( const variant& var,  uint32_t& vo )
{
   vo = static_cast<uint32_t>(var.as_uint64());
}

void to_variant( const int32_t& var,  variant& vo )  { vo = int64_t(var); }
void from_variant( const variant& var,  int32_t& vo )
{
   vo = static_cast<int32_t>(var.as_int64());
}

void from_variant( const variant& var,  int64_t& vo )
{
   vo = var.as_int64();
}

void from_variant( const variant& var,  uint64_t& vo )
{
   vo = var.as_uint64();
}

void from_variant( const variant& var,  bool& vo )
{
   vo = var.as_bool();
}

void from_variant( const variant& var,  double& vo )
{
   vo = var.as_double();
}

void from_variant( const variant& var,  float& vo )
{
   vo = static_cast<float>(var.as_double());
}

void to_variant( const std::string& s, variant& v )
{
    v = variant( fc::string(s) );
}

void from_variant( const variant& var,  string& vo )
{
   vo = var.as_string();
}

void to_variant( const std::vector<char>& var,  variant& vo )
{
  if( var.size() )
      vo = variant(to_hex(var.data(),var.size()));
  else vo = "";
}
void from_variant( const variant& var,  std::vector<char>& vo )
{
     auto str = var.as_string();
     vo.resize( str.size() / 2 );
     if( vo.size() )
     {
        size_t r = from_hex( str, vo.data(), vo.size() );
        FC_ASSERT( r == vo.size() );
     }
//   std::string b64 = base64_decode( var.as_string() );
//   vo = std::vector<char>( b64.c_str(), b64.c_str() + b64.size() );
}

void to_variant( const UInt<8>& n, variant& v ) { v = uint64_t(n); }
// TODO: warn on overflow?
void from_variant( const variant& v, UInt<8>& n ) { n = static_cast<uint8_t>(v.as_uint64()); }

void to_variant( const UInt<16>& n, variant& v ) { v = uint64_t(n); }
// TODO: warn on overflow?
void from_variant( const variant& v, UInt<16>& n ) { n = static_cast<uint16_t>(v.as_uint64()); }

void to_variant( const UInt<32>& n, variant& v ) { v = uint64_t(n); }
// TODO: warn on overflow?
void from_variant( const variant& v, UInt<32>& n ) { n = static_cast<uint32_t>(v.as_uint64()); }

void to_variant( const UInt<64>& n, variant& v ) { v = uint64_t(n); }
void from_variant( const variant& v, UInt<64>& n ) { n = v.as_uint64(); }

string      format_string( const string& format, const variant_object& args )
{
   std::stringstream ss;
   size_t prev = 0;
   auto next = format.find( '$' );
   while( prev != size_t(string::npos) && prev < size_t(format.size()) ) 
   {
     ss << format.substr( prev, size_t(next-prev) );
   
     // if we got to the end, return it.
     if( next == size_t(string::npos) ) 
        return ss.str(); 
   
     // if we are not at the end, then update the start
     prev = next + 1;
   
     if( format[prev] == '{' ) 
     { 
        // if the next char is a open, then find close
         next = format.find( '}', prev );
         // if we found close... 
         if( next != size_t(string::npos) ) 
         {
           // the key is between prev and next
           string key = format.substr( prev+1, (next-prev-1) );

           auto val = args.find( key );
           if( val != args.end() )
           {
              if( val->value().is_object() || val->value().is_array() )
              {
                ss << json::to_string( val->value() );
              } 
              else 
              {
                ss << val->value().as_string();
              }
           } 
           else 
           {
              ss << "${"<<key<<"}";
           }
           prev = next + 1;
           // find the next $
           next = format.find( '$', prev );
         } 
         else 
         {
           // we didn't find it.. continue to while...
         }
     } 
     else  
     {
        ss << format[prev];
        ++prev;
        next = format.find( '$', prev );
     }
   }
   return ss.str();
}
   #ifdef __APPLE__
   #elif !defined(_MSC_VER)
   void to_variant( long long int s, variant& v ) { v = variant( int64_t(s) ); }
   void to_variant( unsigned long long int s, variant& v ) { v = variant( uint64_t(s)); }
   #endif

   variant operator == ( const variant& a, const variant& b )
   {
      if( a.is_string()  || b.is_string() ) return a.as_string() == b.as_string();
      if( a.is_double()  || b.is_double() ) return a.as_double() == b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() == b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() == b.as_uint64();
      return false;
   }

   variant operator != ( const variant& a, const variant& b )
   {
      if( a.is_string()  || b.is_string() ) return a.as_string() != b.as_string();
      if( a.is_double()  || b.is_double() ) return a.as_double() != b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() != b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() != b.as_uint64();
      return false;
   }

   variant operator ! ( const variant& a )
   {
      return !a.as_bool();
   }

   variant operator < ( const variant& a, const variant& b )
   {
      if( a.is_string()  || b.is_string() ) return a.as_string() < b.as_string();
      if( a.is_double()  || b.is_double() ) return a.as_double() < b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() < b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() < b.as_uint64();
      FC_ASSERT( false, "Invalid operation" );
   }

   variant operator > ( const variant& a, const variant& b )
   {
      if( a.is_string()  || b.is_string() ) return a.as_string() > b.as_string();
      if( a.is_double()  || b.is_double() ) return a.as_double() > b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() > b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() > b.as_uint64();
      FC_ASSERT( false, "Invalid operation" );
   }

   variant operator <= ( const variant& a, const variant& b )
   {
      if( a.is_string()  || b.is_string() ) return a.as_string() <= b.as_string();
      if( a.is_double()  || b.is_double() ) return a.as_double() <= b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() <= b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() <= b.as_uint64();
      FC_ASSERT( false, "Invalid operation" );
   }


   variant operator + ( const variant& a, const variant& b )
   {
      if( a.is_array()  && b.is_array() )
      {
         const variants& aa = a.get_array();
         const variants& ba = b.get_array();
         variants result;
         result.reserve( std::max(aa.size(),ba.size()) );
         auto num = std::max(aa.size(),ba.size());
         for( unsigned i = 0; i < num; ++i )
         {
            if( aa.size() > i && ba.size() > i )
               result[i]  = aa[i] + ba[i];
            else if( aa.size() > i )
               result[i]  = aa[i];
            else
               result[i]  = ba[i];
         }
         return result;
      }
      if( a.is_string()  || b.is_string() ) return a.as_string() + b.as_string();
      if( a.is_double()  || b.is_double() ) return a.as_double() + b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() + b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() + b.as_uint64();
      FC_ASSERT( false, "invalid operation ${a} + ${b}", ("a",a)("b",b) );
   }

   variant operator - ( const variant& a, const variant& b )
   {
      if( a.is_array()  && b.is_array() )
      {
         const variants& aa = a.get_array();
         const variants& ba = b.get_array();
         variants result;
         result.reserve( std::max(aa.size(),ba.size()) );
         auto num = std::max(aa.size(),ba.size());
         for( unsigned i = 0; i < num; --i )
         {
            if( aa.size() > i && ba.size() > i )
               result[i]  = aa[i] - ba[i];
            else if( aa.size() > i )
               result[i]  = aa[i];
            else
               result[i]  = ba[i];
         }
         return result;
      }
      if( a.is_string()  || b.is_string() ) return a.as_string() - b.as_string();
      if( a.is_double()  || b.is_double() ) return a.as_double() - b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() - b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() - b.as_uint64();
      FC_ASSERT( false, "invalid operation ${a} + ${b}", ("a",a)("b",b) );
   }
   variant operator * ( const variant& a, const variant& b )
   {
      if( a.is_double()  || b.is_double() ) return a.as_double() * b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() * b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() * b.as_uint64();
      if( a.is_array()  && b.is_array() )
      {
         const variants& aa = a.get_array();
         const variants& ba = b.get_array();
         variants result;
         result.reserve( std::max(aa.size(),ba.size()) );
         auto num = std::max(aa.size(),ba.size());
         for( unsigned i = 0; i < num; ++i )
         {
            if( aa.size() > i && ba.size() > i )
               result[i]  = aa[i] * ba[i];
            else if( aa.size() > i )
               result[i]  = aa[i];
            else
               result[i]  = ba[i];
         }
         return result;
      }
      FC_ASSERT( false, "invalid operation ${a} * ${b}", ("a",a)("b",b) );
   }
   variant operator / ( const variant& a, const variant& b )
   {
      if( a.is_double()  || b.is_double() ) return a.as_double() / b.as_double();
      if( a.is_int64()   || b.is_int64() )  return a.as_int64() / b.as_int64();
      if( a.is_uint64()  || b.is_uint64() ) return a.as_uint64() / b.as_uint64();
      if( a.is_array()  && b.is_array() )
      {
         const variants& aa = a.get_array();
         const variants& ba = b.get_array();
         variants result;
         result.reserve( std::max(aa.size(),ba.size()) );
         auto num = std::max(aa.size(),ba.size());
         for( unsigned i = 0; i < num; ++i )
         {
            if( aa.size() > i && ba.size() > i )
               result[i]  = aa[i] / ba[i];
            else if( aa.size() > i )
               result[i]  = aa[i];
            else
               result[i]  = ba[i];
         }
         return result;
      }
      FC_ASSERT( false, "invalid operation ${a} / ${b}", ("a",a)("b",b) );
   }
} // namespace fc
