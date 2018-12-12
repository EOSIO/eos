#include <cyberway/chaindb/undo_state.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/table_object.hpp>
#include <cyberway/chaindb/journal.hpp>

/** Session exception is a critical errors and they doesn't handle by chain */
#define CYBERWAY_SESSION_ASSERT(expr, FORMAT, ...)                      \
    FC_MULTILINE_MACRO_BEGIN                                            \
        if (!(expr)) {                                                  \
            elog(FORMAT, __VA_ARGS__);                                  \
            FC_THROW_EXCEPTION(session_exception, FORMAT, __VA_ARGS__); \
        }                                                               \
    FC_MULTILINE_MACRO_END

namespace cyberway { namespace chaindb {

    using fc::mutable_variant_object;

    namespace bmi = boost::multi_index;

    enum class undo_stage {
        Unknown,
        New,
        Stack,
    };

    enum class undo_record {
        OldValue,
        RemovedValue,
        NewValue,
        NextPk,
    };

    struct undo_state final {
        undo_state() = default;
        undo_state(revision_t rev): revision_(rev) { }

        using pk_set_t = fc::flat_set<primary_key_t>;
        using pk_value_map_t = fc::flat_map<primary_key_t, variant>;

        revision_t     revision_ = impossible_revision;
        primary_key_t  next_pk_ = unset_primary_key;
        pk_value_map_t old_values_;
        pk_value_map_t removed_values_;
        pk_set_t       new_ids_;
    }; // struct undo_state

    class table_undo_stack final: public table_object::object {
        undo_stage stage_    = undo_stage::Unknown; // <- state machine - changes via actions
        revision_t revision_ = impossible_revision; // <- part of state machine - changes via actions

        std::deque<undo_state> stack_;              // <- access depends on state machine

    public:
        table_undo_stack() = delete;

        table_undo_stack(const table_info& src, const revision_t rev)
        : object(src),
          stage_(undo_stage::New),
          revision_(rev) {
        }

        revision_t revision() const {
            return revision_;
        }

        void start_session(const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(!empty(),
                "The stack of the table ${table} is empty.", ("table", get_full_table_name()));

            CYBERWAY_SESSION_ASSERT(stack_.back().revision_ < rev,
                "Bad revision ${table_revision} (new ${revision}) for the table ${table}.",
                ("table", get_full_table_name())("table_revision", stack_.back().revision_)
                ("revision", rev));

            revision_ = rev;
            stage_ = undo_stage::New;
        }

        undo_state& head() {
            switch (stage_) {
                case undo_stage::New: {
                    stage_ = undo_stage::Stack;
                    stack_.emplace_back(revision_);
                }

                case undo_stage::Stack:
                    return stack_.back();

                case undo_stage::Unknown:
                    break;
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a head.",
                ("table", get_full_table_name())("stage", stage_));
        }

        undo_state& tail() {
            if (!stack_.empty()) {
                return stack_.front();
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a tail.",
                ("table", get_full_table_name())("stage", stage_));
        }

        undo_state& prev_state() {
            switch (stage_) {
                case undo_stage::Unknown:
                    break;

                case undo_stage::Stack:
                    CYBERWAY_SESSION_ASSERT(size() >= 2,
                        "The table ${table} doesn't have 2 states.", ("table", get_full_table_name()));
                    return stack_[stack_.size() - 2];

                case undo_stage::New:
                    CYBERWAY_SESSION_ASSERT(!empty(),
                        "The table ${table} doesn't have any state.", ("table", get_full_table_name()));
                    return stack_.back();
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on getting of a previous state.",
                ("table", get_full_table_name())("stage", stage_));
        }

        void squash() {
            switch (stage_) {
                case undo_stage::Unknown:
                    break;

                case undo_stage::Stack:
                    --stack_.back().revision_;

                case undo_stage::New:
                    --revision_;
                    return;
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on squashing of changes.",
                ("table", get_full_table_name())("stage", stage_));
        }

