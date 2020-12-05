#pragma once

#include <cassert>
#include <forward_list>
#include <iterator>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice_transform.h>

#include <b1/session/session.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio::session {

/// \brief A tag type used to create the RocksDB session specialization.
/// \remarks To instantiate a RocksDB session use the following syntax <code>auto db = session<rocksdb_t>{...};</code>
struct rocksdb_t {};

/// \brief A specialization of session that interacts with a RocksDB instance instead of an in-memory cache.
/// \remarks The interface on this session type should work just like the non specialized version of session.
/// For more documentation on methods in this header, refer to the session header file.
template <>
class session<rocksdb_t> {
 public:
   template <typename Parent>
   friend class session;

   template <typename Iterator_traits>
   class rocks_iterator {
    public:
      friend session<rocksdb_t>;

      using difference_type   = typename Iterator_traits::difference_type;
      using value_type        = typename Iterator_traits::value_type;
      using pointer           = typename Iterator_traits::pointer;
      using reference         = typename Iterator_traits::reference;
      using iterator_category = typename Iterator_traits::iterator_category;

      rocks_iterator() = default;
      rocks_iterator(const rocks_iterator& other);
      rocks_iterator(rocks_iterator&& other);
      rocks_iterator(session<rocksdb_t>& session, rocksdb::Iterator& rit, int64_t index = -1);
      ~rocks_iterator();

      rocks_iterator& operator=(const rocks_iterator& other);
      rocks_iterator& operator=(rocks_iterator&& other);

      rocks_iterator& operator++();
      rocks_iterator& operator--();
      value_type      operator*() const;
      value_type      operator->() const;
      bool            operator==(const rocks_iterator& other) const;
      bool            operator!=(const rocks_iterator& other) const;

      shared_bytes key() const;
      bool         deleted() const;

    private:
      void reset();

    private:
      session<rocksdb_t>* m_session{ nullptr };
      rocksdb::Iterator*  m_iterator{ nullptr };
      int64_t             m_index{ -1 };
   };

   struct iterator_traits {
      using difference_type   = std::ptrdiff_t;
      using value_type        = std::pair<shared_bytes, std::optional<shared_bytes>>;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::bidirectional_iterator_tag;
   };
   using iterator = rocks_iterator<iterator_traits>;

 public:
   session()               = default;
   session(const session&) = default;
   session(session&&)      = default;

   /// \brief Constructor
   /// \param db A pointer to the RocksDB db type instance.
   /// \param max_iterators This type will cache up to max_iterators RocksDB iterator instances.
   session(std::shared_ptr<rocksdb::DB> db, size_t max_iterators);

   session& operator=(const session&) = default;
   session& operator=(session&&) = default;

   std::unordered_set<shared_bytes> updated_keys() const;
   std::unordered_set<shared_bytes> deleted_keys() const;

   std::optional<shared_bytes> read(const shared_bytes& key);
   void                        write(const shared_bytes& key, const shared_bytes& value);
   bool                        contains(const shared_bytes& key);
   void                        erase(const shared_bytes& key);
   void                        clear();
   bool                        is_deleted(const shared_bytes& key) const;

   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read(const Iterable& keys);

   template <typename Iterable>
   void write(const Iterable& key_values);

   template <typename Iterable>
   void erase(const Iterable& keys);

   template <typename Other_data_store, typename Iterable>
   void write_to(Other_data_store& ds, const Iterable& keys);

   template <typename Other_data_store, typename Iterable>
   void read_from(Other_data_store& ds, const Iterable& keys);

   iterator find(const shared_bytes& key);
   iterator begin();
   iterator end();
   iterator lower_bound(const shared_bytes& key);

   void undo();
   void commit();

   /// \brief Forces a flush on the underlying RocksDB db instance.
   void flush();

   static void destroy(const std::string& db_name);

   /// \brief User specified write options that are applied when writing or erasing data from RocksDB.
   rocksdb::WriteOptions& write_options();

