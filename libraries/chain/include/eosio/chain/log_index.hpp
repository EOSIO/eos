#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace eosio {
namespace chain {

template <typename Exception>
class log_index {
   boost::iostreams::mapped_file_source file;

 public:
   log_index() = default;
   log_index(const boost::filesystem::path& path) { open(path); }

   void open(const boost::filesystem::path& path) {
      if (file.is_open())
         file.close();
      file.open(path.generic_string());
      EOS_ASSERT(file.size() % sizeof(uint64_t) == 0, Exception,
                 "The size of ${file} is not a multiple of sizeof(uint64_t)", ("file", path.generic_string()));
   }

   bool is_open() const { return file.is_open(); }

   using iterator = const uint64_t*;
   iterator begin() const { return reinterpret_cast<iterator>(file.data()); }
   iterator end() const { return reinterpret_cast<iterator>(file.data() + file.size()); }

   /// @pre file.size() > 0
   uint64_t back() const { return *(this->end() - 1); }
   int      num_blocks() const { return file.size() / sizeof(uint64_t); }
   uint64_t nth_block_position(uint32_t n) const { return *(begin() + n); }
};

} // namespace chain
} // namespace eosio
