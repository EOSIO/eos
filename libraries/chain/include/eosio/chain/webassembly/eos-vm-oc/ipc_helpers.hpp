#pragma once

#include <eosio/chain/webassembly/eos-vm-oc/ipc_protocol.hpp>

#include <boost/asio/local/datagram_protocol.hpp>

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
         _inuse = other._inuse;
         _fd = other._fd;
         other._inuse = false;
         return *this;
      }

      operator int() const {
         if(_inuse)
            return _fd;
         return -1; ///XXX this should probably assert/throw?
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
void write_message_with_fds(boost::asio::local::datagram_protocol::socket& s, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds = std::vector<wrapped_fd>());
void write_message_with_fds(int fd_to_send_to, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds = std::vector<wrapped_fd>());

wrapped_fd memfd_for_vector(const std::vector<uint8_t>& bytes);
wrapped_fd memfd_for_blob(const shared_blob& blob);
std::vector<uint8_t> vector_for_memfd(const wrapped_fd& memfd);
}}}