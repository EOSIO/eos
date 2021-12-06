#include <fc/log/logger_config.hpp> //set_thread_name
#include <fc/scoped_exit.hpp>

#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/config.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/compile_monitor.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/bit.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/array.hpp>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

#include "IR/Module.h"
#include "IR/Validate.h"
#include "WASM/WASM.h"

#ifndef _POSIX_SYNCHRONIZED_IO
#error _POSIX_SYNCHRONIZED_IO not defined for this platform
#endif

using namespace IR;

namespace eosio { namespace chain { namespace eosvmoc {

static constexpr size_t header_offset = 512u;
static constexpr size_t header_size = 512u;
static constexpr size_t total_header_size = header_offset + header_size;
static constexpr uint64_t header_id = 0x32434f4d56534f45ULL; //"EOSVMOC2" little endian

struct code_cache_header {
   uint64_t id = header_id;
   bool dirty = false;
   uintptr_t serialized_descriptor_index = 0;
} __attribute__ ((packed));
static constexpr size_t header_dirty_bit_offset_from_file_start = header_offset + offsetof(code_cache_header, dirty);
static constexpr size_t descriptor_ptr_from_file_start = header_offset + offsetof(code_cache_header, serialized_descriptor_index);

static_assert(sizeof(code_cache_header) <= header_size, "code_cache_header too big");

namespace impl {
   //allows handing off a naked file descriptor to boost::interprocess::mapped_region; normally one would use a bip::file_mapping
   // but it's not possible to construct a file_mapping with an existing file descriptor. It may be interesting to extend the existing
   // eosvmoc::wrapped_fd to provide get_mapping_handle(), and remove the naked file descriptors from code_cache.
   struct bip_wrapped_handle {
      bip_wrapped_handle(int fd) : fd(fd) {}
      bip_wrapped_handle(const bip_wrapped_handle&) = delete;
      bip_wrapped_handle& operator=(const bip_wrapped_handle&) = delete;
      bip::mapping_handle_t get_mapping_handle() const {return bip::mapping_handle_t{fd, false};}
      int fd;
   };
}

code_cache_async::code_cache_async(const bfs::path data_dir, const eosvmoc::config& eosvmoc_config, code_finder db) :
   code_cache_base(data_dir, eosvmoc_config, std::move(db)),
   _result_queue(eosvmoc_config.threads * 2),
   _threads(eosvmoc_config.threads)
{
   FC_ASSERT(_threads, "EOS VM OC requires at least 1 compile thread");

   wait_on_compile_monitor_message();

   _monitor_reply_thread = std::thread([this]() {
      fc::set_os_thread_name("oc-monitor");
      _ctx.run();
   });
}

code_cache_async::~code_cache_async() {
   _compile_monitor_write_socket.shutdown(local::datagram_protocol::socket::shutdown_send);
   _monitor_reply_thread.join();
   consume_compile_thread_queue();
}

//remember again: wait_on_compile_monitor_message's callback is non-main thread!
void code_cache_async::wait_on_compile_monitor_message() {
   _compile_monitor_read_socket.async_wait(local::datagram_protocol::socket::wait_read, [this](auto ec) {
      if(ec) {
         _ctx.stop();
         return;
      }

      auto [success, message, fds] = read_message_with_fds(_compile_monitor_read_socket);
      if(!success || !std::holds_alternative<wasm_compilation_result_message>(message)) {
         _ctx.stop();
         return;
      }

      _result_queue.push(std::get<wasm_compilation_result_message>(message));

      wait_on_compile_monitor_message();
   });
}


//number processed, bytes available (only if number processed > 0)
std::tuple<size_t, size_t> code_cache_async::consume_compile_thread_queue() {
   size_t bytes_remaining = 0;
   size_t gotsome = _result_queue.consume_all([&](const wasm_compilation_result_message& result) {
      if(_outstanding_compiles_and_poison[result.code] == false) {
         std::visit(overloaded {
            [&](const code_descriptor& cd) {
               _cache_index.push_front(cd);
            },
            [&](const compilation_result_unknownfailure&) {
               wlog("code ${c} failed to tier-up with EOS VM OC", ("c", result.code.code_id));
               _blacklist.emplace(result.code);
            },
            [&](const compilation_result_toofull&) {
               run_eviction_round();
            }
         }, result.result);
      }
      _outstanding_compiles_and_poison.erase(result.code);
      bytes_remaining = result.cache_free_bytes;
   });

   return {gotsome, bytes_remaining};
}

const code_descriptor* const code_cache_async::get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version) {
   //if there are any outstanding compiles, process the result queue now
   if(_outstanding_compiles_and_poison.size()) {
      auto [count_processed, bytes_remaining] = consume_compile_thread_queue();

      if(count_processed)
         check_eviction_threshold(bytes_remaining);

      while(count_processed && _queued_compiles.size()) {
         auto nextup = _queued_compiles.begin();

         //it's not clear this check is required: if apply() was called for code then it existed in the code_index; and then
         // if we got notification of it no longer existing we would have removed it from queued_compiles
         std::string_view const codeobject = _db(nextup->code_id, nextup->vm_version);
         if(!codeobject.empty()) {
            _outstanding_compiles_and_poison.emplace(*nextup, false);
            std::vector<wrapped_fd> fds_to_pass;
            fds_to_pass.emplace_back(memfd_for_bytearray(codeobject));
            FC_ASSERT(write_message_with_fds(_compile_monitor_write_socket, compile_wasm_message{ *nextup }, fds_to_pass), "EOS VM failed to communicate to OOP manager");
            --count_processed;
         }
         _queued_compiles.erase(nextup);
      }
   }

   //check for entry in cache
   code_cache_index::index<by_hash>::type::iterator it = _cache_index.get<by_hash>().find(boost::make_tuple(code_id, vm_version));
   if(it != _cache_index.get<by_hash>().end()) {
      _cache_index.relocate(_cache_index.begin(), _cache_index.project<0>(it));
      return &*it;
   }

   const code_tuple ct = code_tuple{code_id, vm_version};

   if(_blacklist.find(ct) != _blacklist.end())
      return nullptr;
   if(auto it = _outstanding_compiles_and_poison.find(ct); it != _outstanding_compiles_and_poison.end()) {
      it->second = false;
      return nullptr;
   }
   if(_queued_compiles.find(ct) != _queued_compiles.end())
      return nullptr;

   if(_outstanding_compiles_and_poison.size() >= _threads) {
      _queued_compiles.emplace(ct);
      return nullptr;
   }

   std::string_view const codeobject = _db(code_id, vm_version);
   if(codeobject.empty()) //should be impossible right?
      return nullptr;

   _outstanding_compiles_and_poison.emplace(ct, false);
   std::vector<wrapped_fd> fds_to_pass;
   fds_to_pass.emplace_back(memfd_for_bytearray(codeobject));
   write_message_with_fds(_compile_monitor_write_socket, compile_wasm_message{ ct }, fds_to_pass);
   return nullptr;
}

code_cache_sync::~code_cache_sync() {
   //it's exceedingly critical that we wait for the compile monitor to be done with all its work
   //This is easy in the sync case
   _compile_monitor_write_socket.shutdown(local::datagram_protocol::socket::shutdown_send);
   auto [success, message, fds] = read_message_with_fds(_compile_monitor_read_socket);
   if(success)
      elog("unexpected response from EOS VM OC compile monitor during shutdown");
}

const code_descriptor* const code_cache_sync::get_descriptor_for_code_sync(const digest_type& code_id, const uint8_t& vm_version) {
   //check for entry in cache
   code_cache_index::index<by_hash>::type::iterator it = _cache_index.get<by_hash>().find(boost::make_tuple(code_id, vm_version));
   if(it != _cache_index.get<by_hash>().end()) {
      _cache_index.relocate(_cache_index.begin(), _cache_index.project<0>(it));
      return &*it;
   }

   std::string_view const codeobject = _db(code_id, vm_version);
   if(codeobject.empty()) //should be impossible right?
      return nullptr;

   std::vector<wrapped_fd> fds_to_pass;
   fds_to_pass.emplace_back(memfd_for_bytearray(codeobject));

   write_message_with_fds(_compile_monitor_write_socket, compile_wasm_message{ {code_id, vm_version} }, fds_to_pass);
   auto [success, message, fds] = read_message_with_fds(_compile_monitor_read_socket);
   EOS_ASSERT(success, wasm_execution_error, "failed to read response from monitor process");
   EOS_ASSERT(std::holds_alternative<wasm_compilation_result_message>(message), wasm_execution_error, "unexpected response from monitor process");

   wasm_compilation_result_message result = std::get<wasm_compilation_result_message>(message);
   EOS_ASSERT(std::holds_alternative<code_descriptor>(result.result), wasm_execution_error, "failed to compile wasm");

   check_eviction_threshold(result.cache_free_bytes);

   return &*_cache_index.push_front(std::move(std::get<code_descriptor>(result.result))).first;
}

int code_cache_base::get_huge_memfd(size_t map_size, int memfd_flags) const {
   int ret;

   ret = syscall(SYS_memfd_create, "eosvmoc_cc", MFD_CLOEXEC | memfd_flags);
   if(ret < 0)
      return -1;

   auto close_failed_creation = fc::make_scoped_exit([&](){close(ret);});

   if(ftruncate(ret, map_size) == -1)
      return -1;

   //Empirically on 5.13.9 there is no need to pass MAP_HUGETLB to have the mapping actually use hugepages.
   //That makes sense due to the underlying implementation of memfd, but the docs seem ambiguous on expected behavior.

   //The man page states w.r.t. MAP_POPULATE:
   //     The mmap() call doesn't fail if the mapping cannot be populated (for example,
   //     due to limitations on the number of mapped huge pages when using MAP_HUGETLB).
   //Well, empirically on 5.13.9 MAP_POPULATE does cause a failure if there aren't actually enough hugepages available
   // at the given size. It's not clear how to (easily) more robustly handle what the documentation suggests: where the MAP_POPULATE
   // passively ignores that we're overcommitted. We could attempt to force the pages to be allocated in a loop after the mmap(),
   // but we'd just be SIGBUSed if pages aren't available. Handling SIGBUS would fully resolve this I suppose, but that's more
   // complication than I'd like.
   //Thus there seems to be a risk according to the documentation that there may be a nasty situation where the platform indicates
   // 1GB pages are available, MAP_POPULATE passively succeeds, so we then plan to use 1GB pages, but then the later iostreams::copy()
   // fails via a SIGBUS because the hugepages were overcommitted and not enough are actually available. In such a situation a user
   // would be stuck and unable to use heap/locked mode; worse, the SIGBUS may leave the main chainbase database dirty.
   void* mapped = mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE, ret, 0);
   if(mapped == MAP_FAILED)
      return -1;

