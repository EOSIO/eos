#pragma once

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/table_object.hpp>

namespace cyberway { namespace chaindb {

    using fc::variant;

    enum class write_operation {
        Unknown,
        Insert,
        Update,
        UpdateRevision, // <- update only revision
        Delete,
    }; // enum class write_operation

    struct write_value final {
        write_operation operation = write_operation::Unknown;
        revision_t set_revision = impossible_revision;
        revision_t find_revision = impossible_revision;
        variant value;

        write_value() = default;
        write_value(write_value&&) = default;
        write_value& operator=(write_value&&) = default;

        write_value(write_operation op)
        : operation(op) {
        }

        write_value(write_operation op, revision_t set_rev)
        : operation(op),
          set_revision(set_rev) {
        }

        write_value(write_operation op, revision_t set_rev, variant val)
        : operation(op),
          set_revision(set_rev),
          value(std::move(val)) {
        }

        write_value(write_operation op, revision_t set_rev, revision_t find_rev)
        : operation(op),
          set_revision(set_rev),
          find_revision(find_rev) {
        }

        write_value(write_operation op, revision_t set_rev, revision_t find_rev, variant val)
        : operation(op),
          set_revision(set_rev),
          find_revision(find_rev),
          value(std::move(val)) {
        }
    }; // struct write_value

    class journal final {
    public:
        journal();
        ~journal();

        void clear();
        const variant* value(const table_info& table, const primary_key_t pk) const;
        void write(const table_info&, const primary_key_t, write_value data, write_value undo);

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
        using undo_map_t_ = fc::flat_map<revision_t, write_value>;

        struct info_t_ final {
            write_value data;
            undo_map_t_ undo_map;
        }; // struct info_type_

        using info_map_t_ = fc::flat_map<primary_key_t, info_t_>;

        struct table_t_ final: public table_object::object {
            using table_object::object::object;
            info_map_t_ info_map;
        }; // struct table_type_

        using index_t_ = table_object::index<table_t_>;
        index_t_ index_;

        template <typename Ctx>
        void apply_range_changes(Ctx&& ctx, index_t_::iterator begin, index_t_::iterator end) {
            for (auto itr = begin; end != itr; ++itr) {
                auto& table = *itr;
                if (table.info_map.empty()) continue;

                ctx.start_table(table.info());
                for (auto& info_itm: table.info_map) {
                    auto& pk = info_itm.first;
                    auto& info = info_itm.second;

                    if (info.data.operation != write_operation::Unknown) {
                        ctx.add_data(pk, info.data);
                    }

                    for (auto& undo_itm: info.undo_map) {
                        auto& undo = undo_itm.second;

                        switch(undo.operation) {
                            case write_operation::Insert:
                                ctx.add_prepare_undo(pk, undo);
                                break;

                            case write_operation::Update:
                            case write_operation::UpdateRevision:
                            case write_operation::Delete:
                                ctx.add_complete_undo(pk, undo);
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