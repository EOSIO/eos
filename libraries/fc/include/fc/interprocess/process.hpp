#pragma once
#include <fc/interprocess/iprocess.hpp>

namespace fc {

  fc::path find_executable_in_path( const fc::string name );

  /**
   *  @brief start and manage an local process
   *  @note this class implements reference semantics.
   */
  class process  : public iprocess
  {
    public:
      process();
      ~process();

      virtual iprocess& exec( const fc::path&  exe, 
                              std::vector<std::string>   args, 
                              const fc::path&  work_dir = fc::path(), 
                              int              opts     = open_all    );

      
      virtual int                        result(const microseconds& timeout = microseconds::maximum());
      virtual void                       kill();
      virtual fc::buffered_ostream_ptr   in_stream();
      virtual fc::buffered_istream_ptr   out_stream();
      virtual fc::buffered_istream_ptr   err_stream();

      class impl;
    private:
      std::unique_ptr<impl> my;
  };

  typedef std::shared_ptr<process> process_ptr;

} // namespace fc
