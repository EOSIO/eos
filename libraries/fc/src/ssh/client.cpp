#define NOMINMAX // prevent windows from defining min and max macros
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <memory>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <fc/filesystem.hpp>
#include <fc/ssh/client.hpp>
#include <fc/ssh/process.hpp>
#include <fc/time.hpp>
#include <fc/io/iostream.hpp>
#include <fc/thread/thread.hpp>
#include <fc/vector.hpp>
#include <fc/interprocess/file_mapping.hpp>
#include <fc/thread/unique_lock.hpp>
#include <fc/asio.hpp>

#include "client_impl.hpp"

namespace fc { namespace ssh {
  
  namespace detail {
    static int ssh_init = libssh2_init(0);
  }

  client::client():my( new detail::client_impl() ){ (void)detail::ssh_init; /* fix unused warning...*/ }
  client::~client() { my->close(); }

  void client::set_trace_level( int bitmask ) { my->_trace_level = bitmask; }
  int  client::get_trace_level()const         { return my->_trace_level;    }
  const logger& client::get_logger()const     { return my->logr;           }
  void client::set_logger( const logger& l )  { my->logr = l;               }

  void client::connect( const fc::string& user, const fc::string& host, uint16_t port ) {
       my->hostname = host;
       my->uname    = user;
       my->port     = port;
       my->connect();
  }
  void client::connect( const fc::string& user, const fc::string& pass, 
                        const fc::string& host, uint16_t port  ) {
       my->hostname = host;
       my->uname    = user;
       my->upass    = pass;
       my->port     = port;

       my->connect();
  }

  void client::close() { my->close(); }


//  ssh::process client::exec( const fc::string& cmd, const fc::string& pty_type  ) {
//    return ssh::process( *this, cmd, pty_type ); 
//  }

  /**
   *  @todo implement progress reporting.
   */
  void client::scp_send_dir( const fc::path& local_dir, const fc::path& remote_path, 
                              std::function<bool(uint64_t,uint64_t)> progress )
  {
      fc::path remote_dir = remote_path;
      if( remote_dir.filename() == fc::path(".") ) 
         remote_dir /= local_dir.filename();

      fc_dlog( my->logr, "scp -r ${local} ${remote}", ("local",local_dir)("remote",remote_dir) );
      create_directories( remote_dir );

      directory_iterator ditr(local_dir);
      directory_iterator dend;

      while( ditr != dend ) {
          if( (*ditr).filename() == "."  ||
              (*ditr).filename() == ".." )
          { } 
          else if( fc::is_directory(*ditr) )
          {
             scp_send_dir( (*ditr), remote_dir / (*ditr).filename() );
          } else if( fc::is_regular_file(*ditr) ) {
             scp_send( *ditr, remote_dir / (*ditr).filename() );
          } else {
             fc_wlog( my->logr, "Skipping '${path}", ("path",fc::canonical(*ditr)) );
          }
          ++ditr;
      }
  }

