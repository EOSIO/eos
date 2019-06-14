#include <signal.h>
#include <sys/prctl.h>

#include <eosio/chain/webassembly/eos-vm-oc/ipc_protocol.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/compile_monitor.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/ipc_helpers.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/compile_trampoline.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>

#include <eosio/chain/exceptions.hpp>

#include <boost/asio/local/datagram_protocol.hpp>
#include <boost/signals2.hpp>

namespace eosio { namespace chain { namespace eosvmoc {

using namespace boost::asio;

static size_t get_size_of_fd(int fd) {
   struct stat st;
   fstat(fd, &st); //XXX
   return st.st_size;
}

static void copy_memfd_contents_to_pointer(void* dst, int fd) {
   struct stat st;
   fstat(fd, &st); //XXX
   void* contents = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0); //XXX
   memcpy(dst, contents, st.st_size);
   munmap(contents, st.st_size);
}

struct compile_monitor_session {
   compile_monitor_session(boost::asio::io_context& context, local::datagram_protocol::socket&& n, wrapped_fd&& c, wrapped_fd& t) :
      _ctx(context),
      _nodeos_instance_socket(std::move(n)),
      _cache_fd(std::move(c)),
      _trampoline_socket(t) {

      ///XXX we should throw if any problems ctoring

      struct stat st;
      fstat(_cache_fd, &st);
      _code_size = st.st_size;
      _code_mapping = (char*)mmap(nullptr, _code_size, PROT_READ|PROT_WRITE, MAP_SHARED, _cache_fd, 0);
      _allocator = reinterpret_cast<allocator_t*>(_code_mapping);
      
      read_message_from_nodeos();
   }

   ~compile_monitor_session() {
      munmap(_code_mapping, _code_size);
   }
   
   void read_message_from_nodeos() {
      _nodeos_instance_socket.async_wait(local::datagram_protocol::socket::wait_read, [this](auto ec) {
         if(ec) {
            connection_dead_signal();
            return;
         }
         auto [success, message, fds] = read_message_with_fds(_nodeos_instance_socket);
         if(!success) {
            connection_dead_signal();
            return;
         }

         message.visit(overloaded {
            [&, &fds=fds](const compile_wasm_message& compile) {
               ///XXX verify there is 1 fd
               kick_compile_off(compile.code, std::move(fds[0]));
            },
            [&](const evict_wasms_message& evict) {
               ///XXX init mem later
               for(const code_descriptor& cd : evict.codes)
                  _allocator->deallocate(_code_mapping + cd.code_begin);
            },
            [&](const auto&) {
               //anything else is an error
               connection_dead_signal();
               return;
            }
         });

         read_message_from_nodeos();
      });
   }

   void kick_compile_off(const code_tuple& code_id, wrapped_fd&& wasm_code) {
      //prepare a requst to go out to the trampoline
      int socks[2];
      socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, socks);
      local::datagram_protocol::socket response_socket(_ctx);
      response_socket.assign(local::datagram_protocol(), socks[0]);
      std::vector<wrapped_fd> fds_pass_to_trampoline;
      fds_pass_to_trampoline.emplace_back(socks[1]);
      fds_pass_to_trampoline.emplace_back(std::move(wasm_code));

      eosvmoc_message trampoline_compile_request = compile_wasm_message{code_id};
      write_message_with_fds(_trampoline_socket, trampoline_compile_request, fds_pass_to_trampoline);

      current_compiles.emplace_front(code_id, std::move(response_socket));
      read_message_from_compile_task(current_compiles.begin());
   }

