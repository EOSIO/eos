#include <eosio/trace_api/store_provider.hpp>

#include <fc/variant_object.hpp>

namespace {
      static constexpr uint32_t _current_version = 1;
      static constexpr const char* _trace_prefix = "trace_";
      static constexpr const char* _trace_index_prefix = "trace_index_";
      static constexpr const char* _trace_ext = ".log";
      static constexpr uint _max_filename_size = std::char_traits<char>::length(_trace_index_prefix) + 10 + 1 + 10 + std::char_traits<char>::length(_trace_ext) + 1; // "trace_index_" + 10-digits + '-' + 10-digits + ".log" + null-char
}

namespace eosio::trace_api {
   namespace bfs = boost::filesystem;
   store_provider::store_provider(const bfs::path& slice_dir, uint32_t stride_width, std::optional<uint32_t> minimum_irreversible_history_blocks)
   : _slice_directory(slice_dir, stride_width, minimum_irreversible_history_blocks) {
   }

   void store_provider::append(const block_trace_v0& bt) {
      fc::cfile trace;
      fc::cfile index;
      const uint32_t slice_number = _slice_directory.slice_number(bt.number);
      _slice_directory.find_or_create_slice_pair(slice_number, open_state::write, trace, index);
      // storing as static_variant to allow adding other data types to the trace file in the future
      const uint64_t offset = append_store(data_log_entry { bt }, trace);

      auto be = metadata_log_entry { block_entry_v0 { .id = bt.id, .number = bt.number, .offset = offset }};
      append_store(be, index);
   }

   void store_provider::append_lib(uint32_t lib) {
      fc::cfile index;
      const uint32_t slice_number = _slice_directory.slice_number(lib);
      _slice_directory.find_or_create_index_slice(slice_number, open_state::write, index);
      auto le = metadata_log_entry { lib_entry_v0 { .lib = lib }};
      append_store(le, index);
      _slice_directory.cleanup_old_slices(lib);
   }

   get_block_t store_provider::get_block(uint32_t block_height, const yield_function& yield) {
      std::optional<uint64_t> trace_offset;
      bool irreversible = false;
      uint64_t offset = scan_metadata_log_from(block_height, 0, [&block_height, &trace_offset, &irreversible](const metadata_log_entry& e) -> bool {
         if (e.contains<block_entry_v0>()) {
            const auto& block = e.get<block_entry_v0>();
            if (block.number == block_height) {
               trace_offset = block.offset;
            }
         } else if (e.contains<lib_entry_v0>()) {
            auto lib = e.get<lib_entry_v0>().lib;
            if (lib >= block_height) {
               irreversible = true;
               return false;
            }
         }
         return true;
      }, yield);
      if (!trace_offset) {
         return get_block_t{};
      }
      std::optional<data_log_entry> entry = read_data_log(block_height, *trace_offset);
      if (!entry) {
         return get_block_t{};
      }
      const auto bt = entry->get<block_trace_v0>();
      return std::make_tuple( bt, irreversible );
   }

   slice_directory::slice_directory(const bfs::path& slice_dir, uint32_t width, std::optional<uint32_t> minimum_irreversible_history_blocks)
   : _slice_dir(slice_dir)
   , _width(width)
   , _minimum_irreversible_history_blocks(minimum_irreversible_history_blocks) {
      if (!exists(_slice_dir)) {
         bfs::create_directories(slice_dir);
      }
   }

   bool slice_directory::find_or_create_index_slice(uint32_t slice_number, open_state state, fc::cfile& index_file) const {
      const bool found = find_index_slice(slice_number, state, index_file);
      if( !found ) {
         create_new_index_slice_file(index_file);
      }
      return found;
   }

   bool slice_directory::find_index_slice(uint32_t slice_number, open_state state, fc::cfile& index_file, bool open_file) const {
      const bool found = find_slice(_trace_index_prefix, slice_number, index_file, open_file);
      if( !found || !open_file ) {
         return found;
      }

      validate_existing_index_slice_file(index_file, state);
      return true;
   }

   void slice_directory::create_new_index_slice_file(fc::cfile& index_file) const {
      index_file.open(fc::cfile::create_or_update_rw_mode);
      index_header h { .version = _current_version };
      append_store(h, index_file);
   }

   void slice_directory::validate_existing_index_slice_file(fc::cfile& index_file, open_state state) const {
      const auto header = extract_store<index_header>(index_file);
      if (header.version != _current_version) {
         throw old_slice_version("Old slice file with version: " + std::to_string(header.version) +
                                 " is in directory, only supporting version: " + std::to_string(_current_version));
      }

      if( state == open_state::write ) {
         index_file.seek_end(0);
      }
   }

