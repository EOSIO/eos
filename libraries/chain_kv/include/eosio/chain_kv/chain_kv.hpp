#pragma once

#include <fc/io/raw.hpp>
#include <optional>
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <stdexcept>
#include <softfloat.hpp>
#include <algorithm>

namespace eosio::chain_kv {

class exception : public std::exception {
   std::string msg;

 public:
   exception(std::string&& msg) : msg(std::move(msg)) {}
   exception(const exception&) = default;
   exception(exception&&)      = default;

   exception& operator=(const exception&) = default;
   exception& operator=(exception&&) = default;

   const char* what() const noexcept override { return msg.c_str(); }
};

inline void check(rocksdb::Status s, const char* prefix) {
   if (!s.ok())
      throw exception(prefix + s.ToString());
}

using bytes = std::vector<char>;

inline rocksdb::Slice to_slice(const bytes& v) { return { v.data(), v.size() }; }

inline bytes to_bytes(const rocksdb::Slice& v) { return { v.data(), v.data() + v.size() }; }

inline std::shared_ptr<bytes> to_shared_bytes(const rocksdb::Slice& v) {
   return std::make_shared<bytes>(v.data(), v.data() + v.size());
}

// Bypasses fc's vector size limit
template <typename Stream>
void pack_bytes(Stream& s, const bytes& b) {
   fc::unsigned_int size(b.size());
   if (size.value != b.size())
      throw exception("bytes is too big");
   fc::raw::pack(s, size);
   s.write(b.data(), b.size());
}

template <typename Stream>
void pack_optional_bytes(Stream& s, const bytes* b) {
   fc::raw::pack(s, bool(b));
   if (b)
      pack_bytes(s, *b);
}

template <typename Stream>
std::pair<const char*, size_t> get_bytes(Stream& s) {
   fc::unsigned_int size;
   fc::raw::unpack(s, size);
   if (size > s.remaining())
      throw exception("bad size for bytes");
   auto data = s.pos();
   s.skip(size.value);
   return { data, size.value };
}

template <typename Stream>
std::pair<const char*, size_t> get_optional_bytes(Stream& s) {
   bool present;
   fc::raw::unpack(s, present);
   if (present)
      return get_bytes(s);
   else
      return { nullptr, 0 };
}

template <typename A, typename B>
inline int compare_blob(const A& a, const B& b) {
   static_assert(std::is_same_v<std::decay_t<decltype(*a.data())>, char> ||
                 std::is_same_v<std::decay_t<decltype(*a.data())>, unsigned char>);
   static_assert(std::is_same_v<std::decay_t<decltype(*b.data())>, char> ||
                 std::is_same_v<std::decay_t<decltype(*b.data())>, unsigned char>);
   auto r = memcmp(a.data(), b.data(), std::min(a.size(), b.size()));
   if (r)
      return r;
   if (a.size() < b.size())
      return -1;
   if (a.size() > b.size())
      return 1;
   return 0;
}

struct less_blob {
   using is_transparent = void;

   template <typename A, typename B>
   bool operator()(const A& a, const B& b) const {
      return compare_blob(a, b) < 0;
   }
};

inline bytes get_next_prefix(const bytes& prefix) {
   bytes next_prefix = prefix;
   while (!next_prefix.empty()) {
      if (++next_prefix.back())
         break;
      next_prefix.pop_back();
   }
   return next_prefix;
}

namespace detail {
   using uint128_t = __uint128_t;

   template<typename T>
   struct value_storage;

   template<typename T>
   struct value_storage<T*> {
      using type = T;
      constexpr T* as_ptr(T* value) const { return (value); }
   };

   template<typename T>
   struct value_storage {
      using type = T;
      constexpr T* as_ptr(T& value) const { return &value; }
   };

   template <typename T, std::size_t N>
   auto append_key(bytes& dest, T value) {

      using t_type = typename value_storage<T>::type;
      t_type t_array[N];
      const t_type* first = value_storage<T>().as_ptr(value);
      const t_type* last = first + N;
      std::reverse_copy(first, last, std::begin(t_array));

      char* t_array_as_char_begin = reinterpret_cast<char*>(t_array);
      char* t_array_as_char_end = reinterpret_cast<char*>(t_array + N);
      std::reverse(t_array_as_char_begin, t_array_as_char_end);
      dest.insert(dest.end(), t_array_as_char_begin, t_array_as_char_end);
   }

   template <typename UInt, typename T>
   UInt float_to_key(T value) {
      static_assert(sizeof(T) == sizeof(UInt), "Expected unsigned int of the same size");
      UInt result;
      std::memcpy(&result, &value, sizeof(T));
      const UInt signbit = (static_cast<UInt>(1) << (std::numeric_limits<UInt>::digits - 1));
      UInt mask    = 0;
      if (result == signbit)
         result = 0;
      if (result & signbit)
         mask = ~mask;
      return result ^ (mask | signbit);
   }