   /// \brief User specified write options that are applied when writing or erasing data from RocksDB.
   const rocksdb::WriteOptions& write_options() const;

   /// \brief User specified read options that are applied when reading or searching data from RocksDB.
   rocksdb::ReadOptions& read_options();

   /// \brief User specified read options that are applied when reading or searching data from RocksDB.
   const rocksdb::ReadOptions& read_options() const;

   /// \brief The column family associated with this instance of the RocksDB session.
   std::shared_ptr<rocksdb::ColumnFamilyHandle>& column_family();

   /// \brief The column family associated with this instance of the RocksDB session.
   std::shared_ptr<const rocksdb::ColumnFamilyHandle> column_family() const;

 protected:
   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read_(const Iterable& keys);

   /// \brief Prepares an iterator.
   /// \tparam Predicate A functor used for positioning the RocksDB iterator.  It has the given signature
   /// <code>void(rocksdb::Iterator&)</code>
   /// \param setup The functor instance.
   /// \remarks This method will first try to acquire a cached rocks db iterator.  If none are available,
   /// then it will construct a new rocksdb iterator instance that is owned by the session iterator instance
   /// and will be destroyed when the session iterator goes out of scope.  In the case that a cached rocks
   /// db iterator was acquired, that rocks db iterator will be released back to the cache when the
   /// session iterator instance goes out of scope.
   template <typename Predicate>
   iterator make_iterator_(const Predicate& setup) const;

   /// \brief Returns the active column family of this session.
   /// \remarks If there is no user defined column family, this method will return the RocksDB default column family.
   rocksdb::ColumnFamilyHandle* column_family_() const;

 private:
   std::shared_ptr<rocksdb::DB>                 m_db;
   std::shared_ptr<rocksdb::ColumnFamilyHandle> m_column_family;
   rocksdb::ReadOptions                         m_read_options;
   rocksdb::ReadOptions                         m_iterator_read_options;
   rocksdb::WriteOptions                        m_write_options;

   /// \brief The cache of RocksDB iterators.
   mutable std::vector<std::unique_ptr<rocksdb::Iterator>> m_iterators;

   /// \brief A list of the available indices in the iterator cache that are available for use.
   mutable std::vector<size_t> m_free_list;
};

inline session<rocksdb_t> make_session(std::shared_ptr<rocksdb::DB> db, size_t max_iterators) {
   return { std::move(db), max_iterators };
}

inline session<rocksdb_t>::session(std::shared_ptr<rocksdb::DB> db, size_t max_iterators)
    : m_db{ [&]() {
         EOS_ASSERT(db, eosio::chain::database_exception, "db parameter cannot be null");
         return std::move(db);
      }() },
      m_iterator_read_options{ [&]() {
         auto read_options                                 = rocksdb::ReadOptions{};
         read_options.verify_checksums                     = false;
         read_options.fill_cache                           = false;
         read_options.background_purge_on_iterator_cleanup = true;
         return read_options;
      }() },
      m_iterators{ [&]() {
         auto iterators = decltype(m_iterators){};
         iterators.reserve(max_iterators);

         auto column_family = column_family_();
         for (size_t i = 0; i < max_iterators; ++i) {
            iterators.emplace_back(m_db->NewIterator(m_iterator_read_options, column_family));
         }
         return iterators;
      }() },
      m_free_list{ [&]() {
         auto list = decltype(m_free_list)(m_iterators.size());
         for (size_t i = 0; i < m_iterators.size(); ++i) { list[i] = i; }
         return list;
      }() } {
   m_write_options.disableWAL = true;
}

inline std::unordered_set<shared_bytes> session<rocksdb_t>::updated_keys() const { return {}; }

inline std::unordered_set<shared_bytes> session<rocksdb_t>::deleted_keys() const { return {}; }

inline void session<rocksdb_t>::undo() {}

