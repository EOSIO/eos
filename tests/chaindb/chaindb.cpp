#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>

#include <bson.h>

#include <eosio/chain/abi_serializer.hpp>

#include "chaindb.h"

using eosio::chain::abi_def;
using eosio::chain::table_def;
using eosio::chain::abi_serializer;
using eosio::chain::name;

using fc::variant;
using fc::variants;
using fc::variant_object;


using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::sub_document;
using bsoncxx::builder::basic::sub_array;
using bsoncxx::builder::basic::kvp;

using bsoncxx::types::b_oid;
using bsoncxx::types::b_bool;
using bsoncxx::types::b_double;
using bsoncxx::types::b_int64;
using bsoncxx::types::b_binary;
using bsoncxx::types::b_array;

using bsoncxx::binary_sub_type;
using bsoncxx::oid;

using mongocxx::model::insert_one;
using mongocxx::model::update_one;

using std::string;

struct abi_info {
    abi_def def;
    abi_serializer serializer;
};

struct abi_table_info {
    const table_def* table;
    const abi_serializer* serializer;
};

struct mongodb_ctx {
    mongocxx::instance mongo_inst;
    mongocxx::client mongo_conn;
    fc::microseconds max_time{15*1000*1000};

    std::unordered_map<account_name_t /* code */, abi_info> abi_map;

    abi_table_info find_table(const account_name_t code, const string& table) const {
        auto itr = abi_map.find(code);
        if (abi_map.end() == itr) return {nullptr, nullptr};

        for (auto titr = itr->second.def.tables.begin(); itr->second.def.tables.end() != titr; ++titr) {
            if (titr->name == table) return {&(*titr), &itr->second.serializer};
        }
        return {nullptr, nullptr};
    }
};

static mongodb_ctx ctx;

oid bsoncxx_oid(const account_name_t scope, const uint64_t pk) {
    constexpr static unsigned int oid_size = 12;
    uint8_t digest[16];
    bson_md5_t md5;
    char digest_str[33];
    unsigned int i;
    uint8_t input_data[sizeof(scope) + sizeof(pk)];

    memcpy((void*)input_data, (void*)&scope, sizeof(scope));
    memcpy((void*)&input_data[sizeof(scope)], (void*)&pk, sizeof(pk));
    bson_md5_init(&md5);
    bson_md5_append(&md5, input_data, sizeof(input_data));
    bson_md5_finish(&md5, digest);

    for (i = 0; i < oid_size; i++) {
        bson_snprintf(&digest_str[i * 2], 3, "%02x", digest[i]);
    }
    digest_str[oid_size * 2] = '\0';

    return oid(string(digest_str));
}

void build_document(sub_document&, const variant_object&);

void build_array(sub_array& array, const variants& obj) {
    for (auto itr = obj.begin(); obj.end() != itr; ++itr) {
        switch (itr->get_type()) {
            case variant::null_type:
                break;
            case variant::int64_type:
                array.append(b_int64{itr->as_int64()});
                break;
            case variant::uint64_type:
                array.append(b_int64{int64_t(itr->as_uint64())});
                break;
            case variant::double_type:
                array.append(b_double{itr->as_double()});
                break;
            case variant::bool_type:
                array.append(b_bool{itr->as_bool()});
                break;
            case variant::string_type:
                array.append(itr->as_string());
                break;
            case variant::array_type:
                array.append([&](sub_array array) { build_array(array, itr->get_array()); });
                break;
            case variant::object_type:
                array.append([&](sub_document sub_doc) { build_document(sub_doc, itr->get_object()); });
                break;
            case variant::blob_type: {
                    auto blob = itr->as_blob();
                    auto size = uint32_t(blob.data.size());
                    auto data = (uint8_t*) blob.data.data();
                    array.append(b_binary{binary_sub_type::k_binary, size, data});
                }
                break;
        }
    }
}