  void client::scp_send( const fc::path& local_path, const fc::path& remote_path, 
                        std::function<bool(uint64_t,uint64_t)> progress ) {
    fc_wlog( my->logr, "scp ${local} ${remote}", ("local",local_path)("remote",remote_path ) );
    if( !fc::exists(local_path) ) {
      FC_THROW( "Source file '${file}' does not exist", ("file",local_path) ) ;
    }
    if( is_directory( local_path ) ) {
      FC_THROW( "Source file '${file}' is a directory, expected a file", ("file",local_path) ) ;
    }

    // memory map the file
    uint64_t fsize = file_size(local_path);
    if( fsize == 0 ) {
        elog( "file size ${file_size}", ("file_size", fsize) );
        // TODO: handle empty file case
        if( progress ) progress(0,0);
        return;
    }
    file_mapping fmap( local_path.string().c_str(), read_only );

    LIBSSH2_CHANNEL*                      chan = 0;
    time_t now;
    memset( &now, 0, sizeof(now) );

    // TODO: preserve creation / modification date
    // TODO: perserve permissions / exec bit?
    try {
      // libssh2_scp_send64 stores state data in the session object, and it calls channel_open which 
      // stores its own state data, so lock both.
      fc::scoped_lock<fc::mutex> channel_open_lock(my->channel_open_mutex);
      fc::scoped_lock<fc::mutex> scp_send_lock(my->scp_send_mutex);
      chan = my->call_ssh2_ptr_function_throw<LIBSSH2_CHANNEL*>(boost::bind(libssh2_scp_send64, my->session, remote_path.generic_string().c_str(), 0700, fsize, now, now ));
    } catch (fc::exception& er) {
      FC_RETHROW_EXCEPTION(er, error, "scp ${local_file} to ${remote_file} failed", ("local_file", local_path)("remote_file",remote_path));
    }
    uint64_t total_bytes_written = 0;
    try {
      const size_t max_mapping_size = 1024*1024*1024; // 1GB
      for (uint64_t current_offset = 0; current_offset < fsize; current_offset += max_mapping_size) {
        uint64_t total_bytes_left_to_send = fsize - current_offset;
        size_t bytes_to_send_this_iteration = (size_t)std::min<uint64_t>(total_bytes_left_to_send, max_mapping_size);
        mapped_region mr( fmap, fc::read_only, current_offset, bytes_to_send_this_iteration);
        size_t bytes_written_this_iteration = 0;
        char* pos = reinterpret_cast<char*>(mr.get_address());
        while( progress(total_bytes_written, fsize) && bytes_written_this_iteration < bytes_to_send_this_iteration) {
            int r = my->call_ssh2_function_throw(boost::bind(libssh2_channel_write_ex, chan, 0, pos, 
                                                             bytes_to_send_this_iteration - bytes_written_this_iteration),
					         "scp failed ${code} - ${message}");
            bytes_written_this_iteration += r;
            total_bytes_written += r;
            pos   += r;
	    // fc_wlog( my->logr, "wrote ${bytes} bytes", ("bytes",r) );
        }
      }
      my->call_ssh2_function(boost::bind(libssh2_channel_send_eof, chan));
      my->call_ssh2_function(boost::bind(libssh2_channel_wait_eof, chan));
      my->call_ssh2_function(boost::bind(libssh2_channel_close, chan));
    } catch ( fc::exception& er ) {
      // clean up chan
      my->call_ssh2_function(boost::bind(libssh2_channel_free, chan));
      throw er;
    }
    my->call_ssh2_function_throw(boost::bind(libssh2_channel_free, chan),
				 "scp failed ${code} - ${message}");
  }


  void client::rm( const fc::path& remote_path ) {
    try {
       auto s = stat(remote_path);
       if( s.is_directory() ) {
         FC_THROW( "sftp cannot remove directory ${path}", ("path",remote_path) );
       }
       else if( !s.exists() ) {
         return; // nothing to do
       }

       {
         fc::scoped_lock<fc::mutex> scp_unlink_lock(my->scp_unlink_mutex);
         my->call_ssh2_function_throw(boost::bind(libssh2_sftp_unlink_ex, my->sftp, remote_path.generic_string().c_str(), remote_path.generic_string().size()),
                                      "sftp rm failed ${code}");
       }
     } catch ( fc::exception& er ) {
         FC_RETHROW_EXCEPTION( er, error, "sftp remove '${remote_path}' failed", ("remote_path",remote_path) );
     }
  }

  void client::rmdir( const fc::path& remote_path ) {
    try {
       auto s = stat(remote_path);
       if( !s.is_directory() )
         FC_THROW( "sftp cannot rmdir non-directory ${path}", ("path",remote_path) );
       else if( !s.exists() )
         return; // nothing to do

       {
         fc::scoped_lock<fc::mutex> scp_rmdir_lock(my->scp_rmdir_mutex);
         my->call_ssh2_function_throw(boost::bind(libssh2_sftp_rmdir_ex, my->sftp, remote_path.generic_string().c_str(), remote_path.generic_string().size()),
                                      "sftp rmdir failed ${code}");
       }
     } catch ( fc::exception& er ) {
       FC_RETHROW_EXCEPTION( er, error, "sftp rmdir '${remote_path}' failed", ("remote_path",remote_path) );
     }
  }

