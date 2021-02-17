#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <fc/io/cfile.hpp>
#include <fc/io/datastream.hpp>
#include <regex>

namespace eosio {
namespace chain {

namespace bfs = boost::filesystem;

template <typename Lambda>
void for_each_file_in_dir_matches(const bfs::path& dir, std::string pattern, Lambda&& lambda) {
   const std::regex        my_filter(pattern);
   std::smatch             what;
   bfs::directory_iterator end_itr; // Default ctor yields past-the-end
   for (bfs::directory_iterator p(dir); p != end_itr; ++p) {
      // Skip if not a file
      if (!bfs::is_regular_file(p->status()))
         continue;
      // skip if it does not match the pattern
      if (!std::regex_match(p->path().filename().string(), what, my_filter))
         continue;
      lambda(p->path());
   }
}

struct null_verifier {
   template <typename LogData>
   void verify(const LogData&, const bfs::path&) {}
};

template <typename LogData, typename LogIndex, typename LogVerifier = null_verifier>
struct log_catalog {
   using block_num_t = uint32_t;

   struct mapped_type {
      block_num_t last_block_num;
      bfs::path   filename_base;
   };
   using collection_t              = boost::container::flat_map<block_num_t, mapped_type>;
   using size_type                 = typename collection_t::size_type;
   static constexpr size_type npos = std::numeric_limits<size_type>::max();

   using mapmode = boost::iostreams::mapped_file::mapmode;

   bfs::path    retained_dir;
   bfs::path    archive_dir;
   size_type    max_retained_files = 10;
   collection_t collection;
   size_type    active_index = npos;
   LogData      log_data;
   LogIndex     log_index;
   LogVerifier  verifier;

   bool empty() const { return collection.empty(); }

   uint32_t first_block_num() const {
      if (empty())
         return 0;
      return collection.begin()->first;
   }

   uint32_t last_block_num() const {
      if (empty())
         return 0;
      return collection.rbegin()->second.last_block_num;
   }

   static bfs::path make_abosolute_dir(const bfs::path& base_dir, bfs::path new_dir) {
      if (new_dir.is_relative())
         new_dir = base_dir / new_dir;

      if (!bfs::is_directory(new_dir))
         bfs::create_directories(new_dir);

      return new_dir;
   }

   void open(const bfs::path& log_dir, const bfs::path& retained_dir, const bfs::path& archive_dir, const char* name,
             const char* suffix_pattern = R"(-\d+-\d+\.log)") {

      this->retained_dir = make_abosolute_dir(log_dir, retained_dir.empty() ? log_dir : retained_dir);
      if (!archive_dir.empty()) {
         this->archive_dir = make_abosolute_dir(log_dir, archive_dir);
      }

      for_each_file_in_dir_matches(this->retained_dir, std::string(name) + suffix_pattern, [this](bfs::path path) {
         auto log_path               = path;
         auto index_path             = path.replace_extension("index");
         auto path_without_extension = log_path.parent_path() / log_path.stem().string();

         LogData log(log_path);

         verifier.verify(log, log_path);

         // check if index file matches the log file
         if (!index_matches_data(index_path, log))
            log.construct_index(index_path);

         auto existing_itr = collection.find(log.first_block_num());
         if (existing_itr != collection.end()) {
            if (log.last_block_num() <= existing_itr->second.last_block_num) {
               wlog("${log_path} contains the overlapping range with ${existing_path}.log, dropping ${log_path} "
                    "from catalog",
                    ("log_path", log_path.string())("existing_path", existing_itr->second.filename_base.string()));
               return;
            } else {
               wlog(
                   "${log_path} contains the overlapping range with ${existing_path}.log, droping ${existing_path}.log "
                   "from catelog",
                   ("log_path", log_path.string())("existing_path", existing_itr->second.filename_base.string()));
            }
         }

         collection.insert_or_assign(log.first_block_num(), mapped_type{log.last_block_num(), path_without_extension});
      });
   }

   bool index_matches_data(const bfs::path& index_path, const LogData& log) const {
      if (!bfs::exists(index_path))
         return false;

      auto num_blocks_in_index = bfs::file_size(index_path) / sizeof(uint64_t);
      if (num_blocks_in_index != log.num_blocks())
         return false;

      // make sure the last 8 bytes of index and log matches
      fc::cfile index_file;
      index_file.set_file_path(index_path);
      index_file.open("r");
      index_file.seek_end(-sizeof(uint64_t));
      uint64_t pos;
      index_file.read(reinterpret_cast<char*>(&pos), sizeof(pos));
      return pos == log.last_block_position();
   }

   std::optional<uint64_t> get_block_position(uint32_t block_num, mapmode mode = mapmode::readonly) {
      try {
         if (active_index != npos) {
            auto active_item = collection.nth(active_index);
            if (active_item->first <= block_num && block_num <= active_item->second.last_block_num &&
                log_data.flags() == mode) {
               return log_index.nth_block_position(block_num - log_data.first_block_num());
            }
         }
         if (collection.empty() || block_num < collection.begin()->first)
            return {};

         auto it = --collection.upper_bound(block_num);

         if (block_num <= it->second.last_block_num) {
            auto name = it->second.filename_base;
            log_data.open(name.replace_extension("log"), mode);
            log_index.open(name.replace_extension("index"));
            this->active_index = collection.index_of(it);
            return log_index.nth_block_position(block_num - log_data.first_block_num());
         }
         return {};
      } catch (...) {
         this->active_index = npos;
         return {};
      }
   }