void build_document(sub_document& doc, const string& name, const variant& obj) {
    switch (obj.get_type()) {
        case variant::null_type:
            break;
        case variant::int64_type:
            doc.append(kvp(name, b_int64{obj.as_int64()}));
            break;
        case variant::uint64_type:
            doc.append(kvp(name, b_int64{int64_t(obj.as_uint64())}));
            break;
        case variant::double_type:
            doc.append(kvp(name, b_double{obj.as_double()}));
            break;
        case variant::bool_type:
            doc.append(kvp(name, b_bool{obj.as_bool()}));
            break;
        case variant::string_type:
            doc.append(kvp(name, obj.as_string()));
            break;
        case variant::array_type:
            doc.append(kvp(name, [&](sub_array array){ build_array(array, obj.get_array()); }));
            break;
        case variant::object_type: {
                doc.append(kvp(name, [&](sub_document sub_doc){ build_document(sub_doc, obj.get_object()); }));
            }
            break;
        case variant::blob_type: {
                auto blob = obj.as_blob();
                auto size = uint32_t(blob.data.size());
                auto data = (uint8_t*)blob.data.data();
                doc.append(kvp(name, b_binary{binary_sub_type::k_binary, size, data}));
            }
            break;
    }
}

void build_document(sub_document& doc, const variant_object& obj) {
    for (auto itr = obj.begin(); obj.end() != itr; ++itr) {
        build_document(doc, itr->key(), itr->value());
    }
}

int32_t chaindb_init(const char* uri_str) {
    try {
        mongocxx::uri uri{uri_str};
        ctx.mongo_conn = mongocxx::client{uri};
        ctx.abi_map.clear();
    } catch (mongocxx::exception & ex) {
        return 0;
    }
    return 1;
}

bool chaindb_set_abi(account_name_t code, abi_def abi) {
    abi_info info;

    info.def = std::move(abi);
    info.serializer.set_abi(info.def, ctx.max_time);
    ctx.abi_map[code] = std::move(info);
    return true;
}

cursor_t chaindb_lower_bound(account_name_t code, account_name_t scope, table_name_t, index_name_t, void* key, size_t) {
    return invalid_cursor;
}

cursor_t chaindb_upper_bound(account_name_t code, account_name_t scope, table_name_t, index_name_t, void* key, size_t) {
    return invalid_cursor;
}

cursor_t chaindb_find(account_name_t code, account_name_t scope, table_name_t, index_name_t, primary_key_t, void* key, size_t) {
    return invalid_cursor;
}

cursor_t chaindb_end(account_name_t code, account_name_t scope, table_name_t, index_name_t) {
    return invalid_cursor;
}

void chaindb_close(cursor_t) {

}

primary_key_t chaindb_current(cursor_t) {
    return unset_primary_key;
}

primary_key_t chaindb_next(cursor_t) {
    return unset_primary_key;
}

primary_key_t chaindb_prev(cursor_t) {
    return unset_primary_key;
}

int32_t chaindb_datasize(cursor_t) {
    return 0;
}

primary_key_t chaindb_data(cursor_t, void* data, const size_t size) {
    return unset_primary_key;
}

primary_key_t chaindb_available_primary_key(account_name_t code, account_name_t scope, table_name_t table) {
    return 1;
}

cursor_t chaindb_insert(
    account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk, void* data, size_t size
) {
    auto table_name = name(table).to_string();
    auto info = ctx.find_table(code, table_name);
    if (!info.table) return invalid_cursor;

    fc::datastream<const char*> ds(static_cast<const char*>(data), size);
    auto obj = info.serializer->binary_to_variant(info.table->type, ds, ctx.max_time);
    if (!obj.is_object()) return invalid_cursor;

    auto db_name = name(code).to_string();
    auto db = ctx.mongo_conn[db_name];
    document doc;

//    ilog(
//        "chaindb_insert to ${db}.${table}: ${object}",
//        ("db", db_name)("table", table_name)(object", obj));

    doc.append(kvp("scope", name(scope).to_string()));
    build_document(doc, obj.get_object());

    db[table_name].insert_one(doc.view());

    return invalid_cursor;
}

primary_key_t chaindb_update(account_name_t code, account_name_t scope, table_name_t, primary_key_t, void* data, size_t) {
    return unset_primary_key;
}

primary_key_t chaindb_delete(account_name_t code, account_name_t scope, table_name_t, primary_key_t) {
    return unset_primary_key;
}