  void client::rmdir_recursive( const fc::path& remote_path ) {
    try {
      auto s = stat(remote_path);
      if( !s.is_directory() )
        FC_THROW( "sftp cannot rmdir non-directory ${path}", ("path",remote_path) );
      else if( !s.exists() )
        return; // nothing to do

      LIBSSH2_SFTP_HANDLE *dir_handle;
      {
        fc::scoped_lock<fc::mutex> scp_open_lock(my->scp_open_mutex);
        dir_handle = 
          my->call_ssh2_ptr_function_throw<LIBSSH2_SFTP_HANDLE*>(boost::bind(libssh2_sftp_open_ex, my->sftp, remote_path.generic_string().c_str(), remote_path.generic_string().size(), 0, 0, LIBSSH2_SFTP_OPENDIR),
                                                                             "sftp libssh2_sftp_opendir failed ${code}");
      }
      do {
        char mem[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;

        int rc;
        {
          fc::scoped_lock<fc::mutex> scp_readdir_lock(my->scp_readdir_mutex);
          rc = my->call_ssh2_function_throw(boost::bind(libssh2_sftp_readdir_ex, dir_handle, mem, sizeof(mem), (char*)NULL, 0, &attrs),
                                            "sftp readdir failed ${code}");
        }
        if (rc > 0) {
          fc::string file_or_dir_name(mem, rc);
          if (file_or_dir_name == "." || file_or_dir_name == "..")
            continue;
          fc::path full_remote_path = remote_path / file_or_dir_name;
          if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions))
            rmdir_recursive(full_remote_path);
          else if (LIBSSH2_SFTP_S_ISREG(attrs.permissions)) {
            fc::scoped_lock<fc::mutex> scp_unlink_lock(my->scp_unlink_mutex);
            my->call_ssh2_function_throw(boost::bind(libssh2_sftp_unlink_ex, my->sftp, full_remote_path.generic_string().c_str(), full_remote_path.generic_string().size()),
                                         "sftp rm failed ${code}");
          }
        } else
          break;
      } while (1);

      {
        fc::scoped_lock<fc::mutex> scp_close_lock(my->scp_close_mutex);
        my->call_ssh2_function_throw(boost::bind(libssh2_sftp_close_handle, dir_handle), "sftp libssh2_sftp_closedir failed ${code}");
      }
      {
        fc::scoped_lock<fc::mutex> scp_rmdir_lock(my->scp_rmdir_mutex);
        my->call_ssh2_function_throw(boost::bind(libssh2_sftp_rmdir_ex, my->sftp, remote_path.generic_string().c_str(), remote_path.generic_string().size()),
                                                 "sftp rmdir failed ${code}");
      }
    } catch ( fc::exception& er ) {
      FC_RETHROW_EXCEPTION( er, error, "sftp rmdir recursive '${remote_path}' failed", ("remote_path",remote_path) );
    }
  }

  file_attrib client::stat( const fc::path& remote_path ){
     my->init_sftp();
     LIBSSH2_SFTP_ATTRIBUTES att;
     int ec;
     {
       fc::scoped_lock<fc::mutex> scp_stat_lock(my->scp_stat_mutex);
       ec = my->call_ssh2_function(boost::bind(libssh2_sftp_stat_ex, my->sftp, 
					       remote_path.generic_string().c_str(), remote_path.generic_string().size(), 
					       LIBSSH2_SFTP_STAT, &att));
     }
     if( ec )
        return file_attrib();
     file_attrib    ft;
     ft.size        = att.filesize;
     ft.permissions = att.permissions;
     return ft;
  }
  void client::create_directories( const fc::path& rdir, int mode ) {
     boost::filesystem::path dir = rdir;
     boost::filesystem::path p;
     auto pitr = dir.begin();
     while( pitr != dir.end() ) {
       p /= *pitr;
       if( !stat( p ).exists() ) {
         mkdir(p,mode);
       } 
       ++pitr;
     }
  }

