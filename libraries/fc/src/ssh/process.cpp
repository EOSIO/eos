#define NOMINMAX // prevent windows from defining min and max macros
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <fc/ssh/client.hpp>
#include <fc/ssh/process.hpp>
#include <fc/io/sstream.hpp>
#include <fc/vector.hpp>
#include <fc/thread/unique_lock.hpp>

#include "client_impl.hpp"

#if defined (_MSC_VER)
#pragma warning (disable : 4355)
#endif

namespace fc { namespace ssh {
  
  namespace detail {
    class process_impl;
    class process_istream : public fc::istream {
       public:
          process_istream( process_impl& p, int c )
          :proc(p),chan(c){}

          virtual size_t readsome( char* buf, size_t len );

          virtual bool eof() const;

          process_impl& proc;
          int           chan;
    };

    class process_ostream : public fc::ostream {
      public:
          process_ostream( process_impl& p )
          :proc(p){}

          virtual size_t writesome( const char* buf, size_t len );
          virtual void   close();
          virtual void   flush();

          process_impl& proc;
    };

    class process_impl {
      public:
        process_impl( client_ptr c );
        ~process_impl();
        //process_impl( const client& c, const fc::string& cmd, const fc::string& pty_type );
        void exec(const fc::path& exe, vector<string> args, 
                  const fc::path& work_dir /* = fc::path() */, fc::iprocess::exec_opts opts /* = open_all */);


        int read_some( char* data, size_t len, int stream_id );
        int write_some( const char* data, size_t len, int stream_id );
        void flush();
        void send_eof();

        LIBSSH2_CHANNEL*      chan;
        client_ptr sshc;
        buffered_ostream_ptr buffered_std_in;
        buffered_istream_ptr buffered_std_out;
        buffered_istream_ptr buffered_std_err;

        fc::string            command;
        fc::promise<int>::ptr result;

        fc::optional<int>     return_code;
        fc::ostring           return_signal;
        fc::ostring           return_signal_message;
    private:
        static fc::string windows_shell_escape(const fc::string& str);
        static fc::string unix_shell_escape(const fc::string& str);
        static fc::string windows_shell_escape_command(const fc::path& exe, const vector<string>& args);
        static fc::string unix_shell_escape_command(const fc::path& exe, const vector<string>& args);
    };

  } // end namespace detail


  process::process(client_ptr c) :
    my(new detail::process_impl(c))
  {}

  process::~process()
  {}

  iprocess& process::exec( const fc::path& exe, vector<string> args, 
                           const fc::path& work_dir /* = fc::path() */, exec_opts opts /* = open_all */ ) {
    my->exec(exe, args, work_dir, opts);
    return *this;
  }

  /**
   *  Blocks until the result code of the process has been returned.
   */
  int process::result() {
    if (!my->return_code && !my->return_signal) {
      // we don't have any cached exit status, so wait and obtain the values now
      my->sshc->my->call_ssh2_function(boost::bind(libssh2_channel_wait_eof, my->chan));
      my->sshc->my->call_ssh2_function_throw(boost::bind(libssh2_channel_wait_closed, my->chan),
                                                        "Error waiting on socket to close: ${message}");

      char* exit_signal;
      char* error_message;
      libssh2_channel_get_exit_signal(my->chan, &exit_signal, NULL, &error_message, NULL, NULL, NULL);
      if (exit_signal) {
        // process terminated with a signal
        my->return_signal = exit_signal;
        libssh2_free(my->chan->session, exit_signal);
        if (error_message) {
          my->return_signal_message = error_message;
          libssh2_free(my->chan->session, error_message);
        }
      } else
        my->return_code = libssh2_channel_get_exit_status(my->chan);
    }
    if (my->return_signal)
      FC_THROW("process terminated with signal ${signal}: ${signal_message}", ("signal", *my->return_signal)
                                                                              ("signal_message", my->return_signal_message ? *my->return_signal_message : ""));
    else
      return *my->return_code;
  }

