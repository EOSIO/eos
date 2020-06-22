#define BOOST_TEST_MODULE trace_trace_file
#include <boost/test/included/unit_test.hpp>
#include <fc/io/cfile.hpp>
#include <eosio/trace_api/test_common.hpp>
#include <eosio/trace_api/store_provider.hpp>
#include <boost/filesystem.hpp>

using namespace eosio;
using namespace eosio::trace_api;
using namespace eosio::trace_api::test_common;
namespace bfs = boost::filesystem;
using open_state = slice_directory::open_state;

namespace {
   struct test_fixture {

      const block_trace_v1 bt {
         {
            "0000000000000000000000000000000000000000000000000000000000000001"_h,
            1,
            "0000000000000000000000000000000000000000000000000000000000000003"_h,
            chain::block_timestamp_type(1),
            "bp.one"_n,
            {}
         },
         "0000000000000000000000000000000000000000000000000000000000000000"_h,
         "0000000000000000000000000000000000000000000000000000000000000000"_h,
         0,
         {
            {
               {
                  "0000000000000000000000000000000000000000000000000000000000000001"_h,
                  {
                     {
                        0,
                        "eosio.token"_n, "eosio.token"_n, "transfer"_n,
                        {{ "alice"_n, "active"_n }},
                        make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                     },
                     {
                        1,
                        "alice"_n, "eosio.token"_n, "transfer"_n,
                        {{ "alice"_n, "active"_n }},
                        make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                     },
                     {
                        2,
                        "bob"_n, "eosio.token"_n, "transfer"_n,
                        {{ "alice"_n, "active"_n }},
                        make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                     }
                  }
               },
               fc::enum_type<uint8_t, chain::transaction_receipt_header::status_enum>{chain::transaction_receipt_header::status_enum::executed},
               10,
               5,
               std::vector<chain::signature_type>{chain::signature_type()},
               chain::transaction_header{chain::time_point(), 1, 0, 100, 50, 0}
            }
         }
      };
      
      const block_trace_v1 bt2 {
         {
            "0000000000000000000000000000000000000000000000000000000000000002"_h,
            5,
            "0000000000000000000000000000000000000000000000000000000000000005"_h,
            chain::block_timestamp_type(2),
            "bp.two"_n
         },
         "0000000000000000000000000000000000000000000000000000000000000000"_h,
         "0000000000000000000000000000000000000000000000000000000000000000"_h,
         0,
         {
            {
               {
                  "f000000000000000000000000000000000000000000000000000000000000004"_h,
                  {}
               },
               fc::enum_type<uint8_t, chain::transaction_receipt_header::status_enum>{chain::transaction_receipt_header::status_enum::executed},
               10,
               5,
               std::vector<chain::signature_type>{chain::signature_type()},
               chain::transaction_header{chain::time_point(), 1, 0, 100, 50, 0}
            }
         }
      };

      const block_trace_v0 bt_v0 {
         "0000000000000000000000000000000000000000000000000000000000000001"_h,
         1,
         "0000000000000000000000000000000000000000000000000000000000000003"_h,
         chain::block_timestamp_type(1),
         "bp.one"_n,
         {
            {
               "0000000000000000000000000000000000000000000000000000000000000001"_h,
               {
                  {
                     0,
                     "eosio.token"_n, "eosio.token"_n, "transfer"_n,
                     {{ "alice"_n, "active"_n }},
                     make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  },
                  {
                     1,
                     "alice"_n, "eosio.token"_n, "transfer"_n,
                     {{ "alice"_n, "active"_n }},
                     make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  },
                  {
                     2,
                     "bob"_n, "eosio.token"_n, "transfer"_n,
                     {{ "alice"_n, "active"_n }},
                     make_transfer_data( "alice"_n, "bob"_n, "0.0001 SYS"_t, "Memo!" )
                  }
               }
            }
         }
      };

      const metadata_log_entry be1 { block_entry_v0 {
         "b000000000000000000000000000000000000000000000000000000000000001"_h, 5, 0
      } };
      const metadata_log_entry le1 { lib_entry_v0 { 4 } };
      const metadata_log_entry be2 { block_entry_v0 {
         "b000000000000000000000000000000000000000000000000000000000000002"_h, 7, 0
      } };
      const metadata_log_entry le2 { lib_entry_v0 { 5 } };