  void client::mkdir( const fc::path& rdir, int mode ) {
     try {
       auto s = stat(rdir);
       if( s.is_directory() ) 
         return;
       else if( s.exists() )
         FC_THROW( "File already exists at path ${path}", ("path",rdir) );
       
       {
         fc::scoped_lock<fc::mutex> scp_mkdir_lock(my->scp_mkdir_mutex);
         my->call_ssh2_function_throw(boost::bind(libssh2_sftp_mkdir_ex, my->sftp, 
                                                  rdir.generic_string().c_str(), rdir.generic_string().size(), mode), 
                                      "sftp mkdir error");
       }
     } catch ( fc::exception& er ) {
          FC_RETHROW_EXCEPTION( er, error, "sftp failed to create directory '${directory}'", ( "directory", rdir ) );
     }
  }

   void client::set_remote_system_is_windows(bool is_windows /* = true */) {
      my->remote_system_is_windows = is_windows;
   }


  file_attrib::file_attrib()
  :size(0),uid(0),gid(0),permissions(0),atime(0),mtime(0)
  { }

  bool file_attrib::is_directory() {
    return  LIBSSH2_SFTP_S_ISDIR(permissions);
  }
  bool file_attrib::is_file() {
    return LIBSSH2_SFTP_S_ISREG(permissions);
  }
  bool file_attrib::exists() {
    return 0 != permissions;
  }
  
  detail::client_impl::client_impl() :
    session(nullptr),
    knownhosts(nullptr),
    sftp(nullptr),
    agent(nullptr),
    _trace_level(0), // was  LIBSSH2_TRACE_ERROR 
    logr(fc::logger::get( "fc::ssh::client" )),
    remote_system_is_windows(false) {
    logr.set_parent( fc::logger::get( "default" ) );
  }

  detail::client_impl::~client_impl() {
    close();
  }

  /* static */ 
  void detail::client_impl::kbd_callback(const char *name, int name_len, 
                                  const char *instruction, int instruction_len, int num_prompts,
                                  const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                                  LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                                  void **abstract) {
    detail::client_impl* self = (client_impl*)*abstract;
        
        
    for (int i = 0; i < num_prompts; i++) {
      fwrite(prompts[i].text, 1, prompts[i].length, stdout);
        
      if( self->upass.size() == 0 ) {
       /** TODO: add keyboard callback here... 
        fgets(buf, sizeof(buf), stdin);
        n = strlen(buf);
        while (n > 0 && strchr("\r\n", buf[n - 1]))
          n--;
        buf[n] = 0;
        
#ifdef WIN32 // fix warning
# define strdup _strdup
#endif
        responses[i].text = strdup(buf);
        responses[i].length = n;
        */
        responses[i].text = nullptr;
        responses[i].length = 0;
      } else {
        responses[i].text = strdup(self->upass.c_str());
        responses[i].length = self->upass.size();
      }        
    }        
  }

