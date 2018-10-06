/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */

#include <arisen/utilities/tempdir.hpp>

#include <cstdlib>

namespace arisen { namespace utilities {

fc::path temp_directory_path()
{
   const char* rsn_tempdir = getenv("RSN_TEMPDIR");
   if( rsn_tempdir != nullptr )
      return fc::path( rsn_tempdir );
   return fc::temp_directory_path() / "arisen-tmp";
}

} } // arisen::utilities
