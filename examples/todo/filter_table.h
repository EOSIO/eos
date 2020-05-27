#pragma once

#include <eosio/eosio.hpp>

static constexpr auto filter_account = "todo.filter"_n;

namespace todo {

struct filter_data {
    eosio::checksum256 hash;
    std::string description;

    auto primary_key() const { return hash; }
};
EOSIO_REFLECT(filter_data, hash, description)
EOSIO_COMPARE(filter_data);

struct filter_table : eosio::kv_table<filter_data> {
    KV_NAMED_INDEX("primary.key", primary_key)

    filter_table() { init(filter_account, "filter.data"_n, eosio::kv_disk, primary_key); }

    static auto& instance() {
        static filter_table t;
        return t;
    }
};

} // todo namespace