#pragma once

#include <iterator>
#include <memory>
#include <unordered_set>
#include <vector>

#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice_transform.h>

#include <b1/session/bytes.hpp>
#include <b1/session/key_value.hpp>
#include <b1/session/rocks_data_store_fwd_decl.hpp>

namespace eosio::session {

// Defines a data store for interacting with a RocksDB database instance.
//
// \tparam allocator The memory allocator to managed the memory used by the key_value instances stored in rockdb.
// \remarks This type implements the "data store" concept.
template <typename allocator>
class rocks_data_store final {
public:
    using allocator_type = allocator;

    template <typename iterator_traits>
    class rocks_iterator final {
    public:
        using difference_type = typename iterator_traits::difference_type;
        using value_type = typename iterator_traits::value_type;
        using pointer = typename iterator_traits::pointer;
        using reference = typename iterator_traits::reference;
        using iterator_category = typename iterator_traits::iterator_category;

        rocks_iterator() = default;
        rocks_iterator(const rocks_iterator& it);
        rocks_iterator(rocks_iterator&&) = default;
        rocks_iterator(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<rocksdb::Iterator> rit, std::shared_ptr<allocator> a);
        
        rocks_iterator& operator=(const rocks_iterator& it);
        rocks_iterator& operator=(rocks_iterator&&) = default;
        
        rocks_iterator& operator++();
        rocks_iterator operator++(int);
        rocks_iterator& operator--();
        rocks_iterator operator--(int);
        value_type operator*() const;
        value_type operator->() const;
        bool operator==(const rocks_iterator& other) const;
        bool operator!=(const rocks_iterator& other) const;
    
    protected:
        rocks_iterator make_iterator_() const;
        
    private:
        std::shared_ptr<rocksdb::DB> m_db;
        std::shared_ptr<rocksdb::Iterator> m_iterator;
        std::shared_ptr<allocator> m_allocator;
    };

    struct iterator_traits {
        using difference_type = long;
        using value_type = key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
    };
    using iterator = rocks_iterator<iterator_traits>;

    struct const_iterator_traits {
        using difference_type = long;
        using value_type = const key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;
    };
    using const_iterator = rocks_iterator<const_iterator_traits>;

public:
    // TODO:  Should ColumnFamilyOptions, WriteOptions, ReadOptions also be passed in on construction
    //        or should this type just decide on its own what options to use.
    rocks_data_store() = default;
    rocks_data_store(const rocks_data_store&) = default;
    rocks_data_store(rocks_data_store&&) = default;
    rocks_data_store(std::shared_ptr<rocksdb::DB> db);
    rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<allocator> memory_allocator);
    
    rocks_data_store& operator=(const rocks_data_store&) = default;
    rocks_data_store& operator=(rocks_data_store&&) = default;
    
    const key_value read(const bytes& key) const;
    void write(key_value kv);
    bool contains(const bytes& key) const;
    void erase(const bytes& key);

    template <typename iterable>
    const std::pair<std::vector<key_value>, std::unordered_set<bytes>> read(const iterable& keys) const;

    template <typename iterable>
    void write(const iterable& key_values);

    template <typename iterable>
    void erase(const iterable& keys);

    template <typename data_store, typename iterable>
    void write_to(data_store& ds, const iterable& keys) const;

    template <typename data_store, typename iterable>
    void read_from(const data_store& ds, const iterable& keys);
    
    iterator find(const bytes& key);
    const_iterator find(const bytes& key) const;
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    iterator lower_bound(const bytes& key);
    const_iterator lower_bound(const bytes& key) const;
    iterator upper_bound(const bytes& key);
    const_iterator upper_bound(const bytes& key) const;
    
    const std::shared_ptr<allocator>& memory_allocator();
    const std::shared_ptr<const allocator> memory_allocator() const;
    
protected:
    template <typename iterable, typename other_allocator>
    const std::pair<std::vector<key_value>, std::unordered_set<bytes>> read_(const iterable& keys, const std::shared_ptr<other_allocator>& a) const;
    
    template <typename predicate>
    iterator make_iterator_(const predicate& setup);
    
    template <typename predicate>
    const_iterator make_iterator_(const predicate& setup) const;
    
private:
    std::shared_ptr<rocksdb::DB> m_db;
    std::shared_ptr<allocator> m_allocator;
};

template <typename allocator>
rocks_data_store<allocator> make_rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<allocator> memory_allocator) {
    return {std::move(db), std::move(memory_allocator)};
}

template <typename allocator>
rocks_data_store<allocator>::rocks_data_store(std::shared_ptr<rocksdb::DB> db)
: m_db{std::move(db)},
  m_allocator{std::make_shared<allocator>()} {
}

