#pragma once

#include <iterator>
#include <memory>
#include <unordered_set>
#include <vector>

#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice_transform.h>

#include <b1/session/key_value.hpp>
#include <b1/session/rocks_data_store_fwd_decl.hpp>
#include <b1/session/shared_bytes.hpp>

namespace eosio::session {

// Defines a data store for interacting with a RocksDB database instance.
//
// \tparam Allocator The memory allocator to managed the memory used by the key_value instances stored in rockdb.
// \remarks This type implements the "data store" concept.
template <typename Allocator>
class rocks_data_store final {
 public:
   using allocator_type = Allocator;

   template <typename Iterator_traits>
   class rocks_iterator final {
    public:
      using difference_type   = typename Iterator_traits::difference_type;
      using value_type        = typename Iterator_traits::value_type;
      using pointer           = typename Iterator_traits::pointer;
      using reference         = typename Iterator_traits::reference;
      using iterator_category = typename Iterator_traits::iterator_category;

      rocks_iterator() = default;
      rocks_iterator(const rocks_iterator& it);
      rocks_iterator(rocks_iterator&&) = default;
      rocks_iterator(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<rocksdb::Iterator> rit,
                     rocksdb::ColumnFamilyHandle* column_family, rocksdb::ReadOptions read_options,
                     std::shared_ptr<Allocator> a);

      rocks_iterator& operator=(const rocks_iterator& it);
      rocks_iterator& operator=(rocks_iterator&&) = default;

      rocks_iterator& operator++();
      rocks_iterator  operator++(int);
      rocks_iterator& operator--();
      rocks_iterator  operator--(int);
      value_type      operator*() const;
      value_type      operator->() const;
      bool            operator==(const rocks_iterator& other) const;
      bool            operator!=(const rocks_iterator& other) const;

    protected:
      rocks_iterator make_iterator_() const;

    private:
      std::shared_ptr<rocksdb::DB>       m_db;
      std::shared_ptr<rocksdb::Iterator> m_iterator;
      rocksdb::ColumnFamilyHandle*       m_column_family;
      rocksdb::ReadOptions               m_read_options;
      std::shared_ptr<Allocator>         m_allocator;
   };

   struct iterator_traits final {
      using difference_type   = long;
      using value_type        = key_value;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::bidirectional_iterator_tag;
   };
   using iterator = rocks_iterator<iterator_traits>;

   struct const_iterator_traits final {
      using difference_type   = long;
      using value_type        = const key_value;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::bidirectional_iterator_tag;
   };
   using const_iterator = rocks_iterator<const_iterator_traits>;

 public:
   // TODO:  Should ColumnFamilyOptions, WriteOptions, ReadOptions also be passed in on construction
   //        or should this type just decide on its own what options to use.
   rocks_data_store()                        = default;
   rocks_data_store(const rocks_data_store&) = default;
   rocks_data_store(rocks_data_store&&)      = default;
   rocks_data_store(std::shared_ptr<rocksdb::DB> db);
   rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<Allocator> memory_allocator);

   rocks_data_store& operator=(const rocks_data_store&) = default;
   rocks_data_store& operator=(rocks_data_store&&) = default;

   const key_value read(const shared_bytes& key) const;
   void            write(key_value kv);
   bool            contains(const shared_bytes& key) const;
   void            erase(const shared_bytes& key);
   void            clear();

   template <typename Iterable>
   const std::pair<std::vector<key_value>, std::unordered_set<shared_bytes>> read(const Iterable& keys) const;

   template <typename Iterable>
   void write(const Iterable& key_values);

   template <typename Iterable>
   void erase(const Iterable& keys);

   template <typename Data_store, typename Iterable>
   void write_to(Data_store& ds, const Iterable& keys) const;

   template <typename Data_store, typename Iterable>
   void read_from(const Data_store& ds, const Iterable& keys);

