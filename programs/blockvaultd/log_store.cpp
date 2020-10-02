#include "log_store.hpp"
#include "utils.hpp"
#include <algorithm>
#include <boost/container/flat_map.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <cassert>
#include <iostream>
#include <libnuraft/logger.hxx>
#include <regex>
#include <unistd.h>
namespace blockvault {

/*
 * The log store is partitioned into segments. Each segments has a fix number of entries (defined by log_stride). Each
 * segments is further stored with 2 files: data file and index file. For example, given the log_stride 1000, the log
 * entries 0 to 999 would be stored in `log-entry-0.data' with index file `log-entry-0.index`, and the entries 1000 to
 * 1999 would be stored in `log-entry-1000.data' with index file `log-entry-1000.index`. The data file is a file to
 * store log entries payload and their correspoding header. Entries should only be appended to the data file. The index
 * file stores each entry positions in the data file that enables O(1) random access lookup by entry index number.
 *
 *                                                Data file format
 * +--------------------+
 * | Data file Preamble |
 * +----------------+-----------------+----------------+-----------------+-----+-------------------+--------------------+
 * | Entry 1 header | Entry 1 payload | Entry 2 header | Entry 2 payload | ... | Last entry header | Last entry payload
 * |
 * +----------------+-----------------+----------------+-----------------+-----+-------------------+--------------------+
 *
 * +--------------------------------+------------------------------+---------+--------------------------+
 * |      Entry 1 end position      |      Entry 2 end position    | ....... | Last entry  end position |
 * +--------------------------------+------------------------------+---------+--------------------------+
 *                                                Index file format
 */

using namespace nuraft;
namespace fs = std::filesystem;

const uint32_t log_store_version = 0;

struct data_file_preamble {
   uint64_t start_index = 0;
   uint32_t version     = log_store_version;
   data_file_preamble() = default;
   data_file_preamble(uint64_t idx)
       : start_index(idx) {}
};

struct __attribute__((__packed__)) log_entry_header {
   uint32_t length; // this should the sum of sizeof(term), sizeof(type) and payload size
   ulong    term;
   byte     type;

   log_entry_header() = default;
   log_entry_header(ulong log_entry_term, nuraft::log_val_type log_entry_type, uint32_t payload_size)
       : length(payload_size + sizeof(term) + sizeof(type))
       , term(log_entry_term)
       , type(static_cast<byte>(log_entry_type)) {}

   uint32_t get_payload_size() const { return length - sizeof(term) - sizeof(type); }
};

ptr<log_entry> empty_log_entry() {
   static ptr<buffer>    buf = buffer::alloc(sz_ulong);
   static ptr<log_entry> r   = cs_new<log_entry>(0, buf);
   return r;
}

using data_collection_t = boost::container::flat_map<ulong, fs::path>;

class tmp_data_collection_t {
   data_collection_t impl_;

 public:
   tmp_data_collection_t()                             = default;
   tmp_data_collection_t(const tmp_data_collection_t&) = delete;
   tmp_data_collection_t& operator=(const tmp_data_collection_t&) = delete;

   ~tmp_data_collection_t() {
      for (auto& v : impl_) {
         fs::remove(v.second);
      }
   }

   void assign(data_collection_t::const_iterator first, data_collection_t::const_iterator last) {
      std::for_each(first, last, [this](const auto& v) {
         fs::path orig_path = v.second;
         char     hard_linked_name[PATH_MAX];
         snprintf(hard_linked_name, PATH_MAX, "%s-XXXXXX", orig_path.c_str());
         mktemp(hard_linked_name);
         fs::path new_path = hard_linked_name;
         impl_.emplace(v.first, new_path);
         fs::create_hard_link(orig_path.replace_extension("data"), new_path.replace_extension("data"));
      });
   }

   data_collection_t::const_iterator cbegin() const { return impl_.cbegin(); }
   data_collection_t::const_iterator cend() const { return impl_.cend(); }
};

struct log_catalog {
   using size_type                 = typename data_collection_t::size_type;
   static constexpr size_type npos = std::numeric_limits<size_type>::max();

