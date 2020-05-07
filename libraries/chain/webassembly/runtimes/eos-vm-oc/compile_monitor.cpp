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
   FC_ASSERT(fstat(fd, &st) == 0, "failed to get size of fd");
   return st.st_size;
}

static void copy_memfd_contents_to_pointer(void* dst, int fd) {
   struct stat st;
   FC_ASSERT(fstat(fd, &st) == 0, "failed to get size of fd");
   if(st.st_size == 0)
      return;
   void* contents = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
   FC_ASSERT(contents != MAP_FAILED, "failed to map memfd file");
   memcpy(dst, contents, st.st_size);
   munmap(contents, st.st_size);
}

struct compile_monitor_session {
   compile_monitor_session(boost::asio::io_context& context, local::datagram_protocol::socket&& n, wrapped_fd&& c, wrapped_fd& t) :
      _ctx(context),
      _nodeos_instance_socket(std::move(n)),
      _cache_fd(std::move(c)),
      _trampoline_socket(t) {

      struct stat st;
      FC_ASSERT(fstat(_cache_fd, &st) == 0, "failed to stat cache fd");
      _code_size = st.st_size;
      _code_mapping = (char*)mmap(nullptr, _code_size, PROT_READ|PROT_WRITE, MAP_SHARED, _cache_fd, 0);
      FC_ASSERT(_code_mapping != MAP_FAILED, "failed to mmap cache file");
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
               if(fds.size() != 1) {
                  connection_dead_signal();
                  return;
               }
               kick_compile_off(compile.code, std::move(fds[0]));
            },
            [&](const evict_wasms_message& evict) {
               for(const code_descriptor& cd : evict.codes) {
                  _allocator->deallocate(_code_mapping + cd.code_begin);
                  _allocator->deallocate(_code_mapping + cd.initdata_begin);
               }
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
      if(write_message_with_fds(_trampoline_socket, trampoline_compile_request, fds_pass_to_trampoline) == false) {
         wasm_compilation_result_message reply{code_id, compilation_result_unknownfailure{}, _allocator->get_free_memory()};
         write_message_with_fds(_nodeos_instance_socket, reply);
         return;
      }

      current_compiles.emplace_front(code_id, std::move(response_socket));
      read_message_from_compile_task(current_compiles.begin());
   }

