#include <eosio/chain/webassembly/eos-vm-oc/ipc_helpers.hpp>
#include <eosio/chain/exceptions.hpp>

#include <sys/syscall.h>

namespace eosio { namespace chain { namespace eosvmoc {

static constexpr size_t max_message_size = 8192;

std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(boost::asio::local::datagram_protocol::socket& s) {
   return read_message_with_fds(s.native_handle());
}

std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(int fd) {
   char buff[max_message_size];

   struct msghdr msg = {};
   struct cmsghdr* cmsg;

   struct iovec io = {
      .iov_base = buff,
      .iov_len = sizeof(buff)
   };
   union {
      char buf[CMSG_SPACE(4 * sizeof(int))]; //XXX this code supports a max of 4, so it should probably throw/assert otherwise
      struct cmsghdr align;
   } u;

   msg.msg_iov = &io;
   msg.msg_iovlen = 1;
   msg.msg_control = u.buf;
   msg.msg_controllen = sizeof(u.buf);

   int red;
   do {
      red = recvmsg(fd, &msg, 0);
   } while(red == -1 && errno == EINTR);
   if(red < 1 || red >= sizeof(buff))
      return {false, eosvmoc_message(), std::vector<wrapped_fd>()};
   fc::datastream<char*> ds(buff, red);
   eosvmoc_message message;
   fc::raw::unpack(ds, message);

   std::vector<wrapped_fd> fds;

   if(msg.msg_controllen) {
      cmsg = CMSG_FIRSTHDR(&msg);
      unsigned num_of_fds = (cmsg->cmsg_len - CMSG_LEN(0))/sizeof(int);
      int* fd_ptr = (int*)CMSG_DATA(cmsg);
      for(unsigned i = 0; i < num_of_fds; ++i)
         fds.push_back(*fd_ptr++);
   }

   return {true, message, std::move(fds)};
}

void write_message_with_fds(boost::asio::local::datagram_protocol::socket& s, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds) {
   write_message_with_fds(s.native_handle(), message, fds);
}

void write_message_with_fds(int fd_to_send_to, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds) {
   struct msghdr msg = {};
   struct cmsghdr* cmsg;

   size_t sz = fc::raw::pack_size(message);
   EOS_ASSERT(sz < max_message_size, misc_exception, "trying to send too large of message");
   char buff[sz];
   fc::datastream<char*> ds(buff, sz);
   fc::raw::pack(ds, message);

   struct iovec io = {
      .iov_base = buff,
      .iov_len = sz
   };
   union {
      char buf[CMSG_SPACE(4 * sizeof(int))]; //XXX this code supports a max of 4, so it should probably throw/assert otherwise
      struct cmsghdr align;
   } u;

   msg.msg_iov = &io;
   msg.msg_iovlen = 1;
   if(fds.size()) {
      msg.msg_control = u.buf;
      msg.msg_controllen = sizeof(u.buf);
      cmsg = CMSG_FIRSTHDR(&msg);
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fds.size());
      unsigned char* p = CMSG_DATA(cmsg);
      for(const wrapped_fd& fd : fds) {
         int thisfd = fd;
         memcpy(p, &thisfd, sizeof(thisfd));
         p += sizeof(thisfd);
      }
   }

   int wrote;
   do {
      wrote = sendmsg(fd_to_send_to, &msg, 0);
   } while(wrote == -1 && errno == EINTR);
}

wrapped_fd memfd_for_vector(const std::vector<uint8_t>& bytes) {
   int fd = syscall(SYS_memfd_create, "eosvmoc_code", MFD_CLOEXEC);
   ftruncate(fd, bytes.size());
   uint8_t* b = (uint8_t*)mmap(nullptr, bytes.size(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
   memcpy(b, bytes.data(), bytes.size());
   munmap(b, bytes.size());
   return wrapped_fd(fd);
}

wrapped_fd memfd_for_blob(const shared_blob& blob) {
   int fd = syscall(SYS_memfd_create, "eosvmoc_code", MFD_CLOEXEC);
   ftruncate(fd, blob.size());
   uint8_t* b = (uint8_t*)mmap(nullptr, blob.size(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
   memcpy(b, blob.data(), blob.size());
   munmap(b, blob.size());
   return wrapped_fd(fd);
}

std::vector<uint8_t> vector_for_memfd(const wrapped_fd& memfd) {
   struct stat st;
   fstat(memfd, &st);
   uint8_t* p = (uint8_t*)mmap(nullptr, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0);
   std::vector<uint8_t> ret(p, p+st.st_size);
   munmap(p, st.st_size);
   return ret;
}

}}}