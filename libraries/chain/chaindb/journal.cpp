#include <cyberway/chaindb/journal.hpp>
#include <cyberway/chaindb/exception.hpp>

namespace cyberway { namespace chaindb {

    namespace { namespace _detail {

        void copy_find_revision(write_operation& dst, revision_t find_revision) {
            if (unset_revision != dst.find_revision) return;

            if (find_revision >= start_revision) {
                dst.find_revision = find_revision;
            } else {
                dst.find_revision = impossible_revision;
            }
        }

        void insert(write_operation src, write_operation& dst) {
            switch (dst.operation) {
                case write_operation::Unknown:
                    dst = std::move(src);
                    return;

                case write_operation::Remove:
                    dst.operation = write_operation::Update;
                    dst.object = std::move(src.object);
                    copy_find_revision(dst, src.find_revision);
                    return;

                case write_operation::Insert:
                case write_operation::Update:
                case write_operation::Revision:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on insert");
            }
        }

        void update_revision(write_operation src, write_operation& dst) {
            switch (dst.operation) {
                case write_operation::Unknown:
                    dst.operation = src.operation;
                case write_operation::Insert:
                case write_operation::Update:
                case write_operation::Revision:
                    dst.object.service = std::move(src.object.service);
                    copy_find_revision(dst, src.find_revision);
                    break;

                case write_operation::Remove:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on update_revision");
            }
        }

        void update(write_operation src, write_operation& dst) {
            switch (dst.operation) {
                case write_operation::Unknown:
                case write_operation::Revision:
                    dst.operation = src.operation;
                case write_operation::Insert:
                case write_operation::Update:
                    dst.object = std::move(src.object);
                    copy_find_revision(dst, src.find_revision);
                    break;

                case write_operation::Remove:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on update");
            }
        }

        void remove(write_operation src, write_operation& dst) {
            switch (dst.operation) {
                case write_operation::Insert:
                    dst.clear();
                    break;

                case write_operation::Unknown:
                case write_operation::Update:
                case write_operation::Revision:
                    dst.operation = src.operation;
                    dst.object = std::move(src.object);
                    copy_find_revision(dst, src.find_revision);
                    break;

                case write_operation::Remove:
                    CYBERWAY_ASSERT(false, driver_write_exception, "Wrong state on delete");
            }
        }

        void write(write_operation src, write_operation& dst) {
            switch (src.operation) {
                case write_operation::Insert:
                    return insert(std::move(src), dst);

                case write_operation::Update:
                    return update(std::move(src), dst);

                case write_operation::Revision:
                    return update_revision(std::move(src), dst);

                case write_operation::Remove:
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

    journal::write_ctx journal::create_ctx(const table_info& table) {
        auto table_itr = table_object::find(index_, table);
        if (index_.end() != table_itr) {
            return write_ctx(const_cast<table_t_&>(*table_itr));
        }

        return write_ctx(table_object::emplace(index_, table));
    }

    journal::info_t_& journal::write_ctx::info(const primary_key_t pk) {
        if (pk == pk_ && info_ != nullptr) return *info_;

        pk_ = pk;

        auto itr = table.info_map.find(pk);
        if (itr == table.info_map.end()) {
            itr = table.info_map.emplace(pk, info_t_()).first;
        }
        info_ = &itr->second;

        return *info_;
    }

    void journal::write_data(write_ctx& ctx, write_operation data) try {
        auto& info = ctx.info(data.object.pk());
        _detail::write(std::move(data), info.data);
    } FC_CAPTURE_LOG_AND_RETHROW(
        (get_full_table_name(data.object.service))
        (get_scope_name(data.object.service))(data.object.service.pk)
    )


    void journal::write_data(const table_info& table, write_operation data) {
        auto ctx = create_ctx(table);
        write_data(ctx, std::move(data));
    }

    void journal::write_undo(write_ctx& ctx, write_operation undo) try {
        _detail::copy_find_revision(undo, impossible_revision);

        auto& info = ctx.info(undo.object.pk());
        const auto rev = undo.object.service.revision;
        const auto find_rev = (undo.find_revision >= start_revision ) ? undo.find_revision : rev;
        auto itr = info.undo_map.find(find_rev);

        if (info.undo_map.end() == itr && rev != find_rev) {
            itr = info.undo_map.find(rev);
        }

        if (info.undo_map.end() == itr) {
            // create the new position
            info.undo_map.emplace(rev, std::move(undo));
        } else {
            // merge two values
            _detail::write(std::move(undo), itr->second);
            if (itr->first != rev) {
                // move to the new position
                undo = std::move(itr->second);
                info.undo_map.erase(itr);
                info.undo_map.emplace(rev, std::move(undo));
            }
        }
    } FC_CAPTURE_LOG_AND_RETHROW(
        (get_full_table_name(undo.object.service))
        (get_scope_name(undo.object.service))(undo.object.service.pk)
    )

    void journal::write(write_ctx& ctx, write_operation data, write_operation undo) {
        write_data(ctx, std::move(data));
        write_undo(ctx, std::move(undo));
    }

} } // namespace cyberway::chaindb