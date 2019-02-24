#pragma once

#include <cyberway/chaindb/common.hpp>

namespace cyberway { namespace chaindb {

    enum class undo_record {
        Unknown,
        OldValue,
        RemovedValue,
        NewValue,
        NextPk,
    }; // enum class undo_type

    struct service_state final {
        primary_key_t pk       = unset_primary_key;
        account_name  payer;
        size_t        size     = 0;

        account_name  code;
        account_name  scope;
        table_name    table;

        primary_key_t undo_pk  = unset_primary_key;
        undo_record   undo_rec = undo_record::Unknown;
        revision_t    revision = impossible_revision;

        service_state(const table_info& table, primary_key_t pk)
        : pk(pk), code(table.code), scope(table.scope), table(table.table->name) {
        }

        service_state(const table_info& table, primary_key_t undo_pk, undo_record rec, revision_t rev)
        : code(table.code), scope(table.scope), table(table.table->name),
          undo_pk(undo_pk), undo_rec(rec), revision(rev) {
        }

        service_state() = default;
        service_state(service_state&&) = default;
        service_state(const service_state&) = default;

        service_state& operator=(service_state&&) = default;
        service_state& operator=(const service_state&) = default;

        void clear() {
            *this = {};
        }
    }; // struct service_state

    struct object_value final {
        service_state service;
        fc::variant   value;

        bool is_null() const {
            return value.get_type() == fc::variant::type_id::null_type;
        }

        object_value& set_revision(const revision_t rev) {
            service.revision = rev;
            return *this;
        }

        object_value& set_undo_pk(const primary_key_t undo_pk) {
            service.undo_pk = undo_pk;
            return *this;
        }

        object_value& set_undo_rec(const undo_record undo_rec) {
            service.undo_rec = undo_rec;
            return *this;
        }

        object_value clone_service() const {
            object_value obj;
            obj.service = service;
            return obj;
        }

        void clear() {
            service.clear();
            value.clear();
        }

        primary_key_t pk() const {
            return service.pk;
        }

        primary_key_t undo_pk() const {
            return service.undo_pk;
        }

        revision_t revision() const {
            return service.revision;
        }
    }; // struct object_value

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM(cyberway::chaindb::undo_record, (Unknown)(OldValue)(RemovedValue)(NewValue)(NextPk))