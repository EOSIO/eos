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

struct rocksdb_t {};

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

      rocks_iterator()                            = default;
      rocks_iterator(const rocks_iterator& other) = delete;
      rocks_iterator(rocks_iterator&& other);
      rocks_iterator(session<rocksdb_t>& session, rocksdb::Iterator& rit, int64_t index = -1);
      ~rocks_iterator();

      rocks_iterator& operator=(const rocks_iterator& other) = delete;
      rocks_iterator& operator                               =(rocks_iterator&& other);

      rocks_iterator& operator++();
      rocks_iterator& operator--();
      value_type      operator*() const;
      value_type      operator->() const;
      bool            operator==(const rocks_iterator& other) const;
      bool            operator!=(const rocks_iterator& other) const;

      shared_bytes key() const;

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
   session(std::shared_ptr<rocksdb::DB> db, size_t max_iterators);

   session& operator=(const session&) = default;
   session& operator=(session&&) = default;

   std::optional<shared_bytes> read(const shared_bytes& key) const;
   void                        write(const shared_bytes& key, const shared_bytes& value);
   bool                        contains(const shared_bytes& key) const;
   void                        erase(const shared_bytes& key);
   void                        clear();
   bool                        is_deleted(const shared_bytes& key) const;

   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read(const Iterable& keys) const;

   template <typename Iterable>
   void write(const Iterable& key_values);

   template <typename Iterable>
   void erase(const Iterable& keys);

   template <typename Other_data_store, typename Iterable>
   void write_to(Other_data_store& ds, const Iterable& keys) const;

   template <typename Other_data_store, typename Iterable>
   void read_from(const Other_data_store& ds, const Iterable& keys);

   iterator find(const shared_bytes& key) const;
   iterator begin() const;
   iterator end() const;
   iterator lower_bound(const shared_bytes& key) const;

   void undo();
   void commit();
   void flush();

   rocksdb::WriteOptions&       write_options();
   const rocksdb::WriteOptions& write_options() const;

   rocksdb::ReadOptions&       read_options();
   const rocksdb::ReadOptions& read_options() const;

   std::shared_ptr<rocksdb::ColumnFamilyHandle>&      column_family();
   std::shared_ptr<const rocksdb::ColumnFamilyHandle> column_family() const;

 protected:
   template <typename Iterable>
   const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
   read_(const Iterable& keys) const;

   template <typename Predicate>
   iterator make_iterator_(const Predicate& setup) const;

   rocksdb::ColumnFamilyHandle* column_family_() const;

 private:
   std::shared_ptr<rocksdb::DB>                 m_db;
   std::shared_ptr<rocksdb::ColumnFamilyHandle> m_column_family;
   rocksdb::ReadOptions                         m_read_options;
   rocksdb::ReadOptions                         m_iterator_read_options;
   rocksdb::WriteOptions                        m_write_options;

   mutable std::vector<std::unique_ptr<rocksdb::Iterator>> m_iterators;
   mutable std::vector<size_t>                             m_free_list;
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
         auto read_options             = rocksdb::ReadOptions{};
         read_options.readahead_size   = 256 * 1024 * 1024;
         read_options.verify_checksums = false;
         read_options.fill_cache       = false;
         // read_options.tailing = true;
         read_options.background_purge_on_iterator_cleanup = true;
         read_options.pin_data                             = true;
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

inline void session<rocksdb_t>::undo() {}

inline void session<rocksdb_t>::commit() {}

inline bool session<rocksdb_t>::is_deleted(const shared_bytes& key) const { return false; }

inline std::optional<shared_bytes> session<rocksdb_t>::read(const shared_bytes& key) const {
   auto key_slice      = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.size() };
   auto pinnable_value = rocksdb::PinnableSlice{};
   auto status         = m_db->Get(m_read_options, column_family_(), key_slice, &pinnable_value);

   if (status.code() != rocksdb::Status::Code::kOk) {
      return {};
   }

   return shared_bytes(pinnable_value.data(), pinnable_value.size());
}

inline void session<rocksdb_t>::write(const shared_bytes& key, const shared_bytes& value) {
   auto key_slice   = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.size() };
   auto value_slice = rocksdb::Slice{ reinterpret_cast<const char*>(value.data()), value.size() };
   auto status      = m_db->Put(m_write_options, column_family_(), key_slice, value_slice);
}

inline bool session<rocksdb_t>::contains(const shared_bytes& key) const {
   auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.size() };
   auto value     = std::string{};
   return m_db->KeyMayExist(m_read_options, column_family_(), key_slice, &value);
}

inline void session<rocksdb_t>::erase(const shared_bytes& key) {
   auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.size() };
   auto status    = m_db->Delete(m_write_options, column_family_(), key_slice);
}

inline void session<rocksdb_t>::clear() {}

