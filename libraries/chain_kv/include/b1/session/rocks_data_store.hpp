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

namespace b1::session
{

// Defines a data store for interacting with a RocksDB database instance.
//
// \tparam allocator The memory allocator to managed the memory used by the key_value instances stored in rockdb.
// \remarks This type implements the "data store" concept.
template <typename allocator>
class rocks_data_store final
{
public:
    using allocator_type = allocator;
    
    class iterator;
    using const_iterator = const iterator;

    class iterator final
    {
    public:
        using difference_type = long;
        using value_type = key_value;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator() = default;
        iterator(const iterator& it);
        iterator(iterator&&) = default;
        iterator(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<rocksdb::Iterator> rit, std::shared_ptr<allocator> a);
        
        auto operator=(const iterator& it) -> iterator&;
        auto operator=(iterator&&) -> iterator& = default;
        
        auto operator++() -> iterator&;
        auto operator++() const -> const_iterator&;
        auto operator++(int) -> iterator;
        auto operator++(int) const -> const_iterator;
        auto operator--() -> iterator&;
        auto operator--() const -> const_iterator&;
        auto operator--(int) -> iterator;
        auto operator--(int) const -> const_iterator;
        auto operator*() const -> const value_type;
        auto operator->() const -> const value_type;
        auto operator==(const_iterator& other) const -> bool;
        auto operator!=(const_iterator& other) const -> bool;
    
    protected:
        auto make_iterator_() const -> iterator;
        
    private:
        std::shared_ptr<rocksdb::DB> m_db;
        std::shared_ptr<rocksdb::Iterator> m_iterator;
        std::shared_ptr<allocator> m_allocator;
    };

public:
    // TODO:  Should ColumnFamilyOptions, WriteOptions, ReadOptions also be passed in on construction
    //        or should this type just decide on its own what options to use.
    rocks_data_store() = default;
    rocks_data_store(const rocks_data_store&) = default;
    rocks_data_store(rocks_data_store&&) = default;
    rocks_data_store(std::shared_ptr<rocksdb::DB> db);
    rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<allocator> memory_allocator);
    
    auto operator=(const rocks_data_store&) -> rocks_data_store& = default;
    auto operator=(rocks_data_store&&) -> rocks_data_store& = default;
    
    auto read(const bytes& key) const -> const key_value;
    auto write(key_value kv) -> void;
    auto contains(const bytes& key) const -> bool;
    auto erase(const bytes& key) -> void;

    template <typename iterable>
    auto read(const iterable& keys) const -> const std::pair<std::vector<key_value>, std::unordered_set<bytes>>;

    template <typename iterable>
    auto write(const iterable& key_values) -> void;

    template <typename iterable>
    auto erase(const iterable& keys) -> void;

    template <typename data_store, typename iterable>
    auto write_to(data_store& ds, const iterable& keys) const -> void;

    template <typename data_store, typename iterable>
    auto read_from(const data_store& ds, const iterable& keys) -> void;
    
    auto find(const bytes& key) -> iterator;
    auto find(const bytes& key) const -> const_iterator;
    auto begin() -> iterator;
    auto begin() const -> const_iterator;
    auto end() -> iterator;
    auto end() const -> const_iterator;
    auto lower_bound(const bytes& key) -> iterator;
    auto lower_bound(const bytes& key) const -> const_iterator;
    auto upper_bound(const bytes& key) -> iterator;
    auto upper_bound(const bytes& key) const -> const_iterator;
    
    auto memory_allocator() -> const std::shared_ptr<allocator>&;
    auto memory_allocator() const -> const std::shared_ptr<const allocator>;
    
protected:
    template <typename iterable, typename other_allocator>
    auto read_(const iterable& keys, const std::shared_ptr<other_allocator>& a) const -> const std::pair<std::vector<key_value>, std::unordered_set<bytes>>;
    
    template <typename predicate>
    auto make_iterator_(const predicate& setup) -> iterator;
    
    template <typename predicate>
    auto make_iterator_(const predicate& setup) const -> const iterator;
    
private:
    std::shared_ptr<rocksdb::DB> m_db;
    std::shared_ptr<allocator> m_allocator;
};

template <typename allocator>
auto make_rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<allocator> memory_allocator) -> rocks_data_store<allocator>
{
    return {std::move(db), std::move(memory_allocator)};
}

template <typename allocator>
rocks_data_store<allocator>::rocks_data_store(std::shared_ptr<rocksdb::DB> db)
: m_db{std::move(db)},
  m_allocator{std::make_shared<allocator>()}
{
}

template <typename allocator>
rocks_data_store<allocator>::rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<allocator> memory_allocator)
: m_db{std::move(db)},
  m_allocator{std::move(memory_allocator)}
{
}

