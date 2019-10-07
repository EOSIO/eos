#include <fc/log/logger_config.hpp> //set_thread_name

#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/config.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/memory.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/compile_monitor.hpp>
#include <eosio/chain/exceptions.hpp>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

#include "IR/Module.h"
#include "IR/Validate.h"
#include "WASM/WASM.h"
#include "LLVMJIT.h"

using namespace IR;
using namespace Runtime;

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

code_cache_async::code_cache_async(const bfs::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db) :
   code_cache_base(data_dir, eosvmoc_config, db),
   _result_queue(eosvmoc_config.threads * 2),
   _threads(eosvmoc_config.threads)
{
   FC_ASSERT(_threads, "EOS VM OC requires at least 1 compile thread");

   wait_on_compile_monitor_message();

   _monitor_reply_thread = std::thread([this]() {
      fc::set_thread_name("oc-monitor");
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
      if(!success || !message.contains<wasm_compilation_result_message>()) {
         _ctx.stop();
         return;
      }

      _result_queue.push(message.get<wasm_compilation_result_message>());

      wait_on_compile_monitor_message();
   });
}


//number processed, bytes available (only if number processed > 0)
std::tuple<size_t, size_t> code_cache_async::consume_compile_thread_queue() {
   size_t bytes_remaining = 0;
   size_t gotsome = _result_queue.consume_all([&](const wasm_compilation_result_message& result) {
      if(_outstanding_compiles_and_poison[result.code] == false) {
         result.result.visit(overloaded {
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
         });
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
         const code_object* const codeobject = _db.find<code_object,by_code_hash>(boost::make_tuple(nextup->code_id, 0, nextup->vm_version));
         if(codeobject) {
            _outstanding_compiles_and_poison.emplace(*nextup, false);
            std::vector<wrapped_fd> fds_to_pass;
            fds_to_pass.emplace_back(memfd_for_bytearray(codeobject->code));
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

   const code_object* const codeobject = _db.find<code_object,by_code_hash>(boost::make_tuple(code_id, 0, vm_version));
   if(!codeobject) //should be impossible right?
      return nullptr;

   _outstanding_compiles_and_poison.emplace(ct, false);
   std::vector<wrapped_fd> fds_to_pass;
   fds_to_pass.emplace_back(memfd_for_bytearray(codeobject->code));
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

   const code_object* const codeobject = _db.find<code_object,by_code_hash>(boost::make_tuple(code_id, 0, vm_version));
   if(!codeobject) //should be impossible right?
      return nullptr;

   std::vector<wrapped_fd> fds_to_pass;
   fds_to_pass.emplace_back(memfd_for_bytearray(codeobject->code));

   write_message_with_fds(_compile_monitor_write_socket, compile_wasm_message{ {code_id, vm_version} }, fds_to_pass);
   auto [success, message, fds] = read_message_with_fds(_compile_monitor_read_socket);
   EOS_ASSERT(success, wasm_execution_error, "failed to read response from monitor process");
   EOS_ASSERT(message.contains<wasm_compilation_result_message>(), wasm_execution_error, "unexpected response from monitor process");

   wasm_compilation_result_message result = message.get<wasm_compilation_result_message>();
   EOS_ASSERT(result.result.contains<code_descriptor>(), wasm_execution_error, "failed to compile wasm");

   check_eviction_threshold(result.cache_free_bytes);

   return &*_cache_index.push_front(std::move(result.result.get<code_descriptor>())).first;
}

code_cache_base::code_cache_base(const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db) :
   _db(db),
   _cache_file_path(data_dir/"code_cache.bin")
{
   static_assert(sizeof(allocator_t) <= header_offset, "header offset intersects with allocator");

   bfs::create_directories(data_dir);

   if(!bfs::exists(_cache_file_path)) {
      EOS_ASSERT(eosvmoc_config.cache_size >= allocator_t::get_min_size(total_header_size), database_exception, "configured code cache size is too small");
      std::ofstream ofs(_cache_file_path.generic_string(), std::ofstream::trunc);
      EOS_ASSERT(ofs.good(), database_exception, "unable to create EOS VM Optimized Compiler code cache");
      bfs::resize_file(_cache_file_path, eosvmoc_config.cache_size);
      bip::file_mapping creation_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
      bip::mapped_region creation_region(creation_mapping, bip::read_write);
      new (creation_region.get_address()) allocator_t(eosvmoc_config.cache_size, total_header_size);
      new ((char*)creation_region.get_address() + header_offset) code_cache_header;
   }

   code_cache_header cache_header;
   {
      char header_buff[total_header_size];
      std::ifstream hs(_cache_file_path.generic_string(), std::ifstream::binary);
      hs.read(header_buff, sizeof(header_buff));
      EOS_ASSERT(!hs.fail(), bad_database_version_exception, "failed to read code cache header");
      memcpy((char*)&cache_header, header_buff + header_offset, sizeof(cache_header));
   }

   EOS_ASSERT(cache_header.id == header_id, bad_database_version_exception, "existing EOS VM OC code cache not compatible with this version");
   EOS_ASSERT(!cache_header.dirty, database_exception, "code cache is dirty");

   set_on_disk_region_dirty(true);

   auto existing_file_size = bfs::file_size(_cache_file_path);
   if(eosvmoc_config.cache_size > existing_file_size) {
      bfs::resize_file(_cache_file_path, eosvmoc_config.cache_size);

      bip::file_mapping resize_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
      bip::mapped_region resize_region(resize_mapping, bip::read_write);

      allocator_t* resize_allocator = reinterpret_cast<allocator_t*>(resize_region.get_address());
      resize_allocator->grow(eosvmoc_config.cache_size - existing_file_size);
   }

   _cache_fd = ::open(_cache_file_path.generic_string().c_str(), O_RDWR | O_CLOEXEC);
   EOS_ASSERT(_cache_fd >= 0, database_exception, "failure to open code cache");

   //load up the previous cache index
   char* code_mapping = (char*)mmap(nullptr, eosvmoc_config.cache_size, PROT_READ|PROT_WRITE, MAP_SHARED, _cache_fd, 0);
   EOS_ASSERT(code_mapping != MAP_FAILED, database_exception, "failure to mmap code cache");

   allocator_t* allocator = reinterpret_cast<allocator_t*>(code_mapping);

   if(cache_header.serialized_descriptor_index) {
      fc::datastream<const char*> ds(code_mapping + cache_header.serialized_descriptor_index, eosvmoc_config.cache_size - cache_header.serialized_descriptor_index);
      unsigned number_entries;
      fc::raw::unpack(ds, number_entries);
      for(unsigned i = 0; i < number_entries; ++i) {
         code_descriptor cd;
         fc::raw::unpack(ds, cd);
         if(cd.codegen_version != 0)
            continue;
         _cache_index.push_back(std::move(cd));
      }
      allocator->deallocate(code_mapping + cache_header.serialized_descriptor_index);

      ilog("EOS VM Optimized Compiler code cache loaded with ${c} entries; ${f} of ${t} bytes free", ("c", number_entries)("f", allocator->get_free_memory())("t", allocator->get_size()));
   }
   munmap(code_mapping, eosvmoc_config.cache_size);

   _free_bytes_eviction_threshold = eosvmoc_config.cache_size * .1;

   wrapped_fd compile_monitor_conn = get_connection_to_compile_monitor(_cache_fd);

   //okay, let's do this by the book: we're not allowed to write & read on different threads to the same asio socket. So create two fds
   //representing the same unix socket. we'll read on one and write on the other
   int duped = dup(compile_monitor_conn);
   _compile_monitor_write_socket.assign(local::datagram_protocol(), duped);
   _compile_monitor_read_socket.assign(local::datagram_protocol(), compile_monitor_conn.release());
}

void code_cache_base::set_on_disk_region_dirty(bool dirty) {
   bip::file_mapping dirty_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
   bip::mapped_region dirty_region(dirty_mapping, bip::read_write);

   *((char*)dirty_region.get_address()+header_dirty_bit_offset_from_file_start) = dirty;
   if(dirty_region.flush(0, 0, false) == false)
      elog("Syncing code cache failed");
}

template <typename T>
void code_cache_base::serialize_cache_index(fc::datastream<T>& ds) {
   unsigned entries = _cache_index.size();
   fc::raw::pack(ds, entries);
   for(const code_descriptor& cd : _cache_index)
      fc::raw::pack(ds, cd);
}

code_cache_base::~code_cache_base() {
   //reopen the code cache in our process
   struct stat st;
   if(fstat(_cache_fd, &st))
      return;
   char* code_mapping = (char*)mmap(nullptr, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, _cache_fd, 0);
   if(code_mapping == MAP_FAILED)
      return;

   allocator_t* allocator = reinterpret_cast<allocator_t*>(code_mapping);

   //serialize out the cache index
   fc::datastream<size_t> dssz;
   serialize_cache_index(dssz);
   size_t sz = dssz.tellp();

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

   msync(code_mapping, allocator->get_size(), MS_SYNC);
   munmap(code_mapping, allocator->get_size());
   close(_cache_fd);
   set_on_disk_region_dirty(false);

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
   if(free_bytes < _free_bytes_eviction_threshold)
      run_eviction_round();
}

}}}