#define NOMINMAX
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <boost/asio.hpp>

#include <fc/ssh/client.hpp>
#include <fc/ssh/process.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/spin_lock.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/log/logger.hpp>

#include <fc/asio.hpp>

// include this to get acess to the details of the LIBSSH2_SESSION structure, so
// we can verify that all data has really been sent when libssh2 says it has.
#include <../src/libssh2_priv.h>

namespace fc { namespace ssh {
  
  namespace detail {

    class client_impl {
      public:
        client_impl();
        ~client_impl();

        LIBSSH2_SESSION*            session;
        LIBSSH2_KNOWNHOSTS*         knownhosts;
        LIBSSH2_SFTP*               sftp;
        LIBSSH2_AGENT*              agent;

        std::unique_ptr<boost::asio::ip::tcp::socket> sock;
        boost::asio::ip::tcp::endpoint                endpt;

        fc::mutex                   ssh_session_mutex;
        fc::mutex                   channel_open_mutex;
        fc::mutex                   process_startup_mutex;
        fc::mutex                   scp_send_mutex;
        fc::mutex                   scp_stat_mutex;
        fc::mutex                   scp_mkdir_mutex;
        fc::mutex                   scp_rmdir_mutex;
        fc::mutex                   scp_unlink_mutex;
        fc::mutex                   scp_close_mutex;
        fc::mutex                   scp_readdir_mutex;
        fc::mutex                   scp_open_mutex;

        fc::string                  uname;
        fc::string                  upass;
        fc::string                  pubkey;
        fc::string                  privkey;
        fc::string                  passphrase;
        fc::string                  hostname;
        uint16_t                    port;
        bool                        session_connected;
        fc::promise<boost::system::error_code>::ptr      read_prom;
        fc::promise<boost::system::error_code>::ptr      write_prom;
        fc::spin_lock                                    _spin_lock;
        int    _trace_level;
        logger logr;

        bool                        remote_system_is_windows; // true if windows, false if unix, used for command-line quoting and maybe filename translation

        LIBSSH2_CHANNEL*   open_channel( const fc::string& pty_type );
        static void kbd_callback(const char *name, int name_len, 
                     const char *instruction, int instruction_len, int num_prompts,
                     const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                     LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                     void **abstract);

        void connect();
        
        static void handle_trace( LIBSSH2_SESSION* session, void* context, const char* data, size_t length );

        void close();
        void authenticate();

        bool try_pass();
        bool try_keyboard();
        bool try_pub_key();

        // don't call this "unlocked" version directly
        template <typename T>
        int call_ssh2_function_unlocked(const T& lambda, bool check_for_errors = true);

        // calls into libssh2, waits and retries the function if we get LIBSSH2_ERROR_EAGAIN
        template <typename T>
        int call_ssh2_function(const T& lambda, bool check_for_errors = true);

        // calls into libssh2, waits and retries the function if we get LIBSSH2_ERROR_EAGAIN
        // if libssh2 returns an error, get extended info and throw a message with ${code} and ${message} 
        // set appropriately.
        template <typename T>
        int call_ssh2_function_throw(const T& lambda, const char* message = "libssh2 call failed ${code} - ${message}", bool check_for_errors = true);

        // this version is a little different, it handles functions like libssh2_sftp_init which return 
        // a pointer instead of an int.  These retry automatically if the result is NULL and the error
        // is LIBSSH2_ERROR_EAGAIN
        template <typename return_type>
        return_type call_ssh2_ptr_function_throw(std::function<return_type()> lambda, const char* message = "libssh2 call failed ${code} - ${message}", bool check_for_errors = true);

        void wait_on_socket(int additionalDirections = 0);

        void init_sftp();
    };