   void read_message_from_compile_task(std::list<std::tuple<code_tuple, local::datagram_protocol::socket>>::iterator current_compile_it) {
      auto& [code, socket] = *current_compile_it;
      socket.async_wait(local::datagram_protocol::socket::wait_read, [this, current_compile_it](auto ec) {
         //at this point we only expect 1 of 2 things to happen: we either get a reply (success), or we get no reply (failure)
         auto& [code, socket] = *current_compile_it;
         auto [success, message, fds] = read_message_with_fds(socket);
         
         wasm_compilation_result_message reply{code, compilation_result_unknownfailure{}, _allocator->get_free_memory()};
         
         void* code_ptr = nullptr;
         void* mem_ptr = nullptr;
         try {
            if(success && message.contains<code_compilation_result_message>() && fds.size() == 2) {
               code_compilation_result_message& result = message.get<code_compilation_result_message>();
               code_ptr = _allocator->allocate(get_size_of_fd(fds[0]));
               mem_ptr = _allocator->allocate(get_size_of_fd(fds[1]));

               if(code_ptr == nullptr || mem_ptr == nullptr) {
                  _allocator->deallocate(code_ptr);
                  _allocator->deallocate(mem_ptr);
                  reply.result = compilation_result_toofull();
               }
               else {
                  copy_memfd_contents_to_pointer(code_ptr, fds[0]);
                  copy_memfd_contents_to_pointer(mem_ptr, fds[1]);

                  reply.result = code_descriptor {
                     code.code_id,
                     code.vm_version,
                     0,
                     (uintptr_t)code_ptr - (uintptr_t)_code_mapping,
                     result.start,
                     result.apply_offset,
                     result.starting_memory_pages,
                     (uintptr_t)mem_ptr - (uintptr_t)_code_mapping,
                     (unsigned)get_size_of_fd(fds[1]),
                     result.initdata_prologue_size
                  };
               }
            }
         }
         catch(...) {
            _allocator->deallocate(code_ptr);
            _allocator->deallocate(mem_ptr);
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
         if(!success) {   //failure reading indicates that nodeos has shut down
            ctx.stop();
            return;
         }
         if(!message.contains<initialize_message>() || fds.size() != 2) {
            ctx.stop();
            return;
         }
         try {
            local::datagram_protocol::socket _socket_for_comm(ctx);
            _socket_for_comm.assign(local::datagram_protocol(), fds[0].release());
            _compile_sessions.emplace_front(ctx, std::move(_socket_for_comm), std::move(fds[1]), _trampoline_socket);
            _compile_sessions.front().connection_dead_signal.connect([&, it = _compile_sessions.begin()]() {
               ctx.post([&]() {
                  _compile_sessions.erase(it);
               });
            });
            write_message_with_fds(_nodeos_socket, initalize_response_message());
         }
         catch(const fc::exception& e) {
            write_message_with_fds(_nodeos_socket, initalize_response_message{e.what()});
         }
         catch(...) {
            write_message_with_fds(_nodeos_socket, initalize_response_message{"Failed to create compile process"});
         }

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

   struct sigaction sa;
   sa.sa_handler =  SIG_DFL;
   sa.sa_flags = SA_NOCLDWAIT;
   sigaction(SIGCHLD, &sa, nullptr);

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
         std::cerr << "ERROR: EOS VM OC compiler monitor exiting with active sessions" << std::endl;
   }
   
   _exit(0);
}

struct compile_monitor_trampoline {
   void start() {
      //create communication socket; let's hold off on asio usage until all forks are done
      int socks[2];
      socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, socks);
      compile_manager_fd = socks[0];

      compile_manager_pid = fork();
      if(compile_manager_pid == 0) {
         close(socks[0]);
         launch_compile_monitor(socks[1]);
      }
      close(socks[1]);
   }

   pid_t compile_manager_pid = -1;
   int compile_manager_fd = -1;
};

static compile_monitor_trampoline the_compile_monitor_trampoline;
extern "C" int __real_main(int, char*[]);
extern "C" int __wrap_main(int argc, char* argv[]) {


   the_compile_monitor_trampoline.start();
   return __real_main(argc, argv);
}

wrapped_fd get_connection_to_compile_monitor(int cache_fd) {
   FC_ASSERT(the_compile_monitor_trampoline.compile_manager_pid >= 0, "EOS VM oop connection doesn't look active");

   int socks[2]; //0: our socket to compile_manager_session, 1: socket we'll give to compile_maanger_session
   socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, socks);
   wrapped_fd socket_to_monitor_session(socks[0]);
   wrapped_fd socket_to_hand_to_monitor_session(socks[1]);

   //we don't own cache_fd, so try to be extra careful not to accidentally close it: don't stick it in a wrapped_fd
   // to hand off to write_message_with_fds even temporarily. make a copy of it.
   int dup_of_cache_fd = dup(cache_fd);
   FC_ASSERT(dup_of_cache_fd != -1, "failed to dup cache_fd");
   wrapped_fd dup_cache_fd(dup_of_cache_fd);

   std::vector<wrapped_fd> fds_to_pass; 
   fds_to_pass.emplace_back(std::move(socket_to_hand_to_monitor_session));
   fds_to_pass.emplace_back(std::move(dup_cache_fd));
   write_message_with_fds(the_compile_monitor_trampoline.compile_manager_fd, initialize_message(), fds_to_pass);

   auto [success, message, fds] = read_message_with_fds(the_compile_monitor_trampoline.compile_manager_fd);
   EOS_ASSERT(success, misc_exception, "failed to read response from monitor process");
   EOS_ASSERT(message.contains<initalize_response_message>(), misc_exception, "unexpected response from monitor process");
   EOS_ASSERT(!message.get<initalize_response_message>().error_message, misc_exception, "Error message from monitor process: ${e}", ("e", *message.get<initalize_response_message>().error_message));
   return socket_to_monitor_session;
}

}}}
