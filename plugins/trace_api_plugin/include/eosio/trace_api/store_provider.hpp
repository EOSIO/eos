#pragma once

#include <ios>
#include <fc/io/cfile.hpp>
#include <boost/filesystem.hpp>
#include <fc/variant.hpp>
#include <eosio/trace_api/common.hpp>
#include <eosio/trace_api/metadata_log.hpp>
#include <eosio/trace_api/data_log.hpp>

namespace eosio::trace_api {
   using namespace boost::filesystem;

   class path_does_not_exist : public std::runtime_error {
   public:
      explicit path_does_not_exist(const char* what_arg)
         :std::runtime_error(what_arg)
      {}
      explicit path_does_not_exist(const std::string& what_arg)
         :std::runtime_error(what_arg)
      {}
   };

   class old_slice_version : public std::runtime_error {
   public:
      explicit old_slice_version(const char* what_arg)
         :std::runtime_error(what_arg)
      {}
      explicit old_slice_version(const std::string& what_arg)
         :std::runtime_error(what_arg)
      {}
   };

   class incompatible_slice_files : public std::runtime_error {
   public:
      explicit incompatible_slice_files(const char* what_arg)
         :std::runtime_error(what_arg)
      {}
      explicit incompatible_slice_files(const std::string& what_arg)
         :std::runtime_error(what_arg)
      {}
   };

   class malformed_slice_file : public std::runtime_error {
   public:
      explicit malformed_slice_file(const char* what_arg)
         :std::runtime_error(what_arg)
      {}
      explicit malformed_slice_file(const std::string& what_arg)
         :std::runtime_error(what_arg)
      {}
   };

   /**
    * append an entry to the store
    *
    * @param entry : the entry to append
    * @param file : the file to append entry to
    * @return the offset in the file where that entry is written
    */
   template<typename DataEntry, typename File>
   static uint64_t append_store(const DataEntry &entry, File &file) {
      auto data = fc::raw::pack(entry);
      const auto offset = file.tellp();
      file.write(data.data(), data.size());
      file.flush();
      file.sync();
      return offset;
   }

   /**
    * extract an entry from the data log
    *
    * @param file : the file to extract entry from
    * @return the extracted data log
    */
   template<typename DataEntry, typename File>
   static DataEntry extract_store( File& file ) {
      DataEntry entry;
      auto ds = file.create_datastream();
      fc::raw::unpack(ds, entry);
      return entry;
   }


   class store_provider;

   /**
    * Provides access to the slice directory.  It is only intended to be used by store_provider
    * and unit tests.
    */
   class slice_directory {
   public:
      struct index_header {
         uint32_t version;
      };
      slice_directory(const boost::filesystem::path& slice_dir, uint32_t width);

      /**
       * Return the slice number that would include the passed in block_height
       *
       * @param block_height : height of the requested data
       * @return the slice number for the block_height
       */
      uint32_t slice_number(uint32_t block_height) const {
         return block_height / _width;
      }

      /**
       * Find or create the index file associated with the indicated slice_number
       *
       * @param slice_number : slice number of the requested slice file
       * @param append : indicate if the file is going to be appended (or read)
       * @param index_file : the cfile that will be set to the appropriate slice filename
       *                     and opened to that file
       * @return the true if file was found (i.e. already existed)
       */
      bool find_or_create_index_slice(uint32_t slice_number, bool append, fc::cfile& index_file) const;

      /**
       * Find the index file associated with the indicated slice_number
       *
       * @param slice_number : slice number of the requested slice file
       * @param append : indicate if the file is going to be appended (or read)
       * @param index_file : the cfile that will be set to the appropriate slice filename (always)
       *                     and opened to that file (if it was found)
       * @return the true if file was found (i.e. already existed), if not found index_file
       *         is set to the appropriate file, but not open
       */
      bool find_index_slice(uint32_t slice_number, bool append, fc::cfile& index_file) const;

      /**
       * Find or create the trace file associated with the indicated slice_number
       *
       * @param slice_number : slice number of the requested slice file
       * @param append : indicate if the file is going to be appended (or read)
       * @param trace_file : the cfile that will be set to the appropriate slice filename
       *                     and opened to that file
       * @return the true if file was found (i.e. already existed)
       */
      bool find_or_create_trace_slice(uint32_t slice_number, bool append, fc::cfile& trace_file) const;