   template <typename T, typename UInt>
   T key_to_float(UInt value) {
      static_assert(sizeof(T) == sizeof(UInt), "Expected unsigned int of the same size");
      // encoded signbit indicates positive value
      const UInt signbit = (static_cast<UInt>(1) << (std::numeric_limits<UInt>::digits - 1));
      UInt mask    = 0;
      if ((value & signbit) == 0)
         mask = ~mask;
      value = {value ^ (mask | signbit)};
      T float_result;
      std::memcpy(&float_result, &value, sizeof(UInt));
      return float_result;
   }

   template<typename Key, std::size_t N>
   bool extract_key(bytes::const_iterator& key_loc, bytes::const_iterator key_end, Key& key) {
      const auto distance = std::distance(key_loc, key_end);
      using t_type = typename value_storage<Key>::type;
      constexpr static auto key_size = sizeof(t_type) * N;
      if (distance < key_size)
         return false;

      key_end = key_loc + key_size;
      t_type t_array[N];
      char* t_array_as_char_begin = reinterpret_cast<char*>(t_array);
      std::copy(key_loc, key_loc + key_size, t_array_as_char_begin);
      key_loc = key_end;
      char* t_array_as_char_end = reinterpret_cast<char*>(t_array + N);
      std::reverse(t_array_as_char_begin, t_array_as_char_end);

      t_type* key_ptr = value_storage<Key>().as_ptr(key);
      std::reverse_copy(std::begin(t_array), std::end(t_array), key_ptr);

      return true;
   }
}

template <typename T>
auto append_key(bytes& dest, T value) -> std::enable_if_t<std::is_unsigned_v<T>, void> {
   detail::append_key<T, 1>(dest, value);
}

template <typename T, std::size_t N>
auto append_key(bytes& dest, std::array<T, N> value) -> std::enable_if_t<std::is_unsigned_v<T>, void> {
   detail::append_key<T*, N>(dest, value.data());
}

inline void append_key(bytes& dest, float64_t value) {
   auto float_key = detail::float_to_key<uint64_t>(value);
   detail::append_key<uint64_t, 1>(dest, float_key);
}

inline void append_key(bytes& dest, float128_t value) {
   auto float_key = detail::float_to_key<detail::uint128_t>(value);
   // underlying storage is implemented as uint64_t[2], but it is laid out like it is uint128_t
   detail::append_key<detail::uint128_t, 1>(dest, float_key);
}

template<typename T>
auto extract_key(bytes::const_iterator& key_loc, bytes::const_iterator key_end, T& key) -> std::enable_if_t<std::is_unsigned_v<T>, bool> {
   return detail::extract_key<T, 1>(key_loc, key_end, key);
}

template <typename T, std::size_t N>
auto extract_key(bytes::const_iterator& key_loc, bytes::const_iterator key_end, std::array<T, N>& key) -> std::enable_if_t<std::is_unsigned_v<T>, bool> {
   T* key_ptr = key.data();
   return detail::extract_key<T*, N>(key_loc, key_end, key_ptr);
}

inline bool extract_key(bytes::const_iterator& key_loc, bytes::const_iterator key_end, float64_t& key) {
   uint64_t int_key;
   const bool extract = detail::extract_key<uint64_t, 1>(key_loc, key_end, int_key);
   if (!extract)
      return false;

   key = detail::key_to_float<float64_t>(int_key);
   return true;
}

inline bool extract_key(bytes::const_iterator& key_loc, bytes::const_iterator key_end, float128_t& key) {
   detail::uint128_t int_key;
   const bool extract = detail::extract_key<detail::uint128_t, 1>(key_loc, key_end, int_key);
   if (!extract)
      return false;

   key = detail::key_to_float<float128_t>(int_key);
   return true;
}

template <typename T>
bytes create_full_key(const bytes& prefix, uint64_t contract, const T& key) {
   bytes result;
   result.reserve(prefix.size() + sizeof(contract) + key.size());
   result.insert(result.end(), prefix.begin(), prefix.end());
   append_key(result, contract);
   result.insert(result.end(), key.data(), key.data() + key.size());
   return result;
}

struct database {
   std::unique_ptr<rocksdb::DB> rdb;