   if(munmap(mapped, map_size))
      return -1;

   close_failed_creation.cancel();
   return ret;
}

code_cache_base::code_cache_base(const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, code_finder db) :
   _db(std::move(db))
{
   static_assert(sizeof(allocator_t) <= header_offset, "header offset intersects with allocator");

   bfs::create_directories(data_dir);
   boost::filesystem::path cache_file_path = data_dir/"code_cache.bin";

   if(!bfs::exists(cache_file_path)) {
      EOS_ASSERT(eosvmoc_config.cache_size >= allocator_t::get_min_size(total_header_size), database_exception, "configured code cache size is too small");
      std::ofstream ofs(cache_file_path.generic_string(), std::ofstream::trunc);
      EOS_ASSERT(ofs.good(), database_exception, "unable to create EOS VM Optimized Compiler code cache");
      bfs::resize_file(cache_file_path, eosvmoc_config.cache_size);
      bip::file_mapping creation_mapping(cache_file_path.generic_string().c_str(), bip::read_write);
      bip::mapped_region creation_region(creation_mapping, bip::read_write);
      new (creation_region.get_address()) allocator_t(eosvmoc_config.cache_size, total_header_size);
      new ((char*)creation_region.get_address() + header_offset) code_cache_header;
   }

   _cache_file_fd = open(cache_file_path.c_str(), O_RDWR);
   EOS_ASSERT(_cache_file_fd >= 0, database_exception, "failure to open code cache file");
   auto cleanup_cache_file_fd_on_ctor_exception = fc::make_scoped_exit([&](){close(_cache_file_fd);});

   code_cache_header cache_header;
   char header_buff[total_header_size];
   EOS_ASSERT(read(_cache_file_fd, header_buff, sizeof(header_buff)) == sizeof(header_buff), bad_database_version_exception, "failed to read code cache header");
   memcpy((char*)&cache_header, header_buff + header_offset, sizeof(cache_header));

   EOS_ASSERT(cache_header.id == header_id, bad_database_version_exception, "existing EOS VM OC code cache not compatible with this version");
   EOS_ASSERT(!cache_header.dirty, database_exception, "code cache is dirty");

   set_on_disk_region_dirty(true);

   auto existing_file_size = bfs::file_size(cache_file_path);
   size_t on_disk_size = existing_file_size;
   if(eosvmoc_config.cache_size > existing_file_size) {
      EOS_ASSERT(!ftruncate(_cache_file_fd, eosvmoc_config.cache_size), database_exception, "Failed to grow code cache file: ${e}", ("e", strerror(errno)));

      impl::bip_wrapped_handle wh(_cache_file_fd);

      bip::mapped_region resize_region(wh, bip::read_write);
      allocator_t* resize_allocator = reinterpret_cast<allocator_t*>(resize_region.get_address());
      resize_allocator->grow(eosvmoc_config.cache_size - existing_file_size);
      on_disk_size = eosvmoc_config.cache_size;
   }

   auto cleanup_cache_handle_fd_on_ctor_exception = fc::make_scoped_exit([&]() {if(_cache_fd >= 0) close(_cache_fd);});

   if(eosvmoc_config.map_mode == chainbase::pinnable_mapped_file::map_mode::mapped) {
      _cache_fd = dup(_cache_file_fd);
      EOS_ASSERT(_cache_fd >= 0, database_exception, "failure to open code cache");
      _mapped_size = on_disk_size;
   }
   else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
      const unsigned _1gb = 1u<<30u;
      const unsigned _2mb = 1u<<21u;
#pragma GCC diagnostic pop


      _populate_on_map = true;
      _mlock_map = (eosvmoc_config.map_mode == chainbase::pinnable_mapped_file::map_mode::locked);

      auto round_up_file_size_to = [&](size_t r) {
         return (on_disk_size + (r-1u))/r*r;
      };

#if defined(MFD_HUGETLB) && defined(MFD_HUGE_1GB)
      if((_cache_fd = get_huge_memfd(_mapped_size = round_up_file_size_to(_1gb), MFD_HUGETLB|MFD_HUGE_1GB)) >= 0) {
         ilog("EOS VM OC code cache using 1GB pages");
      } else
#endif
#if defined(MFD_HUGETLB) && defined(MFD_HUGE_2MB)
      if((_cache_fd = get_huge_memfd(_mapped_size = round_up_file_size_to(_2mb), MFD_HUGETLB|MFD_HUGE_2MB)) >= 0) {
         ilog("EOS VM OC code cache using 2MB pages");
      } else
#endif
      {
         _cache_fd = get_huge_memfd(_mapped_size = round_up_file_size_to(getpagesize()), 0);
         EOS_ASSERT(_cache_fd >= 0, database_exception, "Failed to allocate code cache memory");
      }

      ilog("Preloading EOS VM OC code cache. This may take a moment...");

      EOS_ASSERT(lseek(_cache_file_fd, 0, SEEK_SET) == 0, database_exception, "Failed to seek in code cache file");
      impl::bip_wrapped_handle wh(_cache_fd);
      bip::mapped_region load_region(wh, bip::read_write);

      boost::iostreams::file_descriptor_source source(_cache_file_fd, boost::iostreams::never_close_handle);
      boost::iostreams::array_sink sink((char*)load_region.get_address(), load_region.get_size());
      std::streamsize copied = boost::iostreams::copy(source, sink, 1024*1024);
      EOS_ASSERT(std::bit_cast<size_t>(copied) >= on_disk_size, database_exception, "Failed to preload code cache memory");
   }

