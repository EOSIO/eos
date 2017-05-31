#pragma once
#include <fc/fwd.hpp>
#include <fc/utility.hpp>

namespace boost {
  namespace interprocess {
    class file_mapping;
    class mapped_region;
  }
}
namespace fc {
  enum mode_t {
    read_only, 
    write_only,
    read_write
  };

  class file_mapping {
    public:
      file_mapping( const char* file, mode_t );
      ~file_mapping();
    private:
      friend class mapped_region;
    #ifdef _WIN64
      fc::fwd<boost::interprocess::file_mapping,0x38> my;
    #else
      fc::fwd<boost::interprocess::file_mapping,0x24> my;
    #endif
  };

  class mapped_region {
    public:
      mapped_region( const file_mapping& fm, mode_t m, uint64_t start, size_t size );
      mapped_region( const file_mapping& fm, mode_t m );
      ~mapped_region();
      void  flush();
      void* get_address()const;
      size_t get_size()const;
    private:
      fc::fwd<boost::interprocess::mapped_region,40> my;
  };
}
