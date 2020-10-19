#include <chainbase/pinnable_mapped_file.hpp>
#include <chainbase/environment.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>

#ifdef __linux__
#include <linux/mman.h>
#endif

namespace chainbase {

const char* chainbase_error_category::name() const noexcept {
   return "chainbase";
}

std::string chainbase_error_category::message(int ev) const {
   switch(ev) {
      case db_error_code::ok: 
         return "Ok";
      case db_error_code::dirty:
         return "Database dirty flag set";
      case db_error_code::incompatible:
         return "Database incompatible; All environment parameters must match";
      case db_error_code::incorrect_db_version:
         return "Database format not compatible with this version of chainbase";
      case db_error_code::not_found:
         return "Database file not found";
      case db_error_code::bad_size:
         return "Bad size";
      case db_error_code::unsupported_win32_mode:
         return "Heap and locked mode are not supported on win32";
      case db_error_code::bad_header:
         return "Failed to read DB header";
      case db_error_code::no_access:
         return "Could not gain write access to the shared memory file";
      case db_error_code::aborted:
         return "Database load aborted";
      case db_error_code::no_mlock:
         return "Failed to mlock database";
      default:
         return "Unrecognized error code";
   }
}

const std::error_category& chainbase_error_category() {
   static class chainbase_error_category the_category;
   return the_category;
}

pinnable_mapped_file::pinnable_mapped_file(const bfs::path& dir, bool writable, uint64_t shared_file_size, bool allow_dirty, map_mode mode) :
   _data_file_path(bfs::absolute(dir/"shared_memory.bin")),
   _database_name(dir.filename().string()),
   _writable(writable)
{
   if(shared_file_size % _db_size_multiple_requirement) {
      std::string what_str("Database must be mulitple of " + std::to_string(_db_size_multiple_requirement) + " bytes");
      BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::bad_size), what_str));
   }
#ifdef _WIN32
   if(mode != mapped)
      BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::unsupported_win32_mode)));