   //load up the previous cache index
   impl::bip_wrapped_handle wh(_cache_fd);

   bip::mapped_region load_region(wh, bip::read_write);
   allocator_t* allocator = reinterpret_cast<allocator_t*>(load_region.get_address());

   if(cache_header.serialized_descriptor_index) {
      fc::datastream<const char*> ds((char*)load_region.get_address() + cache_header.serialized_descriptor_index, eosvmoc_config.cache_size - cache_header.serialized_descriptor_index);
      unsigned number_entries;
      fc::raw::unpack(ds, number_entries);
      for(unsigned i = 0; i < number_entries; ++i) {
         code_descriptor cd;
         fc::raw::unpack(ds, cd);
         if(cd.codegen_version != 0) {
            allocator->deallocate((char*)load_region.get_address() + cd.code_begin);
            allocator->deallocate((char*)load_region.get_address() + cd.initdata_begin);
            continue;
         }
         _cache_index.push_back(std::move(cd));
      }
      allocator->deallocate((char*)load_region.get_address() + cache_header.serialized_descriptor_index);

      ilog("EOS VM Optimized Compiler code cache loaded with ${c} entries; ${f} of ${t} bytes free", ("c", number_entries)("f", allocator->get_free_memory())("t", allocator->get_size()));
   }

