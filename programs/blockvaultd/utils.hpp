#include <filesystem>
#include <regex>
#include <unistd.h>

namespace blockvault {

namespace fs = std::filesystem;

template <typename Lambda>
void for_each_file_in_dir_matches(const fs::path& dir, std::string pattern, Lambda&& lambda) {
   const std::regex       base_regex(pattern);
   std::smatch            base_match;
   fs::directory_iterator end_itr; // Default ctor yields past-the-end
   for (fs::directory_iterator p(dir); p != end_itr; ++p) {
      // Skip if not a file
      if (!fs::is_regular_file(p->status()))
         continue;
      // skip if it does not match the pattern
      std::string fname(p->path().filename().c_str());
      if (!std::regex_match(fname, base_match, base_regex))
         continue;
      lambda(p->path());
   }
}

class file_ptr : public std::unique_ptr<FILE, decltype(&fclose)> {
 public:
   file_ptr(FILE* f = nullptr)
       : std::unique_ptr<FILE, decltype(&fclose)>(f, &fclose) {}

   file_ptr(fs::path fname, const char* mode)
       : file_ptr(fopen(fname.c_str(), mode)) {
      if (this->get() == nullptr)
         throw fs::filesystem_error("file open error", fname, std::make_error_code(static_cast<std::errc>(errno)));
   }
};

class file_writer {
   file_ptr fp_;
   fs::path fname_;

 public:
   file_writer() = default;
   file_writer(fs::path path, const char* mode)
       : fp_(path, mode)
       , fname_(path) {}
   file_writer(file_writer&&) = default;
   file_writer& operator=(file_writer&&) = default;

   void sync() {
      fflush(fp_.get());
      fsync(fileno(fp_.get()));
   }

   int write(const void* data, int num_bytes) { return fwrite(data, 1, num_bytes, fp_.get()); }

   template <typename T>
   int write(const T& data) {
      return write(&data, sizeof(data));
   }

   void sync_write(const void* data, int num_bytes) {
      int r = fwrite(data, 1, num_bytes, fp_.get());
      if (r != num_bytes)
         throw fs::filesystem_error("file write error", fname_, std::make_error_code(static_cast<std::errc>(errno)));
      sync();
   }

   template <typename T>
   void sync_write(const T& data) {
      sync_write(&data, sizeof(data));
   }

   uint64_t cur_position() const { return ftell(fp_.get()); }

   bool is_open() const { return fp_.get(); }

   void seek(long int offset, int origin = SEEK_SET) {
      if (fseek(fp_.get(), offset, origin) != 0)
         throw fs::filesystem_error("file seek error", fname_, std::make_error_code(static_cast<std::errc>(errno)));
   }

   void close() { fp_.reset(); }
};

class file_reader {
   file_ptr fp_;
   fs::path fname_;

 public:
   file_reader() = default;
   file_reader(fs::path path)
       : fp_(path, "r")
       , fname_(path) {}
   file_reader(file_reader&&) = default;

   file_reader& operator=(file_reader&&) = default;

   void seek(long int offset, int origin = SEEK_SET) {
      if (fseek(fp_.get(), offset, origin) != 0)
         throw fs::filesystem_error("file seek error", fname_, std::make_error_code(static_cast<std::errc>(errno)));
   }

   void read(void* buf, std::size_t size) {
      if (fread(buf, 1, size, fp_.get()) != size)
         throw fs::filesystem_error("file read error", fname_, std::make_error_code(static_cast<std::errc>(errno)));
   }

   template <typename T>
   void read(T& obj) {
      if (fread(&obj, 1, sizeof(T), fp_.get()) != sizeof(T))
         throw fs::filesystem_error("file read error", fname_, std::make_error_code(static_cast<std::errc>(errno)));
   }

   std::size_t size() const { return fs::file_size(fname_); }

   fs::path path() const { return fname_; }
};

}