   iterator       find(const shared_bytes& key);
   const_iterator find(const shared_bytes& key) const;
   iterator       begin();
   const_iterator begin() const;
   iterator       end();
   const_iterator end() const;
   iterator       lower_bound(const shared_bytes& key);
   const_iterator lower_bound(const shared_bytes& key) const;
   iterator       upper_bound(const shared_bytes& key);
   const_iterator upper_bound(const shared_bytes& key) const;

   const std::shared_ptr<Allocator>&      memory_allocator();
   const std::shared_ptr<const Allocator> memory_allocator() const;

   rocksdb::WriteOptions&       write_options();
   const rocksdb::WriteOptions& write_options() const;

   rocksdb::ReadOptions&       read_options();
   const rocksdb::ReadOptions& read_options() const;

   std::shared_ptr<rocksdb::ColumnFamilyHandle>&             column_family();
   const std::shared_ptr<const rocksdb::ColumnFamilyHandle>& column_family() const;

 protected:
   template <typename Iterable, typename Other_allocator>
   const std::pair<std::vector<key_value>, std::unordered_set<shared_bytes>>
   read_(const Iterable& keys, const std::shared_ptr<Other_allocator>& a) const;

   template <typename Predicate>
   iterator make_iterator_(const Predicate& setup);

   template <typename Predicate>
   const_iterator make_iterator_(const Predicate& setup) const;

   rocksdb::ColumnFamilyHandle* column_family_() const;

 private:
   std::shared_ptr<rocksdb::DB>                 m_db;
   std::shared_ptr<Allocator>                   m_allocator;
   std::shared_ptr<rocksdb::ColumnFamilyHandle> m_column_family;
   rocksdb::ReadOptions                         m_read_options;
   rocksdb::WriteOptions                        m_write_options;
};

template <typename Allocator>
rocks_data_store<Allocator> make_rocks_data_store(std::shared_ptr<rocksdb::DB> db,
                                                  std::shared_ptr<Allocator>   memory_allocator) {
   return { std::move(db), std::move(memory_allocator) };
}

template <typename Allocator>
rocks_data_store<Allocator>::rocks_data_store(std::shared_ptr<rocksdb::DB> db)
    : m_db{ std::move(db) }, m_allocator{ std::make_shared<Allocator>() } {}

template <typename Allocator>
rocks_data_store<Allocator>::rocks_data_store(std::shared_ptr<rocksdb::DB> db,
                                              std::shared_ptr<Allocator>   memory_allocator)
    : m_db{ std::move(db) }, m_allocator{ std::move(memory_allocator) } {}

template <typename Allocator>
const key_value rocks_data_store<Allocator>::read(const shared_bytes& key) const {
   if (!m_db) {
      return key_value::invalid;
   }

   auto key_slice      = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() };
   auto pinnable_value = rocksdb::PinnableSlice{};
   auto status         = m_db->Get(m_read_options, column_family_(), key_slice, &pinnable_value);

   if (status.code() != rocksdb::Status::Code::kOk) {
      return key_value::invalid;
   }

   return make_kv(key_slice.data(), key_slice.size(), pinnable_value.data(), pinnable_value.size(), m_allocator);
}

template <typename Allocator>
void rocks_data_store<Allocator>::write(key_value kv) {
   if (!m_db) {
      return;
   }

   auto key_slice   = rocksdb::Slice{ reinterpret_cast<const char*>(kv.key().data()), kv.key().length() };
   auto value_slice = rocksdb::Slice{ reinterpret_cast<const char*>(kv.value().data()), kv.value().length() };
   auto status      = m_db->Put(m_write_options, column_family_(), key_slice, value_slice);
}

template <typename Allocator>
bool rocks_data_store<Allocator>::contains(const shared_bytes& key) const {
   if (!m_db) {
      return false;
   }

   auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() };
   auto value     = std::string{};
   return m_db->KeyMayExist(m_read_options, column_family_(), key_slice, &value);
}