  void detail::client_impl::connect() {
    try {
      if( libssh2_init(0) < 0  )
        FC_THROW( "Unable to init libssh2" );
            
      auto eps = fc::asio::tcp::resolve( hostname, boost::lexical_cast<std::string>(port) );
      if( eps.size() == 0 )
        FC_THROW( "Unable to resolve host '${host}'", ("host",hostname) );

      sock.reset( new boost::asio::ip::tcp::socket( fc::asio::default_io_service() ) );
            
      bool resolved = false;
      for( uint32_t i = 0; i < eps.size(); ++i ) {
        std::stringstream ss; ss << eps[i];
        try {
          boost::system::error_code ec;
          fc_ilog( logr, "Attempting to connect to ${endpoint}", ("endpoint",ss.str().c_str()) );
          fc::asio::tcp::connect( *sock, eps[i] );
          endpt = eps[i];
          resolved = true;
          break;
        } catch ( fc::exception& er ) {
          fc_ilog( logr, "Failed to connect to ${endpoint}\n${error_reprot}", 
                    ("endpoint",ss.str().c_str())("error_report", er.to_detail_string()) );
          sock->close();
        }
      }
      if( !resolved )
        FC_THROW( "Unable to connect to any resolved endpoint for ${host}:${port}", 
                  ("host", hostname).set("port",port) );

      session = libssh2_session_init(); 
      libssh2_trace( session, _trace_level );
      libssh2_trace_sethandler( session, this, client_impl::handle_trace );

      *libssh2_session_abstract(session) = this;
            
      libssh2_session_set_blocking( session, 0 );
      try {
        call_ssh2_function_throw(boost::bind(libssh2_session_handshake, session, sock->native()),
                                 "SSH Handshake error: ${code} - ${message}");
      } catch (fc::exception& er) {
        FC_RETHROW_EXCEPTION( er, error, "Error during SSH handshake" );;
      }
      //const char* fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
      //slog( "fingerprint: %s", fingerprint );
             
      // try to authenticate, throw on error.
      try {
        authenticate();
      } catch (fc::exception& er) {
        FC_RETHROW_EXCEPTION( er, error, "Error during SSH authentication" );;
      }
      //slog(".");
    } catch ( fc::exception& er ) {
      elog( "Unable to connect to ssh server: ${detail}", ("detail", er.to_detail_string().c_str()) );
      close();
      FC_RETHROW_EXCEPTION( er, error, "Unable to connect to ssh server" );;
    } catch ( ... ) {
      close();
      FC_THROW( "Unable to connect to ssh server", ("exception", fc::except_str() ) );
    }
  }

  /* static */
  void detail::client_impl::handle_trace( LIBSSH2_SESSION* session, void* context, const char* data, size_t length ) {
    client_impl* my = (client_impl*)context;
    fc::string str(data,length);
    fc_wlog( my->logr, "${message}", ("message",str) );
  }

  void detail::client_impl::close() {
    if( session ) {
      if( sftp ) {
        try {
	  call_ssh2_function(boost::bind(libssh2_sftp_shutdown, sftp));
        }catch(...){
	  fc_wlog( logr, "caught closing sftp session" );
        }
        sftp = 0;
      }

      if (agent) {
        libssh2_agent_disconnect(agent);
        libssh2_agent_free(agent);
        agent = nullptr;
      }

      try {
        call_ssh2_function(boost::bind(libssh2_session_disconnect_ex, session, SSH_DISCONNECT_BY_APPLICATION, "exit cleanly", ""));
        call_ssh2_function(boost::bind(libssh2_session_free, session), false);
      } catch ( ... ){
        fc_wlog( logr, "caught freeing session" );
      }
      session = 0;
      try {
        if( sock )
          sock->close();
      } catch ( ... ){
        fc_wlog( logr, "caught error closing socket" );
      }
      sock.reset(0);
      try {
        if( read_prom )
          read_prom->wait();
      } catch ( ... ){
        fc_wlog( logr, "caught error waiting on read" );
      }
      try {
        if( write_prom )
          write_prom->wait();
      } catch ( ... ){
        fc_wlog( logr, "caught error waiting on write" );
      }
    }
  }

  void detail::client_impl::authenticate() {
    try {
      char * alist = NULL;
      // libssh2_userauth_list has strange enough behavior that we can't use the 
      // call_blocking_libssh2_function-type functions to wait and retry, so we must
      // explicitly lock around the libssh2_userauth_list calls.  
      // hence, this anonymous scope:
        {
        fc::unique_lock<fc::mutex> lock(ssh_session_mutex);

        alist = libssh2_userauth_list(session, uname.c_str(),uname.size());

        if(alist==NULL) {
          char * msg   = 0;
          int    ec    = 0;
          if(libssh2_userauth_authenticated(session))
            return; // CONNECTED!
          ec = libssh2_session_last_error(session,&msg,NULL,0);

          while( !alist && (ec == LIBSSH2_ERROR_EAGAIN) ) {
            wait_on_socket();
            alist = libssh2_userauth_list(session, uname.c_str(), uname.size());
            ec = libssh2_session_last_error(session,&msg,NULL,0);
          }
          if( !alist ) {
            FC_THROW( "Error getting authorization list: ${code} - ${message}", 
                              ("code",ec).set("message",msg));
          }
        }
      } // end anonymous scope

    std::vector<std::string> split_alist;
    bool pubkey = false;
    bool pass   = false;
    bool keybd   = false;
    boost::split( split_alist, alist, boost::is_any_of(",") );
    std::for_each( split_alist.begin(), split_alist.end(), [&](const std::string& s){
      if( s == "publickey" )
        pubkey = true;
      else if( s == "password" )
        pass = true;
      else if( s == "keyboard-interactive" )
        keybd = true;
      else
        fc_dlog( logr, "Unknown/unsupported authentication type '${auth_type}'", ("auth_type",s.c_str()));
    });

    if( pubkey && try_pub_key() ) 
      return;
    if( pass && try_pass() )
      return;
    if( keybd && try_keyboard() )
      return;
    } catch ( fc::exception& er ) {
      FC_RETHROW_EXCEPTION( er, error, "Unable to authenticate ssh connection" );
    }
    FC_THROW( "Unable to authenticate ssh connection" );
  } // authenticate()

