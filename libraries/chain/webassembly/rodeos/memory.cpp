#include <eosio/chain/webassembly/rodeos/memory.hpp>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

#include <Runtime/Intrinsics.h>

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

   //layout the intrinsic jump table
   uintptr_t* const intrinsic_jump_table = reinterpret_cast<uintptr_t* const>(zeropage_base - first_intrinsic_offset);
   const std::map<std::string,Intrinsics::Function*>& intrinsics = Intrinsics::getIntrinsicFunctions();
   for(const auto& func : intrinsics)
      intrinsic_jump_table[-func.second->getOffset()] = (uintptr_t)func.second->getNativeFunc();
}

memory::~memory() {
   munmap(mapbase, mapsize);
}

}}}