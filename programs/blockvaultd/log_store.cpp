#include "log_store.hpp"
#include <cassert>
#include <iostream>

namespace blockvault {

using namespace nuraft;
namespace fs = std::filesystem;

ptr<log_entry> empty_log_entry() {
   static ptr<buffer>    buf = buffer::alloc(sz_ulong);
   static ptr<log_entry> r   = cs_new<log_entry>(0, buf);
   return r;
}

fs::path path_for_index(const fs::path& log_dir, ulong index) {
   // the log is stored in log_dir/<log_index>.log
   return (log_dir / std::to_string(index)).concat(".log");
}

fs::path store_log(const fs::path& log_dir, const unsigned char* data, int sz) {
   namespace fs = std::filesystem;

   auto file = log_dir / std::tmpnam(nullptr);
   auto fp   = std::unique_ptr<FILE, decltype(&fclose)>{fopen(file.c_str(), "w"), &fclose};
   if (!fp)
      throw fs::filesystem_error("file open error", file, std::make_error_code(static_cast<std::errc>(errno)));
   fwrite(data, 1, sz, fp.get());
   return file;
}

struct log_entry_meta {
   ulong term;
   byte  type;

   log_entry_meta(ulong tm = 0, log_val_type tp = log_val_type::app_log)
       : term(tm)
       , type(static_cast<byte>(tp)) {}
};

constexpr int log_entry_meta_size = sizeof(ulong) + sizeof(byte);

fs::path store_log(const fs::path& log_dir, nuraft::ptr<nuraft::log_entry>& entry) {
   namespace fs = std::filesystem;

   auto file = log_dir / std::tmpnam(nullptr);
   auto fp   = std::unique_ptr<FILE, decltype(&fclose)>{fopen(file.c_str(), "w"), &fclose};
   if (!fp)
      throw fs::filesystem_error("file open error", file, std::make_error_code(static_cast<std::errc>(errno)));

   log_entry_meta meta{entry->get_term(), entry->get_val_type()};

   fwrite(&meta, log_entry_meta_size, 1, fp.get());
   auto& buf = entry->get_buf();
   buf.pos(0); // reset the position
   fwrite(buf.data(), 1, buf.size(), fp.get());
   return file;
}

class log_entry_reader {
 public:
   log_entry_reader() = default;
   log_entry_reader(fs::path log_dir, ulong index) { set(log_dir, index); }
   ~log_entry_reader() {
      if (!fname.empty()) {
         std::error_code ec;
         fs::remove(fname, ec);
      }
   }

   log_entry_reader(const log_entry_reader&) = delete;
   log_entry_reader& operator=(const log_entry_reader&) = delete;
   log_entry_reader(log_entry_reader&& other) { fname.swap(other.fname); }

   void set(fs::path log_dir, ulong index) {
      auto original = path_for_index(log_dir, index);
      if (fs::exists(original)) {
         fname = log_dir / std::tmpnam(nullptr);
         fs::create_hard_link(path_for_index(log_dir, index), fname);
      }
   }

   ptr<log_entry> get() {
      auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(fname.c_str(), "r"), &fclose);
      if (fp) {
         int            sz  = std::filesystem::file_size(fname) - log_entry_meta_size;
         ptr<buffer>    buf = buffer::alloc(sz);
         log_entry_meta meta;
         fread(&meta, log_entry_meta_size, 1, fp.get());
         fread(buf->get_raw(sz), sz, 1, fp.get());
         buf->pos(0); // reset the position
         return cs_new<log_entry>(meta.term, buf, static_cast<log_val_type>(meta.type));
      }

      return empty_log_entry();
   }

   size_t size() const {
      std::error_code ec;
      size_t          sz = fs::file_size(fname, ec);
      return ec ? 0 : sz - log_entry_meta_size;
   }

 private:
   fs::path fname;
};

log_store::log_store(std::filesystem::path p)
    : log_dir_(p)
    , start_idx_(1)
    , next_slot_(1) {}

ulong log_store::next_slot() const { return next_slot_; }

ulong log_store::start_index() const { return start_idx_; }

ptr<log_entry> log_store::last_entry() const {
   log_entry_reader reader;
   {
      std::lock_guard<std::mutex> l(logs_lock_);
      reader.set(log_dir_, next_slot_ - 1);
   }
   return reader.get();
}

ulong log_store::append(ptr<log_entry>& entry) {

   auto                        tmp_path = store_log(log_dir_, entry);
   std::lock_guard<std::mutex> l(logs_lock_);
   fs::rename(tmp_path, path_for_index(log_dir_, next_slot_));
   return next_slot_++;
}

void log_store::write_at(ulong index, ptr<log_entry>& entry) {

   auto tmp_path = store_log(log_dir_, entry);
   // Discard all logs equal to or greater than `index.
   std::lock_guard<std::mutex> l(logs_lock_);

   for (ulong i = index; i < next_slot_; ++i) {
      auto            p = (log_dir_ / std::to_string(i)).concat(".log");
      std::error_code ec;
      fs::remove(p, ec);
   }
   fs::rename(tmp_path, path_for_index(log_dir_, index));

   next_slot_ = index + 1;
}