template <typename allocator>
rocks_data_store<allocator>::rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<allocator> memory_allocator)
: m_db{std::move(db)},
  m_allocator{std::move(memory_allocator)} {
}

template <typename allocator>
const key_value rocks_data_store<allocator>::read(const bytes& key) const {
    if (!m_db) {
        return key_value::invalid;
    }

    auto read_options = rocksdb::ReadOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
    auto pinnable_value = rocksdb::PinnableSlice{};
    auto status = m_db->Get(read_options, m_db->DefaultColumnFamily(), key_slice, &pinnable_value);
    
    if (status.code() != rocksdb::Status::Code::kOk) {
        return key_value::invalid;
    }
    
    return make_kv(key_slice.data(), key_slice.size(), pinnable_value.data(), pinnable_value.size(), m_allocator);
}

template <typename allocator>
void rocks_data_store<allocator>::write(key_value kv) {
    if (!m_db) {
        return;
    }

    auto write_options = rocksdb::WriteOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(kv.key().data()), kv.key().length()};
    auto value_slice = rocksdb::Slice{reinterpret_cast<const char*>(kv.value().data()), kv.value().length()};
    auto status = m_db->Put(write_options, key_slice, value_slice);
}

template <typename allocator>
bool rocks_data_store<allocator>::contains(const bytes& key) const {
    if (!m_db) {
        return false;
    }

    auto read_options = rocksdb::ReadOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
    auto value = std::string{};
    return m_db->KeyMayExist(read_options, key_slice, &value);
}

template <typename allocator>
void rocks_data_store<allocator>::erase(const bytes& key) {
    if (!m_db) {
        return;
    }
    
    auto write_options = rocksdb::WriteOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
    auto status = m_db->Delete(write_options, key_slice);
}

template <typename allocator>
template <typename iterable, typename other_allocator>
const std::pair<std::vector<key_value>, std::unordered_set<bytes>> rocks_data_store<allocator>::read_(const iterable& keys, const std::shared_ptr<other_allocator>& a) const {
    if (!m_db) {
        return {};
    }

    auto not_found = std::unordered_set<bytes>{};
    auto key_slices = std::vector<rocksdb::Slice>{};
    
    for (const auto& key : keys) {
        key_slices.emplace_back(reinterpret_cast<const char*>(key.data()), key.length());
        not_found.emplace(key);
    }
    
    auto read_options = rocksdb::ReadOptions{};
    auto values = std::vector<std::string>{};
    values.reserve(key_slices.size());
    auto status = m_db->MultiGet(read_options, key_slices, &values);
    
    auto kvs = std::vector<key_value>{};
    kvs.reserve(key_slices.size());
    
    for (size_t i = 0; i < values.size(); ++i) {
        if (status[i].code() != rocksdb::Status::Code::kOk) {
            continue;
        }
        
        not_found.erase(make_bytes(key_slices[i].data(), key_slices[i].size()));
        kvs.emplace_back(make_kv(key_slices[i].data(), key_slices[i].size(), values[i].data(), values[i].size(), a));
    }
    
    return {std::move(kvs), std::move(not_found)};
}

// Reads a batch of keys from rocksdb.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
// \returns An std::pair where the first item is list of the found key_values and the second item is a set of the keys not found.
template <typename allocator>
template <typename iterable>
const std::pair<std::vector<key_value>, std::unordered_set<bytes>> rocks_data_store<allocator>::read(const iterable& keys) const {
    return read_(keys, m_allocator);
}

// Writes a batch of key_values to rocksdb.
//
// \tparam iterable Any type that can be used within a range based for loop and returns key_value instances in its iterator.
// \param key_values An iterable instance that returns key_value instances in its iterator.
template <typename allocator>
template <typename iterable>
void rocks_data_store<allocator>::write(const iterable& key_values) {
    if (!m_db) {
        return;
    }

    auto write_options = rocksdb::WriteOptions{};
    auto batch = rocksdb::WriteBatch{1024 * 1024};
    
    for (const auto& kv : key_values) {
        batch.Put({reinterpret_cast<const char*>(kv.key().data()), kv.key().length()},
                  {reinterpret_cast<const char*>(kv.value().data()), kv.value().length()});
    }
    
    auto status = m_db->Write(write_options, &batch);
}

// Erases a batch of key_values from rocksdb.
//
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename allocator>
template <typename iterable>
void rocks_data_store<allocator>::erase(const iterable& keys) {
    if (!m_db) {
        return;
    }

    auto write_options = rocksdb::WriteOptions{};
    for (const auto& key : keys) {
        auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
        auto status = m_db->Delete(write_options, key_slice);
    }
}

