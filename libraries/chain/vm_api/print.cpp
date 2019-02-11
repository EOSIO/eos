/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
extern "C" {

void prints( const char* cstr ) {
//   wlog("++++++${n}", ("n", cstr));
   ctx().console_append<const char*>(cstr);
}

void prints_l( const char* cstr, uint32_t len) {
//   wlog("++++++${n}", ("n", cstr));
   ctx().console_append(string(cstr, len));
}

void printi( int64_t val ) {
//   wlog("++++++${n}", ("n", val));
   ctx().console_append(val);
}

void printui( uint64_t val ) {
//   wlog("++++++${n}", ("n", val));
   ctx().console_append(val);
}

void printi128( const int128_t* val ) {
//   wlog("++++++${n}", ("n", (uint64_t)val));
   bool is_negative = (*val < 0);
   unsigned __int128 val_magnitude;

   if( is_negative )
      val_magnitude = static_cast<unsigned __int128>(-*val); // Works even if val is at the lowest possible value of a int128_t
   else
      val_magnitude = static_cast<unsigned __int128>(*val);

   fc::uint128_t v(val_magnitude>>64, static_cast<uint64_t>(val_magnitude) );

   if( is_negative ) {
      ctx().console_append("-");
   }

   ctx().console_append(fc::variant(v).get_string());
}

void printui128( const uint128_t* val ) {
//   wlog("++++++${n}", ("n", (uint64_t)val));
   fc::uint128_t v(*val>>64, static_cast<uint64_t>(*val) );
   ctx().console_append(fc::variant(v).get_string());
}

void printsf(float val) {
//   wlog("++++++${n}", ("n", val));
   // Assumes float representation on native side is the same as on the WASM side
   auto& console = ctx().get_console_stream();
   auto orig_prec = console.precision();

   console.precision( std::numeric_limits<float>::digits10 );
   ctx().console_append(val);

   console.precision( orig_prec );
}

void printdf(double val) {
//   wlog("++++++${n}", ("n", val));
   // Assumes double representation on native side is the same as on the WASM side
   auto& console = ctx().get_console_stream();
   auto orig_prec = console.precision();

   console.precision( std::numeric_limits<double>::digits10 );
   ctx().console_append(val);

   console.precision( orig_prec );
}

void printqf(const float128_t* val) {
//   wlog("++++++${n}", ("n", (uint64_t)val));
   auto& console = ctx().get_console_stream();
   auto orig_prec = console.precision();

   console.precision( std::numeric_limits<long double>::digits10 );

   extFloat80_t val_approx;
   f128M_to_extF80M((float128_t*)val, &val_approx);
   ctx().console_append( *(long double*)(&val_approx) );

   console.precision( orig_prec );
}

void printn( uint64_t n ) {
//   wlog("++++++${n}", ("n", n));
   ctx().console_append(name(n).to_string());
}

void printhex( const void* data, uint32_t datalen ) {
//   wlog("++++++${n}", ("n", (uint64_t)data));
   ctx().console_append(fc::to_hex((char*)data, datalen));
}

}