   data_collection_t collection_;
   size_type         active_index_ = npos;
   file_reader       log_data_;
   file_reader       log_index_;

   bool empty() const { return collection_.empty(); }

   void clear() {
      for (const auto& e : collection_) {
         auto prefix = e.second;
         fs::remove(prefix.replace_extension("index"));
         fs::remove(prefix.replace_extension("data"));
      }

      collection_.clear();
      active_index_ = npos;
   }

   uint32_t start_index() const {
      if (empty())
         return 0;
      return collection_.begin()->first;
   }

   fs::path current_log_prefix() { return collection_.rbegin()->second; }

   int open(const fs::path& log_dir) {

      const char*                               pattern = "log-entry-\\d+\\.index";
      boost::container::flat_map<ulong, size_t> num_entries;

      for_each_file_in_dir_matches(log_dir, pattern, [this, &num_entries](const fs::path& index_path) {
         auto index_file_size = fs::file_size(index_path);
         if (index_file_size % sizeof(uint64_t) != 0)
            throw std::runtime_error(std::string("log_store: invalid index file ") + index_path.string());

         auto path_without_extension = fs::path(index_path).replace_extension();
         auto data_path              = fs::path(index_path).replace_extension("data");

         file_reader        log(data_path);
         data_file_preamble preamble;
         log.read(preamble);

         num_entries[preamble.start_index] = index_file_size / sizeof(uint64_t);
         collection_[preamble.start_index] = path_without_extension;
      });

      ulong next_start_index = 0;
      // make sure there's no missing entries in between log entries files.
      auto i = collection_.cbegin();
      auto j = num_entries.cbegin();
      for (; i < collection_.cend(); ++i, ++j) {
         if (next_start_index != 0 && next_start_index != i->first) {
            throw std::runtime_error("log_store: the persistent log store has missing entries");
         }
         next_start_index = i->first + j->second;
      }

      if (next_start_index > 0) {
         // when the index and data file does not match, we trust index instead of the data file,
         // because it's earier to make writing to the index file transactional.
         auto last_log_index               = next_start_index - 1;
         auto [last_seg_index, seg_prefix] = *collection_.rbegin();
         auto     expect_data_file_size    = end_of_entry_position(collection_.end() - 1, last_log_index);
         fs::path last_seg_data_file       = fs::path(seg_prefix).replace_extension("data");
         auto     actual_data_file_size    = fs::file_size(last_seg_data_file);
         if (actual_data_file_size < expect_data_file_size) {
            // this shouldn't happen unless someone manually messes with index or data files.
            throw std::runtime_error(
                "log_store: the persisted log store is not in a consistent state which can be recovered from");
         } else if (actual_data_file_size > expect_data_file_size) {
            // this could happen when the system crashes while writing the log data
            fs::resize_file(last_seg_data_file, expect_data_file_size);
         }
      }
      return next_start_index;
   }

   uint64_t set_entry_position_by_index_offset(ulong index_offset) {
      uint64_t data_offset;
      if (index_offset == 0)
         data_offset = sizeof(data_file_preamble);
      else {
         log_index_.seek((index_offset - 1) * sizeof(uint64_t));
         log_index_.read(data_offset);
      }
      log_data_.seek(data_offset);
      return data_offset;
   }

   uint64_t set_entry_position(ulong entry_index) {
      if (active_index_ != npos) {
         auto active_item = collection_.nth(active_index_);
         auto next_item   = active_item + 1;
         if (active_item->first <= entry_index && (next_item == collection_.end() || entry_index < next_item->first)) {
            return set_entry_position_by_index_offset(entry_index - active_item->first);
         }
      }
      if (collection_.empty() || entry_index < collection_.begin()->first)
         throw std::runtime_error("log_store: invalid log entry index");

      auto it             = --collection_.upper_bound(entry_index);
      auto name           = it->second;
      log_data_           = file_reader(name.replace_extension("data"));
      log_index_          = file_reader(name.replace_extension("index"));
      this->active_index_ = collection_.index_of(it);
      return set_entry_position_by_index_offset(entry_index - it->first);
   }

