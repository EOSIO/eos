#pragma once
#include <fc/thread/future.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <vector>
#include <fc/string.hpp>
#include <fc/filesystem.hpp>

namespace fc
{
    /**
     *   @brief abstract interface for interacting with external processes
     *
     *   At the very least we have ssh::process and direct child processes, and
     *   there may be other processes that need to implement this protocol.
     */
    class iprocess
    {
       public:
         enum exec_opts {
           open_none   = 0,
           open_stdin  = 0x01, 
           open_stdout = 0x02, 
           open_stderr = 0x04,
           open_all    = open_stdin|open_stdout|open_stderr,
           suppress_console = 0x08
         };

          virtual ~iprocess(){}

          /**
           *  
           *  @return *this
           */
          virtual iprocess& exec( const path& exe, std::vector<std::string> args, 
                                  const path& work_dir = path(), int opts = open_all ) = 0;

          /**
           *  @return blocks until the process exits
           */
          virtual int result(const microseconds& timeout = microseconds::maximum()) = 0;


          /**
           *  Forcefully kills the process.
           */
          virtual void kill() = 0;
          
          /**
           *  @brief returns a stream that writes to the process' stdin
           */
          virtual buffered_ostream_ptr in_stream() = 0;
          
          /**
           *  @brief returns a stream that reads from the process' stdout
           */
          virtual buffered_istream_ptr out_stream() = 0;
          /**
           *  @brief returns a stream that reads from the process' stderr
           */
          virtual buffered_istream_ptr err_stream() = 0;

    };

    typedef std::shared_ptr<iprocess> iprocess_ptr;
    

} // namespace fc