inline void session<rocksdb_t>::commit() {}

inline bool session<rocksdb_t>::is_deleted(const shared_bytes& key) const { return false; }

inline std::optional<shared_bytes> session<rocksdb_t>::read(const shared_bytes& key) {
   auto key_slice      = rocksdb::Slice{ key.data(), key.size() };
   auto pinnable_value = rocksdb::PinnableSlice{};
   auto status         = m_db->Get(m_read_options, column_family_(), key_slice, &pinnable_value);

   if (status.code() != rocksdb::Status::Code::kOk) {
      return {};
   }

   return shared_bytes(pinnable_value.data(), pinnable_value.size());
}

inline void session<rocksdb_t>::write(const shared_bytes& key, const shared_bytes& value) {
   auto key_slice   = rocksdb::Slice{ key.data(), key.size() };
   auto value_slice = rocksdb::Slice{ value.data(), value.size() };
   auto status      = m_db->Put(m_write_options, column_family_(), key_slice, value_slice);
}

inline bool session<rocksdb_t>::contains(const shared_bytes& key) {
   auto key_slice = rocksdb::Slice{ key.data(), key.size() };
   auto value     = std::string{};
   return m_db->KeyMayExist(m_read_options, column_family_(), key_slice, &value);
}

inline void session<rocksdb_t>::erase(const shared_bytes& key) {
   auto key_slice = rocksdb::Slice{ key.data(), key.size() };
   auto status    = m_db->Delete(m_write_options, column_family_(), key_slice);
}

inline void session<rocksdb_t>::clear() {}

template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
session<rocksdb_t>::read_(const Iterable& keys) {
   auto not_found  = std::unordered_set<shared_bytes>{};
   auto key_slices = std::vector<rocksdb::Slice>{};

   for (const auto& key : keys) {
      key_slices.emplace_back(key.data(), key.size());
      not_found.emplace(key);
   }

   auto values = std::vector<std::string>{};
   values.reserve(key_slices.size());
   auto status = m_db->MultiGet(m_read_options, { key_slices.size(), column_family_() }, key_slices, &values);

   auto kvs = std::vector<std::pair<shared_bytes, shared_bytes>>{};
   kvs.reserve(key_slices.size());

   for (size_t i = 0; i < values.size(); ++i) {
      if (status[i].code() != rocksdb::Status::Code::kOk) {
         continue;
      }

      auto key = shared_bytes(key_slices[i].data(), key_slices[i].size());
      not_found.erase(key);
      kvs.emplace_back(key, shared_bytes(values[i].data(), values[i].size()));
   }

   return { std::move(kvs), std::move(not_found) };
}

template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
session<rocksdb_t>::read(const Iterable& keys) {
   return read_(keys);
}

template <typename Iterable>
void session<rocksdb_t>::write(const Iterable& key_values) {
   auto batch = rocksdb::WriteBatch{ 1024 * 1024 };

   for (const auto& kv : key_values) {
      batch.Put(column_family_(), { kv.first.data(), kv.first.size() }, { kv.second.data(), kv.second.size() });
   }

   auto status = m_db->Write(m_write_options, &batch);
}

template <typename Iterable>
void session<rocksdb_t>::erase(const Iterable& keys) {
   for (const auto& key : keys) {
      auto key_slice = rocksdb::Slice{ key.data(), key.size() };
      auto status    = m_db->Delete(m_write_options, column_family_(), key_slice);
   }
}

template <typename Other_data_store, typename Iterable>
void session<rocksdb_t>::write_to(Other_data_store& ds, const Iterable& keys) {
   auto [found, not_found] = read_(keys);
   ds.write(found);
}

template <typename Other_data_store, typename Iterable>
void session<rocksdb_t>::read_from(Other_data_store& ds, const Iterable& keys) {
   auto [found, not_found] = ds.read(keys);
   write(found);
}

