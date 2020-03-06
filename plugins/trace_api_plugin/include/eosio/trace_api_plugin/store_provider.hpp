#pragma once

#include <ios>
#include <fc/io/cfile.hpp>
#include <boost/filesystem.hpp>
#include <fc/variant.hpp>
#include <eosio/trace_api_plugin/metadata_log.hpp>
#include <eosio/trace_api_plugin/data_log.hpp>

namespace eosio::trace_api_plugin {
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

   class store_provider;


   class slice_provider {
   public:
      struct index_header {
         uint32_t version;
      };
      slice_provider(const boost::filesystem::path& slice_dir, uint32_t width);

      uint32_t slice_number(uint32_t block_height) const {
         return block_height / _width;
      }

      bool find_index_slice(uint32_t slice_number, bool append, fc::cfile& slice_file) const;
      bool find_trace_slice(uint32_t slice_number, bool append, fc::cfile& slice_file) const;

   private:
      // returns true if slice is created
      bool find_slice(const char* slice_prefix, uint32_t slice_number, fc::cfile& slice_file) const;

      const boost::filesystem::path _slice_dir;
      const uint32_t _width;

      static constexpr uint32_t _current_version = 1;

      static constexpr uint _slice_size = 10;
      static constexpr uint _null_term_size = 1;
      static constexpr const char* _trace_prefix = "trace_";
      static constexpr const char* _trace_index_prefix = "trace_index_";
      static constexpr uint _max_file_size = 12 + 10 + 1 + 10 + 4 + 1; // "trace_index_" + 10-digits + '-' + 10-digits + ".log" + null-char
   };

   class store_provider {
   public:
      store_provider(const boost::filesystem::path& slice_dir, uint32_t width);

      void append(const block_trace_v0& bt);
      void append_lib(uint32_t lib);

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
      uint64_t scan_metadata_log_from( uint32_t block_height, uint64_t offset, Fn&& fn ) {
         // ignoring offset
         offset = 0;
         fc::cfile index;
         const uint32_t slice_number = _slice_provider.slice_number(block_height);
         _slice_provider.find_index_slice(slice_number, false, index);
         const uint64_t end = file_size(index.get_file_path());
         offset = index.tellp();
         uint64_t last_read_offset = offset;
         bool cont = true;
         while (offset < end && cont) {
            const auto metadata = extract_store<metadata_log_entry>(index);
            cont = fn(metadata);
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
         const uint32_t slice_number = _slice_provider.slice_number(block_height);
         fc::cfile trace;
         if (_slice_provider.find_trace_slice(slice_number, false, trace)) {
            const std::string offset_str = boost::lexical_cast<std::string>(offset);
            const std::string bh_str = boost::lexical_cast<std::string>(block_height);
            throw malformed_slice_file("Requested offset: " + offset_str + " to retrieve block number: " + bh_str + " but this trace file is new, so there are no traces present.");
         }
         const uint64_t end = file_size(trace.get_file_path());
         if (offset >= end) {
            const std::string offset_str = boost::lexical_cast<std::string>(offset);
            const std::string bh_str = boost::lexical_cast<std::string>(block_height);
            const std::string end_str = boost::lexical_cast<std::string>(end);
            throw malformed_slice_file("Requested offset: " + offset_str + " to retrieve block number: " + bh_str + " but this trace file only goes to offset: " + end_str);
         }
         trace.seek(offset);
         return extract_store<data_log_entry>(trace);
      }

      /**
       * append an entry to the store
       *
       * @param entry : the entry to append
       * @param slice : the slice to append entry to
       * @return the offset in the slice where that entry is written
       */
      template<typename DataEntry, typename Slice>
      static uint64_t append_store(const DataEntry &entry, Slice &slice) {
         auto data = fc::raw::pack(entry);
         const auto offset = slice.tellp();
         slice.write(data.data(), data.size());
         slice.flush();
         slice.sync();
         return offset;
      }

      /**
       * extract an entry from the data log
       *
       * @param slice : the slice to extract entry from
       * @return the extracted data log
       */
      template<typename DataEntry, typename Slice>
      static DataEntry extract_store( Slice& slice ) {
         DataEntry entry;
         auto ds = slice.create_datastream();
         fc::raw::unpack(ds, entry);
         return entry;
      }
   private:
      void find_slice_pair(uint32_t block_height, bool append, fc::cfile& trace, fc::cfile& index);
      slice_provider _slice_provider;
   };

}

FC_REFLECT(eosio::trace_api_plugin::slice_provider::index_header, (version))
