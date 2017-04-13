#include <eos/types/native.hpp>
#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <eos/types/generated.hpp>
#include <eos/types/native_impl.hpp>

#include <fc/io/datastream.hpp>

int main( int argc, char** argv ) {

   try {
      EOS::Message m;
      m.from = EOS::AccountName( "ned" );
      m.to = EOS::AccountName( "dan" );

      idump( (EOS::toVariant(m) ) );

      fc::datastream<size_t> ps;
      EOS::toBinary( ps, m );
      idump((ps.tellp()));

      std::vector<char> data(ps.tellp());
      fc::datastream<char*> ds( data.data(), data.size());
      EOS::toBinary( ds, m );

      EOS::Message m2;
      fc::datastream<const char*> ds2( data.data(), data.size());
      EOS::fromBinary( ds2, m2 );

      wdump( (EOS::toVariant(m2) ) );

   } catch ( const fc::exception& e ) {
      elog( "${e}", ("e", e.to_detail_string() ) );
   }

   return 0;
}
