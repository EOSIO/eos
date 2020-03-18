#include <eosio/chain/webassembly/eos-vm-oc/anon_mem_fd.hpp>

#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <errno.h> ///XXX
#include <string.h> ///XXX

#include <atomic>

namespace eosio { namespace chain { namespace eosvmoc {

int anon_mem_fd(const char* const label) {
   char nom[128] = {0};
   struct timespec tv;
   if(clock_gettime(CLOCK_MONOTONIC, &tv))
      return -1;
   static std::atomic<unsigned long> count;
   //!! the max length macos will "bother with" is 31 characters
   //we probably want to loop on this a few times in case of collision? should be
   //using random bytes instead?
   snprintf(nom, sizeof(nom), "eosio%lu%lu", tv.tv_nsec/1000, count++);
   int fd = shm_open(nom, O_RDWR | O_CREAT | O_EXCL, 0600);
   if(fd < 0)
      return fd;
   shm_unlink(nom);
   return fd;
}

}}}