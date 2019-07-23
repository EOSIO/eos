#include <eosio/chain/webassembly/rodeos/code_cache.hpp>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

namespace eosio { namespace chain { namespace rodeos {

code_cache::code_cache() {
   //placeholder code for now
   const size_t sz = 1024*1024*1024;
   _cache_fd = syscall(SYS_memfd_create, "rodeos_code", MFD_CLOEXEC);
   ftruncate(_cache_fd, sz);

   //phase 1 only stuff below; normally this will live in sandbox
   _code_mapping_size = sz;
   _code_mapping = (uint8_t*)mmap(nullptr, _code_mapping_size, PROT_READ|PROT_WRITE, MAP_SHARED, _cache_fd, 0);
   _segment_manager = new (_code_mapping) segment_manager(_code_mapping_size);
}

code_cache::~code_cache() {
   close(_cache_fd);
}

const code_descriptor* const code_cache::get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version) {
   code_cache_index::index<by_hash>::type::iterator it = _cache_index.get<by_hash>().find(boost::make_tuple(code_id, vm_version));
   if(it != _cache_index.get<by_hash>().end())
      return &*it;
   
   return nullptr;
}

}}}