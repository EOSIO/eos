#pragma once
#include <fc/interprocess/iprocess.hpp>

namespace fc { namespace ssh 
{

  class client;

  namespace detail {
    class process_impl;
  };

  /**
   *  Enables communication with a process executed via 
   *  client::exec().  
   *
   *  Process can only be created by mace::ssh::client.
   */
  class process  : public iprocess
  {
    public:
      virtual iprocess& exec( const fc::path& exe, std::vector<std::string> args, 
                              const fc::path& work_dir = fc::path(), exec_opts opts = open_all );

      /**
       *  Blocks until the result code of the process has been returned.
       */
      virtual int result();


      /**
       * Not supported.  libssh2 does not support sending signals to remote processes.
       * closing in_stream() is the best you can do
       */
      virtual void kill();


      /**
       *  @brief returns a stream that writes to the process' stdin
       */
      virtual fc::buffered_ostream_ptr in_stream();
      
      /**
       *  @brief returns a stream that reads from the process' stdout
       */
      virtual fc::buffered_istream_ptr out_stream();
      /**
       *  @brief returns a stream that reads from the process' stderr
       */
      virtual fc::buffered_istream_ptr err_stream();

      process( fc::ssh::client_ptr c );
      ~process();
    private:
      std::unique_ptr<detail::process_impl> my;
  };

} } // fc::ssh