   database(const char* db_path, bool create_if_missing, std::optional<uint32_t> threads = {},
            std::optional<int> max_open_files = {}) {

      rocksdb::Options options;
      options.create_if_missing                    = create_if_missing;
      options.level_compaction_dynamic_level_bytes = true;
      options.bytes_per_sync                       = 1048576;

      if (threads)
         options.IncreaseParallelism(*threads);

      options.OptimizeLevelStyleCompaction(256ull << 20);

      if (max_open_files)
         options.max_open_files = *max_open_files;

      rocksdb::BlockBasedTableOptions table_options;
      table_options.format_version               = 4;
      table_options.index_block_restart_interval = 16;
      options.table_factory.reset(NewBlockBasedTableFactory(table_options));

      rocksdb::DB* p;
      check(rocksdb::DB::Open(options, db_path, &p), "database::database: rocksdb::DB::Open: ");
      rdb.reset(p);

      // Sentinels with keys 0x00 and 0xff simplify iteration logic.
      // Views have prefixes which must start with a byte within the range 0x01 - 0xfe.
      rocksdb::WriteBatch batch;
      bool                modified       = false;
      auto                write_sentinal = [&](const bytes& k) {
         rocksdb::PinnableSlice v;
         auto                   stat = rdb->Get(rocksdb::ReadOptions(), rdb->DefaultColumnFamily(), to_slice(k), &v);
         if (stat.IsNotFound()) {
            check(batch.Put(to_slice(k), {}), "database::database: rocksdb::WriteBatch::Put: ");
            modified = true;
         } else {
            check(stat, "database::database: rocksdb::DB::Get: ");
         }
      };
      write_sentinal({ 0x00 });
      write_sentinal({ (char)0xff });
      if (modified)
         write(batch);
   }

   database(database&&) = default;
   database& operator=(database&&) = default;

   void flush(bool allow_write_stall, bool wait) {
      rocksdb::FlushOptions op;
      op.allow_write_stall = allow_write_stall;
      op.wait              = wait;
      rdb->Flush(op);
   }

   void write(rocksdb::WriteBatch& batch) {
      rocksdb::WriteOptions opt;
      opt.disableWAL = true;
      check(rdb->Write(opt, &batch), "database::write: rocksdb::DB::Write (batch)");
      batch.Clear();
   }
}; // database

struct key_value {
   rocksdb::Slice key   = {};
   rocksdb::Slice value = {};
};

inline int compare_key(const std::optional<key_value>& a, const std::optional<key_value>& b) {
   // nullopt represents end; everything else is before end
   if (!a && !b)
      return 0;
   else if (!a && b)
      return 1;
   else if (a && !b)
      return -1;
   else
      return compare_blob(a->key, b->key);
}

inline int compare_value(const std::shared_ptr<const bytes>& a, const std::shared_ptr<const bytes>& b) {
   // nullptr represents erased; everything else, including empty, is after erased
   if (!a && !b)
      return 0;
   else if (!a && b)
      return -1;
   else if (a && !b)
      return 1;
   else
      return compare_blob(*a, *b);
}

using cache_map = std::map<bytes, struct cached_value, less_blob>;

// The cache serves these needs:
//    * Keep track of changes that need to be written to rocksdb
//    * Support reading writes
//    * Support iteration logic
struct cached_value {
   uint64_t                     num_erases       = 0; // For iterator invalidation
   std::shared_ptr<const bytes> orig_value       = {};
   std::shared_ptr<const bytes> current_value    = {};
   bool                         in_change_list   = false;
   cache_map::iterator          change_list_next = {};
};

struct undo_state {
   uint8_t               format_version    = 0;
   int64_t               revision          = 0;
   std::vector<uint64_t> undo_stack        = {}; // Number of undo segments needed to go back each revision
   uint64_t              next_undo_segment = 0;
};

template <typename Stream>
void pack_undo_segment(Stream& s, const bytes& key, const bytes* old_value, const bytes* new_value) {
   pack_bytes(s, key);
   pack_optional_bytes(s, old_value);
   pack_optional_bytes(s, new_value);
}

class undo_stack {
 private:
   database&  db;
   bytes      undo_prefix;
   uint64_t   target_segment_size;
   bytes      state_prefix;
   bytes      segment_prefix;
   bytes      segment_next_prefix;
   undo_state state;

