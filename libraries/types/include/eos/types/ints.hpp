#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include <fc/log/logger.hpp>

namespace EOS {

   using namespace boost::multiprecision;

   typedef number<cpp_int_backend<8, 8, unsigned_magnitude, unchecked, void> >     UInt8;
   typedef number<cpp_int_backend<16, 16, unsigned_magnitude, unchecked, void> >   UInt16;
   typedef number<cpp_int_backend<32, 32, unsigned_magnitude, unchecked, void> >   UInt32;
   typedef number<cpp_int_backend<64, 64, unsigned_magnitude, unchecked, void> >   UInt64;
   typedef boost::multiprecision::uint128_t UInt128;
   typedef boost::multiprecision::uint256_t UInt256;
   typedef number<cpp_int_backend<8, 8, signed_magnitude, unchecked, void> >       Int8;
   typedef number<cpp_int_backend<16, 16, signed_magnitude, unchecked, void> >     Int16;
   typedef number<cpp_int_backend<32, 32, signed_magnitude, unchecked, void> >     Int32;
   typedef number<cpp_int_backend<64, 64, signed_magnitude, unchecked, void> >     Int64;
   typedef boost::multiprecision::int128_t Int128;
   typedef boost::multiprecision::int256_t Int256;


   template<typename Stream, typename Number>
   void fromBinary( Stream& st, boost::multiprecision::number<Number>& value ) {
      unsigned char data[(std::numeric_limits<decltype(value)>::digits+1)/8];
      st.read( (char*)data, sizeof(data) );
      boost::multiprecision::import_bits( value, data, data + sizeof(data), 1 );
   }
   template<typename Stream, typename Number>
   void toBinary( Stream& st, const boost::multiprecision::number<Number>& value ) {
      unsigned char data[(std::numeric_limits<decltype(value)>::digits+1)/8];
      boost::multiprecision::export_bits( value, data, 1 );
      st.write( (const char*)data, sizeof(data) );
   }
} // namespace EOS
