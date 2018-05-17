/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */

#include <enumivo/utilities/tempdir.hpp>

#include <cstdlib>

namespace enumivo { namespace utilities {

fc::path temp_directory_path()
{
   const char* eos_tempdir = getenv("ENU_TEMPDIR");
   if( eos_tempdir != nullptr )
      return fc::path( eos_tempdir );
   return fc::temp_directory_path() / "eos-tmp";
}

} } // enumivo::utilities