 public:
   undo_stack(database& db, const bytes& undo_prefix, uint64_t target_segment_size = 64 * 1024 * 1024)
       : db{ db }, undo_prefix{ undo_prefix }, target_segment_size{ target_segment_size } {
      if (this->undo_prefix.empty())
         throw exception("undo_prefix is empty");

      // Sentinels reserve 0x00 and 0xff. This keeps rocksdb iterators from going
      // invalid during iteration.
      if (this->undo_prefix[0] == 0x00 || this->undo_prefix[0] == (char)0xff)
         throw exception("undo_stack may not have a prefix which begins with 0x00 or 0xff");

      state_prefix = this->undo_prefix;
      state_prefix.push_back(0x00);
      segment_prefix = this->undo_prefix;
      segment_prefix.push_back(0x80);
      segment_next_prefix = get_next_prefix(segment_prefix);

      rocksdb::PinnableSlice v;
      auto stat = db.rdb->Get(rocksdb::ReadOptions(), db.rdb->DefaultColumnFamily(), to_slice(this->state_prefix), &v);
      if (!stat.IsNotFound())
         check(stat, "undo_stack::undo_stack: rocksdb::DB::Get: ");
      if (stat.ok()) {
         auto format_version = fc::raw::unpack<uint8_t>(v.data(), v.size());
         if (format_version)
            throw exception("invalid undo format");
         state = fc::raw::unpack<undo_state>(v.data(), v.size());
      }
   }

   int64_t revision() const { return state.revision; }
   int64_t first_revision() const { return state.revision - state.undo_stack.size(); }

   void set_revision(uint64_t revision, bool write_now = true) {
      if (state.undo_stack.size() != 0)
         throw exception("cannot set revision while there is an existing undo stack");
      if (revision > std::numeric_limits<int64_t>::max())
         throw exception("revision to set is too high");
      if (revision < state.revision)
         throw exception("revision cannot decrease");
      state.revision = revision;
      if (write_now)
         write_state();
   }

   // Create a new entry on the undo stack
   void push(bool write_now = true) {
      state.undo_stack.push_back(0);
      ++state.revision;
      if (write_now)
         write_state();
   }

   // Combine the top two states on the undo stack
   void squash(bool write_now = true) {
      if (state.undo_stack.empty()) {
         return;
      } else if (state.undo_stack.size() == 1) {
         rocksdb::WriteBatch batch;
         check(batch.DeleteRange(to_slice(create_segment_key(0)),
                                 to_slice(create_segment_key(state.next_undo_segment))),
               "undo_stack::squash: rocksdb::WriteBatch::DeleteRange: ");
         state.undo_stack.clear();
         --state.revision;
         write_state(batch);
         db.write(batch);
         return;
      }
      auto n = state.undo_stack.back();
      state.undo_stack.pop_back();
      state.undo_stack.back() += n;
      --state.revision;
      if (write_now)
         write_state();
   }

   // Reset the contents to the state at the top of the undo stack
   void undo(bool write_now = true) {
      if (state.undo_stack.empty())
         throw exception("nothing to undo");
      rocksdb::WriteBatch batch;

      std::unique_ptr<rocksdb::Iterator> rocks_it{ db.rdb->NewIterator(rocksdb::ReadOptions()) };
      auto                               first = create_segment_key(state.next_undo_segment - state.undo_stack.back());
      rocks_it->Seek(to_slice(segment_next_prefix));
      if (rocks_it->Valid())
         rocks_it->Prev();

      while (rocks_it->Valid()) {
         auto segment_key = rocks_it->key();
         if (compare_blob(segment_key, first) < 0)
            break;
         write_now                           = true;
         auto                        segment = rocks_it->value();
         fc::datastream<const char*> ds(segment.data(), segment.size());
         while (ds.remaining()) {
            auto [key, key_size]             = get_bytes(ds);
            auto [old_value, old_value_size] = get_optional_bytes(ds);
            get_optional_bytes(ds);
            if (old_value)
               check(batch.Put({ key, key_size }, { old_value, old_value_size }),
                     "undo_stack::undo: rocksdb::WriteBatch::Put: ");
            else
               check(batch.Delete({ key, key_size }), "undo_stack::undo: rocksdb::WriteBatch::Delete: ");
         }
         check(batch.Delete(segment_key), "undo_stack::undo: rocksdb::WriteBatch::Delete: ");
         rocks_it->Prev();
      }
      check(rocks_it->status(), "undo_stack::undo: iterate rocksdb: ");

      state.next_undo_segment -= state.undo_stack.back();
      state.undo_stack.pop_back();
      --state.revision;
      if (write_now) {
         write_state(batch);
         db.write(batch);
      }
   }

   // Discard all undo history prior to revision
   void commit(int64_t revision) {
      revision            = std::min(revision, state.revision);
      auto first_revision = state.revision - state.undo_stack.size();
      if (first_revision < revision) {
         rocksdb::WriteBatch batch;
         state.undo_stack.erase(state.undo_stack.begin(), state.undo_stack.begin() + (revision - first_revision));
         uint64_t keep_undo_segment = state.next_undo_segment;
         for (auto n : state.undo_stack) //
            keep_undo_segment -= n;
         check(batch.DeleteRange(to_slice(create_segment_key(0)), to_slice(create_segment_key(keep_undo_segment))),
               "undo_stack::commit: rocksdb::WriteBatch::DeleteRange: ");
         write_state(batch);
         db.write(batch);
      }
   }

