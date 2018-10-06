/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */

#include <arisen/utilities/tempdir.hpp>

#include <cstdlib>

namespace arisen { namespace utilities {

fc::path temp_directory_path()
{
   const char* eos_tempdir = getenv("EOS_TEMPDIR");
   if( eos_tempdir != nullptr )
      return fc::path( eos_tempdir );
   return fc::temp_directory_path() / "arisen-tmp";
}

} } // arisen::utilities