ptr<std::vector<ptr<log_entry>>> log_store::log_entries(ulong start, ulong end) {

   std::vector<log_entry_reader> readers;
   readers.reserve(end - start);
   {
      std::lock_guard<std::mutex> l(logs_lock_);
      for (ulong ii = start; ii < end; ++ii) {
         readers.emplace_back(log_dir_, ii);
      }
   }

   ptr<std::vector<ptr<log_entry>>> ret = cs_new<std::vector<ptr<log_entry>>>();
   ret->reserve(readers.size());
   for (auto& r : readers) {
      ret->emplace_back(r.get());
   }
   return ret;
}

ptr<std::vector<ptr<log_entry>>> log_store::log_entries_ext(ulong start, ulong end, int64 batch_size_hint_in_bytes) {
   ptr<std::vector<ptr<log_entry>>> ret = cs_new<std::vector<ptr<log_entry>>>();
   if (batch_size_hint_in_bytes < 0) {
      return ret;
   }
   std::vector<log_entry_reader> readers;
   readers.reserve(end - start);

   {
      size_t accum_size = 0;

      std::lock_guard<std::mutex> l(logs_lock_);
      for (ulong ii = start; ii < end; ++ii) {
         readers.emplace_back(log_dir_, ii);
         accum_size += readers.back().size();
         if (batch_size_hint_in_bytes && accum_size >= (ulong)batch_size_hint_in_bytes)
            break;
      }
   }
   ret->reserve(readers.size());
   for (auto& r : readers) {
      ret->emplace_back(r.get());
   }
   return ret;
}

ptr<log_entry> log_store::entry_at(ulong index) {
   log_entry_reader reader;
   {
      std::lock_guard<std::mutex> l(logs_lock_);
      reader.set(log_dir_, index);
   }
   return reader.get();
}

ulong log_store::term_at(ulong index) {
   ulong                       term = 0;
   std::lock_guard<std::mutex> l(logs_lock_);
   auto                        p = path_for_index(log_dir_, index);
   if (fs::exists(p)) {
      auto fp = std::unique_ptr<FILE, decltype(&fclose)>{fopen(p.c_str(), "r"), &fclose};
      fread(&term, sizeof(term), 1, fp.get());
   }

   return term;
}

ptr<buffer> log_store::pack(ulong index, int32 cnt) {
   std::vector<ptr<buffer>> logs;

   size_t                      size_total = 0;
   std::lock_guard<std::mutex> l(logs_lock_);
   ulong                       last = std::min(index + cnt, next_slot_);
   for (ulong ii = index; ii < last; ++ii) {
      auto p = (log_dir_ / std::to_string(ii)).concat(".log");
      size_total += std::filesystem::file_size(p);
   }

   ptr<buffer> buf_out = buffer::alloc(sizeof(int32) + cnt * sizeof(int32) + size_total);
   buf_out->pos(0);
   buf_out->put((int32)cnt);

   for (ulong ii = index; ii < last; ++ii) {
      auto  p  = (log_dir_ / std::to_string(ii)).concat(".log");
      int32 sz = std::filesystem::file_size(p);
      buf_out->put(sz);
      auto fp = std::unique_ptr<FILE, decltype(&fclose)>{fopen(p.c_str(), "r"), &fclose};
      fread(buf_out->get_raw(sz), 1, sz, fp.get());
   }
   return buf_out;
}

void log_store::apply_pack(ulong index, buffer& pack) {
   pack.pos(0);
   int32                 num_logs = pack.get_int();
   std::vector<fs::path> tmp_paths;
   for (int32 ii = 0; ii < num_logs; ++ii) {
      size_t buf_size;
      auto   data = pack.get_bytes(buf_size);
      tmp_paths.push_back(store_log(log_dir_, data, buf_size));
   }

   std::lock_guard<std::mutex> l(logs_lock_);
   for (int32 ii = 0; ii < num_logs; ++ii) {
      fs::rename(tmp_paths[ii], path_for_index(log_dir_, index + ii));
   }
   if (index >= next_slot_) {
      // TODO: remove all pre-existing logs
      start_idx_ = index;
   } else
      start_idx_ = std::min(start_idx_, index);

   next_slot_ = index + num_logs;
}

bool log_store::compact(ulong last_log_index) {
   std::lock_guard<std::mutex> l(logs_lock_);
   for (ulong ii = start_idx_; ii <= last_log_index; ++ii) {
      auto            p = (log_dir_ / std::to_string(ii)).concat(".log");
      std::error_code ec;
      std::filesystem::remove(p, ec);
   }

   // WARNING:
   //   Even though nothing has been erased,
   //   we should set `start_idx_` to new index.
   start_idx_ = last_log_index + 1;
   return true;
}

} // namespace blockvault
