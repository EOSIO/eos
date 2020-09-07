#pragma once

#include <rocksdb/db.h>

namespace eosio::session {

template <typename Allocator>
class rocks_data_store;

template <typename Allocator>
rocks_data_store<Allocator> make_rocks_data_store(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<Allocator> memory_allocator);

}
