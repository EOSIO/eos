#include <cyberway/chaindb/journal.hpp>
#include <cyberway/chaindb/exception.hpp>

namespace cyberway { namespace chaindb {

    namespace { namespace _detail {

        void insert(write_value src, write_value& dst) {
            switch (dst.operation) {
                case write_operation::Unknown:
                case write_operation::Delete:
                    dst = std::move(src);
                    return;

                case write_operation::Insert:
                case write_operation::Update:
                case write_operation::UpdateRevision:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on insert");
            }
        }

        void update_revision(write_value src, write_value& dst) {
            switch (dst.operation) {
                case write_operation::Unknown:
                    dst.operation = src.operation;
                case write_operation::Insert:
                case write_operation::Update:
                case write_operation::UpdateRevision:
                    dst.set_revision = src.set_revision;
                    dst.find_revision = src.find_revision;
                    break;

                case write_operation::Delete:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on update_revision");
            }
        }

        void update(write_value src, write_value& dst) {
            switch (dst.operation) {
                case write_operation::Unknown:
                case write_operation::UpdateRevision:
                    dst.operation = src.operation;
                case write_operation::Insert:
                case write_operation::Update:
                    dst.set_revision = src.set_revision;
                    dst.find_revision = src.find_revision;
                    dst.value = std::move(src.value);
                    break;

                case write_operation::Delete:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on update");
            }
        }

        void remove(write_value src, write_value& dst) {
            switch (dst.operation) {
                case write_operation::Insert:
                    dst.operation = write_operation::Unknown;
                    dst.value = {};
                    break;

                case write_operation::Unknown:
                case write_operation::Update:
                case write_operation::UpdateRevision:
                    dst = std::move(src);
                    break;

                case write_operation::Delete:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on delete");
            }
        }

        void write(write_value src, write_value& dst) {
            switch (src.operation) {
                case write_operation::Insert:
                    return insert(std::move(src), dst);

                case write_operation::Update:
                    return update(std::move(src), dst);

                case write_operation::UpdateRevision:
                    return update_revision(std::move(src), dst);

                case write_operation::Delete:
                    return remove(std::move(src), dst);

                case write_operation::Unknown:
                    break;
            }
        }
    } } // namespace _detail

    journal::journal() = default;

    journal::~journal() = default;

    void journal::clear() {
        index_.clear();
    }

    const variant* journal::value(const table_info& table, const primary_key_t pk) const {
        auto table_itr = table_object::find(index_, table);
        if (index_.end() == table_itr) return nullptr;

        auto& info_map = table_itr->info_map;
        auto info_itr = info_map.find(pk);
        if (info_map.end() == info_itr) return nullptr;

        auto& data = info_itr->second.data;
        switch(data.operation) {
            case write_operation::Insert:
            case write_operation::Update:
                return &data.value;

            case write_operation::UpdateRevision:
            case write_operation::Delete:
            case write_operation::Unknown:
                break;
        }

        return nullptr;
    }

    void journal::write(const table_info& table, const primary_key_t pk, write_value data, write_value undo) { try {
        CYBERWAY_ASSERT(data.operation != write_operation::Unknown || undo.operation != write_operation::Unknown,
            driver_write_exception,
            "Bad operation type on writing to journal for the primary key ${pk} "
            "for the table ${table} in the scope '${scope}'",
            ("pk", pk)("table", get_full_table_name(table))("scope", get_scope_name(table)));

        auto table_itr = table_object::find(index_, table);
        if (index_.end() == table_itr) {
            table_itr = index_.emplace(table).first;
        }

        // not critical, because map is not a part of the index key
        auto& info_map = const_cast<table_t_&>(*table_itr).info_map;
        auto info_itr = info_map.find(pk);
        if (info_map.end() == info_itr) {
            info_t_ info{std::move(data)};
            info_itr = info_map.emplace(pk, std::move(info)).first;
        } else if (write_operation::Unknown != data.operation) {
            auto& info = info_itr->second;
            _detail::write(std::move(data), info.data);
        }

        if (BOOST_LIKELY(write_operation::Unknown != undo.operation)) {
            auto& undo_map = info_itr->second.undo_map;

            auto rev = (impossible_revision != undo.find_revision) ? undo.find_revision : undo.set_revision;
            auto undo_itr = undo_map.find(rev);
            if (undo_map.end() == undo_itr) {
                rev = (impossible_revision != undo.set_revision) ? undo.set_revision : undo.find_revision;
                undo_itr = undo_map.emplace(rev, std::move(undo)).first;
            } else if (impossible_revision != undo.set_revision && undo_itr->first) {
                undo_map.erase(undo_itr);
                undo_map.emplace(undo.set_revision, std::move(undo));
            } else {
                _detail::write(std::move(undo), undo_itr->second);
            }
        }
    } FC_LOG_AND_RETHROW() }

} } // namespace cyberway::chaindb