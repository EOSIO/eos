#include <eosio/chain/webassembly/rodeos/code_cache.hpp>
#include <eosio/chain/webassembly/rodeos/config.hpp>
#include <eosio/chain/exceptions.hpp>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

namespace eosio { namespace chain { namespace rodeos {

static constexpr size_t header_offset = 512u;
static constexpr size_t header_size = 512u;
static constexpr size_t total_header_size = header_offset + header_size;
static constexpr uint64_t header_id = 0x31434f4d56534f45ULL; //"EOSVMOC1" little endian

struct code_cache_header {
   uint64_t id = header_id;
   bool dirty = false;
   uintptr_t serialized_descriptor_index = 0;
} __attribute__ ((packed));
static constexpr size_t header_dirty_bit_offset_from_file_start = header_offset + offsetof(code_cache_header, dirty);
static constexpr size_t descriptor_ptr_from_file_start = header_offset + offsetof(code_cache_header, serialized_descriptor_index);

static_assert(sizeof(code_cache_header) <= header_size, "code_cache_header too big");

code_cache::code_cache(const boost::filesystem::path data_dir, const rodeos::config& rodeos_config) :
   _cache_file_path(data_dir/"code_cache.bin")
{
   static_assert(sizeof(allocator_t) <= header_offset, "header offset intersects with allocator");

   bfs::create_directories(data_dir);

   if(!bfs::exists(_cache_file_path)) {
      std::ofstream ofs(_cache_file_path.generic_string(), std::ofstream::trunc);
      bfs::resize_file(_cache_file_path, rodeos_config.cache_size);
      bip::file_mapping creation_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
      bip::mapped_region creation_region(creation_mapping, bip::read_write);
      new (creation_region.get_address()) allocator_t(rodeos_config.cache_size, total_header_size);
      new ((char*)creation_region.get_address() + header_offset) code_cache_header;
   }

   code_cache_header* cache_header = nullptr;
   {
      char header[total_header_size];
      std::ifstream hs(_cache_file_path.generic_string(), std::ifstream::binary);
      hs.read(header, sizeof(header));
      EOS_ASSERT(!hs.fail(), bad_database_version_exception, "failed to read code cache header");

      cache_header = reinterpret_cast<code_cache_header*>(header+header_offset);
      EOS_ASSERT(cache_header->id == header_id, bad_database_version_exception, "code cache header magic not as expected");
      EOS_ASSERT(!cache_header->dirty, database_exception, "code cache is dirty");
   }

   set_on_disk_region_dirty(true);

   auto existing_file_size = bfs::file_size(_cache_file_path);
   if(rodeos_config.cache_size > existing_file_size) {
      bfs::resize_file(_cache_file_path, rodeos_config.cache_size);

      bip::file_mapping resize_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
      bip::mapped_region resize_region(resize_mapping, bip::read_write);

      allocator_t* resize_allocator = reinterpret_cast<allocator_t*>(resize_region.get_address());
      resize_allocator->grow(rodeos_config.cache_size - existing_file_size);
   }

   _cache_fd = ::open(_cache_file_path.generic_string().c_str(), O_RDWR | O_CLOEXEC);
   EOS_ASSERT(_cache_fd >= 0, database_exception, "failure to open code cache");
   _code_mapping = (char*)mmap(nullptr, rodeos_config.cache_size, PROT_READ|PROT_WRITE, MAP_SHARED, _cache_fd, 0);
   EOS_ASSERT(_code_mapping != MAP_FAILED, database_exception, "failure to mmap code cache");

   allocator = reinterpret_cast<allocator_t*>(_code_mapping);

   if(cache_header->serialized_descriptor_index) {
      fc::datastream<const char*> ds(_code_mapping + cache_header->serialized_descriptor_index, rodeos_config.cache_size - cache_header->serialized_descriptor_index);
      unsigned number_entries;
      fc::raw::unpack(ds, number_entries);
      for(unsigned i = 0; i < number_entries; ++i) {
         code_descriptor cd;
         fc::raw::unpack(ds, cd);
         _cache_index.push_back(std::move(cd));
      }
      allocator->deallocate(_code_mapping + cache_header->serialized_descriptor_index);
   }
}

void code_cache::set_on_disk_region_dirty(bool dirty) {
   bip::file_mapping dirty_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
   bip::mapped_region dirty_region(dirty_mapping, bip::read_write);

   *((char*)dirty_region.get_address()+header_dirty_bit_offset_from_file_start) = dirty;
   if(dirty_region.flush(0, 0, false) == false)
      elog("Syncing code cache failed");
}

template <typename T>
void code_cache::serialize_cache_index(fc::datastream<T>& ds) {
   unsigned entries = _cache_index.size();
   fc::raw::pack(ds, entries);
   for(const code_descriptor& cd : _cache_index)
      fc::raw::pack(ds, cd);
}

code_cache::~code_cache() {
   fc::datastream<size_t> dssz;
   serialize_cache_index(dssz);
   size_t sz = dssz.tellp();

   char* p = (char*)allocator->allocate(sz);
   fc::datastream<char*> ds(p, sz);
   serialize_cache_index(ds);

   uintptr_t ptr_offset = p-_code_mapping;
   *((uintptr_t*)(_code_mapping+descriptor_ptr_from_file_start)) = ptr_offset;

   msync(_code_mapping, allocator->get_size(), MS_SYNC);
   munmap(_code_mapping, allocator->get_size());
   close(_cache_fd);
   set_on_disk_region_dirty(false);
}

const code_descriptor* const code_cache::get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version) {
   code_cache_index::index<by_hash>::type::iterator it = _cache_index.get<by_hash>().find(boost::make_tuple(code_id, vm_version));
   if(it != _cache_index.get<by_hash>().end()) {
      _cache_index.relocate(_cache_index.begin(), _cache_index.project<0>(it));
      return &*it;
   }
   
   return nullptr;
}

}}}