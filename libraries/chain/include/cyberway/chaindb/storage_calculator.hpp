#pragma once

namespace fc{
    class variant;
} // namespace fc

namespace cyberway { namespace chaindb {
    struct table_info;
    int calc_storage_usage(const table_info&, const fc::variant&);
} } // namespace cyberway::chaindb