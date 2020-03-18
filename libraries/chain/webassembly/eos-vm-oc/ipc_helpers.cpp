#include <eosio/chain/webassembly/eos-vm-oc/ipc_helpers.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/asio/local/stream_protocol.hpp>

namespace eosio { namespace chain { namespace eosvmoc {

static constexpr size_t max_message_size = 8192;
static constexpr size_t max_num_fds = 4;

std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(boost::asio::local::stream_protocol::socket& s) {
   return read_message_with_fds(s.native_handle());
}

std::tuple<bool, eosvmoc_message, std::vector<wrapped_fd>> read_message_with_fds(int fd) {
   char buff;

   struct msghdr msg = {};
   struct cmsghdr* cmsg;

   eosvmoc_message message;
   std::vector<wrapped_fd> fds;

   struct iovec io = {
      .iov_base = &buff,
      .iov_len = sizeof(buff)
   };
   union {
      char buf[CMSG_SPACE(max_num_fds * sizeof(int))];
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
   if(red < 1)
      return {false, message, std::move(fds)};

   if(msg.msg_controllen) {
      cmsg = CMSG_FIRSTHDR(&msg);
      unsigned num_of_fds = (cmsg->cmsg_len - CMSG_LEN(0))/sizeof(int);
      if(num_of_fds > max_num_fds)
         return {false, message, std::move(fds)};
      int* fd_ptr = (int*)CMSG_DATA(cmsg);
      for(unsigned i = 0; i < num_of_fds; ++i)
         fds.push_back(*fd_ptr++);
   }

   if(fds.size() == 0)
      return {false, message, std::move(fds)};

   try {
      fc::raw::unpack(vector_for_memfd(fds[0]), message);
   }
   catch(...) {
      fds.clear();
      return {false, message, std::move(fds)};
   }
   fds.erase(fds.begin());

   return {true, message, std::move(fds)};
}

bool write_message_with_fds(boost::asio::local::stream_protocol::socket& s, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds) {
   return write_message_with_fds(s.native_handle(), message, fds);
}

bool write_message_with_fds(int fd_to_send_to, const eosvmoc_message& message, const std::vector<wrapped_fd>& fds) {
   struct msghdr msg = {};
   struct cmsghdr* cmsg;

   wrapped_fd message_memfd;
   try {
      message_memfd = memfd_for_bytearray(fc::raw::pack(message));
   }
   catch(...) {
      return false;
   }

   if(fds.size() > max_num_fds-1)
      return false;

   struct iovec io = {
      .iov_base = (void*)" ",
      .iov_len = 1
   };
   union {
      char buf[CMSG_SPACE(max_num_fds * sizeof(int))];
      struct cmsghdr align;
   } u;

   msg.msg_iov = &io;
   msg.msg_iovlen = 1;

#if 1
   msg.msg_control = u.buf;
   msg.msg_controllen = CMSG_SPACE((fds.size()+1)*sizeof(int));
   cmsg = CMSG_FIRSTHDR(&msg);
   cmsg->cmsg_level = SOL_SOCKET;
   cmsg->cmsg_type = SCM_RIGHTS;
   cmsg->cmsg_len = CMSG_LEN((fds.size()+1)*sizeof(int));
   unsigned char* p = CMSG_DATA(cmsg);

   int msgfd = message_memfd;
   memcpy(p, &msgfd, sizeof(msgfd));
   p += sizeof(msgfd);
   for(const wrapped_fd& fd : fds) {
      int thisfd = fd;
      memcpy(p, &thisfd, sizeof(thisfd));
      p += sizeof(thisfd);
   }
#endif
   int wrote;
   do {
      wrote = sendmsg(fd_to_send_to, &msg, 0);
   } while(wrote == -1 && errno == EINTR);

   return wrote >= 0;
}

std::vector<char> vector_for_memfd(const wrapped_fd& memfd) {
   struct stat st;
   FC_ASSERT(fstat(memfd, &st) == 0, "failed to get memfd size");

   char* p = (char*)mmap(nullptr, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0);
   FC_ASSERT(p != MAP_FAILED, "failed to map memfd");
   uint32_t* sz = (uint32_t*)p;
   std::vector<char> ret(p+sizeof(uint32_t), p+sizeof(uint32_t)+*sz);
   munmap(p, st.st_size);
   return ret;
}

}}}
