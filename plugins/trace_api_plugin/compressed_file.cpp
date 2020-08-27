#include <eosio/trace_api/compressed_file.hpp>

#include <zlib.h>

namespace {
   using seek_point_entry = std::tuple<uint64_t, uint64_t>;
   constexpr size_t expected_seek_point_entry_size = 16;

   using seek_point_count_type = uint16_t;
   constexpr size_t expected_seek_point_count_size = 2;

   constexpr int raw_zlib_window_bits = -15;

   // These are hard-coded expectations in the written file format
   //
   static_assert(sizeof(seek_point_entry) == expected_seek_point_entry_size, "unexpected size for seek point");
   static_assert(sizeof(seek_point_count_type) == expected_seek_point_count_size, "Unexpected size for seek point count");
}

namespace eosio::trace_api {

struct compressed_file_impl {
   static constexpr size_t read_buffer_size = 4*1024;
   static constexpr size_t compressed_buffer_size = 4*1024;

   ~compressed_file_impl()
   {
      if (initialized) {
         inflateEnd(&strm);
         initialized = false;
      }
   }

   void read( char* d, size_t n, fc::cfile& file )
   {
      if (!initialized) {
         if (Z_OK != inflateInit2(&strm, raw_zlib_window_bits)) {
            throw std::runtime_error("failed to initialize decompression");
         }

         remaining_read_buffer = 0;
         strm.avail_in = 0;
         initialized = true;
      }

      size_t written = 0;

      // consume the left over from the last read if there is any
      if (remaining_read_buffer > 0) {
         auto to_read = std::min(remaining_read_buffer, n);
         std::memcpy(d, read_buffer.data(), to_read );
         remaining_read_buffer -= to_read;
         written += to_read;

         if ( remaining_read_buffer > 0 ) {
            std::memmove(read_buffer.data(), read_buffer.data() + to_read, remaining_read_buffer);
            return;
         }
      }


      // decompress more chunks
      while (written < n) {
         if ( strm.avail_in == 0 ) {
            size_t remaining = file_size - file.tellp();
            size_t to_read = std::min((size_t)compressed_buffer.size(), remaining);
            file.read(reinterpret_cast<char*>(compressed_buffer.data()), to_read);
            strm.avail_in = to_read;
            strm.next_in = compressed_buffer.data();
         }

         do {
            strm.avail_out = read_buffer.size();
            strm.next_out = read_buffer.data();
            auto ret = inflate(&strm, Z_NO_FLUSH);

            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
               throw compressed_file_error("Error decompressing: " + std::string(strm.msg));
            }



            auto bytes_decompressed = read_buffer.size() - strm.avail_out;
            if (bytes_decompressed > 0) {
               auto to_copy = std::min(bytes_decompressed, n - written);
               std::memcpy(d + written, read_buffer.data(), to_copy);
               written += to_copy;

               if (bytes_decompressed > to_copy) {
                  // move remaining to the front of the buffer
                  std::memmove(read_buffer.data(), read_buffer.data() + to_copy, bytes_decompressed - to_copy);
                  remaining_read_buffer = bytes_decompressed - to_copy;
               }
            }

            if (written < n && ret == Z_STREAM_END) {
               throw std::ios_base::failure("Attempting to read past the end of a compressed file");
            }

            if (ret == Z_BUF_ERROR) {
               // need more input
               if (strm.avail_in != 0) {
                  throw compressed_file_error("Error decompressing cannot continue processing input");
               }
               break;
            }

         } while (strm.avail_out == 0 && written < n);
      }
   }

   void seek( long loc, fc::cfile& file ) {
      if (initialized) {
         inflateEnd(&strm);
         initialized = false;
      }

      long remaining = loc;

      // read in the seek point map
      file.seek_end(-expected_seek_point_count_size);
      seek_point_count_type seek_point_count = 0;
      file.read(reinterpret_cast<char*>(&seek_point_count), sizeof(seek_point_count));

      if (seek_point_count > 0) {
         int seek_map_size = sizeof(seek_point_entry) * seek_point_count;
         file.seek_end(-expected_seek_point_count_size - seek_map_size);

         std::vector<seek_point_entry> seek_point_map(seek_point_count);
         file.read(reinterpret_cast<char*>(seek_point_map.data()), seek_point_map.size() * sizeof(seek_point_entry));

         // seek to the neareast seek point
         auto iter = std::lower_bound(seek_point_map.begin(), seek_point_map.end(), (uint64_t)loc, []( const auto& lhs, const auto& rhs ){
            return std::get<0>(lhs) < rhs;
         });

         // special case when there is a seek point that is exact
         if ( iter != seek_point_map.end() && std::get<0>(*iter) == static_cast<unsigned long>(loc) ) {
            file.seek(std::get<1>(*iter));
            return;
         }

         // special case when this is before the first seek point
         if ( iter == seek_point_map.begin() ) {
            file.seek(0);
         } else {
            // if lower bound wasn't exact iter will be one past the seek point we need
            const auto& seek_pt = *(iter - 1);
            file.seek(std::get<1>(seek_pt));
            remaining -= std::get<0>(seek_pt);
         }
      } else {
         file.seek(0);
      }

      // read up to the expected offset
      if (remaining > 0) {
         auto pre_read_buffer = std::vector<char>(remaining);
         read(pre_read_buffer.data(), pre_read_buffer.size(), file);
      }
   }