   // Write changes in `change_list`. Everything in `change_list` must belong to `cache`.
   //
   // It is undefined behavior if any `orig_value` in `change_list` doesn't match the
   // database's current state.
   void write_changes(cache_map& cache, cache_map::iterator change_list) {
      rocksdb::WriteBatch batch;
      bytes               segment;
      segment.reserve(target_segment_size);

      auto write_segment = [&] {
         if (segment.empty())
            return;
         auto key = create_segment_key(state.next_undo_segment++);
         check(batch.Put(to_slice(key), to_slice(segment)), "undo_stack::write_changes: rocksdb::WriteBatch::Put: ");
         ++state.undo_stack.back();
         segment.clear();
      };

      auto append_segment = [&](auto f) {
         fc::datastream<size_t> size_stream;
         f(size_stream);
         if (segment.size() + size_stream.tellp() > target_segment_size)
            write_segment();
         auto orig_size = segment.size();
         segment.resize(segment.size() + size_stream.tellp());
         fc::datastream<char*> ds(segment.data() + orig_size, size_stream.tellp());
         f(ds);
      };

      auto it = change_list;
      while (it != cache.end()) {
         if (compare_value(it->second.orig_value, it->second.current_value)) {
            if (it->second.current_value)
               check(batch.Put(to_slice(it->first), to_slice(*it->second.current_value)),
                     "undo_stack::write_changes: rocksdb::WriteBatch::Put: ");
            else
               check(batch.Delete(to_slice(it->first)), "undo_stack::write_changes: rocksdb::WriteBatch::Erase: ");
            if (!state.undo_stack.empty()) {
               append_segment([&](auto& stream) {
                  pack_undo_segment(stream, it->first, it->second.orig_value.get(), it->second.current_value.get());
               });
            }
         }
         it = it->second.change_list_next;
      }

      write_segment();
      write_state(batch);
      db.write(batch);
   } // write_changes()

   void write_state() {
      rocksdb::WriteBatch batch;
      write_state(batch);
      db.write(batch);
   }

 private:
   void write_state(rocksdb::WriteBatch& batch) {
      check(batch.Put(to_slice(state_prefix), to_slice(fc::raw::pack(state))),
            "undo_stack::write_state: rocksdb::WriteBatch::Put: ");
   }

   bytes create_segment_key(uint64_t segment) {
      bytes key;
      key.reserve(segment_prefix.size() + sizeof(segment));
      key.insert(key.end(), segment_prefix.begin(), segment_prefix.end());
      append_key(key, segment);
      return key;
   }
}; // undo_stack

// Supports reading and writing through a cache_map
//
// Caution: write_session will misbehave if it's used to read or write a key that
// that is changed elsewhere during write_session's lifetime. e.g. through another
// simultaneous write_session, unless that write_session is never written to the
// database.
//
// Extra keys stored in the cache used only as sentinels are exempt from this
// restriction.
struct write_session {
   database&                db;
   const rocksdb::Snapshot* snapshot;
   cache_map                cache;
   cache_map::iterator      change_list = cache.end();

   write_session(database& db, const rocksdb::Snapshot* snapshot = nullptr) : db{ db }, snapshot{ snapshot } {}

   rocksdb::ReadOptions read_options() {
      rocksdb::ReadOptions r;
      r.snapshot = snapshot;
      return r;
   }

   // Add item to change_list
   void changed(cache_map::iterator it) {
      if (it->second.in_change_list)
         return;
      it->second.in_change_list   = true;
      it->second.change_list_next = change_list;
      change_list                 = it;
   }

   // Get a value. Includes any changes written to cache. Returns nullptr
   // if key-value doesn't exist.
   std::shared_ptr<const bytes> get(bytes&& k) {
      auto it = cache.find(k);
      if (it != cache.end())
         return it->second.current_value;

      rocksdb::PinnableSlice v;
      auto                   stat = db.rdb->Get(read_options(), db.rdb->DefaultColumnFamily(), to_slice(k), &v);
      if (stat.IsNotFound())
         return nullptr;
      check(stat, "write_session::get: rocksdb::DB::Get: ");

      auto value          = to_shared_bytes(v);
      cache[std::move(k)] = cached_value{ 0, value, value };
      return value;
   }