template <typename Allocator>
void rocks_data_store<Allocator>::erase(const shared_bytes& key) {
   if (!m_db) {
      return;
   }

   auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() };
   auto status    = m_db->Delete(m_write_options, column_family_(), key_slice);
}

template <typename Allocator>
void rocks_data_store<Allocator>::clear() {
   // TODO:
}

template <typename Allocator>
template <typename Iterable, typename Other_allocator>
const std::pair<std::vector<key_value>, std::unordered_set<shared_bytes>>
rocks_data_store<Allocator>::read_(const Iterable& keys, const std::shared_ptr<Other_allocator>& a) const {
   if (!m_db) {
      return {};
   }

   auto not_found  = std::unordered_set<shared_bytes>{};
   auto key_slices = std::vector<rocksdb::Slice>{};

   for (const auto& key : keys) {
      key_slices.emplace_back(reinterpret_cast<const char*>(key.data()), key.length());
      not_found.emplace(key);
   }

   auto values = std::vector<std::string>{};
   values.reserve(key_slices.size());
   auto status = m_db->MultiGet(m_read_options, { key_slices.size(), column_family_() }, key_slices, &values);

   auto kvs = std::vector<key_value>{};
   kvs.reserve(key_slices.size());

   for (size_t i = 0; i < values.size(); ++i) {
      if (status[i].code() != rocksdb::Status::Code::kOk) {
         continue;
      }

      not_found.erase(make_shared_bytes(key_slices[i].data(), key_slices[i].size()));
      kvs.emplace_back(make_kv(key_slices[i].data(), key_slices[i].size(), values[i].data(), values[i].size(), a));
   }

   return { std::move(kvs), std::move(not_found) };
}

// Reads a batch of keys from rocksdb.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An Iterable instance that returns shared_bytes instances in its iterator. \returns An std::pair
// where the first item is list of the found key_values and the second item is a set of the keys not found.
template <typename Allocator>
template <typename Iterable>
const std::pair<std::vector<key_value>, std::unordered_set<shared_bytes>>
rocks_data_store<Allocator>::read(const Iterable& keys) const {
   return read_(keys, m_allocator);
}

// Writes a batch of key_values to rocksdb.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns key_value instances in its
// iterator. \param key_values An Iterable instance that returns key_value instances in its iterator.
template <typename Allocator>
template <typename Iterable>
void rocks_data_store<Allocator>::write(const Iterable& key_values) {
   if (!m_db) {
      return;
   }

   auto batch = rocksdb::WriteBatch{ 1024 * 1024 };

   for (const auto& kv : key_values) {
      batch.Put(column_family_(), { reinterpret_cast<const char*>(kv.key().data()), kv.key().length() },
                { reinterpret_cast<const char*>(kv.value().data()), kv.value().length() });
   }

   auto status = m_db->Write(m_write_options, &batch);
}

// Erases a batch of key_values from rocksdb.
//
// \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances in its
// iterator. \param keys An Iterable instance that returns shared_bytes instances in its iterator.
template <typename Allocator>
template <typename Iterable>
void rocks_data_store<Allocator>::erase(const Iterable& keys) {
   if (!m_db) {
      return;
   }

   for (const auto& key : keys) {
      auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() };
      auto status    = m_db->Delete(m_write_options, column_family_(), key_slice);
   }
}

// Writes a batch of key_values from rocksdb into the given data_store instance.
//
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that
// concept. \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances
// in its iterator. \param ds A data store instance. \param keys An Iterable instance that returns shared_bytes
// instances in its iterator.
template <typename Allocator>
template <typename Data_store, typename Iterable>
void rocks_data_store<Allocator>::write_to(Data_store& ds, const Iterable& keys) const {
   if (!m_db) {
      return;
   }

   auto [found, not_found] = read_(keys, ds.memory_allocator());
   ds.write(found);
}