      /**
       * Find the trace file associated with the indicated slice_number
       *
       * @param slice_number : slice number of the requested slice file
       * @param append : indicate if the file is going to be appended (or read)
       * @param trace_file : the cfile that will be set to the appropriate slice filename (always)
       *                     and opened to that file (if it was found)
       * @return the true if file was found (i.e. already existed), if not found index_file
       *         is set to the appropriate file, but not open
       */
      bool find_trace_slice(uint32_t slice_number, bool append, fc::cfile& trace_file) const;

   private:
      // returns true if slice is found, slice_file will always be set to the appropriate path for
      // the slice_prefix and slice_number, but will only be opened if found
      bool find_slice(const char* slice_prefix, uint32_t slice_number, fc::cfile& slice_file) const;

      // take an index file that is initialized to a file and open it and write its header
      void create_new_index_slice_file(fc::cfile& index_file) const;

      // take an open index slice file and verify its header is valid and prepare the file to be appended to (or read from)
      void validate_existing_index_slice_file(fc::cfile& index_file, bool append) const;

      const boost::filesystem::path _slice_dir;
      const uint32_t _width;
   };

   /**
    * Provides read and write access to block trace data.
    */
   class store_provider {
   public:
      store_provider(const boost::filesystem::path& slice_dir, uint32_t width);

      void append(const block_trace_v0& bt);
      void append_lib(uint32_t lib);

      /**
       * Read the trace for a given block
       * @param block_height : the height of the data being read
       * @return empty optional if the data cannot be read OTHERWISE
       *         optional containing a 2-tuple of the block_trace and a flag indicating irreversibility
       */
      get_block_t get_block(uint32_t block_height, const yield_function& yield= {});

   protected:
      /**
       * Read the metadata log font-to-back starting at an offset passing each entry to a provided functor/lambda
       *
       * @tparam Fn : type of the functor/lambda
       * @param block_height : height of the requested data
       * @param offset : initial offset to read from
       * @param fn : the functor/lambda
       * @return the highest offset read during this scan
       */
      template<typename Fn>
      uint64_t scan_metadata_log_from( uint32_t block_height, uint64_t offset, Fn&& fn, const yield_function& yield ) {
         // ignoring offset
         offset = 0;
         fc::cfile index;
         const uint32_t slice_number = _slice_directory.slice_number(block_height);
         const bool found = _slice_directory.find_index_slice(slice_number, false, index);
         if( !found ) {
            return 0;
         }
         const uint64_t end = file_size(index.get_file_path());
         offset = index.tellp();
         uint64_t last_read_offset = offset;
         while (offset < end) {
            yield();
            const auto metadata = extract_store<metadata_log_entry>(index);
            if(! fn(metadata)) {
               break;
            }
            last_read_offset = offset;
            offset = index.tellp();
         }
         return last_read_offset;
      }

      /**
       * Read from the data log
       * @param block_height : the block_height of the data being read
       * @param offset : the offset in the datalog to read
       * @return empty optional if the data log does not exist, data otherwise
       * @throws std::exception : when the data is not the correct type or if the log is corrupt in some way
       *
       */
      std::optional<data_log_entry> read_data_log( uint32_t block_height, uint64_t offset ) {
         const uint32_t slice_number = _slice_directory.slice_number(block_height);
         fc::cfile trace;
         if( !_slice_directory.find_trace_slice(slice_number, false, trace) ) {
            const std::string offset_str = boost::lexical_cast<std::string>(offset);
            const std::string bh_str = boost::lexical_cast<std::string>(block_height);
            throw malformed_slice_file("Requested offset: " + offset_str + " to retrieve block number: " + bh_str + " but this trace file is new, so there are no traces present.");
         }
         const uint64_t end = file_size(trace.get_file_path());
         if( offset >= end ) {
            const std::string offset_str = boost::lexical_cast<std::string>(offset);
            const std::string bh_str = boost::lexical_cast<std::string>(block_height);
            const std::string end_str = boost::lexical_cast<std::string>(end);
            throw malformed_slice_file("Requested offset: " + offset_str + " to retrieve block number: " + bh_str + " but this trace file only goes to offset: " + end_str);
         }
         trace.seek(offset);
         return extract_store<data_log_entry>(trace);
      }

   private:
      void find_or_create_slice_pair(uint32_t block_height, bool append, fc::cfile& trace, fc::cfile& index);
      void initialize_new_index_slice_file(fc::cfile& index);
      void validate_existing_index_slice_file(fc::cfile& index, bool append);
      slice_directory _slice_directory;
   };

}

FC_REFLECT(eosio::trace_api::slice_directory::index_header, (version))