   // Write a key-value to cache and add to change_list if changed.
   void set(bytes&& k, const rocksdb::Slice& v) {
      auto it = cache.find(k);
      if (it != cache.end()) {
         if (!it->second.current_value || compare_blob(*it->second.current_value, v)) {
            it->second.current_value = to_shared_bytes(v);
            changed(it);
         }
         return;
      }

      rocksdb::PinnableSlice orig_v;
      auto                   stat = db.rdb->Get(read_options(), db.rdb->DefaultColumnFamily(), to_slice(k), &orig_v);
      if (stat.IsNotFound()) {
         auto [it, b] =
               cache.insert(cache_map::value_type{ std::move(k), cached_value{ 0, nullptr, to_shared_bytes(v) } });
         changed(it);
         return;
      }

      check(stat, "write_session::set: rocksdb::DB::Get: ");
      if (compare_blob(v, orig_v)) {
         auto [it, b] = cache.insert(
               cache_map::value_type{ std::move(k), cached_value{ 0, to_shared_bytes(orig_v), to_shared_bytes(v) } });
         changed(it);
      } else {
         auto value          = to_shared_bytes(orig_v);
         cache[std::move(k)] = cached_value{ 0, value, value };
      }
   }

   // Mark key as erased in the cache and add to change_list if changed. Bumps `num_erases` to invalidate iterators.
   void erase(bytes&& k) {
      {
         auto it = cache.find(k);
         if (it != cache.end()) {
            if (it->second.current_value) {
               ++it->second.num_erases;
               it->second.current_value = nullptr;
               changed(it);
            }
            return;
         }
      }

      rocksdb::PinnableSlice orig_v;
      auto                   stat = db.rdb->Get(read_options(), db.rdb->DefaultColumnFamily(), to_slice(k), &orig_v);
      if (stat.IsNotFound()) {
         cache[std::move(k)] = cached_value{ 0, nullptr, nullptr };
         return;
      }

      check(stat, "write_session::erase: rocksdb::DB::Get: ");
      auto [it, b] =
            cache.insert(cache_map::value_type{ std::move(k), cached_value{ 1, to_shared_bytes(orig_v), nullptr } });
      changed(it);
   }

   // Fill cache with a key-value pair read from the database. Does not undo any changes (e.g. set() or erase())
   // already made to the cache. Returns an iterator to the freshly-created or already-existing cache entry.
   cache_map::iterator fill_cache(const rocksdb::Slice& k, const rocksdb::Slice& v) {
      cache_map::iterator it;
      bool                b;
      it = cache.find(k);
      if (it != cache.end())
         return it;
      auto value      = to_shared_bytes(v);
      std::tie(it, b) = cache.insert(cache_map::value_type{ to_bytes(k), cached_value{ 0, value, value } });
      return it;
   }

   // Write changes in `change_list` to database. See undo_stack::write_changes.
   //
   // Caution: write_changes wipes the cache, which invalidates iterators
   void write_changes(undo_stack& u) {
      u.write_changes(cache, change_list);
      wipe_cache();
   }

   // Wipe the cache. Invalidates iterators.
   void wipe_cache() {
      cache.clear();
      change_list = cache.end();
   }
}; // write_session

// A view of the database with a restricted range (prefix). Implements part of
// https://github.com/EOSIO/spec-repo/blob/master/esr_key_value_database_intrinsics.md,class
// including iterator wrap-around behavior.
//
// Keys have this format: prefix, contract, user-provided key.
class view {
 public:
   class iterator;

   chain_kv::write_session& write_session;
   const bytes              prefix;

 private:
   struct iterator_impl {
      friend chain_kv::view;
      friend iterator;

      chain_kv::view&                    view;
      bytes                              prefix;
      size_t                             hidden_prefix_size;
      bytes                              next_prefix;
      cache_map::iterator                cache_it;
      uint64_t                           cache_it_num_erases = 0;
      std::unique_ptr<rocksdb::Iterator> rocks_it;

      iterator_impl(chain_kv::view& view, uint64_t contract, const rocksdb::Slice& prefix)
          : view{ view },                                                                         //
            prefix{ create_full_key(view.prefix, contract, prefix) },                             //
            hidden_prefix_size{ view.prefix.size() + sizeof(contract) },                          //
            rocks_it{ view.write_session.db.rdb->NewIterator(view.write_session.read_options()) } //
      {
         next_prefix = get_next_prefix(this->prefix);

         // Fill the cache with sentinel keys to simplify iteration logic. These may be either
         // the reserved 0x00 or 0xff sentinels, or keys from regions neighboring prefix.
         rocks_it->Seek(to_slice(this->prefix));
         check(rocks_it->status(), "view::iterator_impl::iterator_impl: rocksdb::Iterator::Seek: ");
         view.write_session.fill_cache(rocks_it->key(), rocks_it->value());
         rocks_it->Prev();
         check(rocks_it->status(), "view::iterator_impl::iterator_impl: rocksdb::Iterator::Prev: ");
         view.write_session.fill_cache(rocks_it->key(), rocks_it->value());
         rocks_it->Seek(to_slice(next_prefix));
         check(rocks_it->status(), "view::iterator_impl::iterator_impl: rocksdb::Iterator::Seek: ");
         view.write_session.fill_cache(rocks_it->key(), rocks_it->value());

         move_to_end();
      }