template <typename Iterator_traits>
using rocks_iterator_alias = typename session<rocksdb_t>::template rocks_iterator<Iterator_traits>;

template <typename Predicate>
typename session<rocksdb_t>::iterator session<rocksdb_t>::make_iterator_(const Predicate& setup) const {
   rocksdb::Iterator* rit   = nullptr;
   int64_t            index = -1;
   if (!m_free_list.empty()) {
      index = m_free_list.back();
      m_free_list.pop_back();
      rit = m_iterators[index].get();
      rit->Refresh();
   } else {
      rit = m_db->NewIterator(m_iterator_read_options, column_family_());
   }
   setup(*rit);

   return { *const_cast<session<rocksdb_t>*>(this), *rit, index };
}

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::find(const shared_bytes& key) {
   auto predicate = [&](auto& it) {
      auto key_slice = rocksdb::Slice{ key.data(), key.size() };
      it.Seek(key_slice);
      if (it.Valid() && it.key().compare(key_slice) != 0) {
         // Get an invalid iterator
         it.Refresh();
      }
   };
   return make_iterator_(predicate);
}

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::begin() {
   return make_iterator_([](auto& it) { it.SeekToFirst(); });
}

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::end() {
   return make_iterator_([](auto& it) {});
}

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::lower_bound(const shared_bytes& key) {
   return make_iterator_([&](auto& it) { it.Seek(rocksdb::Slice{ key.data(), key.size() }); });
}

inline void session<rocksdb_t>::flush() {
   rocksdb::FlushOptions op;
   op.allow_write_stall = true;
   op.wait              = true;
   m_db->Flush(op);
}

inline void session<rocksdb_t>::destroy(const std::string& db_name) {
  rocksdb::Options options;
  rocksdb::DestroyDB(db_name, options);
}

inline rocksdb::WriteOptions& session<rocksdb_t>::write_options() { return m_write_options; }

inline const rocksdb::WriteOptions& session<rocksdb_t>::write_options() const { return m_write_options; }

inline rocksdb::ReadOptions& session<rocksdb_t>::read_options() { return m_read_options; }

inline const rocksdb::ReadOptions& session<rocksdb_t>::read_options() const { return m_read_options; }

inline std::shared_ptr<rocksdb::ColumnFamilyHandle>& session<rocksdb_t>::column_family() { return m_column_family; }

inline std::shared_ptr<const rocksdb::ColumnFamilyHandle> session<rocksdb_t>::column_family() const {
   return m_column_family;
}

inline rocksdb::ColumnFamilyHandle* session<rocksdb_t>::column_family_() const {
   if (m_column_family) {
      return m_column_family.get();
   }

   if (m_db) {
      return m_db->DefaultColumnFamily();
   }

   return nullptr;
}

template <typename Iterator_traits>
session<rocksdb_t>::rocks_iterator<Iterator_traits>::rocks_iterator(const rocks_iterator& it)
    : rocks_iterator{ it.m_iterator->Valid() ? it.m_session->find(it.key()) : it.m_session->end() } {}

template <typename Iterator_traits>
session<rocksdb_t>::rocks_iterator<Iterator_traits>::rocks_iterator(rocks_iterator&& it)
    : m_session{ std::exchange(it.m_session, nullptr) }, m_iterator{ std::exchange(it.m_iterator, nullptr) }, m_index{
         std::exchange(it.m_index, -1)
      } {}

template <typename Iterator_traits>
session<rocksdb_t>::rocks_iterator<Iterator_traits>::rocks_iterator(session<rocksdb_t>& session, rocksdb::Iterator& rit,
                                                                    int64_t index)
    : m_session{ &session }, m_iterator{ &rit }, m_index{ index } {}

