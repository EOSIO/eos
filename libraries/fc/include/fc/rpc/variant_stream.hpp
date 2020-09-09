#pragma once

namespace fc 
{

   /**
    * Thread-safe, circular buffer for passing variants
    * between threads.
    */
   class variant_stream
   {
     public:
        variant_stream( size_t s );
        ~variant_stream();

        /** producer api */
        int64_t free(); // number of spaces available
        int64_t claim( int64_t num );
        int64_t publish( int64_t pos );
        int64_t wait_free(); // wait for free space

        // producer/consumer api
        variant&  get( int64_t pos );

        /** consumer api */
        int64_t   begin(); // returns the first index ready to be read
        int64_t   end();   // returns the first index that cannot be read
        int64_t   wait(); // wait for variants to be posted

     private:
        std::vector<fc::variant>  _variants;
        uint64_t              _read_pos;
        uint64_t              _write_pos;
   };

}
