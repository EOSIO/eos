#pragma once

#include <system_error>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/io_service.hpp>

namespace chainbase {

namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

enum db_error_code {
   ok = 0,
   dirty,
   incompatible,
   incorrect_db_version,
   not_found,
   bad_size,
   unsupported_win32_mode,
   bad_header,
   no_access,
   aborted,
   no_mlock
};

const std::error_category& chainbase_error_category();

inline std::error_code make_error_code(db_error_code e) noexcept {
   return std::error_code(static_cast<int>(e), chainbase_error_category());
}

class chainbase_error_category : public std::error_category {
public:
   const char* name() const noexcept override;
   std::string message(int ev) const override;
};

class pinnable_mapped_file {
   public:
      typedef typename bip::managed_mapped_file::segment_manager segment_manager;

      enum map_mode {
         mapped,
         heap,
         locked
      };

      pinnable_mapped_file(const bfs::path& dir, bool writable, uint64_t shared_file_size, bool allow_dirty, map_mode mode);
      pinnable_mapped_file(pinnable_mapped_file&& o);
      pinnable_mapped_file& operator=(pinnable_mapped_file&&);
      pinnable_mapped_file(const pinnable_mapped_file&) = delete;
      pinnable_mapped_file& operator=(const pinnable_mapped_file&) = delete;
      ~pinnable_mapped_file();

      segment_manager* get_segment_manager() const { return _segment_manager;}

   private:
      void                                          set_mapped_file_db_dirty(bool);
      void                                          load_database_file(boost::asio::io_service& sig_ios);
      void                                          save_database_file();
      bool                                          all_zeros(char* data, size_t sz);
      void                                          setup_non_file_mapping();

      bip::file_lock                                _mapped_file_lock;
      bfs::path                                     _data_file_path;
      std::string                                   _database_name;
      bool                                          _writable;

      bip::file_mapping                             _file_mapping;
      bip::mapped_region                            _file_mapped_region;
      void*                                         _non_file_mapped_mapping = nullptr;
      size_t                                        _non_file_mapped_mapping_size = 0;

#ifdef _WIN32
      bip::permissions                              _db_permissions;
#else
      bip::permissions                              _db_permissions{S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH};
#endif

      segment_manager*                              _segment_manager = nullptr;

      constexpr static unsigned                     _db_size_multiple_requirement = 1024*1024; //1MB
};

std::istream& operator>>(std::istream& in, pinnable_mapped_file::map_mode& runtime);
std::ostream& operator<<(std::ostream& osm, pinnable_mapped_file::map_mode m);

}
