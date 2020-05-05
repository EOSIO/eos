#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <fc/io/raw.hpp>
#include <deque>
#include <array>
#include <fc/exception/exception.hpp>

namespace fc {
  template <uint32_t buffer_len>
  class mb_datastream;
  template <uint32_t buffer_len>
  class mb_peek_datastream;

  /**
   *  @brief abstraction for a message buffer that spans a chain of physical buffers
   *
   *  This message buffer abstraction will allocate individual character arrays
   *  of size buffer_len from a boost::singleton_pool.  It supports creation of a
   *  vector of boost::mutable_buffer for use with async_read() and async_read_some().
   *  It also supports use with the fc unpack() functionality via a datastream
   *  helper class.
   */
  template <uint32_t buffer_len>
  class message_buffer {
  public:
    /*
     *  index abstraction that references a point in the chain of buffers.
     *  first refers to the buffer's index in the deque.
     *  second refers to the byte in the character buffer.
     */
    typedef std::pair<uint32_t, uint32_t> index_t;

    message_buffer() : buffers{malloc()}, read_ind{0,0}, write_ind{0,0}, sanity_check (1) { }

    ~message_buffer() {
      while (buffers.size() > 0) {
        free(buffers.back());
        buffers.pop_back();
      }
    }

    /*
     *  Returns the current read index referencing the byte in the buffer
     *  that is next to be read.
     */
    index_t read_index() const { return read_ind; }

    /*
     *  Returns the current write index referencing the byte in the buffer
     *  that is next to be written to.
     */
    index_t write_index() const { return write_ind; }

    /*
     *  Returns the current read pointer pointing to the memory location
     *  of the next byte to be read.
     */
    char* read_ptr() {
      return &buffers[read_ind.first]->at(read_ind.second);
    }

    /*
     *  Returns the current write pointer pointing to the memory location
     *  of the next byte to be written to.
     */
    char* write_ptr() {
      return &buffers[write_ind.first]->at(write_ind.second);
    }

    /*
     *  Adds an additional buffer of buffer_len to the chain of buffers.
     *  Does not affect the read or write pointer.
     */
    void add_buffer_to_chain() {
      sanity_check++;
      buffers.push_back(malloc());
    }

    /*
     *  Adds additional buffers of length buffer_len to the chain of buffers
     *  in order to add at least bytes to the space available.
     *  Does not affect the read or write pointer.
     */
    void add_space(uint32_t bytes) {
      int buffers_to_add = bytes / buffer_len + 1;
      for (int i = 0; i < buffers_to_add; i++) {
        sanity_check++;
        buffers.push_back(malloc());
      }
    }

    /*
     *  Resets the message buffer to the initial state. Any unread data is
     *  discarded.
     */
    void reset() {
      // some condition exists that can send *both* buffers.size() and sanity_check to well over 10^6.
      // this seems to be related to some sort of memory overrun possibly. By forcing an exit here, an
      // external watchdog can be used to restart the process and avoid hanging.
      if( buffers.size() != sanity_check || buffers.size() > 1000000) {
         elog( "read_ind = ${r1}, ${r2} write_ind = ${w1}, ${w2}, buff.size = ${bs}, sanity = ${s}",
               ( "r1", read_ind.first )( "r2", read_ind.second )( "w1", write_ind.first )( "w2", write_ind.second )
               ( "bs", buffers.size() )( "s", sanity_check ) );
        elog("Buffer manager overwrite detected. Terminating to allow external restart");
        exit(1);
      }
      while (buffers.size() > 1) {
        sanity_check--;
        free(buffers.back());
        buffers.pop_back();
      }

      read_ind = { 0, 0 };
      write_ind = { 0, 0 };
    }

    /*
     *  Returns the current number of bytes remaining to be read.
     *  Logically, this is the different between where the read and write pointers are.
     */
    uint32_t bytes_to_read() const {
      return bytes_to_read_from_index(read_ind);
    }