    // #define OLD_BLOCKING, 
    // the OLD_BLOCKING version of these functions will ensure that if a libssh2 function returns 
    // LIBSSH2_ERROR_EAGAIN, no other libssh2 functions will be called until that function has been
    // called again and returned some other value.
    //
    // if you don't define this and use the new version of this, we will release the lock and let
    // other libssh2 functions be called *unless* it appears that there was unwritten data.
    //
    // the OLD_BLOCKING version is too conservative -- if you try to read on a channel that doesn't
    // have any data, you're likely to deadlock.  The new version is not heavily tested and may be 
    // too lax, time will tell.
#ifdef OLD_BLOCKING
    // don't call this "unlocked" version directly
    template <typename T>
    int client_impl::call_ssh2_function_unlocked(const T& lambda, bool check_for_errors /* = true */) {
      int ec = lambda();
      while (ec == LIBSSH2_ERROR_EAGAIN ) {
        wait_on_socket();
        ec = lambda();
      }

      // this assert catches bugs in libssh2 if libssh2 returns ec != LIBSSH2_ERROR_EAGAIN
      // but the internal session data indicates a data write is still in progress
      // set check_for_errors to false when closing the connection
      assert(!check_for_errors || !session->packet.olen);
	  
      return ec;
    }

    // calls into libssh2, waits and retries the function if we get LIBSSH2_ERROR_EAGAIN
    template <typename T>
    int client_impl::call_ssh2_function(const T& lambda, bool check_for_errors /* = true */) {
      fc::scoped_lock<fc::mutex> lock(ssh_session_mutex);
      return call_ssh2_function_unlocked(lambda, check_for_errors);
    }
#else
    // calls into libssh2, waits and retries the function if we get LIBSSH2_ERROR_EAGAIN
    template <typename T>
    int client_impl::call_ssh2_function(const T& lambda, bool check_for_errors /* = true */) {
      fc::unique_lock<fc::mutex> lock(ssh_session_mutex);
      int ec = lambda();
      while (ec == LIBSSH2_ERROR_EAGAIN) {
        bool unlock_to_wait = !session->packet.olen;
        if (unlock_to_wait)
          lock.unlock();
        wait_on_socket();
        if (unlock_to_wait)
          lock.lock();
        ec = lambda();
      }
      // this assert catches bugs in libssh2 if libssh2 returns ec != LIBSSH2_ERROR_EAGAIN
      // but the internal session data indicates a data write is still in progress
      // set check_for_errors to false when closing the connection
      assert(!check_for_errors || !session->packet.olen);
      return ec;
    }
#endif

#ifdef OLD_BLOCKING
    // calls into libssh2, waits and retries the function if we get LIBSSH2_ERROR_EAGAIN
    // if libssh2 returns an error, get extended info and throw a message with ${code} and ${message} 
    // set appropriately.
    template <typename T>
    int client_impl::call_ssh2_function_throw(const T& lambda, const char* message /* = "libssh2 call failed ${code} - ${message}" */, bool check_for_errors /* = true */) {
      fc::scoped_lock<fc::mutex> lock(ssh_session_mutex);
      int ec = call_ssh2_function_unlocked(lambda, check_for_errors);

      if (ec == LIBSSH2_ERROR_SFTP_PROTOCOL && sftp) {
        ec = libssh2_sftp_last_error(sftp);
        FC_THROW(message, ("code", ec).set("message", "SFTP protocol error"));
      } else if( ec < 0 ) {
        char* msg = 0;
        ec = libssh2_session_last_error( session, &msg, 0, 0 );
        FC_THROW(message, ("code",ec).set("message",msg));
      }
      return ec;
    }
#else
    // calls into libssh2, waits and retries the function if we get LIBSSH2_ERROR_EAGAIN
    // if libssh2 returns an error, get extended info and throw a message with ${code} and ${message} 
    // set appropriately.
    template <typename T>
    int client_impl::call_ssh2_function_throw(const T& lambda, const char* message /* = "libssh2 call failed ${code} - ${message}" */, bool check_for_errors /* = true */) {
      fc::unique_lock<fc::mutex> lock(ssh_session_mutex);
      int ec = lambda();
      while (ec == LIBSSH2_ERROR_EAGAIN) {
        bool unlock_to_wait = !session->packet.olen;
        if (unlock_to_wait)
          lock.unlock();
        wait_on_socket();
        if (unlock_to_wait)
          lock.lock();
        ec = lambda();
      }
      // this assert catches bugs in libssh2 if libssh2 returns ec != LIBSSH2_ERROR_EAGAIN
      // but the internal session data indicates a data write is still in progress
      // set check_for_errors to false when closing the connection
      assert(!check_for_errors || !session->packet.olen);

      if (ec == LIBSSH2_ERROR_SFTP_PROTOCOL && sftp) {
        ec = libssh2_sftp_last_error(sftp);
        FC_THROW(message, ("code", ec).set("message", "SFTP protocol error"));
      } else if( ec < 0 ) {
        char* msg = 0;
        ec = libssh2_session_last_error( session, &msg, 0, 0 );
        FC_THROW(message, ("code",ec).set("message",msg));
      }
      return ec;
    }
#endif

#ifdef OLD_BLOCKING
    // this version is a little different, it handles functions like libssh2_sftp_init which return 
    // a pointer instead of an int.  These retry automatically if the result is NULL and the error
    // is LIBSSH2_ERROR_EAGAIN
    template <typename return_type>
    return_type client_impl::call_ssh2_ptr_function_throw(std::function<return_type()> lambda, const char* message /* = "libssh2 call failed ${code} - ${message}" */, bool check_for_errors /* = true */) {
      fc::scoped_lock<fc::mutex> lock(ssh_session_mutex);
      return_type ret = lambda();
      while (!ret) {
        char* msg = 0;
        int   ec = libssh2_session_last_error(session,&msg,NULL,0);
        if ( ec == LIBSSH2_ERROR_EAGAIN ) {
          wait_on_socket();
          ret = lambda();
        } else if (ec == LIBSSH2_ERROR_SFTP_PROTOCOL && sftp) {
          ec = libssh2_sftp_last_error(sftp);
          FC_THROW(message, ("code", ec).set("message", "SFTP protocol error"));
        } else {
          ec = libssh2_session_last_error( session, &msg, 0, 0 );
          FC_THROW(message, ("code",ec).set("message",msg));
        }
      }
      assert(!check_for_errors || !session->packet.olen);

      return ret;
    }
#else
    // this version is a little different, it handles functions like libssh2_sftp_init which return 
    // a pointer instead of an int.  These retry automatically if the result is NULL and the error
    // is LIBSSH2_ERROR_EAGAIN
    template <typename return_type>
    return_type client_impl::call_ssh2_ptr_function_throw(std::function<return_type()> lambda, const char* message /* = "libssh2 call failed ${code} - ${message}" */, bool check_for_errors /* = true */) {
      fc::unique_lock<fc::mutex> lock(ssh_session_mutex);
      return_type ret = lambda();
      while (!ret) {
        char* msg = 0;
        int   ec = libssh2_session_last_error(session,&msg,NULL,0);
        if ( ec == LIBSSH2_ERROR_EAGAIN ) {
          bool unlock_to_wait = !session->packet.olen;
          if (unlock_to_wait)
            lock.unlock();
          wait_on_socket();
          if (unlock_to_wait)
            lock.lock();
          ret = lambda();
        } else if (ec == LIBSSH2_ERROR_SFTP_PROTOCOL && sftp) {
          ec = libssh2_sftp_last_error(sftp);
          FC_THROW(message, ("code", ec).set("message", "SFTP protocol error"));
        } else {
          ec = libssh2_session_last_error( session, &msg, 0, 0 );
          FC_THROW(message, ("code",ec).set("message",msg));
        }
      }
      assert(!check_for_errors || !session->packet.olen);

      return ret;
    }
#endif
  }

} }