      bool create_non_empty_trace_slice( slice_directory& sd, uint32_t slice_number, fc::cfile& file) {
         const uint8_t bad_which = 0x7F;
         if (!sd.find_or_create_trace_slice(slice_number, open_state::write, file)) {
            file.write(reinterpret_cast<const char*>(&bad_which), sizeof(uint8_t));
            file.close();
            return sd.find_or_create_trace_slice(slice_number, open_state::read, file);
         }
         return false;
      }
   };

   struct test_store_provider : public store_provider {
      test_store_provider(const bfs::path& slice_dir, uint32_t width, std::optional<uint32_t> minimum_irreversible_history_blocks = std::optional<uint32_t>(), std::optional<uint32_t> minimum_uncompressed_irreversible_history_blocks = std::optional<uint32_t>(), size_t compression_seek_point_stride = 0)
         : store_provider(slice_dir, width, minimum_irreversible_history_blocks, minimum_uncompressed_irreversible_history_blocks, compression_seek_point_stride) {
      }
      using store_provider::scan_metadata_log_from;
      using store_provider::read_data_log;
   };

   class vslice_datastream;

   struct vslice {
      enum mode { read_mode, write_mode};
      vslice(mode m = write_mode) : _mode(m) {}
      long tellp() const {
         return _pos;
      }

      void seek( long loc ) {
         if (_mode == read_mode) {
            if (loc > _buffer.size()) {
               throw std::ios_base::failure( "read vslice unable to seek to: " + std::to_string(loc) + ", end is at: " + std::to_string(_buffer.size()));
            }
         }
         _pos = loc;
      }

      void seek_end( long loc ) {
         _pos = _buffer.size();
      }

      void read( char* d, size_t n ) {
         if( _pos + n > _buffer.size() ) {
            throw std::ios_base::failure( "vslice unable to read " + std::to_string( n ) + " bytes; only can read " + std::to_string( _buffer.size() - _pos ) );
         }
         std::memcpy( d, _buffer.data() + _pos, n);
         _pos += n;
      }

      void write( const char* d, size_t n ) {
         if (_mode == read_mode) {
            throw std::ios_base::failure( "read vslice should not have write called" );
         }
         if (_buffer.size() < _pos + n) {
            _buffer.resize(_pos + n);
         }
         std::memcpy( _buffer.data() + _pos, d, n);
         _pos += n;
      }

      void flush() {
         _flush = true;
      }

      void sync() {
         _sync = true;
      }

      vslice_datastream create_datastream();

      std::vector<char> _buffer;
      mode _mode = write_mode;
      long _pos = 0l;
      bool _flush = false;
      bool _sync = false;
   };

   class vslice_datastream {
   public:
      explicit vslice_datastream( vslice& vs ) : _vs(vs) {}

      void skip( size_t s ) {
         std::vector<char> d( s );
         read( &d[0], s );
      }

      bool read( char* d, size_t s ) {
         _vs.read( d, s );
         return true;
      }

      bool get( unsigned char& c ) { return get( *(char*)&c ); }

      bool get( char& c ) { return read(&c, 1); }

   private:
      vslice& _vs;
   };

   inline vslice_datastream vslice::create_datastream() {
      return vslice_datastream(*this);
   }
}