  void process::kill() {
    elog("error: fc::ssh::process::kill() not supported");
  }


  /**
   *  @brief returns a stream that writes to the procss' stdin
   */
  fc::buffered_ostream_ptr process::in_stream() {
    return my->buffered_std_in;
  }

  /**
   *  @brief returns a stream that reads from the process' stdout
   */
  fc::buffered_istream_ptr process::out_stream() {
    return my->buffered_std_out;
  }

  /**
   *  @brief returns a stream that reads from the process' stderr
   */
  fc::buffered_istream_ptr process::err_stream() {
    return my->buffered_std_err;
  }

  void detail::process_impl::flush() {
      if( !chan ) return;
     /*    channel_flush deleates input buffer, and does not ensure writes go out 
      *
      int ec = libssh2_channel_flush_ex( chan, LIBSSH2_CHANNEL_FLUSH_EXTENDED_DATA);
      while( ec == LIBSSH2_ERROR_EAGAIN ) {
        sshc.my->wait_on_socket();
        ec = libssh2_channel_flush_ex( chan, LIBSSH2_CHANNEL_FLUSH_EXTENDED_DATA );
      }
      ec = libssh2_channel_flush( chan );
      while( ec == LIBSSH2_ERROR_EAGAIN ) {
        sshc.my->wait_on_socket();
        ec = libssh2_channel_flush( chan );
      }
      if( ec < 0 ) {
        FC_THROW( "ssh flush failed", ( "channel_error", ec)  );
      }
      */
  }

  int detail::process_impl::read_some( char* data, size_t len, int stream_id ){
       if( !sshc->my->session ) { FC_THROW( "Session closed" ); }
       int rc;
       char* buf = data;
       size_t buflen = len;
       do {
           rc = sshc->my->call_ssh2_function_throw(boost::bind(libssh2_channel_read_ex, chan, stream_id, buf, buflen),
                                                              "read failed: ${message}");
           if( rc > 0 ) {
              buf += rc;
              buflen -= rc;
              return buf-data;
           } else if( rc == 0 ) {
              if( libssh2_channel_eof( chan ) )
                return -1; // eof
              sshc->my->wait_on_socket();
           }
       } while( rc >= 0 && buflen);
       return buf-data;
  }

  int detail::process_impl::write_some( const char* data, size_t len, int stream_id ) {
     if( !sshc->my->session ) { FC_THROW( "Session closed" ); }

     int rc;
     const char* buf = data;
     size_t buflen = len;
     do {
         rc = sshc->my->call_ssh2_function_throw(boost::bind(libssh2_channel_write_ex, chan, stream_id, buf, buflen),
                                                            "write failed: ${message}");
         if( rc > 0 ) {
            buf += rc;
            buflen -= rc;
            return buf-data;
         } else if( rc == 0 ) {
            if( libssh2_channel_eof( chan ) )  {
              FC_THROW( "EOF" );
              //return -1; // eof
            }
         }
     } while( rc >= 0 && buflen);
     return buf-data;
  }

  void detail::process_impl::send_eof() {
    if( sshc->my->session )
      sshc->my->call_ssh2_function_throw(boost::bind(libssh2_channel_send_eof, chan),
                                                    "send eof failed: ${message}");
  }

   size_t detail::process_istream::readsome( char* buf, size_t len ) {
     int bytesRead = proc.read_some(buf, len, chan);
     if (bytesRead < 0)
       FC_THROW("EOF");
     else
       return bytesRead;
   }

   bool detail::process_istream::eof()const { 
      return 0 != libssh2_channel_eof( proc.chan );
   }

   size_t detail::process_ostream::writesome( const char* buf, size_t len ) {
     return proc.write_some(buf, len, 0);
   }

   void   detail::process_ostream::close(){
      proc.send_eof();
   }

   void   detail::process_ostream::flush(){
      proc.flush();
   }