   _free_bytes_eviction_threshold = on_disk_size * .1;

   wrapped_fd compile_monitor_conn = get_connection_to_compile_monitor(_cache_fd);

   //okay, let's do this by the book: we're not allowed to write & read on different threads to the same asio socket. So create two fds
   //representing the same unix socket. we'll read on one and write on the other
   int duped = dup(compile_monitor_conn);
   EOS_ASSERT(duped >= 0, database_exception, "failure to dup compile monitor socket");
   _compile_monitor_write_socket.assign(local::datagram_protocol(), duped);
   _compile_monitor_read_socket.assign(local::datagram_protocol(), compile_monitor_conn.release());

   cleanup_cache_file_fd_on_ctor_exception.cancel();
   cleanup_cache_handle_fd_on_ctor_exception.cancel();
}

void code_cache_base::set_on_disk_region_dirty(bool dirty) {
   if(lseek(_cache_file_fd, header_dirty_bit_offset_from_file_start, SEEK_SET) == -1) {
      elog("Seeking to code cache dirty bit failed");
      return;
   }
   char write_me = dirty;
   if(::write(_cache_file_fd, &write_me, 1) != 1)
      elog("Writing dirty bit in code cache failed");
   if(fsync(_cache_file_fd))
      elog("Syncing code cache dirtyness failed");
}

