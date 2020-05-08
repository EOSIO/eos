#pragma once
#include <fc/io/raw.hpp>
#include <fc/interprocess/file_mapping.hpp>
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>

namespace fc
{
    namespace raw
    {
        template<typename T>
        void unpack_file( const fc::path& filename, T& obj )
        {
           try {
               fc::file_mapping fmap( filename.generic_string().c_str(), fc::read_only);
               fc::mapped_region mapr( fmap, fc::read_only, 0, fc::file_size(filename) );
               auto cs  = (const char*)mapr.get_address();

               fc::datastream<const char*> ds( cs, mapr.get_size() );
               fc::raw::unpack(ds,obj);
           } FC_RETHROW_EXCEPTIONS( info, "unpacking file ${file}", ("file",filename) );
        }
   }
}