      iterator_impl(const iterator_impl&) = delete;
      iterator_impl& operator=(const iterator_impl&) = delete;

      void move_to_begin() { lower_bound_full_key(prefix); }

      void move_to_end() { cache_it = view.write_session.cache.end(); }

      void lower_bound(const char* key, size_t size) {
         auto x = compare_blob(rocksdb::Slice{ key, size }, rocksdb::Slice{ prefix.data() + hidden_prefix_size,
                                                                            prefix.size() - hidden_prefix_size });
         if (x < 0) {
            key  = prefix.data() + hidden_prefix_size;
            size = prefix.size() - hidden_prefix_size;
         }

         bytes full_key;
         full_key.reserve(hidden_prefix_size + size);
         full_key.insert(full_key.end(), prefix.data(), prefix.data() + hidden_prefix_size);
         full_key.insert(full_key.end(), key, key + size);
         lower_bound_full_key(full_key);
      }

      void lower_bound_full_key(const bytes& full_key) {
         rocks_it->Seek(to_slice(full_key));
         check(rocks_it->status(), "view::iterator_impl::lower_bound_full_key: rocksdb::Iterator::Seek: ");
         cache_it = view.write_session.fill_cache(rocks_it->key(), rocks_it->value());
         if (compare_blob(cache_it->first, to_slice(full_key)))
            cache_it = view.write_session.cache.lower_bound(full_key);
         while (!cache_it->second.current_value) {
            while (compare_blob(rocks_it->key(), cache_it->first) <= 0) {
               rocks_it->Next();
               check(rocks_it->status(), "view::iterator_impl::lower_bound_full_key: rocksdb::Iterator::Next: ");
               view.write_session.fill_cache(rocks_it->key(), rocks_it->value());
            }
            ++cache_it;
         }
         if (compare_blob(cache_it->first, next_prefix) >= 0)
            cache_it = view.write_session.cache.end();
         else
            cache_it_num_erases = cache_it->second.num_erases;
      }

      std::optional<key_value> get_kv() {
         if (cache_it == view.write_session.cache.end())
            return {};
         if (cache_it_num_erases != cache_it->second.num_erases)
            throw exception("kv iterator is at an erased value");
         return key_value{ rocksdb::Slice{ cache_it->first.data() + hidden_prefix_size,
                                           cache_it->first.size() - hidden_prefix_size },
                           to_slice(*cache_it->second.current_value) };
      }

      bool is_end() { return cache_it == view.write_session.cache.end(); }

      bool is_valid() {
         return cache_it != view.write_session.cache.end() && cache_it_num_erases == cache_it->second.num_erases;
      }

      bool is_erased() {
         return cache_it != view.write_session.cache.end() && cache_it_num_erases != cache_it->second.num_erases;
      }

      iterator_impl& operator++() {
         if (cache_it == view.write_session.cache.end()) {
            move_to_begin();
            return *this;
         } else if (cache_it_num_erases != cache_it->second.num_erases)
            throw exception("kv iterator is at an erased value");
         do {
            while (compare_blob(rocks_it->key(), cache_it->first) <= 0) {
               rocks_it->Next();
               check(rocks_it->status(), "view::iterator_impl::operator++: rocksdb::Iterator::Next: ");
               view.write_session.fill_cache(rocks_it->key(), rocks_it->value());
            }
            ++cache_it;
         } while (!cache_it->second.current_value);
         if (compare_blob(cache_it->first, next_prefix) >= 0)
            cache_it = view.write_session.cache.end();
         else
            cache_it_num_erases = cache_it->second.num_erases;
         return *this;
      }

      iterator_impl& operator--() {
         if (cache_it == view.write_session.cache.end()) {
            rocks_it->Seek(to_slice(next_prefix));
            check(rocks_it->status(), "view::iterator_impl::operator--: rocksdb::Iterator::Seek: ");
            cache_it = view.write_session.fill_cache(rocks_it->key(), rocks_it->value());
            if (compare_blob(cache_it->first, to_slice(next_prefix)))
               cache_it = view.write_session.cache.lower_bound(next_prefix);
         } else if (cache_it_num_erases != cache_it->second.num_erases)
            throw exception("kv iterator is at an erased value");
         do {
            while (compare_blob(rocks_it->key(), cache_it->first) >= 0) {
               rocks_it->Prev();
               check(rocks_it->status(), "view::iterator_impl::operator--: rocksdb::Iterator::Prev: ");
               view.write_session.fill_cache(rocks_it->key(), rocks_it->value());
            }
            --cache_it;
         } while (!cache_it->second.current_value);
         if (compare_blob(cache_it->first, prefix) < 0)
            cache_it = view.write_session.cache.end();
         else
            cache_it_num_erases = cache_it->second.num_erases;
         return *this;
      }
   }; // iterator_impl