// Reads a batch of key_values from the given data store into the rocksdb.
//
// \tparam Data_store A type that implements the "data store" concept.  cache is an example of an implementation of that
// concept. \tparam Iterable Any type that can be used within a range based for loop and returns shared_bytes instances
// in its iterator. \param ds A data store instance. \param keys An Iterable instance that returns shared_bytes
// instances in its iterator.
template <typename Allocator>
template <typename Data_store, typename Iterable>
void rocks_data_store<Allocator>::read_from(const Data_store& ds, const Iterable& keys) {
   if (!m_db) {
      return;
   }

   auto [found, not_found] = ds.read(keys);
   write(found);
}

template <typename Allocator, typename Iterator_traits>
using rocks_iterator_alias = typename rocks_data_store<Allocator>::template rocks_iterator<Iterator_traits>;

// Instantiates an iterator for iterating over the rocksdb data store.
//
// \tparam Predicate A function used for preparing the initial iterator.  It has the signature of
// void(std::shared_ptr<rocksdb::Iterator>&)
template <typename Allocator>
template <typename Predicate>
typename rocks_data_store<Allocator>::iterator rocks_data_store<Allocator>::make_iterator_(const Predicate& setup) {
   auto rit = std::shared_ptr<rocksdb::Iterator>{ m_db->NewIterator(m_read_options, column_family_()) };
   setup(rit);
   return { m_db, rit, column_family_(), m_read_options, m_allocator };
}

// Instantiates an iterator for iterating over the rocksdb data store.
//
// \tparam Predicate A function used for preparing the initial iterator.  It has the signature of
// void(std::shared_ptr<rocksdb::Iterator>&)
template <typename Allocator>
template <typename Predicate>
typename rocks_data_store<Allocator>::const_iterator
rocks_data_store<Allocator>::make_iterator_(const Predicate& setup) const {
   auto rit = std::shared_ptr<rocksdb::Iterator>{ m_db->NewIterator(m_read_options, column_family_()) };
   setup(rit);
   return { m_db, rit, column_family_(), m_read_options, m_allocator };
}

template <typename Allocator>
typename rocks_data_store<Allocator>::iterator rocks_data_store<Allocator>::find(const shared_bytes& key) {
   auto predicate = [&](auto& it) {
      auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() };
      it->Seek(key_slice);
      if (it->Valid() && it->key().compare(key_slice) != 0) {
         // Get an invalid iterator
         it->SeekToLast();
         if (it->Valid()) {
            it->Next();
         }
      }
   };
   return make_iterator_(predicate);
}

template <typename Allocator>
typename rocks_data_store<Allocator>::const_iterator rocks_data_store<Allocator>::find(const shared_bytes& key) const {
   auto predicate = [&](auto& it) {
      auto key_slice = rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() };
      it->Seek(key_slice);
      if (it->Valid() && it->key().compare(key_slice) != 0) {
         // Get an invalid iterator
         it->SeekToLast();
         if (it->Valid()) {
            it->Next();
         }
      }
   };
   return make_iterator_(predicate);
}

template <typename Allocator>
typename rocks_data_store<Allocator>::iterator rocks_data_store<Allocator>::begin() {
   return make_iterator_([](auto& it) { it->SeekToFirst(); });
}

template <typename Allocator>
typename rocks_data_store<Allocator>::const_iterator rocks_data_store<Allocator>::begin() const {
   return make_iterator_([](auto& it) { it->SeekToFirst(); });
}

template <typename Allocator>
typename rocks_data_store<Allocator>::iterator rocks_data_store<Allocator>::end() {
   return make_iterator_([](auto& it) {
      it->SeekToLast();
      if (it->Valid())
         it->Next();
   });
}

template <typename Allocator>
typename rocks_data_store<Allocator>::const_iterator rocks_data_store<Allocator>::end() const {
   return make_iterator_([](auto& it) {
      it->SeekToLast();
      if (it->Valid())
         it->Next();
   });
}

template <typename Allocator>
typename rocks_data_store<Allocator>::iterator rocks_data_store<Allocator>::lower_bound(const shared_bytes& key) {
   return make_iterator_([&](auto& it) {
      it->Seek(rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() });
   });
}

