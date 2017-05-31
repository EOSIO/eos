#include <fc/interprocess/mmap_struct.hpp>

#include <fc/filesystem.hpp>

#include <string.h>
#include <fstream>
#include <algorithm>

namespace fc
{
   namespace detail
   {
      size_t mmap_struct_base::size()const { return _mapped_region->get_size(); }
      void mmap_struct_base::flush() 
      { 
        _mapped_region->flush();  
      }

      void mmap_struct_base::open( const fc::path& file, size_t s, bool create )
      {
         if( !fc::exists( file ) || fc::file_size(file) != s )
         {
            std::ofstream out( file.generic_string().c_str() );
            char buffer[1024];
            memset( buffer, 0, sizeof(buffer) );

            size_t bytes_left = s;
            while( bytes_left > 0 )
            {
               size_t to_write = std::min<size_t>(bytes_left, sizeof(buffer) );
               out.write( buffer, to_write );
               bytes_left -= to_write;
            }
         }

         std::string filePath = file.to_native_ansi_path(); 

         _file_mapping.reset( new fc::file_mapping( filePath.c_str(), fc::read_write ) );
         _mapped_region.reset( new fc::mapped_region( *_file_mapping, fc::read_write, 0, s ) );
      }
   } // namespace fc

} // namespace fc