// Writes a batch of key_values from rocksdb into the given data_store instance.
//
// \tparam data_store A type that implements the "data store" concept.  cache is an example of an implementation of that concept.
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param ds A data store instance.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename allocator>
template <typename data_store, typename iterable>
void rocks_data_store<allocator>::write_to(data_store& ds, const iterable& keys) const {
    if (!m_db) {
        return;
    }

    auto [found, not_found] = read_(keys, ds.memory_allocator());
    ds.write(found);
}

// Reads a batch of key_values from the given data store into the rocksdb.
//
// \tparam data_store A type that implements the "data store" concept.  cache is an example of an implementation of that concept.
// \tparam iterable Any type that can be used within a range based for loop and returns bytes instances in its iterator.
// \param ds A data store instance.
// \param keys An iterable instance that returns bytes instances in its iterator.
template <typename allocator>
template <typename data_store, typename iterable>
void rocks_data_store<allocator>::read_from(const data_store& ds, const iterable& keys) {
    if (!m_db) {
        return;
    }
    
    auto [found, not_found] = ds.read(keys);
    write(found);
}

template <typename allocator, typename iterator_traits>
using rocks_iterator_alias = typename rocks_data_store<allocator>::template rocks_iterator<iterator_traits>;

// Instantiates an iterator for iterating over the rocksdb data store.
//
// \tparam predicate A function used for preparing the initial iterator.  It has the signature of void(std::shared_ptr<rocksdb::Iterator>&)
template <typename allocator>
template <typename predicate>
typename rocks_data_store<allocator>::iterator rocks_data_store<allocator>::make_iterator_(const predicate& setup)  {
    auto read_options = rocksdb::ReadOptions{};
    auto rit = std::shared_ptr<rocksdb::Iterator>{m_db->NewIterator(read_options)};
    setup(rit);
    return {m_db, rit, m_allocator};
}

// Instantiates an iterator for iterating over the rocksdb data store.
//
// \tparam predicate A function used for preparing the initial iterator.  It has the signature of void(std::shared_ptr<rocksdb::Iterator>&)
template <typename allocator>
template <typename predicate>
typename rocks_data_store<allocator>::const_iterator rocks_data_store<allocator>::make_iterator_(const predicate& setup) const {
    auto read_options = rocksdb::ReadOptions{};
    auto rit = std::shared_ptr<rocksdb::Iterator>{m_db->NewIterator(read_options)};
    setup(rit);
    return {m_db, rit, m_allocator};
}