template <typename Allocator>
typename rocks_data_store<Allocator>::const_iterator
rocks_data_store<Allocator>::lower_bound(const shared_bytes& key) const {
   return make_iterator_([&](auto& it) {
      it->Seek(rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() });
   });
}

template <typename Allocator>
typename rocks_data_store<Allocator>::iterator rocks_data_store<Allocator>::upper_bound(const shared_bytes& key) {
   return make_iterator_([&](auto& it) {
      it->SeekForPrev(rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() });
      if (it->Valid())
         it->Next();
   });
}

template <typename Allocator>
typename rocks_data_store<Allocator>::const_iterator
rocks_data_store<Allocator>::upper_bound(const shared_bytes& key) const {
   return make_iterator_([&](auto& it) {
      it->SeekForPrev(rocksdb::Slice{ reinterpret_cast<const char*>(key.data()), key.length() });
      if (it->Valid())
         it->Next();
   });
}

template <typename Allocator>
const std::shared_ptr<Allocator>& rocks_data_store<Allocator>::memory_allocator() {
   return m_allocator;
}

template <typename Allocator>
const std::shared_ptr<const Allocator> rocks_data_store<Allocator>::memory_allocator() const {
   return m_allocator;
}

template <typename Allocator>
rocksdb::WriteOptions& rocks_data_store<Allocator>::write_options() {
   return m_read_options;
}

template <typename Allocator>
const rocksdb::WriteOptions& rocks_data_store<Allocator>::write_options() const {
   return m_read_options;
}

template <typename Allocator>
rocksdb::ReadOptions& rocks_data_store<Allocator>::read_options() {
   return m_write_options;
}

template <typename Allocator>
const rocksdb::ReadOptions& rocks_data_store<Allocator>::read_options() const {
   return m_write_options;
}

template <typename Allocator>
std::shared_ptr<rocksdb::ColumnFamilyHandle>& rocks_data_store<Allocator>::column_family() {
   return m_column_family;
}

template <typename Allocator>
const std::shared_ptr<const rocksdb::ColumnFamilyHandle>& rocks_data_store<Allocator>::column_family() const {
   return m_column_family;
}

template <typename Allocator>
rocksdb::ColumnFamilyHandle* rocks_data_store<Allocator>::column_family_() const {
   if (m_column_family) {
      return m_column_family.get();
   }

   if (m_db) {
      return m_db->DefaultColumnFamily();
   }

   return nullptr;
}

template <typename Allocator>
template <typename Iterator_traits>
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::rocks_iterator(const rocks_iterator& it)
    : m_db{ it.m_db }, m_iterator{ [&]() {
         auto new_it =
               std::shared_ptr<rocksdb::Iterator>{ it.m_db->NewIterator(it.m_read_options, it.m_column_family) };
         if (it.m_iterator->Valid()) {
            new_it->Seek(it.m_iterator->key());
         }
         return new_it;
      }() },
      m_column_family{ it.m_column_family }, m_read_options{ it.m_read_options }, m_allocator{ it.m_allocator } {}

template <typename Allocator>
template <typename Iterator_traits>
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::rocks_iterator(std::shared_ptr<rocksdb::DB>       db,
                                                                             std::shared_ptr<rocksdb::Iterator> rit,
                                                                             rocksdb::ColumnFamilyHandle* column_family,
                                                                             rocksdb::ReadOptions         read_options,
                                                                             std::shared_ptr<Allocator>   a)
    : m_db{ std::move(db) }, m_iterator{ std::move(rit) }, m_column_family{ column_family },
      m_read_options{ std::move(read_options) }, m_allocator{ std::move(a) } {}

template <typename Allocator>
template <typename Iterator_traits>
rocks_iterator_alias<Allocator, Iterator_traits>&
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator=(const rocks_iterator& it) {
   if (this == &it) {
      return *this;
   }

   m_db            = it.m_db;
   m_allocator     = it.m_allocator;
   m_column_family = it.m_column_family;
   m_read_options  = it.m_read_options;
   m_iterator      = std::shared_ptr<rocksdb::Iterator>{ m_db->NewIterator(m_read_options, m_column_family) };
   if (it.m_iterator->Valid()) {
      m_iterator->Seek(it.m_iterator->key());
   }
   return *this;
}

