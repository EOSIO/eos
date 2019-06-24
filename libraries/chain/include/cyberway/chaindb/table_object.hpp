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
        table_info info_;

    public:
        object(const table_info& src)
        : info_(src) {
        }

        const table_info& info() const {
            return info_;
        }

        table_name_t table() const {
            return info_.table_name();
        }

        account_name_t code() const {
            return info_.code;
        }

        scope_name_t scope() const {
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
                    bmi::const_mem_fun<object, account_name_t, &object::code>,
                    bmi::const_mem_fun<object, table_name_t,   &object::table>,
                    bmi::const_mem_fun<object, scope_name_t,   &object::scope>>>>>;

    template <typename Object>
    typename index<Object>::iterator find(index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table_name(), table.scope));
    }

    template <typename Object>
    typename index<Object>::iterator find(const index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table_name(), table.scope));
    }

    template <typename Object>
    typename index<Object>::iterator find_without_scope(index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table_name(), 0));
    }

    template <typename Object>
    typename index<Object>::iterator find_without_scope(const index<Object>& idx, const table_info& table) {
        return idx.find(std::make_tuple(table.code, table.table_name(), 0));
    }

    template <typename Object, typename... Args>
    Object& emplace(index<Object>& idx, Args&&... args) {
        return const_cast<Object&>(*idx.emplace(std::forward<Args>(args)...).first);
    }

} } } // namespace cyberway::chaindb::table_object