   z_stream strm;
   std::vector<uint8_t> compressed_buffer = std::vector<uint8_t>(compressed_buffer_size);
   std::vector<uint8_t> read_buffer = std::vector<uint8_t>(read_buffer_size);
   size_t remaining_read_buffer = 0;
   bool initialized = false;
   size_t file_size = 0;
};

compressed_file::compressed_file( fc::path file_path )
:file_path(std::move(file_path))
,file_ptr(nullptr)
,impl(std::make_unique<compressed_file_impl>())
{
   impl->file_size = fc::file_size(file_path);
}

compressed_file::~compressed_file()
{}

void compressed_file::seek( long loc ) {
   impl->seek(loc, *file_ptr);

}

void compressed_file::read( char* d, size_t n ) {
   impl->read(d, n, *file_ptr);
}

// these are defaulted now that the opaque impl type is known
//
compressed_file::compressed_file( compressed_file&& ) = default;
compressed_file& compressed_file::operator= ( compressed_file&& ) = default;


bool compressed_file::process( const fc::path& input_path, const fc::path& output_path, size_t seek_point_stride  ) {
   if (!fc::exists(input_path)) {
      throw std::ios_base::failure(std::string("Attempting to create compressed_file from file that does not exist: ") + input_path.generic_string());
   }

   const size_t input_size = fc::file_size(input_path);
   if (input_size == 0) {
      throw std::ios_base::failure(std::string("Attempting to create compressed_file from file that is empty: ") + input_path.generic_string());
   }

   // subtract 1 to make sure that the truncated division will only create a seek point if there is at least one byte
   // in the next stride.  So, a file size of N and a stride >= N results in 0 seek points.  N + 1 will have a seek
   // point for the last byte as will XN + 1 which will create X seek points (the last of which is for the last byte)
   // of the file
   const auto seek_point_count = (input_size - 1) / seek_point_stride;
   std::vector<seek_point_entry> seek_point_map(seek_point_count);

   fc::cfile input_file;
   input_file.set_file_path(input_path);
   input_file.open("rb");

   fc::cfile output_file;
   output_file.set_file_path(output_path);
   output_file.open("wb");

   z_stream strm;
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;

   if (deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, raw_zlib_window_bits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
      return false;
   }

   constexpr size_t buffer_size = 64*1024;
   auto input_buffer = std::vector<uint8_t>(buffer_size);
   auto output_buffer = std::vector<uint8_t>(buffer_size);

   auto bytes_remaining_before_sync = seek_point_stride;
   unsigned int next_sync_point = 0;

   // process a single chunk of input completely,
   // this may sometime loop multiple times if the compressor state combined with input data creates more than a
   // single buffer's worth of data
   //
   auto process_chunk = [&]( size_t input_size, int mode ) {
      strm.avail_in = input_size;
      strm.next_in = input_buffer.data();

      do {
         strm.avail_out = output_buffer.size();
         strm.next_out = output_buffer.data();
         auto ret = deflate(&strm, mode);

         const bool success = ret == Z_OK || (mode == Z_FINISH && ret == Z_STREAM_END);
         if (!success) {
            return ret;
         }

         output_file.write(reinterpret_cast<const char*>(output_buffer.data()), output_buffer.size() - strm.avail_out);
      } while (strm.avail_out == 0);

      return Z_OK;
   };

   size_t read_offset = 0;
   while (read_offset < input_size) {
      const auto bytes_remaining = input_size - read_offset;
      const auto read_size = std::min({ buffer_size, bytes_remaining, bytes_remaining_before_sync });
      input_file.read(reinterpret_cast<char*>(input_buffer.data()), read_size);

      auto ret = process_chunk(read_size, Z_NO_FLUSH);
      if (ret != Z_OK) {
         throw compressed_file_error(std::string("deflate failed: ") + std::to_string(ret));
      }
      read_offset += read_size;

      if (read_size == bytes_remaining ) {
         // finish the file out by draining remaining output
         ret = process_chunk(0, Z_FINISH);
         if (ret != Z_OK) {
            throw compressed_file_error(std::string("failed to finalize file compression: ") + std::to_string(ret));
         }
      } else if ( read_size == bytes_remaining_before_sync ) {
         // create a sync point by flushing the compressor so a decompressor can start at this offset
         ret = process_chunk(0, Z_FULL_FLUSH);
         if (ret != Z_OK) {
            throw compressed_file_error(std::string("failed to create sync point: ") + std::to_string(ret));
         }

         seek_point_map.at(next_sync_point++) = {read_offset, output_file.tellp()};

         if (next_sync_point == seek_point_count) {
            // if we are out of sync points, set this value one past the end (disabling it)
            bytes_remaining_before_sync = input_size - read_offset + 1;
         } else {
            bytes_remaining_before_sync = seek_point_stride;
         }
      } else {
         bytes_remaining_before_sync -= read_size;
      }
   }

   deflateEnd(&strm);
   input_file.close();

   // write out the seek point table
   if (seek_point_map.size() > 0) {
      output_file.write(reinterpret_cast<const char*>(seek_point_map.data()), seek_point_map.size() * sizeof(seek_point_entry));
   }

   // write out the seek point count
   output_file.write(reinterpret_cast<const char*>(&seek_point_count), sizeof(seek_point_count_type));

   output_file.close();
   return true;
}

}