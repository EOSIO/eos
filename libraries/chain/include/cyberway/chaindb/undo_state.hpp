#pragma once

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/object_value.hpp>

namespace cyberway { namespace chaindb {

    class cache_map;
    class driver_interface;
    class journal;
    struct table_info;

    class undo_stack final {
    public:
        undo_stack(chaindb_controller&, driver_interface&, journal&, cache_map&);

        undo_stack(const undo_stack&) = delete;
        undo_stack(undo_stack&&) = delete;

        ~undo_stack();

        void add_abi_tables(eosio::chain::abi_def&) const;

        void restore() const;

        void clear() const;

        revision_t start_undo_session(bool enabled) const;

        void set_revision(revision_t rev) const;
        revision_t revision() const {
            return revision_;
        }
        bool enabled() const;

        /**
         *  Restores the state to how it was prior to the current session discarding all changes
         *  made between the last revision and the current revision.
         */
        void undo(revision_t rev) const;

        /**
         *  This method works similar to git squash, it merges the change set from the two most
         *  recent revision numbers into one revision number (reducing the head revision number)
         *
         *  This method does not change the state of the index, only the state of the undo buffer.
         */
        void squash(revision_t rev) const;

        /**
         * Discards all undo history prior to revision
         */
        void commit(revision_t rev) const;

        /**
         * Event on create objects
         */
        void insert(const table_info&, object_value obj) const;

        /**
         * Event on modify objects
         */
        void update(const table_info&, object_value orig_obj, object_value obj) const;

        /**
         * Event on remove objects
         */
        void remove(const table_info&, object_value orig_obj) const;

    private:
        struct undo_stack_impl_;
        std::unique_ptr<undo_stack_impl_> impl_;
        revision_t revision_;
    }; // class table_undo_stack

} } // namespace cyberway::chaindb