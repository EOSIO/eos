#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/indexed_by.hpp>

#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb { namespace table_object {

    class object {
        const table_def table_def_;  // <- copy to replace pointer in info_
        const order_def pk_order_;   // <- copy to replace pointer in info_
        table_info      info_;       // <- for const reference

    public:
        object(const table_info& src)
        : table_def_(*src.table),    // <- one copy per X - it is not very critical
          pk_order_(*src.pk_order),  // <- one copy per X - it is not very critical
          info_(src) {
            info_.table = &table_def_;
            info_.pk_order = &pk_order_;
        }

        const table_info& info() const {
            return info_;
        }

        const table_name& table() const {
            return table_def_.name;
        }

        const account_name& code() const {
            return info_.code;
        }

        const account_name& scope() const {
            return info_.scope;
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
                    bmi::const_mem_fun<object, const account_name&, &object::code>,
                    bmi::const_mem_fun<object, const table_name&,   &object::table>,
                    bmi::const_mem_fun<object, const account_name&, &object::scope>>>>>;

    template <typename Object>
    typename index<Object>::iterator find(index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table->name, table.scope));
    }

    template <typename Object>
    typename index<Object>::iterator find(const index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table->name, table.scope));
    }

    template <typename Object>
    typename index<Object>::iterator find_without_scope(index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table->name, account_name()));
    }

    template <typename Object>
    typename index<Object>::iterator find_without_scope(const index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table->name, account_name()));
    }

    template <typename Object, typename... Args>
    Object& emplace(index<Object>& idx, Args&&... args) {
        return const_cast<Object&>(*idx.emplace(std::forward<Args>(args)...).first);
    }

} } } // namespace cyberway::chaindb::table_object