   ptr<log_entry> get_entry(ulong entry_index) {
      set_entry_position(entry_index);
      log_entry_header header;
      log_data_.read(header);
      auto        payload_size = header.get_payload_size();
      ptr<buffer> buf          = buffer::alloc(payload_size);
      log_data_.read(buf->get_raw(payload_size), payload_size);
      buf->pos(0);
      return cs_new<log_entry>(header.term, buf, static_cast<log_val_type>(header.type));
   }

   ulong term_at(ulong entry_index) {
      set_entry_position(entry_index);

      log_entry_header header;
      log_data_.read(header);
      return header.term;
   }

   void add(ulong start_entry_index, const fs::path& new_path) {
      this->collection_.emplace(start_entry_index, new_path);
   }

   void compact_back(ulong last_log_index) {
      auto itr = collection_.lower_bound(last_log_index);
      if (itr == collection_.end()) {
         return;
      }

      std::for_each(collection_.rbegin(), data_collection_t::reverse_iterator(itr), [](const auto& entry) {
         auto filebase = entry.second;
         fs::remove(filebase.replace_extension("index"));
         fs::remove(filebase.replace_extension("data"));
      });

      auto [seg_index, prefix] = *collection_.rbegin();
      collection_.erase(itr, collection_.end());

      if (!collection_.empty()) {
         auto [seg_index, prefix] = *collection_.rbegin();
         ulong num_entry_to_keep  = last_log_index - seg_index + 1;
         auto  data_file_size     = end_of_entry_position(collection_.end() - 1, last_log_index);
         fs::resize_file(fs::path(prefix).replace_extension("index"), num_entry_to_keep * sizeof(uint64_t));
         fs::resize_file(fs::path(prefix).replace_extension("data"), data_file_size);
      }
      active_index_ = npos;
   }

   void compact_front(ulong last_log_index) {
      data_collection_t::const_iterator itr = collection_.upper_bound(last_log_index);
      if (itr == collection_.cbegin()) {
         return;
      }

      if (itr->first != last_log_index + 1)
         --itr;
      std::for_each(collection_.cbegin(), itr, [](const auto& entry) {
         auto filebase = entry.second;
         fs::remove(filebase.replace_extension("index"));
         fs::remove(filebase.replace_extension("data"));
      });

      collection_.erase(collection_.begin(), itr);
      active_index_ = npos;
   }

   struct pack_read_segment {
      fs::path path;
      uint64_t start, end;
   };

   uint64_t start_of_entry_position(data_collection_t::const_iterator it, ulong entry_index) {
      if (it->first == entry_index) {
         return sizeof(data_file_preamble);
      }
      return end_of_entry_position(it, entry_index - 1);
   }

   uint64_t end_of_entry_position(data_collection_t::const_iterator it, ulong idx) {
      file_reader index_reader(fs::path(it->second).replace_extension("index"));
      uint64_t    position;
      int         offset = (idx - it->first) * sizeof(uint64_t);
      index_reader.seek(offset);
      index_reader.read(position);
      return position;
   }