template <typename allocator>
auto rocks_data_store<allocator>::read(const bytes& key) const -> const key_value
{
    auto read_options = rocksdb::ReadOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
    auto pinnable_value = rocksdb::PinnableSlice{};
    auto status = m_db->Get(read_options, m_db->DefaultColumnFamily(), key_slice, &pinnable_value);
    
    if (status.code() != rocksdb::Status::Code::kOk)
    {
        return key_value::invalid;
    }
    
    return make_kv(key_slice.data(), key_slice.size(), pinnable_value.data(), pinnable_value.size(), m_allocator);
}

template <typename allocator>
auto rocks_data_store<allocator>::write(key_value kv) -> void
{
    auto write_options = rocksdb::WriteOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(kv.key().data()), kv.key().length()};
    auto value_slice = rocksdb::Slice{reinterpret_cast<const char*>(kv.value().data()), kv.value().length()};
    auto status = m_db->Put(write_options, key_slice, value_slice);
}

template <typename allocator>
auto rocks_data_store<allocator>::contains(const bytes& key) const -> bool
{
    auto read_options = rocksdb::ReadOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
    auto value = std::string{};
    return !m_db->KeyMayExist(read_options, key_slice, &value);
}

template <typename allocator>
auto rocks_data_store<allocator>::erase(const bytes& key) -> void
{
    auto write_options = rocksdb::WriteOptions{};
    auto key_slice = rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()};
    auto status = m_db->Delete(write_options, key_slice);
}

