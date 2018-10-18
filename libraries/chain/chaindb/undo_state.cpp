#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/indexed_by.hpp>

#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb {

    namespace bmi = boost::multi_index;

    static constexpr int64_t impossible_revision = std::numeric_limits<int64_t>::max();

    enum class undo_stage {
        Unknown,
        New,
        Stack,
        Rollback,
    };

    struct undo_state final {
        undo_state() = default;
        undo_state(int64_t value): revision(value) { }

        using pk_set = std::set<primary_key_t>;
        using pk_value_map = std::map<primary_key_t, variant>;

        int64_t      revision = impossible_revision;
        pk_value_map old_values;
        pk_value_map removed_values;
        pk_set       new_ids;
    }; // struct undo_state

    class table_undo_stack final {

        const table_def table_def_;  // <- copy to replace pointer in info_
        table_info      info_;       // <- for const reference
        undo_stage      stage_ = undo_stage::Unknown;    // <- state machine - changes via actions
        int64_t         revision_ = impossible_revision; // <- part of state machine - changes via actions

        std::deque<undo_state> stack_;                   // <- access depends on state machine

    public:
        // key part
        const account_name code;
        const account_name scope;
        const table_name   table;
        const field_name   pk_field; // <- for case when contract change its primary key

        table_undo_stack(const table_info& src)
        : table_def_(*src.table),   // <- one copy per serie of sessions - it is not very critical
          info_(src),
          code(src.code),
          scope(src.scope),
          table(src.table->name),
          pk_field(*src.pk_field) { // <- one copy per serie of sessions - it is not very critical
            info_.table = &table_def_;
            info_.pk_field = &pk_field;
        }

        const table_info& info() const {
            return info_;
        }

        int64_t revision() const {
            return revision_;
        }

        void start_session(const int64_t revision) {
            CYBERWAY_ASSERT(!empty(), session_exception,
                "The table ${table} stack is empty.", ("table", get_full_table_name(info_)));

            CYBERWAY_ASSERT(stack_.back().revision < revision, session_exception,
                "Bad revision ${table_revision} (new ${revision}) for the table ${table}.",
                ("table", get_full_table_name(info_))("table_revision", stack_.back().revision)
                ("revision", revision));

            revision_ = revision;
            stage_ = undo_stage::New;
        }

        undo_state& head() {
            switch (stage_) {
                case undo_stage::Unknown:
                case undo_stage::Rollback:
                    break;

                case undo_stage::New: {
                    stage_ = undo_stage::Stack;
                    stack_.emplace_back(revision_);
                }

                case undo_stage::Stack:
                    return stack_.front();

            }

            CYBERWAY_ASSERT(false, session_exception,
                "Wrong stage ${stage} of the table ${table}",
                ("table", get_full_table_name(info_))("stage", stage_));
        }

        undo_state& prev_state() {
            switch (stage_) {
                case undo_stage::Unknown:
                case undo_stage::Rollback:
                    break;

                case undo_stage::Stack:
                    CYBERWAY_ASSERT(size() >= 2, session_exception,
                        "The table ${table} doesn't have 2 states",
                        ("table", get_full_table_name(info_)));
                    return stack_[stack_.size() - 2];

                case undo_stage::New:
                    CYBERWAY_ASSERT(!empty(), session_exception,
                        "The table ${table} doesn't have 1 state",
                        ("table", get_full_table_name(info_)));
                    return stack_.back();
            }

            CYBERWAY_ASSERT(false, session_exception,
                "Wrong stage ${stage} of the table ${table}",
                ("table", get_full_table_name(info_))("stage", stage_));
        }

        void commit(const int64_t revision) {
            CYBERWAY_ASSERT(!empty(), session_exception,
                "The table ${table} stack is empty.", ("table", get_full_table_name(info_)));

            if (stack_.back().revision <= revision) {
                stack_.clear();
            } else {
                while (!stack_.empty() && stack_.front().revision <= revision) {
                    stack_.pop_front();
                }
            }
        }

        void undo() {
            switch (stage_) {
                case undo_stage::Unknown:
                case undo_stage::Rollback:
                    break;

                case undo_stage::Stack: {
                    stack_.pop_back();
                    if (empty()) {
                        stage_    = undo_stage::Rollback;
                        revision_ = impossible_revision; // for validation in other places
                        return; // table should be removed
                    }
                }

                case undo_stage::New:
                    stage_    = undo_stage::Rollback;
                    revision_ = stack_.back().revision;
                    return;
            }

            CYBERWAY_ASSERT(false, session_exception,
                "Wrong stage ${stage} of the table ${table}",
                ("table", get_full_table_name(info_))("stage", stage_));
        }

        size_t size() const {
            return stack_.size();
        }

        bool empty() const {
            return stack_.empty();
        }

    }; // struct table_undo_stack

    using table_undo_stack_index = bmi::multi_index_container<
        table_undo_stack,
        bmi::indexed_by<
            bmi::ordered_unique<
                bmi::composite_key<
                    table_undo_stack,
                    bmi::member<table_undo_stack, const account_name, &table_undo_stack::code>,
                    bmi::member<table_undo_stack, const account_name, &table_undo_stack::scope>,
                    bmi::member<table_undo_stack, const table_name,   &table_undo_stack::table>,
                    bmi::member<table_undo_stack, const field_name,   &table_undo_stack::pk_field>>>>>;

    struct undo_stack::undo_stack_impl_ final {
        undo_stack_impl_(driver_interface& driver)
        : driver_(driver)
        { }

        void set_revision(const int64_t value) {
            CYBERWAY_ASSERT(!tables_.empty(), session_exception,
                "Cannot set revision while there is an existing undo stack.");
            revision_ = value;
        }

        int64_t revision() const {
            return revision_;
        }

        int64_t start_undo_session(bool enabled) {
            if (enabled) {
                ++revision_;
                for (auto& const_table: tables_) {
                    auto& table = const_cast<table_undo_stack&>(const_table); // not critical
                    table.start_session(revision_);
                }
                return revision_;
            } else {
                return -1;
            }
        }

        void push(const int64_t push_revision) {
            CYBERWAY_ASSERT(push_revision == revision_, session_exception, "Wrong push revision");
        }

        void undo(const int64_t undo_revision) {
            for_tables([&](auto& table){
                undo(table, undo_revision, revision_);
            });
            --revision_;
        }

        void squash(const int64_t squash_revision) {
            for_tables([&](auto& table){
                squash(table, squash_revision, revision_);
            });
            --revision_;
        }

        void commit(const int64_t commit_revision) {
            for_tables([&](auto& table){
                commit(table, commit_revision);
            });
            revision_ = commit_revision;
        }

        void undo_all() {
            for_tables([&](auto& table) {
                undo_all(table);
            });
        }

        void update(const table_info& table, const primary_key_t pk, const variant& value) {
            update(get_table(table), pk, value);
        }

        void remove(const table_info& table, const primary_key_t pk, const variant& value) {
            remove(get_table(table), pk, value);
        }

        void insert(const table_info& table, const primary_key_t pk, const variant& value) {
            insert(get_table(table), pk, value);
        }

    private:
        table_undo_stack& get_table(const table_info& table) {
            auto itr = tables_.find(std::make_tuple(table.code, table.scope, table.table->name, *table.pk_field));
            if (tables_.end() != itr) return const_cast<table_undo_stack&>(*itr); // not critical

            auto result = tables_.emplace(table);
            CYBERWAY_ASSERT(result.second, session_exception,
                "Fail to create stack for the table ${table} with the primary key ${pk}",
                ("table", get_full_table_name(table))("pk", *table.pk_field));

            return const_cast<table_undo_stack&>(*result.first);
        }

        template <typename Lambda>
        void for_tables(Lambda&& lambda) {
            for (auto itr = tables_.begin(), etr = tables_.end(); etr != itr; ) {
                auto& table = const_cast<table_undo_stack&>(*itr);
                lambda(table);

                if (table.empty()) {
                    tables_.erase(itr++);
                } else {
                    ++itr;
                }
            }
        }

        void undo(table_undo_stack& table, const int64_t undo_revision, const int64_t test_revision) {
            if (undo_revision > table.revision()) return;

            const auto& head = table.head();

            CYBERWAY_ASSERT(head.revision == undo_revision == test_revision, session_exception, "Wrong undo revision");

            for (auto& item: head.old_values) try {
                driver_.update(table.info(), item.first, std::move(item.second));
            } catch (const driver_update_exception&) {
                CYBERWAY_ASSERT(false, session_exception, "Could not modify object");
            }

            for (auto pk: head.new_ids) try {
                driver_.remove(table.info(), pk);
            } catch (const driver_delete_exception&) {
                CYBERWAY_ASSERT(false, session_exception, "Could not remove object");
            }

            for (auto& item: head.removed_values) try {
                driver_.insert(table.info(), item.first, std::move(item.second));
            } catch (const driver_insert_exception&) {
                CYBERWAY_ASSERT(false, session_exception, "Could not insert object");
            }

            table.undo();
        }

        void squash(table_undo_stack& table, const int64_t squash_revision, const int64_t test_revision) {
            if (squash_revision > table.revision()) return;

            auto& state = table.head();
            CYBERWAY_ASSERT(state.revision == squash_revision == test_revision, session_exception, "Wrong squash revision");

            // Only one stack item
            if (table.size() == 1) {
                table.undo();
                return;
            }

            auto& prev_state = table.prev_state();

            // Two stack items but they are not neighbours
            if (prev_state.revision != state.revision + 1) {
                --state.revision;
                return;
            }

            // An object's relationship to a state can be:
            // in new_ids            : new
            // in old_values (was=X) : upd(was=X)
            // in removed (was=X)    : del(was=X)
            // not in any of above   : nop
            //
            // When merging A=prev_state and B=state we have a 4x4 matrix of all possibilities:
            //
            //                   |--------------------- B ----------------------|
            //
            //                +------------+------------+------------+------------+
            //                | new        | upd(was=Y) | del(was=Y) | nop        |
            //   +------------+------------+------------+------------+------------+
            // / | new        | N/A        | new       A| nop       C| new       A|
            // | +------------+------------+------------+------------+------------+
            // | | upd(was=X) | N/A        | upd(was=X)A| del(was=X)C| upd(was=X)A|
            // A +------------+------------+------------+------------+------------+
            // | | del(was=X) | N/A        | N/A        | N/A        | del(was=X)A|
            // | +------------+------------+------------+------------+------------+
            // \ | nop        | new       B| upd(was=Y)B| del(was=Y)B| nop      AB|
            //   +------------+------------+------------+------------+------------+
            //
            // Each entry was composed by labelling what should occur in the given case.
            //
            // Type A means the composition of states contains the same entry as the first of the two merged states for that object.
            // Type B means the composition of states contains the same entry as the second of the two merged states for that object.
            // Type C means the composition of states contains an entry different from either of the merged states for that object.
            // Type N/A means the composition of states violates causal timing.
            // Type AB means both type A and type B simultaneously.
            //
            // The merge() operation is defined as modifying prev_state in-place to be the state object which represents the composition of
            // state A and B.
            //
            // Type A (and AB) can be implemented as a no-op; prev_state already contains the correct value for the merged state.
            // Type B (and AB) can be implemented by copying from state to prev_state.
            // Type C needs special case-by-case logic.
            // Type N/A can be ignored or assert(false) as it can only occur if prev_state and state have illegal values
            // (a serious logic error which should never happen).
            //

            // We can only be outside type A/AB (the nop path) if B is not nop, so it suffices to iterate through B's three containers.

            for (const auto& item : state.old_values) {
                // new+upd -> new, type A
                if (prev_state.new_ids.find(item.first) != prev_state.new_ids.end()) {
                    continue;
                }

                // upd(was=X) + upd(was=Y) -> upd(was=X), type A
                if (prev_state.old_values.find(item.first) != prev_state.old_values.end()) {
                    continue;
                }

                // del+upd -> N/A
                CYBERWAY_ASSERT(prev_state.removed_values.find(item.first) == prev_state.removed_values.end(), session_exception,
                    "UB for the table ${table}: Delete + Update", ("table", get_full_table_name(table.info())));

                // nop+upd(was=Y) -> upd(was=Y), type B
                prev_state.old_values.emplace(item.first, std::move(item.second));
            }

            // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
            for (auto id : state.new_ids) {
                prev_state.new_ids.insert(id);
            }

            // *+del
            for (auto& obj : state.removed_values) {
                // new + del -> nop (type C)
                if (prev_state.new_ids.find(obj.first) != prev_state.new_ids.end()) {
                    prev_state.new_ids.erase(obj.first);
                    continue;
                }

                // upd(was=X) + del(was=Y) -> del(was=X)
                auto it = prev_state.old_values.find(obj.first);
                if (it != prev_state.old_values.end()) {
                    prev_state.removed_values.emplace(std::move(*it));
                    prev_state.old_values.erase(obj.first);
                    continue;
                }

                // del + del -> N/A
                CYBERWAY_ASSERT(prev_state.removed_values.find(obj.first) == prev_state.removed_values.end(), session_exception,
                    "UB for the table ${table}: Delete + Delete", ("table", get_full_table_name(table)));

                // nop + del(was=Y) -> del(was=Y)
                prev_state.removed_values.emplace(std::move(obj)); //[obj.second->id] = std::move(obj.second);
            }

            table.undo();
        }

        void commit(table_undo_stack& table, const int64_t revision) {
            table.commit(revision);
        }

        void undo_all(table_undo_stack& table) {
            while (!table.empty()) {
                undo(table, table.revision(), table.revision());
            }
        }

        void update(table_undo_stack& table, const primary_key_t pk, const variant& value) {
            auto& head = table.head();

            if (head.new_ids.find(pk) != head.new_ids.end()) {
                return;
            }

            auto itr = head.old_values.find(pk);
            if (itr != head.old_values.end()) {
                return;
            }

            head.old_values.emplace(pk, value);
        }

        void remove(table_undo_stack& table, const primary_key_t pk, const variant& value) {
            auto& head = table.head();

            if (head.new_ids.count(pk)) {
                head.new_ids.erase(pk);
                return;
            }

            auto itr = head.old_values.find(pk);
            if (itr != head.old_values.end()) {
                head.removed_values.emplace(std::move(*itr));
                head.old_values.erase(pk);
                return;
            }

            if (head.removed_values.count(pk)) {
                return;
            }

            head.removed_values.emplace(pk, value);
        }

        void insert(table_undo_stack& table, const primary_key_t pk, const variant& value) {
            auto& head = table.head();
            head.new_ids.insert(pk);
        }

        int64_t revision_ = 0;
        driver_interface& driver_;
        table_undo_stack_index tables_;

    }; // struct undo_stack::undo_stack_impl_

    undo_stack::undo_stack(driver_interface& driver)
    : impl_(new undo_stack_impl_(driver))
    { }

    session undo_stack::start_undo_session(bool enabled) {
        return session(*this, impl_->start_undo_session(enabled));
    }

    void undo_stack::set_revision(const int64_t value) {
        impl_->set_revision(value);
    }

    int64_t undo_stack::revision() const {
        return impl_->revision();
    }

    void undo_stack::push(const int64_t push_revision) {
        impl_->push(push_revision);
    }

    void undo_stack::undo(const int64_t undo_revision) {
        impl_->undo(undo_revision);
    }

    void undo_stack::squash(const int64_t squash_revision) {
        impl_->squash(squash_revision);
    }

    void undo_stack::commit(const int64_t commit_revision) {
        impl_->commit(commit_revision);
    }

    void undo_stack::undo_all() {
        impl_->undo_all();
    }

    void undo_stack::update(const table_info& table, const primary_key_t pk, const variant& value) {
        impl_->update(table, pk, value);
    }

    void undo_stack::remove(const table_info& table, const primary_key_t pk, const variant& value) {
        impl_->remove(table, pk, value);
    }

    void undo_stack::insert(const table_info& table, const primary_key_t pk, const variant& value) {
        impl_->insert(table, pk, value);
    }

    //------

    session::session(session&& mv)
    : stack_(mv.stack_),
      apply_(mv.apply_),
      revision_(mv.revision_) {
        mv.apply_ = false;
    }

    session::~session() {
        if (apply_) {
            stack_.undo(revision_);
        }
    }

    void session::push() {
        if (apply_) {
            apply_ = false;
            stack_.push(revision_);
        }
    }

    void session::squash() {
        if (apply_) {
            stack_.squash(revision_);
        }
        apply_ = false;
    }

    void session::undo() {
        if (apply_) {
            stack_.undo(revision_);
        }
        apply_ = false;
    }

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM(cyberway::chaindb::undo_stage, (Unknown)(New)(Stack)(Rollback))