    /*
     *  Returns the current number of bytes remaining to be read from a given index
     *  Logically, this is the different between where the given index is and the write pointers.
     */
    uint32_t bytes_to_read_from_index(const index_t& ind) const {
      return (write_ind.first - ind.first) * buffer_len + write_ind.second - ind.second;
    }

    /*
     *  Returns the current number of bytes available to be written.
     *  Logically, this is the different between the write pointer and the
     *  end of the buffer.  If this is not enough room, call either
     *  add_buffer_to_chain() or add_room() to create more space.
     */
    uint32_t bytes_to_write() const {
      return total_bytes() - write_ind.first * buffer_len - write_ind.second;
    }

    /*
     *  Returns the total number of bytes in the buffer chain.
     */
    uint32_t total_bytes() const {
      return buffer_len * buffers.size();
    }

    /*
     *  Advances the read pointer/index the supplied number of bytes along
     *  the buffer chain.  Any buffers that the read pointer moves beyond
     *  will be removed from the buffer chain.  If the read pointer becomes
     *  equal to the write pointer, the message buffer will be reset to
     *  its initial state (one buffer with read and write pointers at the
     *  start).
     */
    void advance_read_ptr(uint32_t bytes) {
      advance_index(read_ind, bytes);
      if (read_ind == write_ind) {
        reset();
      } else if (read_ind.first > 0) {
        while (read_ind.first > 0) {
          free(buffers.front());
          buffers.pop_front();
          sanity_check--;
          read_ind.first--;
          write_ind.first--;
        }
      }
    }

    /*
     *  Advances the write pointer/index the supplied number of bytes along
     *  the buffer chain.
     */
    void advance_write_ptr(uint32_t bytes) {
      advance_index(write_ind, bytes);
      while (write_ind.first >= buffers.size()) {
        sanity_check++;
        buffers.push_back(malloc());
      }
    }

    /*
     *  Creates and returns a vector of boost mutable_buffers that can
     *  be passed to boost async_read() and async_read_some() functions.
     *  The beginning of the vector will be the write pointer, which
     *  should be advanced the number of bytes read after the read returns.
     */
    std::vector<boost::asio::mutable_buffer> get_buffer_sequence_for_boost_async_read() {
      std::vector<boost::asio::mutable_buffer> seq;
      FC_ASSERT(write_ind.first < buffers.size());
      seq.push_back(boost::asio::buffer(&buffers[write_ind.first]->at(write_ind.second),
                                                buffer_len - write_ind.second));
      for (std::size_t i = write_ind.first + 1; i < buffers.size(); i++) {
        seq.push_back(boost::asio::buffer(&buffers[i]->at(0), buffer_len));
      }
      return seq;
    }

    /*
     *  Reads size bytes from the buffer chain starting at the read pointer.
     *  The read pointer is advanced size bytes.
     */
    bool read(void* s, uint32_t size) {
      if (bytes_to_read() < size) {
        FC_THROW_EXCEPTION( out_of_range_exception, "tried to read ${r} but only ${s} left",
                            ("r", size)( "s", bytes_to_read() ) );
      }
      if (read_ind.second + size <= buffer_len) {
        memcpy(s, read_ptr(), size);
        advance_read_ptr(size);
      } else {
        uint32_t num_in_buffer = buffer_len - read_ind.second;
        memcpy(s, read_ptr(), num_in_buffer);
        advance_read_ptr(num_in_buffer);
        read((char*)s + num_in_buffer, size - num_in_buffer);
      }
      return true;
    }

    /*
     *  Reads size bytes from the buffer chain starting at the supplied index.
     *  The supplied index is advanced, but the read pointer is unaffected.
     */
    bool peek(void* s, uint32_t size, index_t& index) const {
      if (bytes_to_read_from_index(index) < size) {
        FC_THROW_EXCEPTION( out_of_range_exception, "tried to peek ${r} but only ${s} left",
                            ("r", size)( "s", bytes_to_read_from_index( index ) ) );
      }
      if (index.second + size <= buffer_len) {
        memcpy(s, get_ptr(index), size);
        advance_index(index, size);
      } else {
        uint32_t num_in_buffer = buffer_len - index.second;
        memcpy(s, get_ptr(index), num_in_buffer);
        advance_index(index, num_in_buffer);
        peek((char*)s + num_in_buffer, size - num_in_buffer, index);
      }
      return true;
    }

