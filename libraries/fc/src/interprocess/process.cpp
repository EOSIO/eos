#include <fc/interprocess/process.hpp>
#include <fc/io/iostream.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <fc/asio.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/logger.hpp>
#include <boost/process/all.hpp>
#include <boost/iostreams/stream.hpp>
#include <cstdlib>

namespace fc {
  namespace bp = boost::process;
  namespace io = boost::iostreams;

fc::path find_executable_in_path( const fc::string name ) {
  try {
    return fc::string(bp::find_executable_in_path( std::string(name), "" ));
  } catch (...) {
    const char* p = std::getenv("PATH");
    FC_THROW( "Unable to find executable ${exe} in path.", 
      ("exe", name)
      ("inner", fc::except_str() )
      ("PATH", fc::string(p!=nullptr?p:"") ) );
  }
  return fc::path();
}


class process::impl 
{
  public:
  impl()
  :stat( fc::asio::default_io_service() ){}

  ~impl() 
  {
    try 
    {
      if( _in )
      {
         _in->close();
      }
      if( _exited.valid() && !_exited.ready()) 
      {
         //child->terminate();
         _exited.wait();
      }
    }
    catch(...) 
    {
      wlog( "caught exception cleaning up process: ${except_str}", ("except_str",fc::except_str()) );
    }
  }

  std::shared_ptr<bp::child> child;

  std::shared_ptr<bp::pipe>  _inp;
  std::shared_ptr<bp::pipe>  _outp;
  std::shared_ptr<bp::pipe>  _errp;

  buffered_istream_ptr       _out;
  buffered_istream_ptr       _err;
  buffered_ostream_ptr       _in;

  bp::status                 stat;
  bp::context                pctx;

  fc::future<int>            _exited;
};

process::process()
:my( new process::impl() ){}
process::~process(){}

iprocess& process::exec( const fc::path& exe, 
                         std::vector<std::string> args, 
                         const fc::path& work_dir, int opt  ) 
{

  my->pctx.work_dir = work_dir.string();
  my->pctx.suppress_console = (opt & suppress_console) != 0;
    
  if( opt&open_stdout)
      my->pctx.streams[boost::process::stdout_id] = bp::behavior::async_pipe();
  else 
      my->pctx.streams[boost::process::stdout_id] = bp::behavior::null();


  if( opt& open_stderr )
      my->pctx.streams[boost::process::stderr_id] = bp::behavior::async_pipe();
  else
      my->pctx.streams[boost::process::stderr_id] = bp::behavior::null();

  if( opt& open_stdin )
      my->pctx.streams[boost::process::stdin_id]  = bp::behavior::async_pipe();
  else
      my->pctx.streams[boost::process::stdin_id]  = bp::behavior::close();

  /*
  std::vector<std::string> a;
  a.reserve(size_t(args.size()));
  for( uint32_t i = 0; i < args.size(); ++i ) {
    a.push_back( fc::move(args[i]) ); 
  }
  */
  my->child.reset( new bp::child( bp::create_child( exe.string(), fc::move(args), my->pctx ) ) );

  if( opt & open_stdout ) {
     bp::handle outh = my->child->get_handle( bp::stdout_id );
     my->_outp.reset( new bp::pipe( fc::asio::default_io_service(), outh.release() ) );
  }
  if( opt & open_stderr ) {
     bp::handle errh = my->child->get_handle( bp::stderr_id );
     my->_errp.reset( new bp::pipe( fc::asio::default_io_service(), errh.release() ) );
  }
  if( opt & open_stdin ) {
     bp::handle inh  = my->child->get_handle( bp::stdin_id );
     my->_inp.reset(  new bp::pipe( fc::asio::default_io_service(), inh.release()  ) );
  }


  promise<int>::ptr p(new promise<int>("process"));
  my->stat.async_wait(  my->child->get_id(), [=]( const boost::system::error_code& ec, int exit_code )
    {
      //slog( "process::result %d", exit_code );
      if( !ec ) {
          #ifdef BOOST_POSIX_API
          if( WIFEXITED(exit_code) ) p->set_value(  WEXITSTATUS(exit_code) );
          else
          {
             p->set_exception( 
                 fc::exception_ptr( new fc::exception( 
                     FC_LOG_MESSAGE( error, "process exited with: ${message} ", 
                                     ("message", strsignal(WTERMSIG(exit_code))) ) ) ) );
          }
          #else
          p->set_value(exit_code);
          #endif
       }
       else
       {
          p->set_exception( 
              fc::exception_ptr( new fc::exception( 
                  FC_LOG_MESSAGE( error, "process exited with: ${message} ", 
                                  ("message", boost::system::system_error(ec).what())) ) ) );
       }
    });
  if( opt & open_stdin )
    my->_in     = std::make_shared<buffered_ostream>(std::make_shared<fc::asio::ostream<bp::pipe>>(my->_inp));
  if( opt & open_stdout )
    my->_out    = std::make_shared<buffered_istream>(std::make_shared<fc::asio::istream<bp::pipe>>(my->_outp));
  if( opt & open_stderr )
    my->_err    = std::make_shared<buffered_istream>(std::make_shared<fc::asio::istream<bp::pipe>>(my->_errp));
  my->_exited = p;
  return *this;
}

/**
 *  Forcefully kills the process.
 */
void process::kill() {
  my->child->terminate();
}

/**
 *  @brief returns a stream that writes to the process' stdin
 */
fc::buffered_ostream_ptr process::in_stream() {
  return my->_in;
}

/**
 *  @brief returns a stream that reads from the process' stdout
 */
fc::buffered_istream_ptr process::out_stream() {
  return my->_out;
}
/**
 *  @brief returns a stream that reads from the process' stderr
 */
fc::buffered_istream_ptr process::err_stream() {
  return my->_err;
}

int process::result(const microseconds& timeout /* = microseconds::maximum() */) 
{
    return my->_exited.wait(timeout);
}

}