template <typename allocator>
template <typename iterable, typename other_allocator>
auto rocks_data_store<allocator>::read_(const iterable& keys, const std::shared_ptr<other_allocator>& a) const -> const std::pair<std::vector<key_value>, std::unordered_set<bytes>>
{
    auto not_found = std::unordered_set<bytes>{};
    auto key_slices = std::vector<rocksdb::Slice>{};
    
    for (const auto& key : keys)
    {
        key_slices.emplace_back(reinterpret_cast<const char*>(key.data()), key.length());
        not_found.emplace(key);
    }
    
    auto read_options = rocksdb::ReadOptions{};
    auto values = std::vector<std::string>{};
    values.reserve(key_slices.size());
    auto status = m_db->MultiGet(read_options, key_slices, &values);
    
    auto kvs = std::vector<key_value>{};
    kvs.reserve(key_slices.size());
    
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (status[i].code() != rocksdb::Status::Code::kOk)
        {
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
auto rocks_data_store<allocator>::read(const iterable& keys) const -> const std::pair<std::vector<key_value>, std::unordered_set<bytes>>
{
    return read_(keys, m_allocator);
}

// Writes a batch of key_values to rocksdb.
//
// \tparam iterable Any type that can be used within a range based for loop and returns key_value instances in its iterator.
// \param key_values An iterable instance that returns key_value instances in its iterator.
template <typename allocator>
template <typename iterable>
auto rocks_data_store<allocator>::write(const iterable& key_values) -> void
{
    auto write_options = rocksdb::WriteOptions{};
    auto batch = rocksdb::WriteBatch{1024 * 1024};
    
    for (const auto& kv : key_values)
    {
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
auto rocks_data_store<allocator>::erase(const iterable& keys) -> void
{
    auto write_options = rocksdb::WriteOptions{};
    for (const auto& key : keys)
    {
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
auto rocks_data_store<allocator>::write_to(data_store& ds, const iterable& keys) const -> void
{
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
auto rocks_data_store<allocator>::read_from(const data_store& ds, const iterable& keys) -> void
{
    auto [found, not_found] = ds.read(keys);
    write(found);
}

// Instantiates an iterator for iterating over the rocksdb data store.
//
// \tparam predicate A function used for preparing the initial iterator.  It has the signature of void(std::shared_ptr<rocksdb::Iterator>&)
template <typename allocator>
template <typename predicate>
auto rocks_data_store<allocator>::make_iterator_(const predicate& setup) -> iterator
{
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
auto rocks_data_store<allocator>::make_iterator_(const predicate& setup) const -> const iterator
{
    auto read_options = rocksdb::ReadOptions{};
    auto rit = std::shared_ptr<rocksdb::Iterator>{m_db->NewIterator(read_options)};
    setup(rit);
    return {m_db, rit, m_allocator};
}


template <typename allocator>
auto rocks_data_store<allocator>::find(const bytes& key) -> typename rocks_data_store<allocator>::iterator
{
    return make_iterator_([&](auto& it){ it->Seek(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
auto rocks_data_store<allocator>::find(const bytes& key) const -> typename rocks_data_store<allocator>::const_iterator
{
    return make_iterator_([&](auto& it){ it->Seek(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
auto rocks_data_store<allocator>::begin() -> typename rocks_data_store<allocator>::iterator
{
    return make_iterator_([](auto& it){ it->SeekToFirst(); });
}

template <typename allocator>
auto rocks_data_store<allocator>::begin() const -> typename rocks_data_store<allocator>::const_iterator
{
    return make_iterator_([](auto& it){ it->SeekToFirst(); });
}

template <typename allocator>
auto rocks_data_store<allocator>::end() -> typename rocks_data_store<allocator>::iterator
{
    return make_iterator_([](auto& it){ it->SeekToLast(); });
}

template <typename allocator>
auto rocks_data_store<allocator>::end() const -> typename rocks_data_store<allocator>::const_iterator
{
    return make_iterator_([](auto& it){ it->SeekToLast(); });
}

template <typename allocator>
auto rocks_data_store<allocator>::lower_bound(const bytes& key) -> typename rocks_data_store<allocator>::iterator
{
    return make_iterator_([&](auto& it){ it->Seek(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
auto rocks_data_store<allocator>::lower_bound(const bytes& key) const -> typename rocks_data_store<allocator>::const_iterator
{
    return make_iterator_([&](auto& it){ it->Seek(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
auto rocks_data_store<allocator>::upper_bound(const bytes& key) -> typename rocks_data_store<allocator>::iterator
{
    return make_iterator_([&](auto& it){ it->SeekForPrev(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
auto rocks_data_store<allocator>::upper_bound(const bytes& key) const -> typename rocks_data_store<allocator>::const_iterator
{
    return make_iterator_([&](auto& it){ it->SeekForPrev(rocksdb::Slice{reinterpret_cast<const char*>(key.data()), key.length()}); });
}

template <typename allocator>
auto rocks_data_store<allocator>::memory_allocator() -> const std::shared_ptr<allocator>&
{
    return m_allocator;
}

template <typename allocator>
auto rocks_data_store<allocator>::memory_allocator() const -> const std::shared_ptr<const allocator>
{
    return m_allocator;
}

template <typename allocator>
rocks_data_store<allocator>::iterator::iterator(const iterator& it)
: m_db{it.m_db},
  m_iterator{[&]()
  {
    auto new_it = std::shared_ptr<rocksdb::Iterator>{it.m_db->NewIterator(rocksdb::ReadOptions{})};
    new_it->Seek(it.m_iterator->Key());
    return new_it;
  }()},
  m_allocator{it.m_allocator}
{
}

template <typename allocator>
rocks_data_store<allocator>::iterator::iterator(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<rocksdb::Iterator> rit, std::shared_ptr<allocator> a)
: m_db{std::move(db)},
  m_iterator{std::move(rit)},
  m_allocator{std::move(a)}
{
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator=(const iterator& it) -> typename rocks_data_store<allocator>::iterator&
{
    if (this == &it)
    {
        return *this;
    }
    
    m_db = it.m_db;
    m_allocator = it.m_allocator;
    m_iterator = std::shared_ptr<rocksdb::Iterator>{m_db->NewIterator(rocksdb::ReadOptions{})};
    m_iterator->Seek(it.m_iterator->key());
    
    return *this;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::make_iterator_() const -> typename rocks_data_store<allocator>::iterator
{
    auto read_options = rocksdb::ReadOptions{};
    auto rit = std::shared_ptr<rocksdb::Iterator>{m_db->NewIterator(read_options)};
    rit->Seek(m_iterator->key());
    return {m_db, rit, m_allocator};
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator++() -> typename rocks_data_store<allocator>::iterator&
{
    m_iterator->Next();
    return *this;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator++() const -> typename rocks_data_store<allocator>::const_iterator&
{
    m_iterator->Next();
    return *this;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator++(int) -> typename rocks_data_store<allocator>::iterator
{
    auto new_it = make_iterator_();
    m_iterator->Next();
    return new_it;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator++(int) const -> typename rocks_data_store<allocator>::const_iterator
{
    auto new_it = make_iterator_();
    m_iterator->Next();
    return new_it;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator--() -> typename rocks_data_store<allocator>::iterator&
{
    m_iterator->Prev();
    return *this;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator--() const -> typename rocks_data_store<allocator>::const_iterator&
{
    m_iterator->Prev();
    return *this;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator--(int) -> typename rocks_data_store<allocator>::iterator
{
    auto new_it = make_iterator_();
    m_iterator->Prev();
    return new_it;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator--(int) const -> typename rocks_data_store<allocator>::const_iterator
{
    auto new_it = make_iterator_();
    m_iterator->Prev();
    return new_it;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator*() const -> const value_type
{
    auto key_slice = m_iterator->key();
    auto value = m_iterator->value();
    return make_kv(key_slice.data(), key_slice.size(), value.data(), value.size(), m_allocator);
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator->() const -> const value_type
{
    auto key_slice = m_iterator->key();
    auto value = m_iterator->value();
    return make_kv(key_slice.data(), key_slice.size(), value.data(), value.size(), m_allocator);
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator==(const_iterator& other) const -> bool
{
    return m_iterator->key().compare(other.m_iterator->key()) == 0
        && m_iterator->value().compare(other.m_iterator->value()) == 0;
}

template <typename allocator>
auto rocks_data_store<allocator>::iterator::operator!=(const_iterator& other) const -> bool
{
    return !(*this == other);
}

}