        void undo() {
            switch (stage_) {
                case undo_stage::Unknown:
                    break;

                case undo_stage::Stack: {
                    stack_.pop_back();
                    if (empty()) {
                        stage_    = undo_stage::Unknown;
                        revision_ = impossible_revision;
                    } else {
                        revision_ = stack_.back().revision_;
                    }
                    return;
                }

                case undo_stage::New:
                    stage_    = undo_stage::Unknown;
                    revision_ = impossible_revision;
                    return;
            }

            CYBERWAY_SESSION_ASSERT(false,
                "Wrong stage ${stage} of the table ${table} on undoing of changes.",
                ("table", get_full_table_name())("stage", stage_));
        }

        void commit() {
            if (!stack_.empty()) {
                stack_.pop_front();
            } else {
                CYBERWAY_SESSION_ASSERT(false,
                    "Wrong stage ${stage} of the table ${table} on committing of changes.",
                    ("table", get_full_table_name())("stage", stage_));
            }
        }

        size_t size() const {
            return stack_.size();
        }

        bool empty() const {
            return stack_.empty();
        }

    }; // struct table_undo_stack

    struct undo_stack::undo_stack_impl_ final {
        undo_stack_impl_(driver_interface& driver, journal& jrl, cache_map& cache)
        : driver_(driver),
          journal_(jrl),
          cache_(cache) {
        }

        void clear() {
            tables_.clear();
            revision_ = 0;
            begin_revision_ = 0;
        }

        void set_revision(const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(tables_.empty(), "Cannot set revision while there is an existing undo stack.");
            revision_ = rev;
            begin_revision_ = rev;
        }

        revision_t start_undo_session(bool enabled) {
            if (enabled) {
                ++revision_;
                for (auto& const_table: tables_) {
                    auto& table = const_cast<table_undo_stack&>(const_table); // not critical
                    table.start_session(revision_);
                }
                stage_ = undo_stage::Stack;
                return revision_;
            } else {
                return -1;
            }
        }

        bool enabled() const {
            switch (stage_) {
                case undo_stage::Stack:
                case undo_stage::New:
                    return true;

                case undo_stage::Unknown:
                    break;
            }
            return false;
        }

        void apply_changes(const revision_t apply_rev) {
            CYBERWAY_SESSION_ASSERT(apply_rev == revision_, "Wrong push revision ${apply_revision} != ${revision}",
                ("revision", revision_)("apply_revision", apply_rev));
            driver_.apply_all_changes();
        }

        void undo(const revision_t undo_rev) {
            for_tables([&](auto& table){
                undo(table, undo_rev, revision_);
            });
            --revision_;
            if (revision_ == begin_revision_) {
                stage_ = undo_stage::Unknown;
            }
        }

        void squash(const revision_t squash_rev) {
            for_tables([&](auto& table){
                squash(table, squash_rev, revision_);
            });
            --revision_;
            if (revision_ == begin_revision_) {
                stage_ = undo_stage::Unknown;
            }
        }

        void commit(const revision_t commit_rev) {
            for_tables([&](auto& table){
                commit(table, commit_rev);
            });
            begin_revision_ = commit_rev;
            if (revision_ == begin_revision_) {
                stage_ = undo_stage::Unknown;
            }
        }

        void undo_all() {
            for_tables([&](auto& table) {
                undo_all(table);
            });
            stage_ = undo_stage::Unknown;
            revision_ = begin_revision_;
        }

        void insert(const table_info& table, const primary_key_t pk, variant value) {
            if (enabled()) {
                insert(get_table(table), pk, std::move(value));
            } else {
                journal_.write(table, pk, {write_operation::Insert, revision_ /*set_rev*/, std::move(value)}, {});
            }
        }