BOOST_AUTO_TEST_SUITE(slice_tests)
   BOOST_FIXTURE_TEST_CASE(write_data_trace, test_fixture)
   {
      vslice vs;
      const auto offset = append_store(bt, vs );
      BOOST_REQUIRE_EQUAL(offset,0);

      const auto offset2 = append_store(bt2, vs );
      BOOST_REQUIRE(offset < offset2);

      vs._pos = offset;
      const auto bt_returned = extract_store<block_trace_v1>( vs );
      BOOST_REQUIRE(bt_returned == bt);

      vs._pos = offset2;
      const auto bt_returned2 = extract_store<block_trace_v1>( vs );
      BOOST_REQUIRE(bt_returned2 == bt2);
   }

   BOOST_FIXTURE_TEST_CASE(write_data_multi_trace_version, test_fixture)
   {
      vslice vs;
      const auto offset = append_store(bt_v0, vs );
      BOOST_REQUIRE_EQUAL(offset,0);

      const auto offset2 = append_store(bt, vs );
      BOOST_REQUIRE(offset < offset2);

      vs._pos = offset;
      const auto bt_returned = extract_store<block_trace_v0>( vs );
      BOOST_REQUIRE(bt_returned == bt_v0);

      vs._pos = offset2;
      const auto bt_returned2 = extract_store<block_trace_v1>( vs );
      BOOST_REQUIRE(bt_returned2 == bt);
   }

   BOOST_FIXTURE_TEST_CASE(write_metadata_trace, test_fixture)
   {
      vslice vs;
      const auto offset = append_store( be1, vs );
      auto next_offset = vs._pos;
      BOOST_REQUIRE(offset < next_offset);
      const auto offset2 = append_store( le1, vs );
      BOOST_REQUIRE(next_offset <= offset2);
      BOOST_REQUIRE(offset2 < vs._pos);
      next_offset = vs._pos;
      const auto offset3 = append_store( be2, vs );
      BOOST_REQUIRE(next_offset <= offset3);
      BOOST_REQUIRE(offset3 < vs._pos);
      next_offset = vs._pos;
      const auto offset4 = append_store( le2, vs );
      BOOST_REQUIRE(next_offset <= offset4);
      BOOST_REQUIRE(offset4 < vs._pos);

      vs._pos = offset;
      const auto be_returned1 = extract_store<metadata_log_entry>( vs );
      BOOST_REQUIRE(be_returned1.contains<block_entry_v0>());
      const auto real_be_returned1 = be_returned1.get<block_entry_v0>();
      const auto real_be1 = be1.get<block_entry_v0>();
      BOOST_REQUIRE(real_be_returned1 == real_be1);

      vs._pos = offset2;
      const auto le_returned1 = extract_store<metadata_log_entry>( vs );
      BOOST_REQUIRE(le_returned1.contains<lib_entry_v0>());
      const auto real_le_returned1 = le_returned1.get<lib_entry_v0>();
      const auto real_le1 = le1.get<lib_entry_v0>();
      BOOST_REQUIRE(real_le_returned1 == real_le1);

      vs._pos = offset3;
      const auto be_returned2 = extract_store<metadata_log_entry>( vs );
      BOOST_REQUIRE(be_returned2.contains<block_entry_v0>());
      const auto real_be_returned2 = be_returned2.get<block_entry_v0>();
      const auto real_be2 = be2.get<block_entry_v0>();
      BOOST_REQUIRE(real_be_returned2 == real_be2);

      vs._pos = offset4;
      const auto le_returned2 = extract_store<metadata_log_entry>( vs );
      BOOST_REQUIRE(le_returned2.contains<lib_entry_v0>());
      const auto real_le_returned2 = le_returned2.get<lib_entry_v0>();
      const auto real_le2 = le2.get<lib_entry_v0>();
      BOOST_REQUIRE(real_le_returned2 == real_le2);
   }

   BOOST_FIXTURE_TEST_CASE(slice_number, test_fixture)
   {
      fc::temp_directory tempdir;
      slice_directory sd(tempdir.path(), 100, std::optional<uint32_t>(), std::optional<uint32_t>(), 0);
      BOOST_REQUIRE_EQUAL(sd.slice_number(99), 0);
      BOOST_REQUIRE_EQUAL(sd.slice_number(100), 1);
      BOOST_REQUIRE_EQUAL(sd.slice_number(1599), 15);
      slice_directory sd2(tempdir.path(), 0x10, std::optional<uint32_t>(), std::optional<uint32_t>(), 0);
      BOOST_REQUIRE_EQUAL(sd2.slice_number(0xf), 0);
      BOOST_REQUIRE_EQUAL(sd2.slice_number(0x100), 0x10);
      BOOST_REQUIRE_EQUAL(sd2.slice_number(0x233), 0x23);
   }

   BOOST_FIXTURE_TEST_CASE(slice_file, test_fixture)
   {
      fc::temp_directory tempdir;
      slice_directory sd(tempdir.path(), 100, std::optional<uint32_t>(), std::optional<uint32_t>(), 0);
      fc::cfile slice;

      // create trace slices
      for (uint i = 0; i < 9; ++i) {
         bool found = sd.find_or_create_trace_slice(i, open_state::write, slice);
         BOOST_REQUIRE(!found);
         bfs::path fp = slice.get_file_path();
         BOOST_REQUIRE_EQUAL(fp.parent_path().generic_string(), tempdir.path().generic_string());
         const std::string expected_filename = "trace_0000000" + std::to_string(i) + "00-0000000" + std::to_string(i+1) + "00.log";
         BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
         BOOST_REQUIRE(slice.is_open());
         BOOST_REQUIRE_EQUAL(bfs::file_size(fp), 0);
         BOOST_REQUIRE_EQUAL(slice.tellp(), 0);
         slice.close();
      }

      // create trace index slices
      for (uint i = 0; i < 9; ++i) {
         bool found = sd.find_or_create_index_slice(i, open_state::write, slice);
         BOOST_REQUIRE(!found);
         fc::path fp = slice.get_file_path();
         BOOST_REQUIRE_EQUAL(fp.parent_path().generic_string(), tempdir.path().generic_string());
         const std::string expected_filename = "trace_index_0000000" + std::to_string(i) + "00-0000000" + std::to_string(i+1) + "00.log";
         BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
         BOOST_REQUIRE(slice.is_open());
         slice_directory::index_header h;
         const auto data = fc::raw::pack(h);
         BOOST_REQUIRE_EQUAL(bfs::file_size(fp), data.size());
         BOOST_REQUIRE_EQUAL(slice.tellp(), data.size());
         slice.close();
      }

      // reopen trace slice for append
      bool found = sd.find_or_create_trace_slice(0, open_state::write, slice);
      BOOST_REQUIRE(found);
      fc::path fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.parent_path().generic_string(), tempdir.path().generic_string());
      std::string expected_filename = "trace_0000000000-0000000100.log";
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), 0);
      BOOST_REQUIRE_EQUAL(slice.tellp(), 0);
      uint64_t offset = append_store(bt, slice);
      BOOST_REQUIRE_EQUAL(offset, 0);
      auto data = fc::raw::pack(bt);
      BOOST_REQUIRE(slice.tellp() > 0);
      BOOST_REQUIRE_EQUAL(data.size(), slice.tellp());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), slice.tellp());
      uint64_t trace_file_size = bfs::file_size(fp);
      slice.close();

      // open same file for read
      found = sd.find_or_create_trace_slice(0, open_state::read, slice);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), trace_file_size);
      BOOST_REQUIRE_EQUAL(slice.tellp(), 0);
      slice.close();

      // open same file for append again
      found = sd.find_or_create_trace_slice(0, open_state::write, slice);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), trace_file_size);
      BOOST_REQUIRE_EQUAL(slice.tellp(), trace_file_size);
      slice.close();

      // reopen trace index slice for append
      found = sd.find_or_create_index_slice(1, open_state::write, slice);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.parent_path().generic_string(), tempdir.path().generic_string());
      expected_filename = "trace_index_0000000100-0000000200.log";
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      slice_directory::index_header h;
      data = fc::raw::pack(h);
      const uint64_t header_size = data.size();
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), header_size);
      BOOST_REQUIRE_EQUAL(slice.tellp(), header_size);
      offset = append_store(be1, slice);
      BOOST_REQUIRE_EQUAL(offset, header_size);
      data = fc::raw::pack(be1);
      const auto be1_size = data.size();
      BOOST_REQUIRE_EQUAL(header_size + be1_size, slice.tellp());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), slice.tellp());
      uint64_t index_file_size = bfs::file_size(fp);
      slice.close();

      found = sd.find_or_create_index_slice(1, open_state::read, slice);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), header_size + be1_size);
      BOOST_REQUIRE_EQUAL(slice.tellp(), header_size);
      slice.close();

      found = sd.find_or_create_index_slice(1, open_state::write, slice);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), header_size + be1_size);
      BOOST_REQUIRE_EQUAL(slice.tellp(), header_size + be1_size);
      offset = append_store(le1, slice);
      BOOST_REQUIRE_EQUAL(offset, header_size + be1_size);
      data = fc::raw::pack(le1);
      const auto le1_size = data.size();
      BOOST_REQUIRE_EQUAL(header_size + be1_size + le1_size, slice.tellp());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), slice.tellp());
      slice.close();

      found = sd.find_or_create_index_slice(1, open_state::read, slice);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), header_size + be1_size + le1_size);
      BOOST_REQUIRE_EQUAL(slice.tellp(), header_size);
      slice.close();
   }

   BOOST_FIXTURE_TEST_CASE(slice_file_find_test, test_fixture)
   {
      fc::temp_directory tempdir;
      slice_directory sd(tempdir.path(), 100, std::optional<uint32_t>(), std::optional<uint32_t>(), 0);
      fc::cfile slice;

      // create trace slice
      bool found = sd.find_or_create_trace_slice(1, open_state::write, slice);
      BOOST_REQUIRE(!found);
      bfs::path fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.parent_path().generic_string(), tempdir.path().generic_string());
      const std::string expected_filename = "trace_0000000100-0000000200.log";
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), 0);
      BOOST_REQUIRE_EQUAL(slice.tellp(), 0);
      slice.close();

      // find trace slice (and open)
      found = sd.find_trace_slice(1, open_state::write, slice);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.parent_path().generic_string(), tempdir.path().generic_string());
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), 0);
      slice.close();

      // find trace slice (and don't open)
      found = sd.find_trace_slice(1, open_state::write, slice, false);
      BOOST_REQUIRE(found);
      fp = slice.get_file_path();
      BOOST_REQUIRE_EQUAL(fp.parent_path().generic_string(), tempdir.path().generic_string());
      BOOST_REQUIRE_EQUAL(fp.filename().generic_string(), expected_filename);
      BOOST_REQUIRE(!slice.is_open());
      BOOST_REQUIRE_EQUAL(bfs::file_size(fp), 0);
      slice.close();
   }

   void verify_directory_contents(const bfs::path& tempdir, std::set<bfs::path> expected_files) {
      std::set<bfs::path> unexpected_files;
      for (bfs::directory_iterator itr(tempdir); itr != directory_iterator(); ++itr) {
         const auto filename = itr->path().filename();
         if (expected_files.erase(filename) < 1) {
            unexpected_files.insert(filename);
         }
      }
      if (expected_files.size() + unexpected_files.size() == 0)
         return;

      std::string msg;
      if (expected_files.size()) {
         msg += " Expected the following files to be present, but were not:";
      }
      bool comma = false;
      for(auto file : expected_files) {
         if (comma)
            msg += ",";
         msg += " " + file.generic_string();
      }
      if (unexpected_files.size()) {
         msg += " Did not expect the following files to be present, but they were:";
      }
      for(auto file : expected_files) {
         if (comma)
            msg += ",";
         msg += " " + file.generic_string();
      }
      BOOST_FAIL(msg);
   }

   BOOST_FIXTURE_TEST_CASE(slice_dir_cleanup_height_less_than_width, test_fixture)
   {
      fc::temp_directory tempdir;
      const uint32_t width = 10;
      const uint32_t min_saved_blocks = 5;
      slice_directory sd(tempdir.path(), width, std::optional<uint32_t>(min_saved_blocks), std::optional<uint32_t>(), 0);
      fc::cfile file;

      // verify it cleans up when there is just an index file, just a trace file, or when both are there
      // verify it cleans up all slices that need to be cleaned
      std::set<bfs::path> files;
      BOOST_REQUIRE(!sd.find_or_create_index_slice(0, open_state::read, file));
      files.insert(file.get_file_path().filename());
      verify_directory_contents(tempdir.path(), files);
      BOOST_REQUIRE(!sd.find_or_create_trace_slice(0, open_state::read, file));
      files.insert(file.get_file_path().filename());
      BOOST_REQUIRE(!sd.find_or_create_index_slice(1, open_state::read, file));
      files.insert(file.get_file_path().filename());
      BOOST_REQUIRE(!sd.find_or_create_trace_slice(2, open_state::read, file));
      files.insert(file.get_file_path().filename());
      BOOST_REQUIRE(!sd.find_or_create_index_slice(3, open_state::read, file));
      files.insert(file.get_file_path().filename());
      BOOST_REQUIRE(!sd.find_or_create_index_slice(4, open_state::read, file));
      const auto index4 = file.get_file_path().filename();
      files.insert(index4);
      BOOST_REQUIRE(!sd.find_or_create_trace_slice(4, open_state::read, file));
      const auto trace4 = file.get_file_path().filename();
      files.insert(trace4);
      BOOST_REQUIRE(!sd.find_or_create_index_slice(5, open_state::read, file));
      const auto index5 = file.get_file_path().filename();
      files.insert(index5);
      BOOST_REQUIRE(!sd.find_or_create_trace_slice(6, open_state::read, file));
      const auto trace6 = file.get_file_path().filename();
      files.insert(trace6);
      verify_directory_contents(tempdir.path(), files);

      // verify that the current_slice and the previous are maintained as long as lib - min_saved_blocks is part of previous slice
      uint32_t current_slice = 6;
      uint32_t lib = current_slice * width;
      sd.run_maintenance_tasks(lib, {});
      std::set<bfs::path> files2;
      files2.insert(index5);
      files2.insert(trace6);
      verify_directory_contents(tempdir.path(), files2);

      // saved blocks still in previous slice
      lib += min_saved_blocks - 1;  // current_slice * width + min_saved_blocks - 1
      sd.run_maintenance_tasks(lib, {});
      verify_directory_contents(tempdir.path(), files2);

      // now all saved blocks in current slice
      lib += 1; // current_slice * width + min_saved_blocks
      sd.run_maintenance_tasks(lib, {});
      std::set<bfs::path> files3;
      files3.insert(trace6);
      verify_directory_contents(tempdir.path(), files3);

      // moving lib into next slice, so 1 saved blocks still in 6th slice
      lib += width - 1;
      sd.run_maintenance_tasks(lib, {});
      verify_directory_contents(tempdir.path(), files3);

      // moved last saved block out of 6th slice, so 6th slice is cleaned up
      lib += 1;
      sd.run_maintenance_tasks(lib, {});
      verify_directory_contents(tempdir.path(), std::set<bfs::path>());
   }

   BOOST_FIXTURE_TEST_CASE(slice_dir_compress, test_fixture)
   {
      fc::temp_directory tempdir;
      const uint32_t width = 10;
      const uint32_t min_uncompressed_blocks = 5;
      slice_directory sd(tempdir.path(), width, std::optional<uint32_t>(), std::optional<uint32_t>(min_uncompressed_blocks), 8);
      fc::cfile file;

      using file_vector_t = std::vector<std::tuple<bfs::path, bfs::path, bfs::path>>;
      file_vector_t file_paths;
      for (int i = 0; i < 7 ; i++) {
         BOOST_REQUIRE(!sd.find_or_create_index_slice(i, open_state::read, file));
         auto index_name = file.get_file_path().filename();
         BOOST_REQUIRE(create_non_empty_trace_slice(sd, i, file));
         auto trace_name = file.get_file_path().filename();
         auto compressed_trace_name = trace_name;
         compressed_trace_name.replace_extension(".clog");
         file_paths.emplace_back(index_name, trace_name, compressed_trace_name);
      }

      // initial set is only indices and uncompressed traces
      std::set<bfs::path> files;
      for (const auto& e: file_paths) {
         files.insert(std::get<0>(e));
         files.insert(std::get<1>(e));
      }
      verify_directory_contents(tempdir.path(), files);

      // verify no change up to the last block before a slice becomes compressible
      sd.run_maintenance_tasks(14, {});
      verify_directory_contents(tempdir.path(), files);

      for (int reps = 0; reps < file_paths.size(); reps++) {
         //  leading edge,
         //  compresses one slice
         files.erase(std::get<1>(file_paths.at(reps)));
         files.insert(std::get<2>(file_paths.at(reps)));

         sd.run_maintenance_tasks(15 + (reps * width), {});
         verify_directory_contents(tempdir.path(), files);

         // trailing edge, no change
         sd.run_maintenance_tasks(24 + (reps * width), {});
         verify_directory_contents(tempdir.path(), files);
      }

      // make sure the test is correct and and no uncompressed files remain
      for (const auto& e: file_paths) {
         BOOST_REQUIRE_EQUAL(files.count(std::get<0>(e)), 1);
         BOOST_REQUIRE_EQUAL(files.count(std::get<1>(e)), 0);
         BOOST_REQUIRE_EQUAL(files.count(std::get<2>(e)), 1);
      }
   }

   BOOST_FIXTURE_TEST_CASE(slice_dir_compress_and_delete, test_fixture)
   {
      fc::temp_directory tempdir;
      const uint32_t width = 10;
      const uint32_t min_uncompressed_blocks = 5;
      const uint32_t min_saved_blocks = min_uncompressed_blocks + width;
      slice_directory sd(tempdir.path(), width, std::optional<uint32_t>(min_saved_blocks), std::optional<uint32_t>(min_uncompressed_blocks), 8);
      fc::cfile file;

      using file_vector_t = std::vector<std::tuple<bfs::path, bfs::path, bfs::path>>;
      file_vector_t file_paths;
      for (int i = 0; i < 7 ; i++) {
         BOOST_REQUIRE(!sd.find_or_create_index_slice(i, open_state::read, file));
         auto index_name = file.get_file_path().filename();
         BOOST_REQUIRE(create_non_empty_trace_slice(sd, i, file));
         auto trace_name = file.get_file_path().filename();
         auto compressed_trace_name = trace_name;
         compressed_trace_name.replace_extension(".clog");
         file_paths.emplace_back(index_name, trace_name, compressed_trace_name);
      }

      // initial set is only indices and uncompressed traces
      std::set<bfs::path> files;
      for (const auto& e: file_paths) {
         files.insert(std::get<0>(e));
         files.insert(std::get<1>(e));
      }
      verify_directory_contents(tempdir.path(), files);

      // verify no change up to the last block before a slice becomes compressible
      sd.run_maintenance_tasks(14, {});
      verify_directory_contents(tempdir.path(), files);

      for (int reps = 0; reps < file_paths.size() + 1; reps++) {
         //  leading edge,
         //  compresses one slice IF its not past the end of our test,
         if (reps < file_paths.size()) {
            files.erase(std::get<1>(file_paths.at(reps)));
            files.insert(std::get<2>(file_paths.at(reps)));
         }

         // removes one IF its not the first
         if (reps > 0) {
            files.erase(std::get<0>(file_paths.at(reps-1)));
            files.erase(std::get<2>(file_paths.at(reps-1)));
         }
         sd.run_maintenance_tasks(15 + (reps * width), {});
         verify_directory_contents(tempdir.path(), files);

         // trailing edge, no change
         sd.run_maintenance_tasks(24 + (reps * width), {});
         verify_directory_contents(tempdir.path(), files);
      }

      // make sure the test is correct and ran through the permutations
      BOOST_REQUIRE_EQUAL(files.size(), 0);
   }

   BOOST_FIXTURE_TEST_CASE(store_provider_write_read, test_fixture)
   {
      fc::temp_directory tempdir;
      test_store_provider sp(tempdir.path(), 100);
      sp.append(bt);
      sp.append_lib(54);
      sp.append(bt2);
      const uint32_t bt_bn = bt.number;
      bool found_block = false;
      bool lib_seen = false;
      const uint64_t first_offset = sp.scan_metadata_log_from(9, 0, [&](const metadata_log_entry& e) -> bool {
         if (e.contains<block_entry_v0>()) {
            const auto& block = e.get<block_entry_v0>();
            if (block.number == bt_bn) {
               BOOST_REQUIRE(!found_block);
               found_block = true;
            }
         } else if (e.contains<lib_entry_v0>()) {
            auto best_lib = e.get<lib_entry_v0>();
            BOOST_REQUIRE(!lib_seen);
            BOOST_REQUIRE_EQUAL(best_lib.lib, 54);
            lib_seen = true;
            return false;
         }
         return true;
      }, []() {});
      BOOST_REQUIRE(found_block);
      BOOST_REQUIRE(lib_seen);

      std::vector<uint32_t> block_nums;
      std::vector<uint64_t> block_offsets;
      lib_seen = false;
      uint64_t offset = sp.scan_metadata_log_from(9, 0, [&](const metadata_log_entry& e) -> bool {
         if (e.contains<block_entry_v0>()) {
            const auto& block = e.get<block_entry_v0>();
            block_nums.push_back(block.number);
            block_offsets.push_back(block.offset);
         } else if (e.contains<lib_entry_v0>()) {
            auto best_lib = e.get<lib_entry_v0>();
            BOOST_REQUIRE(!lib_seen);
            BOOST_REQUIRE_EQUAL(best_lib.lib, 54);
            lib_seen = true;
         }
         return true;
      }, []() {});
      BOOST_REQUIRE(lib_seen);
      BOOST_REQUIRE_EQUAL(block_nums.size(), 2);
      BOOST_REQUIRE_EQUAL(block_nums[0], bt.number);
      BOOST_REQUIRE_EQUAL(block_nums[1], bt2.number);
      BOOST_REQUIRE_EQUAL(block_offsets.size(), 2);
      BOOST_REQUIRE(block_offsets[0] < block_offsets[1]);
      BOOST_REQUIRE(first_offset < offset);

      std::optional<data_log_entry> bt_data = sp.read_data_log(block_nums[0], block_offsets[0]);
      BOOST_REQUIRE(bt_data);
      BOOST_REQUIRE_EQUAL(*bt_data, bt);

      bt_data = sp.read_data_log(block_nums[1], block_offsets[1]);
      BOOST_REQUIRE(bt_data);
      BOOST_REQUIRE_EQUAL(*bt_data, bt2);

      block_nums.clear();
      block_offsets.clear();
      lib_seen = false;
      int counter = 0;
      try {
         offset = sp.scan_metadata_log_from(9, 0, [&](const metadata_log_entry& e) -> bool {
            if (e.contains<block_entry_v0>()) {
               const auto& block = e.get<block_entry_v0>();
               block_nums.push_back(block.number);
               block_offsets.push_back(block.offset);
            } else if (e.contains<lib_entry_v0>()) {
               auto best_lib = e.get<lib_entry_v0>();
               BOOST_REQUIRE(!lib_seen);
               BOOST_REQUIRE_EQUAL(best_lib.lib, 54);
               lib_seen = true;
            }
            return true;
         }, [&counter]() {
            if( ++counter == 3 ) {
               throw yield_exception("");
            }
         });
         BOOST_FAIL("Should not have completed scan");
      } catch (const yield_exception& ex) {
      }
      BOOST_REQUIRE(lib_seen);
      BOOST_REQUIRE_EQUAL(block_nums.size(), 1);
      BOOST_REQUIRE_EQUAL(block_nums[0], bt.number);
      BOOST_REQUIRE_EQUAL(block_offsets.size(), 1);
      BOOST_REQUIRE(first_offset < offset);
   }

   BOOST_FIXTURE_TEST_CASE(test_get_block, test_fixture)
   {
      fc::temp_directory tempdir;
      store_provider sp(tempdir.path(), 100, std::optional<uint32_t>(), std::optional<uint32_t>(), 0);
      sp.append(bt);
      sp.append_lib(1);
      sp.append(bt2);
      int count = 0;
      get_block_t block1 = sp.get_block(1, [&count]() {
         if (++count >= 3) {
            throw yield_exception("");
         }
      });
      BOOST_REQUIRE(block1);
      BOOST_REQUIRE(std::get<1>(*block1));
      const auto block1_bt = std::get<0>(*block1);
      BOOST_REQUIRE_EQUAL(block1_bt, bt);

      count = 0;
      get_block_t block2 = sp.get_block(5, [&count]() {
         if (++count >= 4) {
            throw yield_exception("");
         }
      });
      BOOST_REQUIRE(block2);
      BOOST_REQUIRE(!std::get<1>(*block2));
      const auto block2_bt = std::get<0>(*block2);
      BOOST_REQUIRE_EQUAL(block2_bt, bt2);

      count = 0;
      try {
         sp.get_block(5,[&count]() {
            if (++count >= 3) {
               throw yield_exception("");
            }
         });
         BOOST_FAIL("Should not have completed scan");
      } catch (const yield_exception& ex) {
      }

      count = 0;
      block2 = sp.get_block(2,[&count]() {
         if (++count >= 4) {
            throw yield_exception("");
         }
      });
      BOOST_REQUIRE(!block2);
   }

BOOST_AUTO_TEST_SUITE_END()