     /*
      *  Advances the supplied index along the buffer chain the specified
      *  number of bytes.
      */
     static void advance_index(index_t& index, uint32_t bytes) {
        index.first += (bytes + index.second) / buffer_len;
        index.second = (bytes + index.second) % buffer_len;
     }

     /*
     *  Creates an mb_datastream object that can be used with the
     *  fc library's unpack functionality.
     */
    mb_datastream<buffer_len> create_datastream();

    /*
     *  Creates an mb_peek_datastream object that can be used with the
     *  fc library's unpack functionality.
     */
    mb_peek_datastream<buffer_len> create_peek_datastream();

  private:
    using buffer_type = std::array<char, buffer_len>;
    using pool_type = boost::singleton_pool<message_buffer, sizeof(buffer_type)>;
    static buffer_type* malloc() { return static_cast<buffer_type*>(pool_type::malloc()); }
    static void free(buffer_type* ptr) { pool_type::free(ptr); }

    /*
     *  Returns the character pointer associated with the supplied index.
     */
    char* get_ptr(const index_t& index) const {
      return &buffers[index.first]->at(index.second);
    }

    std::deque<std::array<char, buffer_len>* > buffers;
    index_t read_ind;
    index_t write_ind;
    size_t sanity_check;

  };

  /*
   *  @brief datastream adapter that adapts message_buffer for use with fc unpack
   *
   *  This class supports unpack functionality but not pack.
   */
  template <uint32_t buffer_len>
  class mb_datastream {
     public:
        explicit mb_datastream( message_buffer<buffer_len>& m ) : mb(m) {}

        inline void skip( size_t s ) { mb.advance_read_ptr(s); }
        inline bool read( char* d, size_t s ) {
          if (mb.bytes_to_read() >= s) {
            mb.read(d, s);
            return true;
          }
          fc::detail::throw_datastream_range_error( "read", mb.bytes_to_read(), s - mb.bytes_to_read());
        }

        inline bool   get( unsigned char& c ) { return mb.read(&c, 1); }
        inline bool   get( char& c ) { return mb.read(&c, 1); }

      private:
        message_buffer<buffer_len>& mb;
  };

  template <uint32_t buffer_len>
  inline mb_datastream<buffer_len> message_buffer<buffer_len>::create_datastream() {
    return mb_datastream<buffer_len>(*this);
  }

  /*
   *  @brief peek datastream adapter that adapts message_buffer for use with fc unpack
   *
   *  This class supports unpack functionality but not pack.
   */
  template <uint32_t buffer_len>
  class mb_peek_datastream {
  public:
     explicit mb_peek_datastream( const message_buffer<buffer_len>& m )
     : mb( m ), index( m.read_index() ) {}

     inline void skip( size_t s ) { message_buffer<buffer_len>::advance_index( index, s ); }

     inline bool read( char* d, size_t s ) {
        if( mb.bytes_to_read_from_index(index) >= s ) {
           mb.peek( d, s, index );
           return true;
        }
        fc::detail::throw_datastream_range_error( "peek",
              mb.bytes_to_read_from_index(index), s - mb.bytes_to_read_from_index(index) );
     }

     inline bool get( unsigned char& c ) { return mb.peek( &c, 1, index ); }
     inline bool get( char& c ) { return mb.peek( &c, 1, index ); }

  private:
     const message_buffer<buffer_len>& mb;
     typename message_buffer<buffer_len>::index_t index{0,0};
  };

  template <uint32_t buffer_len>
  inline mb_peek_datastream<buffer_len> message_buffer<buffer_len>::create_peek_datastream() {
     return mb_peek_datastream<buffer_len>( *this );
  }

} // namespace fc
