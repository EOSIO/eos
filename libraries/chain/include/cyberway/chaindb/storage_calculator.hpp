#pragma once

namespace fc{
    class variant;
} // namespace fc

namespace eosio { namespace chain {
    struct table_def;
} } // namespace eosio::chain

namespace cyberway { namespace chaindb {
    int calc_storage_usage(const eosio::chain::table_def&, const fc::variant&);
} } // namespace cyberway::chaindb