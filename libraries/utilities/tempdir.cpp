/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/utilities/tempdir.hpp>

#include <cstdlib>

namespace eosio { namespace utilities {

fc::path temp_directory_path()
{
   const char* eos_tempdir = getenv("EOS_TEMPDIR");
   if( eos_tempdir != nullptr )
      return fc::path( eos_tempdir );
   return fc::temp_directory_path() / "eos-tmp";
}

} } // eosio::utilities
