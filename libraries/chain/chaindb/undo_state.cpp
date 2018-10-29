#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/indexed_by.hpp>

#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/cache_map.hpp>

/** Session exception is a critical errors and they doesn't handle by chain */
#define CYBERWAY_SESSION_ASSERT(expr, FORMAT, ...)                      \
    FC_MULTILINE_MACRO_BEGIN                                            \
        if (!(expr)) {                                                  \
            elog(FORMAT, __VA_ARGS__);                                  \
            FC_THROW_EXCEPTION(session_exception, FORMAT, __VA_ARGS__); \
        }                                                               \
    FC_MULTILINE_MACRO_END

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
        const order_def pk_order_;   // <- copy to replace pointer in info_
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

        table_undo_stack() = delete;

        table_undo_stack(const table_info& src, const int64_t revision)
        : table_def_(*src.table),   // <- one copy per serie of sessions - it is not very critical
          pk_order_(*src.pk_order), // <- one copy per serie of sessions - it is not very critical
          info_(src),
          stage_(undo_stage::New),
          revision_(revision),
          code(src.code),
          scope(src.scope),
          table(src.table->name),
          pk_field(src.pk_order->field) { // <- one copy per serie of sessions - it is not very critical
            info_.table = &table_def_;
            info_.pk_order = &pk_order_;

        }

        const table_info& info() const {
            return info_;
        }

        int64_t revision() const {
            return revision_;
        }

        void start_session(const int64_t revision) {
            CYBERWAY_SESSION_ASSERT(!empty(),
                "The stack of the table ${table} is empty.", ("table", get_full_table_name(info_)));

            CYBERWAY_SESSION_ASSERT(stack_.back().revision < revision,
                "Bad revision ${table_revision} (new ${revision}) for the table ${table}.",
                ("table", get_full_table_name(info_))("table_revision", stack_.back().revision)
                ("revision", revision));

            revision_ = revision;
            stage_ = undo_stage::New;
        }

        undo_state& head() {
            switch (stage_) {
                case undo_stage::New: {
                    stage_ = undo_stage::Stack;
                    stack_.emplace_back(revision_);
                }

                case undo_stage::Unknown:
                case undo_stage::Rollback:
                    if (stack_.empty()) {
                        break;
                    }

                case undo_stage::Stack:
                    return stack_.back();
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a head.",
                ("table", get_full_table_name(info_))("stage", stage_));
        }

        undo_state& prev_state() {
            switch (stage_) {
                case undo_stage::Unknown:
                case undo_stage::Rollback:
                case undo_stage::Stack:
                    CYBERWAY_SESSION_ASSERT(size() >= 2,
                        "The table ${table} doesn't have 2 states.", ("table", get_full_table_name(info_)));
                    return stack_[stack_.size() - 2];

                case undo_stage::New:
                    CYBERWAY_SESSION_ASSERT(!empty(),
                        "The table ${table} doesn't have any state.", ("table", get_full_table_name(info_)));
                    return stack_.back();
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a previous state.",
                ("table", get_full_table_name(info_))("stage", stage_));
        }

        void commit(const int64_t revision) {
            CYBERWAY_SESSION_ASSERT(!empty(),
                "Stack of the table ${table} is empty.", ("table", get_full_table_name(info_)));

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
                    if (stack_.empty()) {
                        break;
                    }

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

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on undoing of changes.",
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
        undo_stack_impl_(driver_interface& driver, cache_map& cache)
        : driver(driver),
          cache(cache)
        { }

        void set_revision(const int64_t value) {
            CYBERWAY_SESSION_ASSERT(!tables.empty(), "Cannot set revision while there is an existing undo stack.");
            revision = value;
        }

        int64_t start_undo_session(bool enabled) {
            if (enabled) {
                ++revision;
                for (auto& const_table: tables) {
                    auto& table = const_cast<table_undo_stack&>(const_table); // not critical
                    table.start_session(revision);
                }
                stage = undo_stage::New;
                return revision;
            } else {
                return -1;
            }
        }

        bool enabled() const {
            switch (stage) {
                case undo_stage::Stack:
                case undo_stage::New:
                    return true;

                case undo_stage::Unknown:
                case undo_stage::Rollback:
                    break;
            }
            return false;
        }

        void apply_changes(const int64_t apply_revision) {
            CYBERWAY_SESSION_ASSERT(apply_revision == revision,
                "Wrong push revision ${apply_revision} != ${revision}",
                ("revision", revision)("apply_revision", apply_revision));
            driver.apply_changes();
        }

        void push(const int64_t push_revision) {
            CYBERWAY_SESSION_ASSERT(push_revision == revision,
                "Wrong push revision ${push_revision} != ${revision}",
                ("revision", revision)("push_revision", push_revision));
            driver.apply_changes();
            stage = undo_stage::Unknown;
        }

        void undo(const int64_t undo_revision) {
            for_tables([&](auto& table){
                undo(table, undo_revision, revision);
            });
            --revision;
            stage = undo_stage::Rollback;
        }

        void squash(const int64_t squash_revision) {
            for_tables([&](auto& table){
                squash(table, squash_revision, revision);
            });
            --revision;
            stage = undo_stage::Unknown;
        }

        void commit(const int64_t commit_revision) {
            for_tables([&](auto& table){
                commit(table, commit_revision);
            });
            if (tables.empty()) {
                stage = undo_stage::Unknown;
            }
        }

        void undo_all() {
            for_tables([&](auto& table) {
                undo_all(table);
            });
            stage = undo_stage::Rollback;
        }

        void update(const table_info& table, const primary_key_t pk, variant value) {
            CYBERWAY_SESSION_ASSERT(enabled(), "Wrong stage ${stage} on updating of the table ${table}.",
                ("stage", stage)("table", get_full_table_name(table)));
            update(get_table(table), pk, std::move(value));
        }

        void remove(const table_info& table, const primary_key_t pk, variant value) {
            CYBERWAY_SESSION_ASSERT(enabled(), "Wrong stage ${stage} on removing from the table ${table}.",
                ("stage", stage)("table", get_full_table_name(table)));
            remove(get_table(table), pk, std::move(value));
        }

        void insert(const table_info& table, const primary_key_t pk) {
            CYBERWAY_SESSION_ASSERT(enabled(), "Wrong stage ${stage} on inserting into the table ${table}.",
                ("stage", stage)("table", get_full_table_name(table)));
            insert(get_table(table), pk);
        }

        void undo(table_undo_stack& table, const int64_t undo_revision, const int64_t test_revision) {
            if (undo_revision > table.revision()) return;

            const auto& head = table.head();

            CYBERWAY_SESSION_ASSERT(head.revision == undo_revision && undo_revision == test_revision,
                "Wrong undo revision ${undo_revision} != (${revision}, ${test_revision})",
                ("revision", head.revision)("undo_revision", undo_revision)("test_revision", test_revision));

            for (auto& item: head.old_values) try {
                cache.update(table.info(), item.first, item.second);
                driver.update(table.info(), item.first, std::move(item.second));
            } catch (const driver_update_exception&) {
                CYBERWAY_SESSION_ASSERT(false,
                    "Could not modify object in the table ${table}", ("table", get_full_table_name(table)));
            }

            for (auto pk: head.new_ids) try {
                cache.remove(table.info(), pk);
                driver.remove(table.info(), pk);
            } catch (const driver_delete_exception&) {
                CYBERWAY_SESSION_ASSERT(false,
                    "Could not remove object from the table ${table}", ("table", get_full_table_name(table)));
            }

            for (auto& item: head.removed_values) try {
                cache.remove(table.info(), item.first);
                driver.insert(table.info(), item.first, std::move(item.second));
            } catch (const driver_insert_exception&) {
                CYBERWAY_SESSION_ASSERT(false,
                    "Could not insert object into the table ${table}", ("table", get_full_table_name(table)));
            }

            table.undo();
        }

        void squash(table_undo_stack& table, const int64_t squash_revision, const int64_t test_revision) {
            if (squash_revision > table.revision()) return;

            auto& state = table.head();
            CYBERWAY_SESSION_ASSERT(state.revision == squash_revision && squash_revision == test_revision,
                "Wrong squash revision ${squash_revision} != (${revision}, ${test_revision})",
                ("revision", state.revision)("squash_revision", squash_revision)("test_revision", test_revision));

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
                CYBERWAY_SESSION_ASSERT(prev_state.removed_values.find(item.first) == prev_state.removed_values.end(),
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
                CYBERWAY_SESSION_ASSERT(prev_state.removed_values.find(obj.first) == prev_state.removed_values.end(),
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

        void update(table_undo_stack& table, const primary_key_t pk, variant value) {
            auto& head = table.head();

            if (head.new_ids.find(pk) != head.new_ids.end()) {
                return;
            }

            auto itr = head.old_values.find(pk);
            if (itr != head.old_values.end()) {
                return;
            }

            head.old_values.emplace(pk, std::move(value));
        }

        void remove(table_undo_stack& table, const primary_key_t pk, variant value) {
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

            head.removed_values.emplace(pk, std::move(value));
        }

        void insert(table_undo_stack& table, const primary_key_t pk) {
            auto& head = table.head();
            head.new_ids.insert(pk);
        }

        table_undo_stack& get_table(const table_info& table) {
            auto itr = tables.find(std::make_tuple(table.code, table.scope, table.table->name, table.pk_order->field));
            if (tables.end() != itr) return const_cast<table_undo_stack&>(*itr); // not critical

            auto result = tables.emplace(table, revision);
            CYBERWAY_SESSION_ASSERT(result.second,
                "Fail to create stack for the table ${table} with the primary key ${pk}",
                ("table", get_full_table_name(table))("pk", table.pk_order->field));
            return const_cast<table_undo_stack&>(*result.first);
        }

        template <typename Lambda>
        void for_tables(Lambda&& lambda) {
            for (auto itr = tables.begin(), etr = tables.end(); etr != itr; ) {
                auto& table = const_cast<table_undo_stack&>(*itr);
                lambda(table);

                if (table.empty()) {
                    tables.erase(itr++);
                } else {
                    ++itr;
                }
            }
        }

        undo_stage stage = undo_stage::Unknown;
        int64_t revision = 0;
        driver_interface& driver;
        cache_map& cache;
        table_undo_stack_index tables;
    }; // struct undo_stack::undo_stack_impl_

    undo_stack::undo_stack(driver_interface& driver, cache_map& cache)
    : impl_(new undo_stack_impl_(driver, cache))
    { }

    undo_stack::~undo_stack() = default;

    chaindb_session undo_stack::start_undo_session(bool enabled) {
        return chaindb_session(*this, impl_->start_undo_session(enabled));
    }

    void undo_stack::set_revision(const int64_t value) {
        impl_->set_revision(value);
    }

    int64_t undo_stack::revision() const {
        return impl_->revision;
    }

    bool undo_stack::enabled() const {
        return impl_->enabled();
    }

    void undo_stack::apply_changes(const int64_t revision) {
        impl_->apply_changes(revision);
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

    void undo_stack::undo() {
        impl_->undo(impl_->revision);
    }

    void undo_stack::update(const table_info& table, const primary_key_t pk, variant value) {
        impl_->update(table, pk, std::move(value));
    }

    void undo_stack::remove(const table_info& table, const primary_key_t pk, variant value) {
        impl_->remove(table, pk, std::move(value));
    }

    void undo_stack::insert(const table_info& table, const primary_key_t pk) {
        impl_->insert(table, pk);
    }

    //------

    chaindb_session::chaindb_session(undo_stack& stack, int64_t revision)
    : stack_(stack),
      apply_(true),
      revision_(revision) {
        if (revision == -1) {
            apply_ = false;
        }
    }

    chaindb_session::chaindb_session(chaindb_session&& mv)
    : stack_(mv.stack_),
      apply_(mv.apply_),
      revision_(mv.revision_) {
        mv.apply_ = false;
    }

    chaindb_session::~chaindb_session() {
        if (apply_) {
            stack_.undo(revision_);
        }
    }

    void chaindb_session::apply_changes() {
        if (apply_) {
            apply_ = false;
            stack_.apply_changes(revision_);
        }
    }

    void chaindb_session::push() {
        if (apply_) {
            apply_ = false;
            stack_.push(revision_);
        }
    }

    void chaindb_session::squash() {
        if (apply_) {
            stack_.squash(revision_);
        }
        apply_ = false;
    }

    void chaindb_session::undo() {
        if (apply_) {
            stack_.undo(revision_);
        }
        apply_ = false;
    }

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM(cyberway::chaindb::undo_stage, (Unknown)(New)(Stack)(Rollback))