   ptr<buffer> pack(ulong index, int32 cnt) {
      if (cnt == 0)
         return ptr<buffer>{};
      data_collection_t::const_iterator start_pack_itr = collection_.upper_bound(index);
      if (start_pack_itr == collection_.cbegin()) {
         return ptr<buffer>{};
      }
      --start_pack_itr;

      data_collection_t::const_iterator end_pack_itr   = collection_.upper_bound(index + cnt);
      ulong                             end_pack_index = index + cnt;

      std::vector<pack_read_segment> segments;
      segments.reserve(end_pack_itr - start_pack_itr);
      for (auto i = start_pack_itr; i < end_pack_itr && i->first != end_pack_index; ++i) {
         auto path             = fs::path(i->second).replace_extension("data");
         auto last_entry_index = end_pack_index;
         if (i + 1 < end_pack_itr) {
            last_entry_index = (i + 1)->first;
         }
         segments.push_back(
             pack_read_segment{path, sizeof(data_file_preamble), end_of_entry_position(i, last_entry_index - 1)});
      }

      segments.front().start = start_of_entry_position(start_pack_itr, index);

      // calculate the needed buffer size
      uint64_t buf_size = 0;
      for (const auto& seg : segments) {
         buf_size += (seg.end - seg.start);
      }

      ptr<buffer> buf_out = buffer::alloc(sizeof(int32) + buf_size);
      buf_out->pos(0);
      buf_out->put((int32)cnt);

      for (const auto& seg : segments) {
         file_reader reader(seg.path);
         reader.seek(seg.start);
         std::size_t sz = seg.end - seg.start;
         reader.read(buf_out->get_raw(sz), sz);
      }
      buf_out->pos(0);
      return buf_out;
   }

   static void for_each_app_entry(ulong start_index, ulong end_index, uint64_t start_position,
                                  data_collection_t::const_iterator                   start_itr,
                                  data_collection_t::const_iterator                   end_itr,
                                  const std::function<void(ulong, std::string_view)>& callback) {

      ulong cur_idx = start_index;

      for (auto it = start_itr; it != end_itr; ++it) {
         auto                                 data_file_path = fs::path(it->second).replace_extension("data");
         boost::iostreams::mapped_file_source src(data_file_path.generic_string());
         auto                                 data = src.data() + start_position;
         while (data < src.data() + src.size() && cur_idx < end_index) {
            log_entry_header header;
            memcpy(&header, data, sizeof(header));
            if (header.type == log_val_type::app_log) {
               callback(cur_idx, std::string_view(data + sizeof(header), header.get_payload_size()));
            }
            cur_idx++;
            data += sizeof(uint32_t) + header.length;
         }
         start_position = sizeof(data_file_preamble);
      }
   }

   void thr_safe_for_each_app_entry(std::mutex& lock, ulong start_index, ulong end_index,
                                    const std::function<void(ulong, std::string_view)>& callback) {
      uint64_t              start_position;
      tmp_data_collection_t new_collection;
      {
         std::lock_guard<std::mutex> guard(lock);
         auto                        start_itr = collection_.upper_bound(start_index);
         if (start_itr != collection_.cbegin())
            --start_itr;
         else // start_index is smaller than smallest index of kept log entries, adjust it.
            start_index = start_itr->first;

         auto end_itr   = collection_.lower_bound(end_index);
         start_position = start_of_entry_position(start_itr, start_index);
         new_collection.assign(start_itr, end_itr);
      }
      for_each_app_entry(start_index, end_index, start_position, new_collection.cbegin(), new_collection.cend(),
                         callback);
   }
};

struct log_store_impl {
   std::filesystem::path log_dir_;
   log_catalog           catalog_;
   ulong                 next_slot_ = 1;
   file_writer           data_out_;
   file_writer           index_out_;
   uint32_t              log_stride_;
   std::mutex            logs_lock_;

   log_store_impl(std::filesystem::path log_dir, uint32_t log_stride)
       : log_dir_(log_dir)
       , log_stride_(log_stride) {

      // find existing index/data files

      next_slot_ = catalog_.open(log_dir);
      if (next_slot_ == 0) {
         this->append(0, log_val_type::app_log, nullptr, 0);
      }
   }

   void write_data_file_preamble(ulong start_index) {
      char buf[128];
      snprintf(buf, 128, "log-entry-%llu", start_index);

      fs::path active_prefix = log_dir_ / buf;
      catalog_.add(start_index, active_prefix);
      data_out_  = file_writer(active_prefix.replace_extension("data"), "wb");
      index_out_ = file_writer(active_prefix.replace_extension("index"), "wb");

      data_out_.write(data_file_preamble{start_index});
   }

