#pragma once

#include <boost/iostreams/categories.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/cfile.hpp>
#include <fc/io/datastream.hpp>

namespace fc {

///
/// The extensions to existing fc::datastream
///
/// binary_stream provdes all the same interfaces to be used for fc::raw::pack and fc::raw::unpack
/// with some more storage choices.
///
/// The storage can be fc::cfile, std::vector, std::deque or std::streambuf derived classes.
///
/// <code>
///    fc::binary_stream<cfile> strm;
///    ...
///    fc::raw::pack(strm, data);
/// </code>
///
/// <code>
///   fc::binary_stream<std::vector<char>> strm;
///   fc::raw::pack(strm, data);
/// </code>
///
/// To serialize data to file with zlib compressor
/// <code>
///    #include "bio_device_adaptor.hpp"
///
///    namespace bio = boost::iostreams;
///    fc::binary_stream<cfile> file_strm;
///    ....
///    fc::binary_stream<bio::filter_ostreambuf> strm(bio::zlib_compressor() | fc::to_sink(file_strm));
///    fc::raw::pack(strm, data);
///    file_strm.flush();
/// </code>
///
/// To deserialize data from file with zlib decompressor.
/// <code>
///    #include "bio_device_adaptor.hpp"
///   
///    namespace bio = boost::iostreams;
///    fc::binary_stream<cfile> file_strm;
///    ....
///    fc::binary_stream<bio::filter_istreambuf> strm(bio::zlib_decompressor() | fc::to_source(file_strm));
///    fc::raw::unpack(strm, data);
/// </code>
///

template <typename Storage, typename Enable = void>
class binary_stream;

template <>
class binary_stream<fc::cfile, void> : public fc::cfile {
 public:
   using fc::cfile::cfile;

   bool seekp(size_t pos) { return this->seek(pos), true; }

   bool get(char& c) {
      c = this->getc();
      return true;
   }

   bool remaining() { return !this->eof(); }

   fc::cfile&       storage() { return *this; }
   const fc::cfile& storage() const { return *this; }
};

template <>
class binary_stream<char*, void> : public fc::datastream<char*> {
 public:
   using fc::datastream<char*>::datastream;
   binary_stream(std::vector<char>& data)
       : datastream<char*>(data.data(), data.size()) {}
};

template <>
class binary_stream<const char*, void> : public fc::datastream<const char*> {
 public:
   using fc::datastream<const char*>::datastream;
   binary_stream(const std::vector<char>& data)
       : datastream<const char*>(data.data(), data.size()) {}
};

template <typename Streambuf>
class binary_stream<Streambuf, typename std::enable_if_t<std::is_base_of_v<std::streambuf, Streambuf>>> {
 private:
   Streambuf buf;

 public:
   template <typename... Args>
   binary_stream(Args&&... args)
       : buf(std::forward<Args>(args)...) {}

   size_t read(char* data, size_t n) { return buf.sgetn(data, n); }
   size_t write(const char* data, size_t n) { return buf.sputn(data, n); }
   size_t tellp() { return this->pubseekoff(0, std::ios::cur); }
   bool   get(char& c) {
      c = buf.sbumpc();
      return true;
   }
   bool seekp(size_t off) {
      buf.pubseekoff(off, std::ios::beg);
      return true;
   }
   bool remaining() { return buf.in_avail(); }

   Streambuf&       storage() { return buf; }
   const Streambuf& storage() const { return buf; }
};

template <typename Container>
class binary_stream<Container, typename std::enable_if_t<(std::is_same_v<std::vector<char>, Container> ||
                                                          std::is_same_v<std::deque<char>, Container>)>> {
 private:
   Container _container;
   size_t    cur;

 public:
   template <typename... Args>
   binary_stream(Args&&... args)
       : _container(std::forward<Args>(args)...)
       , cur(0) {}

   size_t read(char* s, size_t n) {
      if (cur + n > _container.size()) {
         FC_THROW_EXCEPTION(out_of_range_exception,
                            "read binary_stream<std::vector<char>> of length ${len} over by ${over}",
                            ("len", _container.size())("over", _container.size() - n));
      }
      std::copy_n(_container.begin() + cur, n, s);
      cur += n;
      return n;
   }

   size_t write(const char* s, size_t n) {
      _container.resize(std::max(cur + n, _container.size()));
      std::copy_n(s, n, _container.begin() + cur);
      cur += n;
      return n;
   }

   bool seekp(size_t off) {
      cur = off;
      return true;
   }

   size_t tellp() const { return cur; }

   bool get(char& c) {
      this->read(&c, 1);
      return true;
   }

   size_t remaining() const { return _container.size() - cur; }

   Container&       storage() { return _container; }
   const Container& storage() const { return _container; }
};

} // namespace fc