 public:
   // Iterates through a user-provided prefix within view's keyspace.
   class iterator {
      friend view;

    private:
      std::unique_ptr<iterator_impl> impl;

      void check_initialized() const {
         if (!impl)
            throw exception("kv iterator is not initialized");
      }

    public:
      iterator(view& view, uint64_t contract, const rocksdb::Slice& prefix)
          : impl{ std::make_unique<iterator_impl>(view, contract, std::move(prefix)) } {}

      iterator(const iterator&) = delete;
      iterator(iterator&&)      = default;

      iterator& operator=(const iterator&) = delete;
      iterator& operator=(iterator&&) = default;

      // Compare 2 iterators. Throws if the iterators are from different views.
      // Also throws if either iterator is at an erased kv pair. non-end iterators
      // compare less than end iterators.
      friend int compare(const iterator& a, const iterator& b) {
         a.check_initialized();
         b.check_initialized();
         if (&a.impl->view != &b.impl->view)
            throw exception("iterators are from different views");
         return compare_key(a.impl->get_kv(), b.impl->get_kv());
      }

      friend bool operator==(const iterator& a, const iterator& b) { return compare(a, b) == 0; }
      friend bool operator!=(const iterator& a, const iterator& b) { return compare(a, b) != 0; }
      friend bool operator<(const iterator& a, const iterator& b) { return compare(a, b) < 0; }
      friend bool operator<=(const iterator& a, const iterator& b) { return compare(a, b) <= 0; }
      friend bool operator>(const iterator& a, const iterator& b) { return compare(a, b) > 0; }
      friend bool operator>=(const iterator& a, const iterator& b) { return compare(a, b) >= 0; }

      iterator& operator++() {
         check_initialized();
         ++*impl;
         return *this;
      }

      iterator& operator--() {
         check_initialized();
         --*impl;
         return *this;
      }

      void move_to_begin() {
         check_initialized();
         impl->move_to_begin();
      }

      void move_to_end() {
         check_initialized();
         impl->move_to_end();
      }

      void lower_bound(const char* key, size_t size) {
         check_initialized();
         impl->lower_bound(key, size);
      }

      void lower_bound(const bytes& key) { lower_bound(key.data(), key.size()); }

      bool is_end() const {
         check_initialized();
         return impl->is_end();
      }

      // true if !is_end() and not at an erased kv pair
      bool is_valid() const {
         check_initialized();
         return impl->is_valid();
      }

      // true if !is_end() and is at an erased kv pair
      bool is_erased() const {
         check_initialized();
         return impl->is_erased();
      }

      // Get key_value at current position. Returns nullopt if at end. Throws if at an erased kv pair.
      // The returned key does not include the view's prefix or the contract.
      std::optional<key_value> get_kv() const {
         check_initialized();
         return impl->get_kv();
      }
   };

   view(struct write_session& write_session, bytes prefix)
       : write_session{ write_session }, prefix{ std::move(prefix) } {
      if (this->prefix.empty())
         throw exception("kv view may not have empty prefix");

      // Sentinels reserve 0x00 and 0xff. This keeps rocksdb iterators from going
      // invalid during iteration. This also allows get_next_prefix() to function correctly.
      if (this->prefix[0] == 0x00 || this->prefix[0] == (char)0xff)
         throw exception("view may not have a prefix which begins with 0x00 or 0xff");
   }

   // Get a value. Includes any changes written to cache. Returns nullptr
   // if key doesn't exist.
   std::shared_ptr<const bytes> get(uint64_t contract, const rocksdb::Slice& k) {
      return write_session.get(create_full_key(prefix, contract, k));
   }

   // Set a key-value pair
   void set(uint64_t contract, const rocksdb::Slice& k, const rocksdb::Slice& v) {
      write_session.set(create_full_key(prefix, contract, k), v);
   }

   // Erase a key-value pair
   void erase(uint64_t contract, const rocksdb::Slice& k) { write_session.erase(create_full_key(prefix, contract, k)); }
}; // view

} // namespace eosio::chain_kv

FC_REFLECT(eosio::chain_kv::undo_state, (format_version)(revision)(undo_stack)(next_undo_segment))