template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
session<rocksdb_t>::read_(const Iterable& keys) const {
   auto not_found  = std::unordered_set<shared_bytes>{};
   auto key_slices = std::vector<rocksdb::Slice>{};

   for (const auto& key : keys) {
      key_slices.emplace_back(reinterpret_cast<const char*>(key.data()), key.size());
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

// Reads a batch of keys from rocksdb.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An Iterable instance that returns shared_bytes instances in its iterator. \returns An std::pair
// where the first item is list of the found key/value pairs and the second item is a set of the keys not found.
template <typename Iterable>
const std::pair<std::vector<std::pair<shared_bytes, shared_bytes>>, std::unordered_set<shared_bytes>>
session<rocksdb_t>::read(const Iterable& keys) const {
   return read_(keys);
}

// Writes a batch of key/value pairs to rocksdb.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns key/value pairs in its
// iterator. \param key_values An Iterable instance that returns key/value pairs in its iterator.
template <typename Iterable>
void session<rocksdb_t>::write(const Iterable& key_values) {
   auto batch = rocksdb::WriteBatch{ 1024 * 1024 };

   for (const auto& kv : key_values) {
      batch.Put(column_family_(), { reinterpret_cast<const char*>(kv.first.data()), kv.first.size() },
                { reinterpret_cast<const char*>(kv.second.data()), kv.second.size() });
   }

   auto status = m_db->Write(m_write_options, &batch);
}

// Erases a batch of key/value pairs from rocksdb.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An Iterable instance that returns shared_bytes instances in its iterator.
template <typename Iterable>
void session<rocksdb_t>::erase(const Iterable& keys) {
   for (const auto& key : keys) {
      auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.size() };
      auto status    = m_db->Delete(m_write_options, column_family_(), key_slice);
   }
}

// Writes a batch of key/value pairs from rocksdb into the given data_store instance.
//
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that
// concept. \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances
// in its iterator. \param ds A data store instance. \param keys An Iterable instance that returns shared_bytes
// instances in its iterator.
template <typename Other_data_store, typename Iterable>
void session<rocksdb_t>::write_to(Other_data_store& ds, const Iterable& keys) const {
   auto [found, not_found] = read_(keys);
   ds.write(found);
}

// Reads a batch of key/value pairs from the given data store into the rocksdb.
//
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that
// concept. \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances
// in its iterator. \param ds A data store instance. \param keys An Iterable instance that returns shared_bytes
// instances in its iterator.
template <typename Other_data_store, typename Iterable>
void session<rocksdb_t>::read_from(const Other_data_store& ds, const Iterable& keys) {
   auto [found, not_found] = ds.read(keys);
   write(found);
}

template <typename Iterator_traits>
using rocks_iterator_alias = typename session<rocksdb_t>::template rocks_iterator<Iterator_traits>;

// Instantiates an iterator for iterating over the rocksdb data store.
//
// \tparam Predicate A function used for preparing the initial iterator.  It has the signature of
// void(std::shared_ptr<rocksdb::Iterator>&)
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

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::find(const shared_bytes& key) const {
   auto predicate = [&](auto& it) {
      auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.size() };
      it.Seek(key_slice);
      if (it.Valid() && it.key().compare(key_slice) != 0) {
         // Get an invalid iterator
         it.SeekToLast();
         if (it.Valid()) {
            it.Next();
         }
      }
   };
   return make_iterator_(predicate);
}

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::begin() const {
   return make_iterator_([](auto& it) { it.SeekToFirst(); });
}

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::end() const {
   return make_iterator_([](auto& it) {
      it.SeekToLast();
      if (it.Valid())
         it.Next();
   });
}

inline typename session<rocksdb_t>::iterator session<rocksdb_t>::lower_bound(const shared_bytes& key) const {
   return make_iterator_([&](auto& it) {
      it.Seek(rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.size() });
   });
}

inline void session<rocksdb_t>::flush() {
   rocksdb::FlushOptions op;
   op.allow_write_stall = true;
   op.wait              = true;
   m_db->Flush(op);
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
session<rocksdb_t>::rocks_iterator<Iterator_traits>::rocks_iterator(rocks_iterator&& it)
    : m_session{ it.m_session }, m_iterator{ it.m_iterator }, m_index{ it.m_index } {
   it.m_session  = nullptr;
   it.m_iterator = nullptr;
   it.m_index    = -1;
}

template <typename Iterator_traits>
session<rocksdb_t>::rocks_iterator<Iterator_traits>::rocks_iterator(session<rocksdb_t>& session, rocksdb::Iterator& rit,
                                                                    int64_t index)
    : m_session{ &session }, m_iterator{ &rit }, m_index{ index } {}

template <typename Iterator_traits>
rocks_iterator_alias<Iterator_traits>&
session<rocksdb_t>::rocks_iterator<Iterator_traits>::operator=(rocks_iterator&& it) {
   if (this == &it) {
      return *this;
   }

   m_session     = it.m_session;
   m_iterator    = it.m_iterator;
   m_index       = it.m_index;
   it.m_session  = nullptr;
   it.m_iterator = nullptr;
   it.m_index    = -1;

   return *this;
}

template <typename Iterator_traits>
session<rocksdb_t>::rocks_iterator<Iterator_traits>::~rocks_iterator() {
   if (m_index > -1) {
      m_session->m_free_list.push_back(m_index);
   } else if (m_iterator) {
      delete m_iterator;
   }
   m_iterator = nullptr;
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

      if (!m_iterator->Valid()) {
         // We move backwards past the begin iterator.  We need to clamp it there.
         m_iterator->SeekToLast();
      }
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