#endif
   if(!_writable && !bfs::exists(_data_file_path)){
      std::string what_str("database file not found at " + _data_file_path.string());
      BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::not_found), what_str));
   }

   bfs::create_directories(dir);

   if(bfs::exists(_data_file_path)) {
      char header[header_size];
      std::ifstream hs(_data_file_path.generic_string(), std::ifstream::binary);
      hs.read(header, header_size);
      if(hs.fail())
         BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::bad_header)));

      db_header* dbheader = reinterpret_cast<db_header*>(header);
      if(dbheader->id != header_id) {
         std::string what_str("\"" + _database_name + "\" database format not compatible with this version of chainbase.");
         BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::incorrect_db_version), what_str));
      }
      if(!allow_dirty && dbheader->dirty) {
         std::string what_str("\"" + _database_name + "\" database dirty flag set");
         BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::dirty)));
      }
      if(dbheader->dbenviron != environment()) {
         std::cerr << "CHAINBASE: \"" << _database_name << "\" database was created with a chainbase from a different environment" << std::endl;
         std::cerr << "Current compiler environment:" << std::endl;
         std::cerr << environment();
         std::cerr << "DB created with compiler environment:" << std::endl;
         std::cerr << dbheader->dbenviron;
         BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::incompatible)));
      }
   }

   segment_manager* file_mapped_segment_manager = nullptr;
   if(!bfs::exists(_data_file_path)) {
      std::ofstream ofs(_data_file_path.generic_string(), std::ofstream::trunc);
      //win32 impl of bfs::resize_file() doesn't like the file being open
      ofs.close();
      bfs::resize_file(_data_file_path, shared_file_size);
      _file_mapping = bip::file_mapping(_data_file_path.generic_string().c_str(), bip::read_write);
      _file_mapped_region = bip::mapped_region(_file_mapping, bip::read_write);
      file_mapped_segment_manager = new ((char*)_file_mapped_region.get_address()+header_size) segment_manager(shared_file_size-header_size);
      new (_file_mapped_region.get_address()) db_header;
   }
   else if(_writable) {
         auto existing_file_size = bfs::file_size(_data_file_path);
         size_t grow = 0;
         if(shared_file_size > existing_file_size) {
            grow = shared_file_size - existing_file_size;
            bfs::resize_file(_data_file_path, shared_file_size);
         }
         else if(shared_file_size < existing_file_size) {
             std::cerr << "CHAINBASE: \"" << _database_name << "\" requested size of " << shared_file_size << " is less than "
                "existing size of " << existing_file_size << ". This database will not be shrunk and will "
                "remain at " << existing_file_size << std::endl;
         }
         _file_mapping = bip::file_mapping(_data_file_path.generic_string().c_str(), bip::read_write);
         _file_mapped_region = bip::mapped_region(_file_mapping, bip::read_write);
         file_mapped_segment_manager = reinterpret_cast<segment_manager*>((char*)_file_mapped_region.get_address()+header_size);
         if(grow)
            file_mapped_segment_manager->grow(grow);
   }
   else {
         _file_mapping = bip::file_mapping(_data_file_path.generic_string().c_str(), bip::read_only);
         _file_mapped_region = bip::mapped_region(_file_mapping, bip::read_only);
         file_mapped_segment_manager = reinterpret_cast<segment_manager*>((char*)_file_mapped_region.get_address()+header_size);
   }

   if(_writable) {
      //remove meta file created in earlier versions
      boost::system::error_code ec;
      bfs::remove(bfs::absolute(dir/"shared_memory.meta"), ec);

      _mapped_file_lock = bip::file_lock(_data_file_path.generic_string().c_str());
      if(!_mapped_file_lock.try_lock())
         BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::no_access)));

      set_mapped_file_db_dirty(true);
   }

   if(mode == mapped) {
      _segment_manager = file_mapped_segment_manager;
   }
   else {
      boost::asio::io_service sig_ios;
      boost::asio::signal_set sig_set(sig_ios, SIGINT, SIGTERM);
#ifdef SIGPIPE
      sig_set.add(SIGPIPE);
#endif
      sig_set.async_wait([](const boost::system::error_code&, int) {
         BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::aborted)));
      });

      try {
         setup_non_file_mapping();
         load_database_file(sig_ios);

#ifndef _WIN32
         if(mode == locked) {
            if(mlock(_non_file_mapped_mapping, _non_file_mapped_mapping_size)) {
               std::string what_str("Failed to mlock database \"" + _database_name + "\"");
               BOOST_THROW_EXCEPTION(std::system_error(make_error_code(db_error_code::no_mlock), what_str));
            }
            std::cerr << "CHAINBASE: Database \"" << _database_name << "\" has been successfully locked in memory" << std::endl;
         }
#endif

         _file_mapped_region = bip::mapped_region();
      }
      catch(...) {
         if(_writable)
            set_mapped_file_db_dirty(false);
         throw;
      }

      _segment_manager = reinterpret_cast<segment_manager*>((char*)_non_file_mapped_mapping+header_size);
   }
}