        void update(const table_info& table, const primary_key_t pk, variant orig_value, variant value) {
            if (enabled()) {
                update(get_table(table), pk, std::move(orig_value), std::move(value));
            } else {
                journal_.write(table, pk, {write_operation::Update, revision_ /*set_rev*/, std::move(value)}, {});
            }
        }

        void remove(const table_info& table, const primary_key_t pk, variant orig_value) {
            if (enabled()) {
                remove(get_table(table), pk, std::move(orig_value));
            } else {
                journal_.write(table, pk, {write_operation::Delete}, {});
            }
        }

        void undo(table_undo_stack& table, const revision_t undo_rev, const revision_t test_rev) {
            if (undo_rev > table.revision()) return;

            const auto& head = table.head();

            CYBERWAY_SESSION_ASSERT(head.revision_ == undo_rev && undo_rev == test_rev,
                "Wrong undo revision ${undo_revision} != (${revision}, ${test_revision}) "
                "for the table ${table} for the scope '${scope}",
                ("revision", head.revision_)("undo_revision", undo_rev)("test_revision", test_rev)
                ("table", get_full_table_name(table))("scope", get_scope_name(table)));

            for (const auto& obj: head.old_values_) {
                cache_.update(table.info(), obj.first, obj.second);
                journal_.write(table.info(), obj.first,
                    {write_operation::Update, undo_rev - 1 /*set_rev*/, undo_rev /*find_rev*/, std::move(obj.second)},
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/});
            }

            for (const auto pk: head.new_ids_) {
                cache_.remove(table.info(), pk);
                journal_.write(table.info(), pk,
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/},
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/});
            }

            for (const auto& obj: head.removed_values_) {
                cache_.remove(table.info(), obj.first);
                // insert
                journal_.write(table.info(), obj.first,
                    {write_operation::Insert, undo_rev - 1 /*set_rev*/, std::move(obj.second)},
                    {write_operation::Delete, impossible_revision /*set_rev*/, undo_rev /*find_rev*/});
            }

            if (unset_primary_key != head.next_pk_) {
                cache_.set_next_pk(table.info(), head.next_pk_);
            }

