#pragma once
#include <fc/io/iostream.hpp>

namespace fc
{
   namespace detail 
   { 
       class buffered_istream_impl; 
       class buffered_ostream_impl; 
   }

   /**
    *  @brief Reads data from an unbuffered stream
    *         and enables peek functionality.
    */
   class buffered_istream : public virtual istream
   {
      public:
        buffered_istream( istream_ptr is );
        buffered_istream( buffered_istream&& o );

        buffered_istream& operator=( buffered_istream&& i );

        virtual ~buffered_istream();
        
        /** read at least 1 byte or throw, if no data is available
         *  this method should block cooperatively until data is
         *  available or fc::eof_exception is thrown.
         *
         *  @pre len > 0 
         *  @pre buf != nullptr
         *  @throws fc::eof if at least 1 byte cannot be read
         **/
        virtual std::size_t     readsome( char* buf, std::size_t len );
        virtual size_t          readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset );

        /**
         *  This method may block until at least 1 character is
         *  available.
         */
        virtual char            peek() const;

      private:
        std::unique_ptr<detail::buffered_istream_impl> my;
   };
   typedef std::shared_ptr<buffered_istream> buffered_istream_ptr;


   /**
    * 
    */
   class buffered_ostream : public virtual ostream
   {
      public:
        buffered_ostream( ostream_ptr o, size_t bufsize = 4096 );
        buffered_ostream( buffered_ostream&& m );
        ~buffered_ostream();

        buffered_ostream& operator=( buffered_ostream&& m );
        /**
         *  This method will return immediately unless the buffer
         *  is full, in which case it will flush which may block.
         */
        virtual size_t  writesome( const char* buf, size_t len );
        virtual size_t  writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset );

        virtual void close();
        virtual void flush();
      private:
        std::unique_ptr<detail::buffered_ostream_impl> my;
   };
   typedef std::shared_ptr<buffered_ostream> buffered_ostream_ptr;
}