template <typename T>
void code_cache_base::serialize_cache_index(fc::datastream<T>& ds) {
   unsigned entries = _cache_index.size();
   fc::raw::pack(ds, entries);
   for(const code_descriptor& cd : _cache_index)
      fc::raw::pack(ds, cd);
}

void code_cache_base::cache_mapping_for_execution(const int prot_flags, uint8_t*& addr, size_t& map_size) const {
   int map_flags = MAP_SHARED;
   if(_populate_on_map)
      map_flags |= MAP_POPULATE; //see comments in get_huge_memfd(). This is intended solely to populate page table for existing allocated pages

   addr = (uint8_t*)mmap(nullptr, _mapped_size, prot_flags, map_flags, _cache_fd, 0);
   FC_ASSERT(addr != MAP_FAILED, "failed to map code cache (${e})", ("e", strerror(errno)));

   if(_mlock_map && mlock(addr, _mapped_size)) {
      int lockerr = errno;
      munmap(addr, _mapped_size);
      FC_ASSERT(false, "failed to lock code cache (${e})", ("e", strerror(lockerr)));
   }

   map_size = _mapped_size;
}

code_cache_base::~code_cache_base() {
   //reopen the code cache in our process
   impl::bip_wrapped_handle wh(_cache_fd);
   bip::mapped_region load_region(wh, bip::read_write);
   allocator_t* allocator = reinterpret_cast<allocator_t*>(load_region.get_address());

   //serialize out the cache index
   fc::datastream<size_t> dssz;
   serialize_cache_index(dssz);
   size_t sz = dssz.tellp();

   char* code_mapping = (char*)load_region.get_address();

   char* p = nullptr;
   while(_cache_index.size()) {
      p = (char*)allocator->allocate(sz);
      if(p != nullptr)
         break;
      //in theory, there could be too little free space avaiable to store the cache index
      //try to free up some space
      for(unsigned int i = 0; i < 25 && _cache_index.size(); ++i) {
         allocator->deallocate(code_mapping + _cache_index.back().code_begin);
         allocator->deallocate(code_mapping + _cache_index.back().initdata_begin);
         _cache_index.pop_back();
      }
   }

   if(p) {
      fc::datastream<char*> ds(p, sz);
      serialize_cache_index(ds);

      uintptr_t ptr_offset = p-code_mapping;
      *((uintptr_t*)(code_mapping+descriptor_ptr_from_file_start)) = ptr_offset;
   }
   else
      *((uintptr_t*)(code_mapping+descriptor_ptr_from_file_start)) = 0;

   if(!_populate_on_map) {
      msync(code_mapping, _mapped_size, MS_SYNC);
      munmap(code_mapping, _mapped_size);
   }
   else {
      ilog("Writing EOS VM OC code cache to disk. This may take a moment...");
      struct stat st;
      if(lseek(_cache_file_fd, 0, SEEK_SET) == 0 && fstat(_cache_file_fd, &st) == 0) {
         //don't use the mmaped size for source since it has been rounded up from the file size, use file size
         boost::iostreams::array_source source(code_mapping, st.st_size);
         boost::iostreams::file_descriptor_sink sink(_cache_file_fd, boost::iostreams::never_close_handle);

         if(boost::iostreams::copy(source, sink, 1024*1024) != st.st_size)
            elog("Failed to write out EOS VM OC code cache");
         if(fsync(_cache_file_fd))
            elog("Syncing code cache data failed");
      }
      else {
         elog("Failed to write out EOS VM OC code cache");
      }
   }

   close(_cache_fd);
   set_on_disk_region_dirty(false);
   close(_cache_file_fd);
}