            table.undo();
        }

        void remove_state(table_undo_stack& table, const undo_state& state) {
            auto update = [&](auto& pk) {
                journal_.write(table.info(), pk,
                    {write_operation::UpdateRevision, state.revision_ - 1 /*set_rev*/, state.revision_ /*find_rev*/},
                    {write_operation::Delete,         impossible_revision /*set_rev*/, state.revision_ /*find_rev*/});
            };

            for (const auto& obj: state.old_values_)     update(obj.first);
            for (const auto& pk:  state.new_ids_)        update(pk);
            for (const auto& obj: state.removed_values_) update(obj.first);

            table.undo();
        }

        void squash_state(table_undo_stack& table, const undo_state& state) {
            auto update = [&](auto& pk) {
                journal_.write(table.info(), pk,
                    {write_operation::UpdateRevision, state.revision_ - 1 /*set_rev*/, state.revision_ /*find_rev*/},
                    {write_operation::UpdateRevision, state.revision_ - 1 /*set_rev*/, state.revision_ /*find_rev*/});
            };

            for (const auto& obj: state.old_values_)     update(obj.first);
            for (const auto& pk:  state.new_ids_)        update(pk);
            for (const auto& obj: state.removed_values_) update(obj.first);

            table.squash();
        }

        void squash(table_undo_stack& table, const revision_t squash_rev, const revision_t test_rev) {
            if (squash_rev > table.revision()) return;

            auto& state = table.head();
            CYBERWAY_SESSION_ASSERT(state.revision_ == squash_rev && squash_rev == test_rev,
                "Wrong squash revision ${squash_revision} != (${revision}, ${test_revision})"
                "for the table ${table} for the scope '${scope}",
                ("revision", state.revision_)("squash_revision", squash_rev)("test_revision", test_rev)
                ("table", get_full_table_name(table))("scope", get_scope_name(table)));

            // Only one stack item
            if (table.size() == 1) {
                if (state.revision_ > begin_revision_) {
                    squash_state(table, state);
                } else {
                    remove_state(table, state);
                }
                return;
            }

            auto& prev_state = table.prev_state();

            // Two stack items but they are not neighbours
            if (prev_state.revision_ != state.revision_ + 1) {
                squash_state(table, state);
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

            for (const auto& obj: state.old_values_) {
                // new+upd -> new, type A
                if (prev_state.new_ids_.count(obj.first)) {
                    journal_.write(table.info(), obj.first,
                        {write_operation::UpdateRevision, prev_state.revision_ /*set_rev*/, state.revision_ /*find_rev*/},
                        {write_operation::Delete,         impossible_revision  /*set_rev*/, state.revision_ /*find_rev*/});
                    continue;
                }

                // upd(was=X) + upd(was=Y) -> upd(was=X), type A
                if (prev_state.old_values_.count(obj.first)) {
                    journal_.write(table.info(), obj.first,
                        {write_operation::UpdateRevision, prev_state.revision_ /*set_rev*/, state.revision_ /*find_rev*/},
                        {write_operation::Delete,         impossible_revision  /*set_rev*/, state.revision_ /*find_rev*/});
                    continue;
                }

                // del+upd -> N/A
                CYBERWAY_SESSION_ASSERT(!prev_state.removed_values_.count(obj.first),
                    "UB for the table ${table}: Delete + Update", ("table", get_full_table_name(table.info())));

                // nop+upd(was=Y) -> upd(was=Y), type B
                prev_state.old_values_.emplace(obj.first, std::move(obj.second));

                journal_.write(table.info(), obj.first,
                    {write_operation::UpdateRevision, prev_state.revision_ /*set_rev*/, state.revision_ /*find_rev*/},
                    {write_operation::UpdateRevision, prev_state.revision_ /*set_rev*/, state.revision_ /*find_rev*/});
            }

            // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
            for (const auto& pk: state.new_ids_) {
                prev_state.new_ids_.insert(pk);

                journal_.write(table.info(), pk,
                    {write_operation::UpdateRevision, prev_state.revision_ /*set_rev*/, state.revision_ /*find_rev*/},
                    {write_operation::UpdateRevision, prev_state.revision_ /*set_rev*/, state.revision_ /*find_rev*/});
            }

            if (unset_primary_key != state.next_pk_) {
                prev_state.next_pk_ = state.next_pk_;
            }

            // *+del
            for (const auto& obj: state.removed_values_) {
                // new + del -> nop (type C)
                auto nitr = prev_state.new_ids_.find(obj.first);
                if (nitr != prev_state.new_ids_.end()) {
                    prev_state.new_ids_.erase(nitr);

                    journal_.write(table.info(), obj.first, {},
                        {write_operation::Delete, impossible_revision /*set_rev*/, prev_state.revision_ /*find_rev*/});
                    continue;
                }

                // upd(was=X) + del(was=Y) -> del(was=X)
                auto itr = prev_state.old_values_.find(obj.first);
                if (itr != prev_state.old_values_.end()) {
                    prev_state.removed_values_.emplace(std::move(*itr));
                    prev_state.old_values_.erase(obj.first);

                    journal_.write(table.info(), obj.first, {},
                        {write_operation::Delete, impossible_revision /*set_rev*/, prev_state.revision_ /*find_rev*/});
                    continue;
                }

                // del + del -> N/A
                CYBERWAY_SESSION_ASSERT(!prev_state.removed_values_.count(obj.first),
                    "UB for the table ${table}: Delete + Delete", ("table", get_full_table_name(table)));

                // nop + del(was=Y) -> del(was=Y)
                prev_state.removed_values_.emplace(std::move(obj)); //[obj.second->id] = std::move(obj.second);

                journal_.write(table.info(), obj.first, {},
                    {write_operation::UpdateRevision, prev_state.revision_ /*set_rev*/, state.revision_ /*find_rev*/});
            }

            table.undo();
        }

        void commit(table_undo_stack& table, const revision_t rev) {
            CYBERWAY_SESSION_ASSERT(!table.empty(),
                "Stack of the table ${table} is empty.", ("table", table.get_full_table_name()));

            auto update = [&](auto& pk, auto& rev) {
                journal_.write(table.info(), pk, {},
                   {write_operation::Delete, impossible_revision /*set_rev*/, rev /*find_rev*/});
            };

            while (!table.empty()) {
                auto& state = table.tail();

                if (state.revision_ > rev) return;

                for (const auto& obj: state.old_values_)     update(obj.first, state.revision_);
                for (const auto& pk:  state.new_ids_)        update(pk, state.revision_);
                for (const auto& obj: state.removed_values_) update(obj.first, state.revision_);

                table.commit();
            }
        }

        void undo_all(table_undo_stack& table) {
            while (table.size() > 1) {
                squash(table, table.revision(), table.revision());
            }
            undo(table, table.revision(), table.revision());
        }

        void add_undo_fields(mutable_variant_object& obj, const table_info& table, const undo_record type) const {
            obj.erase(get_revision_field_name());

            obj(get_record_field_name(), type);
            obj(get_code_field_name(), get_code_name(table));
            obj(get_table_field_name(), get_table_name(table));
        }

        variant create_undo_value(const table_info& table, const undo_record type, const variant& value) const {
            mutable_variant_object obj(value.get_object());
            add_undo_fields(obj, table, type);
            return variant(std::move(obj));
        }

        void insert(table_undo_stack& table, const primary_key_t pk, variant value) {
            auto& head = table.head();
            head.new_ids_.insert(pk);

            {
                mutable_variant_object obj(value.get_object());
                add_undo_fields(obj, table.info(), undo_record::NewValue);
                journal_.write(table.info(), pk, {write_operation::Insert, revision_ /*set_rev*/, std::move(value)},
                    {write_operation::Insert, revision_ /*set_rev*/, std::move(obj)});
            }

            if (unset_primary_key == head.next_pk_) {
                head.next_pk_ = pk - 1;

                mutable_variant_object obj;
                add_undo_fields(obj, table.info(), undo_record::NextPk);
                obj(get_pk_field_name(), pk);

                journal_.write(table.info(), pk, {}, {write_operation::Insert, revision_ /*set_rev*/, std::move(obj)});
            }
        }

        void update(table_undo_stack& table, const primary_key_t pk, variant orig_value, variant value) {
            auto& head = table.head();

            if (head.new_ids_.find(pk) != head.new_ids_.end()) {
                journal_.write(table.info(), pk, {write_operation::Update, revision_ /*set_rev*/, std::move(value)}, {});
                return;
            }

            auto itr = head.old_values_.find(pk);
            if (itr != head.old_values_.end()) {
                journal_.write(table.info(), pk, {write_operation::Update, revision_ /*set_rev*/, std::move(value)}, {});
                return;
            }

            auto undo_value = create_undo_value(table.info(), undo_record::OldValue, orig_value);

            journal_.write(table.info(), pk, {write_operation::Update, revision_ /*set_rev*/, value},
                {write_operation::Insert, revision_ /*set_rev*/, std::move(undo_value)});

            head.old_values_.emplace(pk, std::move(value));
        }

        void remove(table_undo_stack& table, const primary_key_t pk, variant orig_value) {
            auto& head = table.head();

            if (head.new_ids_.count(pk)) {
                head.new_ids_.erase(pk);

                journal_.write(table.info(), pk, {write_operation::Delete, revision_ /*set_rev*/},
                    {write_operation::Delete, revision_ /*set_rev*/});
                return;
            }

            auto undo_value = create_undo_value(table.info(), undo_record::RemovedValue, orig_value);

            journal_.write(table.info(), pk, {write_operation::Delete, revision_ /*set_rev*/},
                {write_operation::Insert, revision_ /*set_rev*/, std::move(undo_value)});

            auto itr = head.old_values_.find(pk);
            if (itr != head.old_values_.end()) {
                head.removed_values_.emplace(std::move(*itr));
                head.old_values_.erase(pk);
                return;
            }

            head.removed_values_.emplace(pk, std::move(orig_value));
        }

        table_undo_stack& get_table(const table_info& table) {
            auto itr = table_object::find(tables_, table);
            if (tables_.end() != itr) return const_cast<table_undo_stack&>(*itr); // not critical

            return table_object::emplace(tables_, table, revision_);
        }

        template <typename Lambda>
        void for_tables(Lambda&& lambda) { try {
            for (auto itr = tables_.begin(), etr = tables_.end(); etr != itr; ) {
                auto& table = const_cast<table_undo_stack&>(*itr);
                lambda(table);

                if (table.empty()) {
                    tables_.erase(itr++);
                } else {
                    ++itr;
                }
            }
        } FC_LOG_AND_RETHROW() }

        using index_t_ = table_object::index<table_undo_stack>;

        undo_stage stage_ = undo_stage::Unknown;
        revision_t revision_ = 0;
        revision_t begin_revision_ = 0;
        driver_interface& driver_;
        journal& journal_;
        cache_map& cache_;
        index_t_ tables_;
    }; // struct undo_stack::undo_stack_impl_

    undo_stack::undo_stack(driver_interface& driver, journal& j, cache_map& cache)
    : impl_(new undo_stack_impl_(driver, j, cache)) {
    }

    undo_stack::~undo_stack() = default;

    void undo_stack::clear() {
        impl_->clear();
    }

    chaindb_session undo_stack::start_undo_session(bool enabled) {
        return chaindb_session(*this, impl_->start_undo_session(enabled));
    }

    void undo_stack::set_revision(const revision_t rev) {
        impl_->set_revision(rev);
    }

    revision_t undo_stack::revision() const {
        return impl_->revision_;
    }

    bool undo_stack::enabled() const {
        return impl_->enabled();
    }

    void undo_stack::apply_changes(const revision_t rev) {
        impl_->apply_changes(rev);
    }

    void undo_stack::undo(const revision_t undo_rev) {
        impl_->undo(undo_rev);
    }

    void undo_stack::squash(const revision_t squash_rev) {
        impl_->squash(squash_rev);
    }

    void undo_stack::commit(const revision_t commit_rev) {
        impl_->commit(commit_rev);
    }

    void undo_stack::undo_all() {
        impl_->undo_all();
    }

    void undo_stack::undo() {
        impl_->undo(impl_->revision_);
    }

    void undo_stack::insert(const table_info& table, const primary_key_t pk, variant value) {
        impl_->insert(table, pk, std::move(value));
    }

    void undo_stack::update(const table_info& table, const primary_key_t pk, variant orig_value, variant value) {
        impl_->update(table, pk, std::move(orig_value), std::move(value));
    }

    void undo_stack::remove(const table_info& table, const primary_key_t pk, variant orig_value) {
        impl_->remove(table, pk, std::move(orig_value));
    }

    //------

    chaindb_session::chaindb_session(undo_stack& stack, revision_t rev)
    : stack_(stack),
      apply_(true),
      revision_(rev) {
        if (rev == -1) {
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
            stack_.apply_changes(revision_);
        }
        apply_ = false;
    }

    void chaindb_session::push() {
        apply_ = false;
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

FC_REFLECT_ENUM(cyberway::chaindb::undo_stage, (Unknown)(New)(Stack))
FC_REFLECT_ENUM(cyberway::chaindb::undo_record, (OldValue)(RemovedValue)(NewValue)(NextPk))