#pragma once

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/object_value.hpp>
#include <cyberway/chaindb/table_object.hpp>

namespace cyberway { namespace chaindb {

    struct write_operation final {
        enum operation_t {
            Unknown,
            Insert,
            Update,
            Revision, // <- update only revision
            Remove,
        }; // enum operation_t

        operation_t  operation     = Unknown;
        revision_t   find_revision = unset_revision;
        object_value object;

        write_operation(const operation_t op): operation(op) { }

        write_operation() = default;
        write_operation(write_operation&&) = default;
        write_operation& operator=(write_operation&&) = default;

        static write_operation insert(object_value obj) {
            write_operation op(Insert);
            op.object = std::move(obj);
            return op;
        }

        static write_operation update(object_value obj) {
            write_operation op(Update);
            op.object = std::move(obj);
            return op;
        }

        static write_operation update(const revision_t find_rev, object_value obj) {
            write_operation op(Update);
            op.object = std::move(obj);
            op.find_revision = find_rev;
            return op;
        }

        static write_operation revision(const revision_t find_rev, object_value obj) {
            write_operation op(Revision);
            op.object = std::move(obj);
            op.find_revision = find_rev;
            // see undo_state.cpp - revision is always downgraded to one position
            op.object.service.revision = find_rev - 1;
            return op;
        }

        static write_operation remove(object_value obj) {
            write_operation op(Remove);
            op.object = std::move(obj);
            return op;
        }

        static write_operation remove(const revision_t find_rev, object_value obj) {
            write_operation op(Remove);
            op.object = std::move(obj);
            op.find_revision = find_rev;
            return op;
        }

        void clear() {
            write_operation op(Unknown);
            *this = std::move(op);
        }
    }; // struct write_operation

    class journal final {
        class write_ctx;

    public:
        journal();
        ~journal();

        void clear();

        write_ctx create_ctx(const table_info&);

        void write     (write_ctx&, write_operation data, write_operation undo);
        void write_data(write_ctx&, write_operation);
        void write_undo(write_ctx&, write_operation);

        void write_data(const table_info&, write_operation);

        template <typename Ctx>
        void apply_code_changes(Ctx&& ctx, const account_name& code) {
            auto begin_itr = index_.lower_bound(std::make_tuple(code));
            auto end_itr = index_.upper_bound(std::make_tuple(code));

            apply_range_changes(std::forward<Ctx>(ctx), begin_itr, end_itr);
        }

        template <typename Ctx>
        void apply_table_changes(Ctx&& ctx, const table_info& table) {
            auto begin_itr = index_.lower_bound(std::make_tuple(table.code, table.table->name));
            auto end_itr = index_.upper_bound(std::make_tuple(table.code, table.table->name));

            apply_range_changes(std::forward<Ctx>(ctx), begin_itr, end_itr);
        }

        template <typename Ctx>
        void apply_all_changes(Ctx&& ctx) {
            apply_range_changes(std::forward<Ctx>(ctx), index_.begin(), index_.end());
        }

    private:
        using undo_map_t_ = fc::flat_map<revision_t, write_operation>;

        struct info_t_ final {
            write_operation data;
            undo_map_t_     undo_map;
        }; // struct info_t_

        using info_map_t_ = fc::flat_map<primary_key_t, info_t_>;

        struct table_t_ final: public table_object::object {
            using table_object::object::object;
            info_map_t_ info_map;
        }; // struct table_t_

        class write_ctx final {
            primary_key_t pk_   = unset_primary_key;
            info_t_*      info_ = nullptr;

        public:
            write_ctx(table_t_& table)
            : table(table) {
            }

            table_t_& table;

            info_t_&  info(const primary_key_t);
        }; // class write_ctx

        using index_t_ = table_object::index<table_t_>;
        index_t_ index_;

        template <typename Ctx>
        void apply_range_changes(Ctx&& ctx, index_t_::iterator begin, index_t_::iterator end) {
            if (begin == end) return;

            ctx.init();
            for (auto itr = begin; end != itr; ++itr) {
                auto& table = *itr;
                if (table.info_map.empty()) continue;

                ctx.start_table(table.info());
                for (auto& info_itm: table.info_map) {
                    auto& info = info_itm.second;

                    if (info.data.operation != write_operation::Unknown) {
                        ctx.add_data(info.data);
                    }

                    for (auto& undo_itm: info.undo_map) {
                        auto& undo = undo_itm.second;

                        switch(undo.operation) {
                            case write_operation::Insert:
                                ctx.add_prepare_undo(undo);
                                break;

                            case write_operation::Update:
                            case write_operation::Revision:
                            case write_operation::Remove:
                                ctx.add_complete_undo(undo);
                                break;

                            case write_operation::Unknown:
                                break;
                        }
                    }
                }
            }

            index_.erase(begin, end);
            ctx.write();
        }

    }; // class journal

} } // namespace cyberway::chaindb