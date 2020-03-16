#pragma once

#include <ios>
#include <fc/io/cfile.hpp>

namespace eosio::trace_api {

   class compressed_file_datastream;
   struct compressed_file_impl;
   /**
    * wrapper for read-only access to a compressed file.
    * compressed files support seeking though the efficiency of random access is lower than
    * an uncompressed file in trade for smaller on-disk resource usage
    */
   class compressed_file {
   public:
      compressed_file( fc::path file_path );
      ~compressed_file();

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

      /**
       * Provide default move construction/assignment
       */
      compressed_file( compressed_file&& );
      compressed_file& operator= ( compressed_file&& );

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