   std::pair<fc::datastream<const char*>, uint32_t> ro_stream_for_block(uint32_t block_num) {
      auto pos = get_block_position(block_num, mapmode::readonly);
      if (pos) {
         return log_data.ro_stream_at(*pos);
      }
      return {fc::datastream<const char*>(nullptr, 0), static_cast<uint32_t>(0)};
   }

   std::pair<fc::datastream<char*>, uint32_t> rw_stream_for_block(uint32_t block_num) {
      auto pos = get_block_position(block_num, mapmode::readwrite);
      if (pos) {
         return log_data.rw_stream_at(*pos);
      }
      return {fc::datastream<char*>(nullptr, 0), static_cast<uint32_t>(0)};
   }

   std::optional<block_id_type> id_for_block(uint32_t block_num) {
      auto pos = get_block_position(block_num, mapmode::readonly);
      if (pos) {
         return log_data.block_id_at(*pos);
      }
      return {};
   }

   static void rename_if_not_exists(bfs::path old_name, bfs::path new_name) {
      if (!bfs::exists(new_name)) {
         bfs::rename(old_name, new_name);
      } else {
         bfs::remove(old_name);
         wlog("${new_name} already exists, just removing ${old_name}",
              ("old_name", old_name.string())("new_name", new_name.string()));
      }
   }

   static void rename_bundle(bfs::path orig_path, bfs::path new_path) {
      rename_if_not_exists(orig_path.replace_extension(".log"), new_path.replace_extension(".log"));
      rename_if_not_exists(orig_path.replace_extension(".index"), new_path.replace_extension(".index"));
   }

   /// Add a new entry into the catalog.
   ///
   /// Notice that \c start_block_num must be monotonically increasing between the invocations of this function
   /// so that the new entry would be inserted at the end of the flat_map; otherwise, \c active_index would be
   /// invalidated and the mapping between the log data their block range would be wrong. This function is only used
   /// during the splitting of block log. Using this function for other purpose should make sure if the monotonically
   /// increasing block num guarantee can be met.
   void add(uint32_t start_block_num, uint32_t end_block_num, const bfs::path& dir, const char* name) {

      const int bufsize = 64;
      char      buf[bufsize];
      snprintf(buf, bufsize, "%s-%d-%d", name, start_block_num, end_block_num);
      bfs::path new_path = retained_dir / buf;
      rename_bundle(dir / name, new_path);

      if (this->collection.size() >= max_retained_files) {
         const auto items_to_erase =
             max_retained_files > 0 ? this->collection.size() - max_retained_files + 1 : this->collection.size();
         for (auto it = this->collection.begin(); it < this->collection.begin() + items_to_erase; ++it) {
            auto orig_name = it->second.filename_base;
            if (archive_dir.empty()) {
               // delete the old files when no backup dir is specified
               bfs::remove(orig_name.replace_extension("log"));
               bfs::remove(orig_name.replace_extension("index"));
            } else {
               // move the the archive dir
               rename_bundle(orig_name, archive_dir / orig_name.filename());
            }
         }
         this->collection.erase(this->collection.begin(), this->collection.begin() + items_to_erase);
         this->active_index = this->active_index == npos || this->active_index < items_to_erase
                                  ? npos
                                  : this->active_index - items_to_erase;
      }
      if (max_retained_files > 0)
         this->collection.emplace(start_block_num, mapped_type{end_block_num, new_path});
   }

   /// Truncate the catalog so that the log/index bundle containing the block with \c block_num
   /// would be rename to \c new_name; the log/index bundles with blocks strictly higher
   /// than \c block_num would be deleted, and all the renamed/removed entries would be erased
   /// from the catalog.
   ///
   /// \return if nonzero, it's the starting block number for the log/index bundle being renamed.
   uint32_t truncate(uint32_t block_num, bfs::path new_name) {
      if (collection.empty())
         return 0;

      auto remove_files = [](typename collection_t::const_reference v) {
         auto name = v.second.filename_base;
         bfs::remove(name.replace_extension("log"));
         bfs::remove(name.replace_extension("index"));
      };

      auto it = collection.upper_bound(block_num);

      if (it == collection.begin() || block_num > (it - 1)->second.last_block_num) {
         std::for_each(it, collection.end(), remove_files);
         collection.erase(it, collection.end());
         return 0;
      } else {
         auto truncate_it = --it;
         auto name        = truncate_it->second.filename_base;
         bfs::rename(name.replace_extension("log"), new_name.replace_extension("log"));
         bfs::rename(name.replace_extension("index"), new_name.replace_extension("index"));
         std::for_each(truncate_it + 1, collection.end(), remove_files);
         auto result = truncate_it->first;
         collection.erase(truncate_it, collection.end());
         return result;
      }
   }
};

} // namespace chain
} // namespace eosio