void pinnable_mapped_file::setup_non_file_mapping() {
   int common_map_opts = MAP_PRIVATE|MAP_ANONYMOUS;

   _non_file_mapped_mapping_size = _file_mapped_region.get_size();
   auto round_up_mmaped_size = [this](unsigned r) {
      _non_file_mapped_mapping_size = (_non_file_mapped_mapping_size + (r-1u))/r*r;
   };

   const unsigned _1gb = 1u<<30u;
   const unsigned _2mb = 1u<<21u;

#if defined(MAP_HUGETLB) && defined(MAP_HUGE_1GB)
   _non_file_mapped_mapping = mmap(NULL, _non_file_mapped_mapping_size, PROT_READ|PROT_WRITE, common_map_opts|MAP_HUGETLB|MAP_HUGE_1GB, -1, 0);
   if(_non_file_mapped_mapping != MAP_FAILED) {
      round_up_mmaped_size(_1gb);
      std::cerr << "CHAINBASE: Database \"" << _database_name << "\" using 1GB pages" << std::endl;
      return;
   }
#endif

#if defined(MAP_HUGETLB) && defined(MAP_HUGE_2MB)
   //in the future as we expand to support other platforms, consider not specifying any size here so we get the default size. However
   // when mapping the default hugepage size, we'll need to go figure out that size so that the munmap() can be specified correctly
   _non_file_mapped_mapping = mmap(NULL, _non_file_mapped_mapping_size, PROT_READ|PROT_WRITE, common_map_opts|MAP_HUGETLB|MAP_HUGE_2MB, -1, 0);
   if(_non_file_mapped_mapping != MAP_FAILED) {
      round_up_mmaped_size(_2mb);
      std::cerr << "CHAINBASE: Database \"" << _database_name << "\" using 2MB pages" << std::endl;
      return;
   }
#endif

#if defined(VM_FLAGS_SUPERPAGE_SIZE_2MB)
   round_up_mmaped_size(_2mb);
   _non_file_mapped_mapping = mmap(NULL, _non_file_mapped_mapping_size, PROT_READ|PROT_WRITE, common_map_opts, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
   if(_non_file_mapped_mapping != MAP_FAILED) {
      std::cerr << "CHAINBASE: Database \"" << _database_name << "\" using 2MB pages" << std::endl;
      return;
   }
   _non_file_mapped_mapping_size = _file_mapped_region.get_size();  //restore to non 2MB rounded size
#endif

#ifndef _WIN32
   _non_file_mapped_mapping = mmap(NULL, _non_file_mapped_mapping_size, PROT_READ|PROT_WRITE, common_map_opts, -1, 0);
   if(_non_file_mapped_mapping == MAP_FAILED)
      BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Failed to map database ") + _database_name + ": " + strerror(errno)));
#endif
}

void pinnable_mapped_file::load_database_file(boost::asio::io_service& sig_ios) {
   std::cerr << "CHAINBASE: Preloading \"" << _database_name << "\" database file, this could take a moment..." << std::endl;
   char* const src = (char*)_file_mapped_region.get_address();
   char* const dst = (char*)_non_file_mapped_mapping;
   size_t offset = 0;
   time_t t = time(nullptr);
   while(offset != _file_mapped_region.get_size()) {
      memcpy(dst+offset, src+offset, _db_size_multiple_requirement);
      offset += _db_size_multiple_requirement;

      if(time(nullptr) != t) {
         t = time(nullptr);
         std::cerr << "              " << offset/(_file_mapped_region.get_size()/100) << "% complete..." << std::endl;
      }
      sig_ios.poll();
   }
   std::cerr << "           Complete" << std::endl;
}

bool pinnable_mapped_file::all_zeros(char* data, size_t sz) {
   uint64_t* p = (uint64_t*)data;
   uint64_t* end = p+sz/sizeof(uint64_t);
   while(p != end) {
      if(*p++ != 0)
         return false;
   }
   return true;
}

void pinnable_mapped_file::save_database_file() {
   std::cerr << "CHAINBASE: Writing \"" << _database_name << "\" database file, this could take a moment..." << std::endl;
   char* src = (char*)_non_file_mapped_mapping;
   char* dst = (char*)_file_mapped_region.get_address();
   size_t offset = 0;
   time_t t = time(nullptr);
   while(offset != _file_mapped_region.get_size()) {
      if(!all_zeros(src+offset, _db_size_multiple_requirement))
         memcpy(dst+offset, src+offset, _db_size_multiple_requirement);
      offset += _db_size_multiple_requirement;

      if(time(nullptr) != t) {
         t = time(nullptr);
         std::cerr << "              " << offset/(_file_mapped_region.get_size()/100) << "% complete..." << std::endl;
      }
   }
   std::cerr << "           Syncing buffers..." << std::endl;
   if(_file_mapped_region.flush(0, 0, false) == false)
      std::cerr << "CHAINBASE: ERROR: syncing buffers failed" << std::endl;
   std::cerr << "           Complete" << std::endl;
}

pinnable_mapped_file::pinnable_mapped_file(pinnable_mapped_file&& o) :
   _mapped_file_lock(std::move(o._mapped_file_lock)),
   _data_file_path(std::move(o._data_file_path)),
   _database_name(std::move(o._database_name)),
   _file_mapped_region(std::move(o._file_mapped_region))
{
   _segment_manager = o._segment_manager;
   _writable = o._writable;
   _non_file_mapped_mapping = o._non_file_mapped_mapping;
   o._non_file_mapped_mapping = nullptr;
   o._writable = false; //prevent dtor from doing anything interesting
}

pinnable_mapped_file& pinnable_mapped_file::operator=(pinnable_mapped_file&& o) {
   _mapped_file_lock = std::move(o._mapped_file_lock);
   _data_file_path = std::move(o._data_file_path);
   _database_name = std::move(o._database_name);
   _file_mapped_region = std::move(o._file_mapped_region);
   _non_file_mapped_mapping = o._non_file_mapped_mapping;
   o._non_file_mapped_mapping = nullptr;
   _segment_manager = o._segment_manager;
   _writable = o._writable;
   o._writable = false; //prevent dtor from doing anything interesting
   return *this;
}

pinnable_mapped_file::~pinnable_mapped_file() {
   if(_writable) {
      if(_non_file_mapped_mapping) { //in heap or locked mode
         _file_mapped_region = bip::mapped_region(_file_mapping, bip::read_write);
         save_database_file();
#ifndef _WIN32
         if(munmap(_non_file_mapped_mapping, _non_file_mapped_mapping_size))
            std::cerr << "CHAINBASE: ERROR: unmapping failed: " << strerror(errno) << std::endl;
#endif
      }
      else
         if(_file_mapped_region.flush(0, 0, false) == false)
            std::cerr << "CHAINBASE: ERROR: syncing buffers failed" << std::endl;
      set_mapped_file_db_dirty(false);
   }
}

void pinnable_mapped_file::set_mapped_file_db_dirty(bool dirty) {
   *((char*)_file_mapped_region.get_address()+header_dirty_bit_offset) = dirty;
   if(_file_mapped_region.flush(0, 0, false) == false)
      std::cerr << "CHAINBASE: ERROR: syncing buffers failed" << std::endl;
}

std::istream& operator>>(std::istream& in, pinnable_mapped_file::map_mode& runtime) {
   std::string s;
   in >> s;
   if (s == "mapped")
      runtime = pinnable_mapped_file::map_mode::mapped;
   else if (s == "heap")
      runtime = pinnable_mapped_file::map_mode::heap;
   else if (s == "locked")
      runtime = pinnable_mapped_file::map_mode::locked;
   else
      in.setstate(std::ios_base::failbit);
   return in;
}

std::ostream& operator<<(std::ostream& osm, pinnable_mapped_file::map_mode m) {
   if(m == pinnable_mapped_file::map_mode::mapped)
      osm << "mapped";
   else if (m == pinnable_mapped_file::map_mode::heap)
      osm << "heap";
   else if (m == pinnable_mapped_file::map_mode::locked)
      osm << "locked";

   return osm;
}

static std::string print_os(environment::os_t os) {
   switch(os) {
      case environment::OS_LINUX: return "Linux";
      case environment::OS_MACOS: return "macOS";
      case environment::OS_WINDOWS: return "Windows";
      case environment::OS_OTHER: return "Unknown";
   }
   return "error";
}
static std::string print_arch(environment::arch_t arch) {
   switch(arch) {
      case environment::ARCH_X86_64: return "x86_64";
      case environment::ARCH_ARM: return "ARM";
      case environment::ARCH_RISCV: return "RISC-v";
      case environment::ARCH_OTHER: return "Unknown";
   }
   return "error";
}

std::ostream& operator<<(std::ostream& os, const chainbase::environment& dt) {
   os << std::right << std::setw(17) << "Compiler: " << dt.compiler << std::endl;
   os << std::right << std::setw(17) << "Debug: " << (dt.debug ? "Yes" : "No") << std::endl;
   os << std::right << std::setw(17) << "OS: " << print_os(dt.os) << std::endl;
   os << std::right << std::setw(17) << "Arch: " << print_arch(dt.arch) << std::endl;
   os << std::right << std::setw(17) << "Boost: " << dt.boost_version/100000 << "."
                                                  << dt.boost_version/100%1000 << "."
                                                  << dt.boost_version%100 << std::endl;
   return os;
}

}