   void prepare_out_files_for_append() {
      fs::path active_prefix = catalog_.current_log_prefix();
      data_out_              = file_writer(active_prefix.replace_extension("data"), "ab");
      index_out_             = file_writer(active_prefix.replace_extension("index"), "ab");
   }

   void append(ulong term, log_val_type type, const byte* payload, size_t payload_size) {
      if (next_slot_ % log_stride_ == 0) {
         write_data_file_preamble(next_slot_);
      } else if (!data_out_.is_open() || !index_out_.is_open()) {
         prepare_out_files_for_append();
      }

      data_out_.write(log_entry_header{term, type, static_cast<uint32_t>(payload_size)});
      data_out_.sync_write(payload, payload_size);
      uint64_t position = data_out_.cur_position();
      index_out_.sync_write(position);
      next_slot_++;
   }

   ulong append_entry(ptr<log_entry>& entry) {
      auto&                       buf = entry->get_buf();
      std::lock_guard<std::mutex> l(logs_lock_);
      append(entry->get_term(), entry->get_val_type(), buf.data_begin(), buf.size());
      return next_slot_;
   }

   ptr<log_entry> last_entry() {
      std::lock_guard<std::mutex> l(logs_lock_);
      return catalog_.get_entry(next_slot_ - 1);
   }

   void write_at(ulong idx, ptr<log_entry>& entry) {
      auto&                       buf = entry->get_buf();
      std::lock_guard<std::mutex> l(logs_lock_);
      if (idx < next_slot_) {
         catalog_.compact_back(idx - 1);
         next_slot_ = idx;
         data_out_.close();
         append(entry->get_term(), entry->get_val_type(), buf.data_begin(), buf.size());
      }
   }

   ptr<std::vector<ptr<log_entry>>> log_entries(ulong start, ulong end) {

      ptr<std::vector<ptr<log_entry>>> ret = cs_new<std::vector<ptr<log_entry>>>();
      ret->reserve(end - start);
      std::lock_guard<std::mutex> l(logs_lock_);
      if (start < start_index() || end > this->next_slot_)
         throw std::runtime_error("log_store: log_entries() requests nonexistent entry");
      for (ulong i = start; i < end; ++i) {
         ret->emplace_back(catalog_.get_entry(i));
      }
      return ret;
   }

   ptr<std::vector<ptr<log_entry>>> log_entries_ext(ulong start, ulong end, int64 batch_size_hint_in_bytes) {
      ptr<std::vector<ptr<log_entry>>> ret = cs_new<std::vector<ptr<log_entry>>>();
      ret->reserve(end - start);
      int64_t                     total_size = 0;
      std::lock_guard<std::mutex> l(logs_lock_);
      if (start < start_index() || end > this->next_slot_)
         throw std::runtime_error("log_store: log_entries_ext() requests nonexistent entry");

      for (ulong i = start; i < end && total_size < batch_size_hint_in_bytes; ++i) {
         ret->emplace_back(catalog_.get_entry(i));
         total_size += ret->back()->get_buf().size();
      }
      return ret;
   }

   ulong start_index() const { return catalog_.start_index(); }

   ptr<log_entry> entry_at(ulong index) {
      std::lock_guard<std::mutex> l(logs_lock_);
      if (index >= start_index() && index < next_slot_)
         return catalog_.get_entry(index);
      return empty_log_entry();
   }

   ulong term_at(ulong index) {
      std::lock_guard<std::mutex> l(logs_lock_);
      if (index >= start_index() && index < next_slot_)
         return catalog_.term_at(index);
      return 0;
   }

   ptr<buffer> pack(ulong index, int32 cnt) {
      std::lock_guard<std::mutex> l(logs_lock_);
      return catalog_.pack(index, cnt);
   }