void code_cache_base::free_code(const digest_type& code_id, const uint8_t& vm_version) {
   code_cache_index::index<by_hash>::type::iterator it = _cache_index.get<by_hash>().find(boost::make_tuple(code_id, vm_version));
   if(it != _cache_index.get<by_hash>().end()) {
      write_message_with_fds(_compile_monitor_write_socket, evict_wasms_message{ {*it} });
      _cache_index.get<by_hash>().erase(it);
   }

   //if it's in the queued list, erase it
   _queued_compiles.erase({code_id, vm_version});

   //however, if it's currently being compiled there is no way to cancel the compile,
   //so instead set a poison boolean that indicates not to insert the code in to the cache
   //once the compile is complete
   const std::unordered_map<code_tuple, bool>::iterator compiling_it = _outstanding_compiles_and_poison.find({code_id, vm_version});
   if(compiling_it != _outstanding_compiles_and_poison.end())
      compiling_it->second = true;
}

void code_cache_base::run_eviction_round() {
   evict_wasms_message evict_msg;
   for(unsigned int i = 0; i < 25 && _cache_index.size() > 1; ++i) {
      evict_msg.codes.emplace_back(_cache_index.back());
      _cache_index.pop_back();
   }
   write_message_with_fds(_compile_monitor_write_socket, evict_msg);
}

void code_cache_base::check_eviction_threshold(size_t free_bytes) {
   _last_known_free_bytes = free_bytes;
   if(free_bytes < _free_bytes_eviction_threshold)
      run_eviction_round();
}

}}}