  bool detail::client_impl::try_pass() {
    return !call_ssh2_function(boost::bind(libssh2_userauth_password_ex, session, uname.c_str(), uname.size(), 
					    upass.c_str(), upass.size(), (LIBSSH2_PASSWD_CHANGEREQ_FUNC((*)))NULL));
  }
  bool detail::client_impl::try_keyboard() {
    return !call_ssh2_function(boost::bind(libssh2_userauth_keyboard_interactive_ex, session, 
                                            uname.c_str(), uname.size(), &client_impl::kbd_callback));
  }
  bool detail::client_impl::try_pub_key() {
    if (privkey.size()) {
      if (!call_ssh2_function(boost::bind(libssh2_userauth_publickey_fromfile_ex,
					  session,
					  uname.c_str(), uname.size(),
					  pubkey.c_str(),
					  privkey.c_str(),
					  passphrase.c_str())))
        return true; // successful authentication from file
      fc_ilog( logr, "failed to authenticate with private key from file '${privkey_filename}'", ("privkey_filename",privkey));
    } else
      fc_ilog( logr, "no private key file set, skiping pubkey authorization from file");

    agent = libssh2_agent_init(session);
    if (!agent) {
      fc_wlog( logr, "failed to initialize ssh-agent support");
      return false;
    }

    if (call_ssh2_function(boost::bind(libssh2_agent_connect, agent))) {
      fc_ilog( logr, "failed to connect to ssh-agent");
      return false;
    }

    if (call_ssh2_function(boost::bind(libssh2_agent_list_identities, agent))) {
      fc_ilog( logr, "failed requesting identities from ssh-agent");
      return false;
    }

    struct libssh2_agent_publickey *prev_identity = NULL;
    while (1) {
      struct libssh2_agent_publickey *identity;
      int ec = call_ssh2_function(boost::bind(libssh2_agent_get_identity, agent, &identity, prev_identity));
      if (ec == 1)
        break; // done iterating over keys
      if (ec < 0) {
        fc_ilog( logr, "failed obtaining identity from ssh-agent");
        return false;
      }

      if (call_ssh2_function(boost::bind(libssh2_agent_userauth, agent, uname.c_str(), identity)))
        fc_ilog( logr, "unable to authenticate with public key '${key_comment}'", ("key_comment",identity->comment));
      else {
        fc_ilog( logr, "authenticated with public key '${key_comment}'", ("key_comment",identity->comment));
        return true;
      }
      prev_identity = identity;
    }
    return false;
  }