   bool slice_directory::find_or_create_trace_slice(uint32_t slice_number, open_state state, fc::cfile& trace_file) const {
      const bool found = find_trace_slice(slice_number, state, trace_file);

      if( !found ) {
         trace_file.open(fc::cfile::create_or_update_rw_mode);
      }

      return found;
   }

   bool slice_directory::find_trace_slice(uint32_t slice_number, open_state state, fc::cfile& trace_file, bool open_file) const {
      const bool found = find_slice(_trace_prefix, slice_number, trace_file, open_file);

      if( !found || !open_file ) {
         return found;
      }

      if( state == open_state::write ) {
         trace_file.seek_end(0);
      }
      else {
         trace_file.seek(0); // ensure we are at the start of the file
      }
      return true;
   }

   bool slice_directory::find_slice(const char* slice_prefix, uint32_t slice_number, fc::cfile& slice_file, bool open_file) const {
      char filename[_max_filename_size] = {};
      const uint32_t slice_start = slice_number * _width;
      const int size_written = snprintf(filename, _max_filename_size, "%s%010d-%010d%s", slice_prefix, slice_start, (slice_start + _width), _trace_ext);
      // assert that _max_filename_size is correct
      if ( size_written >= _max_filename_size ) {
         const std::string max_size_str = std::to_string(_max_filename_size - 1); // dropping null character from size
         const std::string size_written_str = std::to_string(size_written);
         throw std::runtime_error("Could not write the complete filename.  Anticipated the max filename characters to be: " +
            max_size_str + " or less, but wrote: " + size_written_str + " characters.  This is likely because the file "
            "format was changed and the code was not updated accordingly. Filename created: " + filename);
      }
      const path slice_path = _slice_dir / filename;
      slice_file.set_file_path(slice_path);

      const bool file_exists = exists(slice_path);
      if( !file_exists || !open_file ) {
         return file_exists;
      }

      slice_file.open(fc::cfile::create_or_update_rw_mode);
      // TODO: this is a temporary fix until fc::cfile handles it internally.  OSX and Linux differ on the read offset
      // when opening in "ab+" mode
      slice_file.seek(0);
      return true;
   }


   void slice_directory::find_or_create_slice_pair(uint32_t slice_number, open_state state, fc::cfile& trace, fc::cfile& index) {
      const bool trace_found = find_or_create_trace_slice(slice_number, state, trace);
      const bool index_found = find_or_create_index_slice(slice_number, state, index);
      if (trace_found != index_found) {
         const std::string trace_status = trace_found ? "existing" : "new";
         const std::string index_status = index_found ? "existing" : "new";
         elog("Trace file is ${ts}, but it's metadata file is ${is}. This means the files are not consistent.", ("ts", trace_status)("is", index_status));
      }
   }

   void slice_directory::cleanup_old_slices(uint32_t lib) {
      if (!_minimum_irreversible_history_blocks)
         return;
      const uint32_t lib_slice_number = slice_number( lib );
      if (lib_slice_number < 1 || (_last_cleaned_up_slice && _last_cleaned_up_slice >= lib_slice_number - 1))
         return;

      // can only cleanup a slice once our last needed history block (lib - *_minimum_irreversible_history_blocks)
      // is out of that slice (... - width)
      const int64_t cleanup_block_number = static_cast<int64_t>(lib) - static_cast<int64_t>(*_minimum_irreversible_history_blocks) - _width;
      if (cleanup_block_number > 0) {
         uint32_t cleanup_slice_num = slice_number(static_cast<uint32_t>(cleanup_block_number));
         // since we subtracted width, we are guaranteed cleanup_slice_num is not the slice that contains LIB
         while (!_last_cleaned_up_slice || *_last_cleaned_up_slice < cleanup_slice_num) {
            fc::cfile trace;
            fc::cfile index;
            const uint32_t slice_to_clean = _last_cleaned_up_slice ? *_last_cleaned_up_slice + 1 : 0;
            // cleanup index first to reduce the likelihood of reader finding index, but not finding trace
            const bool dont_open_file = false;
            const bool index_found = find_index_slice(slice_to_clean, open_state::read, index, dont_open_file);
            if (index_found) {
               bfs::remove(index.get_file_path());
            }
            const bool trace_found = find_trace_slice(slice_to_clean, open_state::read, trace, dont_open_file);
            if (trace_found) {
               bfs::remove(trace.get_file_path());
            }
            _last_cleaned_up_slice = slice_to_clean;
         }
      }
   }
}
