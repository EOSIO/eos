
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main( int argc, char** argv, char** envp )
{
   if( argc != 3 )
   {
      std::cerr << "syntax:  cat-parts DIR OUTFILE" << std::endl;
      return 1;
   }

   boost::filesystem::path p(argv[1]);

   try
   {
      std::vector< boost::filesystem::path > v;

      for( boost::filesystem::directory_iterator it(p);
           it != boost::filesystem::directory_iterator();
           ++it )
      {
         boost::filesystem::path pit = it->path();
         std::string spit = pit.generic_string();
         size_t n = spit.length();
         if( n <= 3 )
            continue;
         if( spit.substr(n-3, 3) != ".hf" )
            continue;
         v.push_back( pit );
      }
      std::sort( v.begin(), v.end() );

      // open each file and grab its contents, concatenating into single stringstream
      std::stringstream ss_data;
      for( const boost::filesystem::path& p : v )
      {
         boost::filesystem::ifstream ifs(p);
         ss_data << ifs.rdbuf();
      }
      std::string new_data = ss_data.str();

      boost::filesystem::path opath(argv[2]);

      if( boost::filesystem::exists( opath ) )
      {
         boost::filesystem::ifstream ifs(opath);
         std::stringstream ss_old_data;
         ss_old_data << ifs.rdbuf();
         std::string old_data = ss_old_data.str();
         if( old_data == new_data )
         {
            std::cerr << "File " << opath << " up-to-date with .d directory" << std::endl;
            return 0;
         }
      }

      {
         boost::filesystem::create_directories(opath.parent_path());
         boost::filesystem::ofstream ofs(opath);
         ofs.write( new_data.c_str(), new_data.length() );
      }

      std::cerr << "Built " << opath << " from .d directory" << std::endl;
   }
   catch( const boost::filesystem::filesystem_error& e )
   {
      std::cout << e.what() << std::endl;
      return 1;
   }
   return 0;
}