template <typename Allocator>
template <typename Iterator_traits>
rocks_iterator_alias<Allocator, Iterator_traits>
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::make_iterator_() const {
   auto read_options = rocksdb::ReadOptions{};
   auto rit          = std::shared_ptr<rocksdb::Iterator>{ m_db->NewIterator(m_read_options, m_column_family) };
   if (m_iterator->Valid()) {
      rit->Seek(m_iterator->key());
   }
   return { m_db, rit, m_column_family, m_read_options, m_allocator };
}

template <typename Allocator>
template <typename Iterator_traits>
rocks_iterator_alias<Allocator, Iterator_traits>&
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator++() {
   if (m_iterator->Valid()) {
      m_iterator->Next();
   }
   return *this;
}

template <typename Allocator>
template <typename Iterator_traits>
rocks_iterator_alias<Allocator, Iterator_traits>
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator++(int) {
   auto new_it = make_iterator_();
   if (m_iterator->Valid()) {
      m_iterator->Next();
   }
   return new_it;
}

template <typename Allocator>
template <typename Iterator_traits>
rocks_iterator_alias<Allocator, Iterator_traits>&
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator--() {
   if (!m_iterator->Valid()) {
      // This means we are at the end iterator and we are iterating backwards.
      m_iterator->SeekToLast();
   } else {
      m_iterator->Prev();

      if (!m_iterator->Valid()) {
         // We move backwards past the begin iterator.  We need to clamp it there.
         m_iterator->SeekToFirst();
      }
   }
   return *this;
}

template <typename Allocator>
template <typename Iterator_traits>
rocks_iterator_alias<Allocator, Iterator_traits>
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator--(int) {
   auto new_it = make_iterator_();
   if (!m_iterator->Valid()) {
      // This means we are at the end iterator and we are iterating backwards.
      m_iterator->SeekToLast();
   } else {
      m_iterator->Prev();

      if (!m_iterator->Valid()) {
         // We move backwards past the begin iterator.  We need to clamp it there.
         m_iterator->SeekToFirst();
      }
   }
   return new_it;
}

template <typename Allocator>
template <typename Iterator_traits>
typename rocks_iterator_alias<Allocator, Iterator_traits>::value_type
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator*() const {
   if (!m_iterator->Valid()) {
      return key_value::invalid;
   }

   auto key_slice = m_iterator->key();
   auto value     = m_iterator->value();
   return make_kv(key_slice.data(), key_slice.size(), value.data(), value.size(), m_allocator);
}

template <typename Allocator>
template <typename Iterator_traits>
typename rocks_iterator_alias<Allocator, Iterator_traits>::value_type
rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator->() const {
   if (!m_iterator->Valid()) {
      return key_value::invalid;
   }

   auto key_slice = m_iterator->key();
   auto value     = m_iterator->value();
   return make_kv(key_slice.data(), key_slice.size(), value.data(), value.size(), m_allocator);
}

template <typename Allocator>
template <typename Iterator_traits>
bool rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator==(const rocks_iterator& other) const {
   if (!m_iterator->Valid() && !other.m_iterator->Valid()) {
      return true;
   }

   if (!m_iterator->Valid() || !other.m_iterator->Valid()) {
      return false;
   }

   return m_iterator->key().compare(other.m_iterator->key()) == 0 &&
          m_iterator->value().compare(other.m_iterator->value()) == 0;
}

template <typename Allocator>
template <typename Iterator_traits>
bool rocks_data_store<Allocator>::rocks_iterator<Iterator_traits>::operator!=(const rocks_iterator& other) const {
   return !(*this == other);
}

} // namespace eosio::session
