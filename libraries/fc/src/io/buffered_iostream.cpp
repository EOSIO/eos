#include <fc/io/buffered_iostream.hpp>
#include <fc/exception/exception.hpp>
#include <boost/asio/streambuf.hpp>
#include <iostream>

#include <fc/log/logger.hpp>

namespace fc
{
    namespace detail
    {
       class buffered_istream_impl
       {
          public:
             buffered_istream_impl( istream_ptr is ) :
               _istr(fc::move(is))
#ifndef NDEBUG
               ,_shared_read_buffer_in_use(false)
#endif
               {}

             istream_ptr            _istr;
             boost::asio::streambuf _rdbuf;
             std::shared_ptr<char>  _shared_read_buffer;
#ifndef NDEBUG
             bool                   _shared_read_buffer_in_use;
#endif
       };
       static const size_t minimum_read_size = 1024;
    }

    buffered_istream::buffered_istream( istream_ptr is )
    :my( new detail::buffered_istream_impl( fc::move(is) ) )
    {
       FC_ASSERT( my->_istr != nullptr, " this shouldn't be null" );
    }

    buffered_istream::buffered_istream( buffered_istream&& o )
    :my( fc::move(o.my) ){}

    buffered_istream& buffered_istream::operator=( buffered_istream&& i )
    {
       my = fc::move(i.my);
       return *this;
    }

    buffered_istream::~buffered_istream(){}

    size_t buffered_istream::readsome( char* buf, size_t len )
    {
        size_t bytes_from_rdbuf = static_cast<size_t>(my->_rdbuf.sgetn(buf, len));
        if (bytes_from_rdbuf)
           return bytes_from_rdbuf;


        if( len > detail::minimum_read_size ) 
           return my->_istr->readsome(buf,len);

        char tmp[detail::minimum_read_size];
        size_t bytes_read = my->_istr->readsome( tmp, detail::minimum_read_size );

        size_t bytes_to_deliver_immediately = std::min<size_t>(bytes_read,len);

        memcpy( buf, tmp, bytes_to_deliver_immediately );
        

        if( bytes_read > len )
        {
           my->_rdbuf.sputn( tmp + len, bytes_read - len );
        }

        return bytes_to_deliver_immediately;
    }

    size_t buffered_istream::readsome( const std::shared_ptr<char>& buf, size_t len, size_t offset )
    {
        size_t bytes_from_rdbuf = static_cast<size_t>(my->_rdbuf.sgetn(buf.get() + offset, len));
        if (bytes_from_rdbuf)
           return bytes_from_rdbuf;


        if( len > detail::minimum_read_size ) 
           return my->_istr->readsome(buf.get() + offset, len);

#ifndef NDEBUG
        // This code was written with the assumption that you'd only be making one call to readsome 
        // at a time so it reuses _shared_read_buffer.  If you really need to make concurrent calls to 
        // readsome(), you'll need to prevent reusing _shared_read_buffer here
        struct check_buffer_in_use {
          bool& _buffer_in_use;
          check_buffer_in_use(bool& buffer_in_use) : _buffer_in_use(buffer_in_use) { assert(!_buffer_in_use); _buffer_in_use = true; }
          ~check_buffer_in_use() { assert(_buffer_in_use); _buffer_in_use = false; }
        } buffer_in_use_checker(my->_shared_read_buffer_in_use);
#endif

        if (!my->_shared_read_buffer)
          my->_shared_read_buffer.reset(new char[detail::minimum_read_size], [](char* p){ delete[] p; });
        size_t bytes_read = my->_istr->readsome( my->_shared_read_buffer, detail::minimum_read_size, 0 );

        size_t bytes_to_deliver_immediately = std::min<size_t>(bytes_read,len);

        memcpy( buf.get() + offset, my->_shared_read_buffer.get(), bytes_to_deliver_immediately );
        
        if( bytes_read > len )
        {
           my->_rdbuf.sputn( my->_shared_read_buffer.get() + len, bytes_read - len );
        }

        return bytes_to_deliver_immediately;
    }

    char  buffered_istream::peek()const
    {
       if( my->_rdbuf.size() )
       {
          return my->_rdbuf.sgetc();
       }

       char tmp[detail::minimum_read_size];
       size_t bytes_read = my->_istr->readsome( tmp, detail::minimum_read_size );
       my->_rdbuf.sputn( tmp, bytes_read );

       if( my->_rdbuf.size() )
       {
          return my->_rdbuf.sgetc();
       }
       FC_THROW_EXCEPTION( assert_exception, 
          "at least one byte should be available, or eof should have been thrown" );
    }


    namespace detail
    {
       class buffered_ostream_impl
       {
          public:
             buffered_ostream_impl( ostream_ptr os ) :
               _ostr(fc::move(os))
#ifndef NDEBUG
               ,_shared_write_buffer_in_use(false)
#endif
             {}

             ostream_ptr            _ostr;
             boost::asio::streambuf _rdbuf;
             std::shared_ptr<char>  _shared_write_buffer;
#ifndef NDEBUG
             bool                   _shared_write_buffer_in_use;
#endif
       };
    }

    buffered_ostream::buffered_ostream( ostream_ptr os, size_t bufsize )
    :my( new detail::buffered_ostream_impl( fc::move(os) ) )
    {
    }

    buffered_ostream::buffered_ostream( buffered_ostream&& o )
    :my( fc::move(o.my) ){}

    buffered_ostream& buffered_ostream::operator=( buffered_ostream&& i )
    {
       my = fc::move(i.my);
       return *this;
    }

    buffered_ostream::~buffered_ostream(){}

    size_t buffered_ostream::writesome( const char* buf, size_t len )
    {
        size_t written = static_cast<size_t>(my->_rdbuf.sputn( buf, len ));
        if( written < len ) { flush(); }
        return written + static_cast<size_t>(my->_rdbuf.sputn( buf+written, len-written ));
    }

    size_t buffered_ostream::writesome( const std::shared_ptr<const char>& buf, size_t len, size_t offset )
    {
      return writesome(buf.get() + offset, len);
    }

    void  buffered_ostream::flush()
    {
#ifndef NDEBUG
        // This code was written with the assumption that you'd only be making one call to flush 
        // at a time so it reuses _shared_write_buffer.  If you really need to make concurrent calls to 
        // flush(), you'll need to prevent reusing _shared_write_buffer here
        struct check_buffer_in_use {
          bool& _buffer_in_use;
          check_buffer_in_use(bool& buffer_in_use) : _buffer_in_use(buffer_in_use) { assert(!_buffer_in_use); _buffer_in_use = true; }
          ~check_buffer_in_use() { assert(_buffer_in_use); _buffer_in_use = false; }
        } buffer_in_use_checker(my->_shared_write_buffer_in_use);
#endif
        const size_t write_buffer_size = 2048;
        if (!my->_shared_write_buffer)
          my->_shared_write_buffer.reset(new char[write_buffer_size], [](char* p){ delete[] p; });

        while( size_t bytes_from_rdbuf = static_cast<size_t>(my->_rdbuf.sgetn(my->_shared_write_buffer.get(), write_buffer_size)) )
           my->_ostr->write( my->_shared_write_buffer, bytes_from_rdbuf );
        my->_ostr->flush();
    }

    void  buffered_ostream::close()
    {
        flush();
        my->_ostr->close();
    }


}
