#pragma once

#include <rocksdb/db.h>

namespace eosio::session {

class rocks_data_store;

rocks_data_store make_rocks_data_store(std::shared_ptr<rocksdb::DB> db);

} // namespace eosio::session