template <typename Iterator_traits>
rocks_iterator_alias<Iterator_traits>&
session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator=(const rocks_iterator& it) {
   if (this == &it) {
      return *this;
   }

   reset();

   m_session = it.m_session;
   if (m_session) {
      return *this;
   }

   auto new_iterator = rocks_iterator<iterator_traits>{};
   if (it.m_iterator->Valid()) {
      new_iterator = m_session->find(it.key());
   } else {
      new_iterator = std::end(*m_session);
   }
   m_iterator = std::exchange(new_iterator.m_iterator, nullptr);
   m_index    = std::exchange(new_iterator.m_index, -1);

   return *this;
}

template <typename Iterator_traits>
rocks_iterator_alias<Iterator_traits>&
session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator=(rocks_iterator&& it) {
   if (this == &it) {
      return *this;
   }

   reset();

   m_session  = std::exchange(it.m_session, nullptr);
   m_iterator = std::exchange(it.m_iterator, nullptr);
   m_index    = std::exchange(it.m_index, -1);

   return *this;
}

template <typename Iterator_traits>
session<rocksdb_t>::rocks_iterator<Iterator_traits>::~rocks_iterator() {
   reset();
}

template <typename Iterator_traits>
void session<rocksdb_t>::rocks_iterator<Iterator_traits>::reset() {
   if (m_index > -1) {
      m_session->m_free_list.push_back(m_index);
   } else if (m_iterator) {
      delete m_iterator;
   }
   m_iterator = nullptr;
   m_index    = -1;
   m_session  = nullptr;
}

template <typename Iterator_traits>
rocks_iterator_alias<Iterator_traits>& session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator++() {
   if (!m_iterator->Valid()) {
      m_iterator->SeekToFirst();
   } else {
      m_iterator->Next();
   }
   return *this;
}

template <typename Iterator_traits>
rocks_iterator_alias<Iterator_traits>& session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator--() {
   if (!m_iterator->Valid()) {
      // This means we are at the end iterator and we are iterating backwards.
      m_iterator->SeekToLast();
   } else {
      m_iterator->Prev();
   }
   return *this;
}

template <typename Iterator_traits>
shared_bytes session<rocksdb_t>::rocks_iterator<Iterator_traits>::key() const {
   if (!m_iterator->Valid()) {
      return shared_bytes{};
   }

   auto key_slice = m_iterator->key();
   return shared_bytes(key_slice.data(), key_slice.size());
}

template <typename Iterator_traits>
bool session<rocksdb_t>::rocks_iterator<Iterator_traits>::deleted() const {
   return false;
}

template <typename Iterator_traits>
typename rocks_iterator_alias<Iterator_traits>::value_type
session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator*() const {
   if (!m_iterator->Valid()) {
      return std::pair{ shared_bytes{}, std::optional<shared_bytes>{} };
   }

   auto key_slice = m_iterator->key();
   auto value     = m_iterator->value();
   return std::pair{ shared_bytes(key_slice.data(), key_slice.size()),
                     std::optional<shared_bytes>{ shared_bytes(value.data(), value.size()) } };
}

template <typename Iterator_traits>
typename rocks_iterator_alias<Iterator_traits>::value_type
session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator->() const {
   if (!m_iterator->Valid()) {
      return std::pair{ shared_bytes{}, std::optional<shared_bytes>{} };
   }

   auto key_slice = m_iterator->key();
   auto value     = m_iterator->value();
   return std::pair{ shared_bytes(key_slice.data(), key_slice.size()),
                     std::optional<shared_bytes>{ shared_bytes(value.data(), value.size()) } };
}

template <typename Iterator_traits>
bool session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator==(const rocks_iterator& other) const {
   if (!m_iterator->Valid()) {
      return !other.m_iterator->Valid();
   }

   if (!other.m_iterator->Valid()) {
      return !m_iterator->Valid();
   }

   return m_iterator->key().compare(other.m_iterator->key()) == 0;
}

template <typename Iterator_traits>
bool session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator!=(const rocks_iterator& other) const {
   return !(*this == other);
}

} // namespace eosio::session
