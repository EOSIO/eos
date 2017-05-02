#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>

#include <eos/types/native.hpp>
#include <eos/types/Asset.hpp>
#include <eos/types/PublicKey.hpp>
#include <eos/types/generated.hpp>

#include <fc/io/datastream.hpp>

int main( int argc, char** argv ) {

   try {
      eos::types::Message m;
      m.sender = eos::types::AccountName( "ned" );
      m.recipient = eos::types::AccountName( "dan" );

      idump( (m) );

      eos::types::Transfer t;
      t.from = m.sender;
      t.to = "other";

      idump( (t) );

      /*
      fc::datastream<size_t> ps;
      eos::toBinary( ps, m );
      idump((ps.tellp()));

      std::vector<char> data(ps.tellp());
      fc::datastream<char*> ds( data.data(), data.size());
      eos::toBinary( ds, m );

      eos::Message m2;
      fc::datastream<const char*> ds2( data.data(), data.size());
      eos::fromBinary( ds2, m2 );

      wdump( (eos::toVariant(m2) ) );
      */

   } catch ( const fc::exception& e ) {
      elog( "${e}", ("e", e.to_detail_string() ) );
   }

   return 0;
}