   detail::process_impl::process_impl( client_ptr c )
     :chan(nullptr),
      sshc(c),
      buffered_std_in(new buffered_ostream(ostream_ptr(new process_ostream(*this)))),
      buffered_std_out(new buffered_istream(istream_ptr(new process_istream(*this, 0)))),
      buffered_std_err(new buffered_istream(istream_ptr(new process_istream(*this, SSH_EXTENDED_DATA_STDERR))))
   {
   }

   detail::process_impl::~process_impl() {
     if (chan) {
       sshc->my->call_ssh2_function(boost::bind(libssh2_channel_free, chan));
       chan = NULL;
     }
   }

   // these rules work pretty well for a standard bash shell on unix
   fc::string detail::process_impl::unix_shell_escape(const fc::string& str) {
     if (str.find_first_of(" ;&|><*?`$(){}[]!#'\"") == fc::string::npos)
        return str;
     fc::string escaped_quotes(str);
     for (size_t start = escaped_quotes.find("'");
          start != fc::string::npos;
          start = escaped_quotes.find("'", start + 5))
        escaped_quotes.replace(start, 1, "'\"'\"'");
     fc::string escaped_str("\'");
     escaped_str += escaped_quotes;
     escaped_str += "\'";
     return escaped_str;
   }
   fc::string detail::process_impl::unix_shell_escape_command(const fc::path& exe, const vector<string>& args) {
     fc::stringstream command_line;
     command_line << unix_shell_escape(exe.string());
     for (unsigned i = 0; i < args.size(); ++i)
       command_line << " " << unix_shell_escape(args[i]);
     return command_line.str();
   }

   // windows command-line escaping rules are a disaster, partly because how the command-line is 
   // parsed depends on what program you're running.  In windows, the command line is passed in
   // as a single string, and the process is left to interpret it as it sees fit.  The standard
   // C runtime uses one set of rules, the function CommandLineToArgvW usually used by
   // GUI-mode programs uses a different set. 
   // Here we try to find a common denominator that works well for simple cases
   // it's only minimally tested right now due to time constraints.
   fc::string detail::process_impl::windows_shell_escape(const fc::string& str) {
     if (str.find_first_of(" \"") == fc::string::npos)
        return str;
     fc::string escaped_quotes(str);
     for (size_t start = escaped_quotes.find("\"");
          start != fc::string::npos;
          start = escaped_quotes.find("\"", start + 2))
        escaped_quotes.replace(start, 1, "\\\"");
     fc::string escaped_str("\"");
     escaped_str += escaped_quotes;
     escaped_str += "\"";
     return escaped_str;
   }
   fc::string detail::process_impl::windows_shell_escape_command(const fc::path& exe, const vector<string>& args) {
     fc::stringstream command_line;
     command_line << windows_shell_escape(exe.string());
     for (unsigned i = 0; i < args.size(); ++i)
       command_line << " " << windows_shell_escape(args[i]);
     return command_line.str();
   }

   void detail::process_impl::exec(const fc::path& exe, vector<string> args, 
                                   const fc::path& work_dir /* = fc::path() */, 
                                   fc::iprocess::exec_opts opts /* = open_all */) { 
        chan = sshc->my->open_channel(""); 

        sshc->my->call_ssh2_function(boost::bind(libssh2_channel_handle_extended_data2, chan, LIBSSH2_CHANNEL_EXTENDED_DATA_NORMAL));

        try {
          fc::scoped_lock<fc::mutex> process_startup_lock(sshc->my->process_startup_mutex);
          fc::string command_line = sshc->my->remote_system_is_windows ? windows_shell_escape_command(exe, args) : unix_shell_escape_command(exe, args);
          sshc->my->call_ssh2_function_throw(boost::bind(libssh2_channel_process_startup, chan, "exec", sizeof("exec") - 1, command_line.c_str(), command_line.size()),
					    "exec failed: ${message}"); // equiv to libssh2_channel_exec(chan, cmd) macro
        } catch (fc::exception& er) {
           elog( "error starting process" );
           FC_RETHROW_EXCEPTION(er, error, "error starting process");
        }
   }

} }
