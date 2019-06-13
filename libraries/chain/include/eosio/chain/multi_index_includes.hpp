/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <cyberway/chaindb/multi_index.hpp>
#include <cyberway/chaindb/index_object.hpp>

#define CHAINDB_TAG(_TYPE, _NAME)                   \
    namespace cyberway { namespace chaindb {        \
        template<> struct tag<_TYPE> {              \
            using type = _TYPE;                     \
            constexpr static uint64_t get_code() {  \
                return N(_NAME);                    \
            }                                       \
        };                                          \
    } }

#define CHAINDB_SET_TABLE_TYPE(_OBJECT, _TABLE)     \
   namespace cyberway { namespace chaindb {         \
       template<> struct object_to_table<_OBJECT> { \
           using type = _TABLE;                     \
       };                                           \
   } }

namespace bmi = boost::multi_index;
using bmi::indexed_by;
using bmi::ordered_unique;
using bmi::ordered_non_unique;
using bmi::composite_key;
using bmi::member;
using bmi::const_mem_fun;
using bmi::tag;
using bmi::composite_key_compare;

struct by_id;
struct by_table;

namespace eosio { namespace chain {

    struct by_parent;
    struct by_owner;
    struct by_name;
    struct by_action_name;
    struct by_permission_name;
    struct by_trx_id;
    struct by_expiration;
    struct by_delay;
    struct by_sender_id;
    struct by_scope_name;

} } // namespace eosio::chain

namespace eosio { namespace chain { namespace resource_limits {

    struct by_owner {};

} } } // namespace eosio::chain::resource_limits

CHAINDB_TAG(by_id, primary) // "primary" for compatibility with contracts
CHAINDB_TAG(by_table, table)
CHAINDB_TAG(eosio::chain::by_parent, parent)
CHAINDB_TAG(eosio::chain::by_owner, owner)
CHAINDB_TAG(eosio::chain::by_name, name)
CHAINDB_TAG(eosio::chain::by_action_name, action)
CHAINDB_TAG(eosio::chain::by_permission_name, permission)
CHAINDB_TAG(eosio::chain::by_trx_id, trxid)
CHAINDB_TAG(eosio::chain::by_expiration, expiration)
CHAINDB_TAG(eosio::chain::by_delay, delay)
CHAINDB_TAG(eosio::chain::by_sender_id, sender)
CHAINDB_TAG(eosio::chain::by_scope_name, scope)
CHAINDB_TAG(eosio::chain::resource_limits::by_owner, owner)

namespace cyberway { namespace chaindb {

    template<typename A>
    struct get_result_type {
        using type = typename A::result_type;
    }; // struct get_result_type

    template<typename Tag, typename KeyFromValue, typename CompareType = std::less<typename KeyFromValue::result_type>>
    struct ordered_unique {
        using tag_type = Tag;
        using key_from_value_type = KeyFromValue;
        using compare_type = CompareType;
        using extractor_type = KeyFromValue;
    }; // struct ordered_unique

    template<typename Value, typename... Fields>
    struct composite_key {
        using result_type = bool;

        using key_type = boost::tuple<typename get_result_type<Fields>::type...>;

        auto operator()(const Value& v) {
            return boost::make_tuple(Fields()(v)...);
            //return key_type();
        }
    }; // struct composite_key

    template<typename... Items>
    struct indexed_by {};

    template<typename Object, typename Items>
    struct table_container_impl;

    struct object_id_extractor {
        template<typename T> primary_key_t operator()(const T& o) const {
            return o.pk();
        }
    }; // struct object_id_extractor

    struct primary_index {
        using tag = tag<by_id>;
        using extractor = object_id_extractor;
    }; // struct primary_index

    template<typename Object, typename... Items>
    struct table_container_impl<Object, indexed_by<Items...>> {
        using type = multi_index<tag<Object>, primary_index, Object, Items...>;
    };

    template<typename Object, typename IndexSpec>
    using table_container = typename table_container_impl<Object, IndexSpec>::type;

} } // namespace cyberway::chaindb