   void read_message_from_compile_task(std::list<std::tuple<code_tuple, local::datagram_protocol::socket>>::iterator current_compile_it) {
      auto& [code, socket] = *current_compile_it;
      socket.async_wait(local::datagram_protocol::socket::wait_read, [this, current_compile_it](auto ec) {
         //at this point we only expect 1 of 2 things to happen: we either get a reply (success), or we get no reply (failure)
         auto& [code, socket] = *current_compile_it;
         auto [success, message, fds] = read_message_with_fds(socket);
         eosvmoc_message reply;
         if(!success) {
            reply = wasm_compilation_result_message{code, compilation_result_unknownfailure{}, _allocator->get_free_memory()};
         }
         else {
            //XXX code_compilation_result_message, we should have 2 fds: code & initialmem
            message.visit(overloaded {
               [&, &fds=fds, &code=code](const code_compilation_result_message& result) {
                  ///XXX verify there are 2 fds
                  void* code_ptr = _allocator->allocate(get_size_of_fd(fds[0]));
                  void* mem_ptr = _allocator->allocate(get_size_of_fd(fds[1]));
                  if(code_ptr == nullptr || mem_ptr == nullptr) {
                     _allocator->deallocate(code_ptr);
                     _allocator->deallocate(mem_ptr);
                     reply = wasm_compilation_result_message{code, compilation_result_toofull{}, _allocator->get_free_memory()};
                     return;
                  }
                  ///XXX check if either is NULL and back out
                  copy_memfd_contents_to_pointer(code_ptr, fds[0]);
                  copy_memfd_contents_to_pointer(mem_ptr, fds[1]);

                  reply = wasm_compilation_result_message{code, code_descriptor {
                        code.code_id,
                        code.vm_version,
                        0,
                        (uintptr_t)code_ptr - (uintptr_t)_code_mapping,
                        result.start,
                        result.apply_offset,
                        result.starting_memory_pages,
                        (uintptr_t)mem_ptr - (uintptr_t)_code_mapping,
                        (unsigned)get_size_of_fd(fds[1]),
                        result.initdata_prolouge_size
                     },
                     _allocator->get_free_memory()
                  };
               },
               [&, &code=code](const auto&) {
                  //XXX anything else is an error
                  reply = wasm_compilation_result_message{code, compilation_result_unknownfailure{}, _allocator->get_free_memory()};
               }
            });
         }

         write_message_with_fds(_nodeos_instance_socket, reply);

         //either way, we are done
         _ctx.post([this, current_compile_it]() {
            current_compiles.erase(current_compile_it);
         });
      });

   }

   boost::signals2::signal<void()> connection_dead_signal;

private:
   boost::asio::io_context& _ctx;
   local::datagram_protocol::socket _nodeos_instance_socket;
   wrapped_fd  _cache_fd;
   wrapped_fd& _trampoline_socket;

   char* _code_mapping;
   size_t _code_size;
   allocator_t* _allocator;

   std::list<std::tuple<code_tuple, local::datagram_protocol::socket>> current_compiles;
};

struct compile_monitor {
   compile_monitor(boost::asio::io_context& ctx, local::datagram_protocol::socket&& n, wrapped_fd&& t) : _nodeos_socket(std::move(n)), _trampoline_socket(std::move(t)) {
      //the only duty of compile_monitor is to create a compile_monitor_session when a code_cache instance
      // in nodeos wants one
      wait_for_new_incomming_session(ctx);
   }

   void wait_for_new_incomming_session(boost::asio::io_context& ctx) {
      _nodeos_socket.async_wait(boost::asio::local::datagram_protocol::socket::wait_read, [this, &ctx](auto ec) {
         if(ec) {
            ctx.stop();
            return;
         }
         auto [success, message, fds] = read_message_with_fds(_nodeos_socket);
         if(!success) {
            ctx.stop();
            return;
         }
         message.visit(overloaded {
            [&, &fds=fds](const initialize_message& init) {
               ///XXX not 2 fds, error
               local::datagram_protocol::socket _socket_for_comm(ctx);
               _socket_for_comm.assign(local::datagram_protocol(), fds[0].release());
               _compile_sessions.emplace_front(ctx, std::move(_socket_for_comm), std::move(fds[1]), _trampoline_socket);
               _compile_sessions.front().connection_dead_signal.connect([&, it = _compile_sessions.begin()]() {
                  ctx.post([&]() {
                     _compile_sessions.erase(it);
                  });
               });
               write_message_with_fds(_nodeos_socket, initalize_response_message());
            },
            [&](const auto&) {
               //anything else is an error
            }
         });

         wait_for_new_incomming_session(ctx);
      });
   }

