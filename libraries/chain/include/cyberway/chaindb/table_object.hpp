#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/indexed_by.hpp>

#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb { namespace table_object {

    class object {
        const table_def table_def_;  // <- copy to replace pointer in info_
        const order_def pk_order_;   // <- copy to replace pointer in info_
        table_info      info_;       // <- for const reference

    public:
        const account_name code;
        const table_name   table;
        const account_name scope;
        const field_name   pk_field; // <- for case when contract change its primary key

        object(const table_info& src)
        : table_def_(*src.table),    // <- one copy per X - it is not very critical
          pk_order_(*src.pk_order),  // <- one copy per X - it is not very critical
          info_(src),
          code(src.code),
          table(src.table->name),
          scope(src.scope),
          pk_field(src.pk_order->field) {
            info_.table = &table_def_;
            info_.pk_order = &pk_order_;
        }

        const table_info& info() const {
            return info_;
        }

        string get_full_table_name() const {
            return cyberway::chaindb::get_full_table_name(info_);
        }

    }; // struct object

    namespace bmi = boost::multi_index;

    template <typename Object>
    using index = bmi::multi_index_container<
        Object,
        bmi::indexed_by<
            bmi::ordered_unique<
                bmi::composite_key<
                    Object,
                    bmi::member<object, const account_name, &object::code>,
                    bmi::member<object, const table_name,   &object::table>,
                    bmi::member<object, const account_name, &object::scope>,
                    bmi::member<object, const field_name,   &object::pk_field>>>>>;

    template <typename Object>
    typename index<Object>::iterator find(index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table->name, table.scope, table.pk_order->field));
    }

    template <typename Object>
    typename index<Object>::iterator find(const index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table->name, table.scope, table.pk_order->field));
    }

    template <typename Object, typename... Args>
    Object& emplace(index<Object>& idx, Args&&... args) {
        return const_cast<Object&>(*idx.emplace(std::forward<Args>(args)...).first);
    }

} } } // namespace cyberway::chaindb::table_object