template <typename allocator>
typename rocks_data_store<allocator>::iterator rocks_data_store<allocator>::find(const bytes& key) {
    auto predicate = [&](auto& it) {
      auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
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

template <typename allocator>
typename rocks_data_store<allocator>::const_iterator rocks_data_store<allocator>::find(const bytes& key) const {
    auto predicate = [&](auto& it) {
      auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
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

template <typename allocator>
typename rocks_data_store<allocator>::iterator rocks_data_store<allocator>::begin() {
    return make_iterator_([](auto& it){ it->SeekToFirst(); });
}

template <typename allocator>
typename rocks_data_store<allocator>::const_iterator rocks_data_store<allocator>::begin() const {
    return make_iterator_([](auto& it){ it->SeekToFirst(); });
}

template <typename allocator>
typename rocks_data_store<allocator>::iterator rocks_data_store<allocator>::end() {
    return make_iterator_([](auto& it){ it->SeekToLast(); if (it->Valid()) it->Next(); });
}

template <typename allocator>
typename rocks_data_store<allocator>::const_iterator rocks_data_store<allocator>::end() const {
    return make_iterator_([](auto& it){ it->SeekToLast(); if (it->Valid()) it->Next(); });
}

template <typename allocator>
typename rocks_data_store<allocator>::iterator rocks_data_store<allocator>::lower_bound(const bytes& key) {
    return make_iterator_([&](auto& it){ it->Seek(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
typename rocks_data_store<allocator>::const_iterator rocks_data_store<allocator>::lower_bound(const bytes& key) const {
    return make_iterator_([&](auto& it){ it->Seek(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
typename rocks_data_store<allocator>::iterator rocks_data_store<allocator>::upper_bound(const bytes& key) {
    return make_iterator_([&](auto& it){ it->SeekForPrev(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); if (it->Valid()) it->Next(); });
}

template <typename allocator>
typename rocks_data_store<allocator>::const_iterator rocks_data_store<allocator>::upper_bound(const bytes& key) const {
    return make_iterator_([&](auto& it){ it->SeekForPrev(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); if (it->Valid()) it->Next(); });
}

template <typename allocator>
const std::shared_ptr<allocator>& rocks_data_store<allocator>::memory_allocator() {
    return m_allocator;
}

template <typename allocator>
const std::shared_ptr<const allocator> rocks_data_store<allocator>::memory_allocator() const {
    return m_allocator;
}

template <typename allocator>
template <typename iterator_traits>
rocks_data_store<allocator>::rocks_iterator<iterator_traits>::rocks_iterator(const rocks_iterator& it)
: m_db{it.m_db},
  m_iterator{[&]() {
    auto new_it = std::shared_ptr<rocksdb::Iterator>{it.m_db->NewIterator(rocksdb::ReadOptions{})};
    if (it.m_iterator->Valid()) {
        new_it->Seek(it.m_iterator->key());
    }
    return new_it;
  }()},
  m_allocator{it.m_allocator} {
}

template <typename allocator>
template <typename iterator_traits>
rocks_data_store<allocator>::rocks_iterator<iterator_traits>::rocks_iterator(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<rocksdb::Iterator> rit, std::shared_ptr<allocator> a)
: m_db{std::move(db)},
  m_iterator{std::move(rit)},
  m_allocator{std::move(a)} {
}

template <typename allocator>
template <typename iterator_traits>
rocks_iterator_alias<allocator, iterator_traits>& rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator=(const rocks_iterator& it) {
    if (this == &it) {
        return *this;
    }
    
    m_db = it.m_db;
    m_allocator = it.m_allocator;
    m_iterator = std::shared_ptr<rocksdb::Iterator>{m_db->NewIterator(rocksdb::ReadOptions{})};
    if (it.m_iterator->Valid()) {
        m_iterator->Seek(it.m_iterator->key());
    }
    return *this;
}

template <typename allocator>
template <typename iterator_traits>
rocks_iterator_alias<allocator, iterator_traits> rocks_data_store<allocator>::rocks_iterator<iterator_traits>::make_iterator_() const {
    auto read_options = rocksdb::ReadOptions{};
    auto rit = std::shared_ptr<rocksdb::Iterator>{m_db->NewIterator(read_options)};
    if (m_iterator->Valid()) {
        rit->Seek(m_iterator->key());
    }
    return {m_db, rit, m_allocator};
}

template <typename allocator>
template <typename iterator_traits>
rocks_iterator_alias<allocator, iterator_traits>& rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator++() {
    if (m_iterator->Valid()) {
        m_iterator->Next();
    }
    return *this;
}

template <typename allocator>
template <typename iterator_traits>
rocks_iterator_alias<allocator, iterator_traits> rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator++(int) {
    auto new_it = make_iterator_();
    if (m_iterator->Valid()) {
        m_iterator->Next();
    }
    return new_it;
}

template <typename allocator>
template <typename iterator_traits>
rocks_iterator_alias<allocator, iterator_traits>& rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator--() {
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

template <typename allocator>
template <typename iterator_traits>
rocks_iterator_alias<allocator, iterator_traits> rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator--(int) {
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

template <typename allocator>
template <typename iterator_traits>
typename rocks_iterator_alias<allocator, iterator_traits>::value_type rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator*() const {
    if (!m_iterator->Valid()) {
        return key_value::invalid;
    }

    auto key_slice = m_iterator->key();
    auto value = m_iterator->value();
    return make_kv(key_slice.data(), key_slice.size(), value.data(), value.size(), m_allocator);
}

template <typename allocator>
template <typename iterator_traits>
typename rocks_iterator_alias<allocator, iterator_traits>::value_type rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator->() const {
    if (!m_iterator->Valid()) {
        return key_value::invalid;
    }

    auto key_slice = m_iterator->key();
    auto value = m_iterator->value();
    return make_kv(key_slice.data(), key_slice.size(), value.data(), value.size(), m_allocator);
}

template <typename allocator>
template <typename iterator_traits>
bool rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator==(const rocks_iterator& other) const {
    if (!m_iterator->Valid() && !other.m_iterator->Valid()) {
      return true;
    }

    if (!m_iterator->Valid() || !other.m_iterator->Valid()) {
      return false;
    }

    return m_iterator->key().compare(other.m_iterator->key()) == 0
        && m_iterator->value().compare(other.m_iterator->value()) == 0;
}

template <typename allocator>
template <typename iterator_traits>
bool rocks_data_store<allocator>::rocks_iterator<iterator_traits>::operator!=(const rocks_iterator& other) const {
    return !(*this == other);
}

}
