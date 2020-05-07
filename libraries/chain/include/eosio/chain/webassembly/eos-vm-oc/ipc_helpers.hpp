#pragma once

#include <eosio/chain/webassembly/eos-vm-oc/ipc_protocol.hpp>

#include <boost/asio/local/datagram_protocol.hpp>

#include <vector>

#include <sys/syscall.h>
#include <linux/memfd.h>

namespace eosio { namespace chain { namespace eosvmoc {

class wrapped_fd {
   public:
      wrapped_fd() : _inuse(false) {}
      wrapped_fd(int fd) : _inuse(true), _fd(fd) {}
      wrapped_fd(const wrapped_fd&) = delete;
      wrapped_fd& operator=(const wrapped_fd&) = delete;
      wrapped_fd(wrapped_fd&& other) : _inuse(other._inuse), _fd(other._fd) {other._inuse = false;}
      wrapped_fd& operator=(wrapped_fd&& other) {
         if(_inuse)
            close(_fd);
         _inuse = other._inuse;
         _fd = other._fd;
         other._inuse = false;
         return *this;
      }

      operator int() const {
         FC_ASSERT(_inuse, "trying to get the value of a not-in-use wrappedfd");
         return _fd;
      }

      int release() {
         _inuse = false;
         return _fd;
      }

      ~wrapped_fd() {
         if(_inuse)
            close(_fd);
      }

   private:
      bool _inuse = false;;
      int _fd;
};

std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(boost::asio::local::datagram_protocol::socket& s);
std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(int fd);
bool write_message_with_fds(boost::asio::local::datagram_protocol::socket& s, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds = std::vector<wrapped_fd>());
bool write_message_with_fds(int fd_to_send_to, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds = std::vector<wrapped_fd>());

template<typename T>
wrapped_fd memfd_for_bytearray(const T& bytes) {
   int fd = syscall(SYS_memfd_create, "eosvmoc_code", MFD_CLOEXEC);
   FC_ASSERT(fd >= 0, "Failed to create memfd");
   FC_ASSERT(ftruncate(fd, bytes.size()) == 0, "failed to grow memfd");
   if(bytes.size()) {
      uint8_t* b = (uint8_t*)mmap(nullptr, bytes.size(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
      FC_ASSERT(b != MAP_FAILED, "failed to mmap memfd");
      memcpy(b, bytes.data(), bytes.size());
      munmap(b, bytes.size());
   }
   return wrapped_fd(fd);
}

std::vector<uint8_t> vector_for_memfd(const wrapped_fd& memfd);
}}}