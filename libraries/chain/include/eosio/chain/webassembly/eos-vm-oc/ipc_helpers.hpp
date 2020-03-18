#pragma once

#include <eosio/chain/webassembly/eos-vm-oc/ipc_protocol.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/anon_mem_fd.hpp>

#include <boost/asio/local/stream_protocol.hpp>

#include <vector>

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

std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(boost::asio::local::stream_protocol::socket& s);
std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(int fd);
bool write_message_with_fds(boost::asio::local::stream_protocol::socket& s, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds = std::vector<wrapped_fd>());
bool write_message_with_fds(int fd_to_send_to, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds = std::vector<wrapped_fd>());

template<typename T>
wrapped_fd memfd_for_bytearray(const T& bytes) {
   int fd = anon_mem_fd("eosvmoc_code");
   FC_ASSERT(fd >= 0, "Failed to create memfd");
   size_t total_to_map = bytes.size()+sizeof(uint32_t);
   FC_ASSERT(ftruncate(fd, total_to_map) == 0, "failed to grow memfd");
   uint8_t* b = (uint8_t*)mmap(nullptr, total_to_map, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
   FC_ASSERT(b != MAP_FAILED, "failed to mmap memfd");
   uint32_t* sz = (uint32_t*)b;
   *sz = bytes.size();
   memcpy(b+sizeof(uint32_t), bytes.data(), bytes.size());
   munmap(b, total_to_map);
   return wrapped_fd(fd);
}

std::vector<char> vector_for_memfd(const wrapped_fd& memfd);
}}}