   void apply_pack_segment(ulong seg_index, std::vector<uint64_t>& segment_positions, const byte* seg_begin,
                           const byte* seg_end) {

      if (catalog_.empty() || seg_index % log_stride_ == 0)
         write_data_file_preamble(seg_index);
      else
         prepare_out_files_for_append();

      auto offset = data_out_.cur_position();

      std::for_each(segment_positions.begin(), segment_positions.end(), [offset](auto& pos) { pos += offset; });
      // write data
      data_out_.sync_write(seg_begin, seg_end - seg_begin);
      // write index
      index_out_.sync_write(&segment_positions.front(), segment_positions.size() * sizeof(uint64_t));
   }

   void apply_pack(ulong index, buffer& pack) {
      pack.pos(0);
      int32                       num_entries = pack.get_int();
      std::lock_guard<std::mutex> l(logs_lock_);
      if (next_slot_ > index)
         catalog_.compact_back(index - 1);
      else {
         data_out_.close();
         index_out_.close();
         catalog_.clear();
      }

      next_slot_ = index;

      std::vector<uint64_t> segment_positions;
      segment_positions.reserve(this->log_stride_);

      const byte* it                = pack.data();
      const byte* end               = pack.data_begin() + pack.size();
      const byte* cur_segment_begin = it;
      ulong       next_index        = index;

      while (it < end) {
         uint32_t size;
         memcpy(&size, it, sizeof(size));
         it += sizeof(uint32_t) + size;
         segment_positions.push_back(it - cur_segment_begin);
         ++next_index;
         if (next_index % this->log_stride_ == 0) {
            apply_pack_segment(next_index - segment_positions.size(), segment_positions, cur_segment_begin, it);
            segment_positions.resize(0);
            cur_segment_begin = it;
            next_slot_        = next_index;
         }
      }

      if (cur_segment_begin != end) {
         apply_pack_segment(next_slot_, segment_positions, cur_segment_begin, it);
         next_slot_ = next_index;
      }

      if (next_slot_ != index + num_entries)
         throw std::runtime_error(
             "log_store: log store received pack whose declared size does not match what it actually contains");
   }

   bool compact(ulong last_log_index) {
      if (last_log_index < start_index())
         return false;
      std::lock_guard<std::mutex> l(logs_lock_);
      catalog_.compact_front(last_log_index);
      data_out_.close();
      return true;
   }
};

log_store::log_store(std::filesystem::path log_dir, uint32_t stride)
    : impl_(new log_store_impl(log_dir, stride)) {}

log_store::~log_store() { delete impl_; }

ulong log_store::next_slot() const { return impl_->next_slot_; }

ulong log_store::start_index() const { return impl_->start_index(); }

ptr<log_entry> log_store::last_entry() const { return impl_->last_entry(); }

ulong log_store::append(ptr<log_entry>& entry) { return impl_->append_entry(entry); }

void log_store::write_at(ulong index, ptr<log_entry>& entry) { impl_->write_at(index, entry); }

ptr<std::vector<ptr<log_entry>>> log_store::log_entries(ulong start, ulong end) {
   return impl_->log_entries(start, end);
}

ptr<std::vector<ptr<log_entry>>> log_store::log_entries_ext(ulong start, ulong end, int64 batch_size_hint_in_bytes) {
   return impl_->log_entries_ext(start, end, batch_size_hint_in_bytes);
}

ptr<log_entry> log_store::entry_at(ulong index) { return impl_->entry_at(index); }

ulong log_store::term_at(ulong index) { return impl_->term_at(index); }

ptr<buffer> log_store::pack(ulong index, int32 cnt) { return impl_->pack(index, cnt); }

void log_store::apply_pack(ulong index, buffer& pack) { impl_->apply_pack(index, pack); }

bool log_store::compact(ulong last_log_index) { return impl_->compact(last_log_index); }

void log_store::for_each_app_entry(ulong start_index, ulong end_index,
                                   const std::function<void(ulong, std::string_view)>& callback) {
   impl_->catalog_.thr_safe_for_each_app_entry(impl_->logs_lock_, start_index, end_index, callback);
}

} // namespace blockvault
