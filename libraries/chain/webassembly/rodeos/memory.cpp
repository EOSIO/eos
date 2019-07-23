#include <eosio/chain/webassembly/rodeos/memory.hpp>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

namespace eosio { namespace chain { namespace rodeos {

memory::memory() {
   int fd = syscall(SYS_memfd_create, "rodeos_mem", MFD_CLOEXEC);
   ftruncate(fd, wasm_memory_size+memory_prologue_size);

   mapsize = total_memory_per_slice*number_slices;
   mapbase = (uint8_t*)mmap(nullptr, mapsize, PROT_NONE, MAP_PRIVATE|MAP_ANON, 0, 0);

   uint8_t* next_slice = mapbase;
   uint8_t* last;

   for(unsigned int p = 0; p < number_slices; ++p) {
      last = (uint8_t*)mmap(next_slice, memory_prologue_size+64u*1024u*p, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, fd, 0);
      next_slice += total_memory_per_slice;
   }

   zeropage_base = mapbase + memory_prologue_size;
   fullpage_base = last + memory_prologue_size;

   close(fd);
}

memory::~memory() {
   munmap(mapbase, mapsize);
#if 0
   RODEOS_MEMORY_PTR_cb_ptr;
   cb_ptr->magic = 0xdeadbeefcafebebeULL;
   cb_ptr->current_call_depth_remaining = 250;

   RODEOS_MEMORY_PTR_first_reserved_intrinsic_ptr;
   RODEOS_MEMORY_PTR_first_intrinsic_ptr;
   first_reserved_intrinsic_ptr[0] = 0xaaaa;
   first_reserved_intrinsic_ptr[-1] = 0xbbbb;
   first_reserved_intrinsic_ptr[-2] = 0xcccc;
   first_reserved_intrinsic_ptr[-3] = 0xdddd;
   first_intrinsic_ptr[0] = 0x1010101;
#endif
}

}}}