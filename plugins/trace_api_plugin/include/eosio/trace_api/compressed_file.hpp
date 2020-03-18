#pragma once

#include <ios>
#include <fc/io/cfile.hpp>

namespace eosio::trace_api {

   class compressed_file_datastream;
   struct compressed_file_impl;
   /**
    * wrapper for read-only access to a compressed file.
    * compressed files support seeking and reading
    *
    * the efficiency of seeking is lower than that of an uncompressed file as each seek translates to
    *  - 2 seeks + 1 read to load and process the seek-point-mapping
    *  - potentially a read/decompress/discard of the data between the seek point and the requested offset
    *
    * More seek points can lower the average amount of data that must be read/decompressed/discarded in order
    * to seek to any given offset.  However, each seek point has some effect on the file size as it represents a
    * flush of the compressor which can degrade compression performance.
    */
   class compressed_file {
   public:
      explicit compressed_file( fc::path file_path );
      ~compressed_file();

      /**
       * Provide default move construction/assignment
       */
      compressed_file( compressed_file&& );
      compressed_file& operator= ( compressed_file&& );


      void open() {
         file_ptr = std::make_unique<fc::cfile>();
         file_ptr->set_file_path(file_path);
         file_ptr->open("rb");
      }

      bool is_open() const { return (bool)file_ptr; }

      void seek( long loc );

      void read( char* d, size_t n );

      void close() {
         file_ptr.reset();
      }

      auto get_file_path() const {
         return file_path;
      }

      compressed_file_datastream create_datastream();
      
      static bool process( const fc::path& input_path, const fc::path& output_path, uint16_t seek_point_count );

   private:
      fc::path file_path;
      std::unique_ptr<fc::cfile> file_ptr;
      std::unique_ptr<compressed_file_impl> impl;
   };

   /*
    *  @brief datastream adapter that adapts cfile for use with fc unpack
    *
    *  This class supports unpack functionality but not pack.
    */
   class compressed_file_datastream {
   public:
      explicit compressed_file_datastream( compressed_file& cf ) : cf(cf) {}

      void skip( size_t s ) {
         std::vector<char> d( s );
         read( &d[0], s );
      }

      bool read( char* d, size_t s ) {
         cf.read( d, s );
         return true;
      }

      bool get( unsigned char& c ) { return get( *(char*)&c ); }

      bool get( char& c ) { return read(&c, 1); }

   private:
      compressed_file& cf;
   };

   inline compressed_file_datastream compressed_file::create_datastream() {
      return compressed_file_datastream(*this);
   }

}