   local::datagram_protocol::socket _nodeos_socket;
   wrapped_fd _trampoline_socket;

   std::list<compile_monitor_session> _compile_sessions;
};

void launch_compile_monitor(int nodeos_fd) {
   prctl(PR_SET_NAME, "oc-monitor");
   prctl(PR_SET_PDEATHSIG, SIGKILL);

   //first off, let's disable shutdown signals to us; we want all shutdown indicators to come from
   // nodeos shutting us down
   sigset_t set;
   sigemptyset(&set);
   sigaddset(&set, SIGHUP);
   sigaddset(&set, SIGTERM);
   sigaddset(&set, SIGPIPE);
   sigaddset(&set, SIGINT);
   sigaddset(&set, SIGQUIT);
   sigprocmask(SIG_BLOCK, &set, nullptr);

   int socks[2]; //0: local trampoline socket, 1: the one we give to trampoline
   socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, socks);
   pid_t child = fork();
   if(child == 0) {
      close(socks[0]);
      run_compile_trampoline(socks[1]);
   }
   close(socks[1]);

   {
      boost::asio::io_context ctx;
      boost::asio::local::datagram_protocol::socket nodeos_socket(ctx);
      nodeos_socket.assign(boost::asio::local::datagram_protocol(), nodeos_fd);
      wrapped_fd trampoline_socket(socks[0]);
      compile_monitor monitor(ctx, std::move(nodeos_socket), std::move(trampoline_socket));
      ctx.run();
      if(monitor._compile_sessions.size())
         std::cerr << "ERROR: EOS-VM OC compiler monitor exiting with active sessions" << std::endl;
   }
   
   _exit(0);
}

struct compile_monitor_trampoline {
   void start() {
      //create communication socket; let's hold off on asio usage until all forks are done
      int socks[2];
      socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, socks);
      compile_manager_fd = socks[0];

      pid_t child = fork();
      if(child == 0) {
         close(socks[0]);
         launch_compile_monitor(socks[1]);
      }
      close(socks[1]);
   }

   pid_t compile_manager_pid;
   int compile_manager_fd;
};

static compile_monitor_trampoline the_compile_monitor_trampoline;
void start_compile_monitor() {
   the_compile_monitor_trampoline.start();
}

wrapped_fd get_connection_to_compile_monitor(int cache_fd) {
   int socks[2]; //0: our socket to compile_manager_session, 1: socket we'll give to compile_maanger_session
   socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, socks);
   wrapped_fd socket_to_monitor_session(socks[0]);

   std::vector<wrapped_fd> fds_to_pass; 
   fds_to_pass.emplace_back(socks[1]);
   fds_to_pass.emplace_back(cache_fd);  //put cache_fd in to a wrapped_fd just temporarily, ick
   write_message_with_fds(the_compile_monitor_trampoline.compile_manager_fd, initialize_message(), fds_to_pass);
   fds_to_pass[1].release();

   auto [success, message, fds] = read_message_with_fds(the_compile_monitor_trampoline.compile_manager_fd);
   EOS_ASSERT(success, misc_exception, "failed to read response from monitor process");
   EOS_ASSERT(message.contains<initalize_response_message>(), misc_exception, "unexpected response from monitor process");
   EOS_ASSERT(!message.get<initalize_response_message>().error_message, misc_exception, "Error message from monitor process: ${e}", ("e", *message.get<initalize_response_message>().error_message));
   return socket_to_monitor_session;
}

}}}