  void detail::client_impl::wait_on_socket(int additionalDirections /* = 0 */) {
    int dir = libssh2_session_block_directions(session);
    dir |= additionalDirections;
    if( !dir ) 
      return;

    fc::promise<boost::system::error_code>::ptr rprom, wprom;
    if( dir & LIBSSH2_SESSION_BLOCK_INBOUND ) {
      fc::scoped_lock<fc::spin_lock> lock(this->_spin_lock);
      if( !read_prom ) {
        read_prom.reset( new fc::promise<boost::system::error_code>("read_prom") );
        sock->async_read_some( boost::asio::null_buffers(),
                                [=]( const boost::system::error_code& e, size_t  ) {
                                  fc::scoped_lock<fc::spin_lock> lock(this->_spin_lock);
                                  this->read_prom->set_value(e);
                                  this->read_prom.reset(nullptr);
                                } );
      }
      rprom = read_prom;
    }
          
    if( dir & LIBSSH2_SESSION_BLOCK_OUTBOUND ) {
      fc::scoped_lock<fc::spin_lock> lock(this->_spin_lock);
      if( !write_prom ) {
          write_prom.reset( new fc::promise<boost::system::error_code>("write_prom") );
          sock->async_write_some( boost::asio::null_buffers(),
                                  [=]( const boost::system::error_code& e, size_t  ) {
                                    fc::scoped_lock<fc::spin_lock> lock(this->_spin_lock);
                                    this->write_prom->set_value(e);
                                    this->write_prom.reset(0);
                                  } );
      }
      wprom = write_prom;
    }

    boost::system::error_code ec;
    if( rprom.get() && wprom.get() ) {
      typedef fc::future<boost::system::error_code> fprom;
      fprom fw(wprom);
      fprom fr(rprom);
#if 0
      // EMF: at present there are known bugs in fc::wait_any, and it will fail to wake up
      // when one of the futures is ready.  
      int r = fc::wait_any( fw, fr, fc::seconds(1) );
#else
      int r;
      while (1) {
        if (fw.ready()) {
          r = 0; break;
        }
        if (fr.ready()) {
          r = 1; break;
        }
        fc::usleep(fc::microseconds(5000));
      }
#endif
      switch( r ) {
        case 0:
          if( wprom->wait() ) { 
            FC_THROW( "Socket Error ${message}", 
                              ( "message", boost::system::system_error(rprom->wait() ).what() ) ); 
          }
          break;
        case 1:
          if( rprom->wait() ) { 
            FC_THROW( "Socket Error ${message}", 
                              ( "message", boost::system::system_error(rprom->wait() ).what() ) ); 
          }
          break;
      }
    } else if( rprom ) {
        if( rprom->wait() ) { 
          FC_THROW( "Socket Error ${message}", 
                            ( "message", boost::system::system_error(rprom->wait() ).what() ) ); 
        }
    } else if( wprom ) {
        if( wprom->wait() ) { 
          FC_THROW( "Socket Error ${message}", 
                            ( "message", boost::system::system_error(wprom->wait() ).what() ) ); 
        }
    }
  }
  void detail::client_impl::init_sftp() {
    if( !sftp )
      sftp = call_ssh2_ptr_function_throw<LIBSSH2_SFTP*>(boost::bind(libssh2_sftp_init,session),
                                                          "init sftp error ${code} - ${message}");
  }


  LIBSSH2_CHANNEL* detail::client_impl::open_channel( const fc::string& pty_type ) {
    LIBSSH2_CHANNEL*                      chan = 0;
    /* anonymous scope */ {
      fc::scoped_lock<fc::mutex> channel_open_lock(channel_open_mutex);

      chan = call_ssh2_ptr_function_throw<LIBSSH2_CHANNEL*>(boost::bind(libssh2_channel_open_ex, session,
								        "session", sizeof("session") - 1,
								        LIBSSH2_CHANNEL_WINDOW_DEFAULT,
								        LIBSSH2_CHANNEL_PACKET_DEFAULT, 
								        (const char*)NULL, 0),
							    "libssh2_channel_open_session failed: ${message}");
    }

    if( pty_type.size() )
      call_ssh2_function_throw(boost::bind(libssh2_channel_request_pty_ex, chan, pty_type.c_str(), pty_type.size(),
					    (char *)NULL, 0, LIBSSH2_TERM_WIDTH, LIBSSH2_TERM_HEIGHT, 
					    LIBSSH2_TERM_WIDTH_PX, LIBSSH2_TERM_HEIGHT_PX),
				"libssh2_channel_req_pty failed: ${message}");
    return chan;
  }

} }
