/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <zmq.hpp>
#include <eosio/chain/trace.hpp>
#include <boost/algorithm/string.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio
{

using namespace chain;
using namespace chainbase;
using boost::signals2::connection;

typedef typename get_index_type<account_object>::type co_index_type;
typedef typename get_index_type<permission_object>::type po_index_type;
typedef typename get_index_type<permission_link_object>::type plo_index_type;
typedef typename get_index_type<table_id_object>::type ti_index_type;
typedef typename get_index_type<key_value_object>::type kv_index_type;

typedef account_object::id_type co_id_type;
typedef permission_usage_object::id_type puo_id_type;
typedef permission_object::id_type po_id_type;
typedef permission_link_object::id_type plo_id_type;
typedef key_value_object::id_type kvo_id_type;

enum op_type_enum
{
    TABLE_REMOVE = 0,
    ROW_CREATE = 1,
    ROW_MODIFY = 2,
    ROW_REMOVE = 3,
    TRX_ACTION = 4,
    REV_UNDO = 5,
    REV_COMMIT = 6
};

struct db_account
{
    account_name name;
    uint8_t vm_type = 0;
    uint8_t vm_version = 0;
    bool privileged = false;

    time_point last_code_update;
    digest_type code_version;
    block_timestamp_type creation_date;
};

struct db_table
{
    op_type_enum op_type;
    account_name code;
    fc::string scope;
    table_name table;
    account_name payer;
};

struct db_op : db_table
{
    uint64_t id;
    fc::variant value;
};

struct db_rev
{
    op_type_enum op_type;
    int64_t revision;
};

struct db_action
{
    uint64_t global_action_seq;
    block_num_type block_num;
    block_timestamp_type block_time;
    fc::variant action_trace;
    uint32_t last_irreversible_block;
};

struct filter_entry
{
    name code;
    name scope;
    name table;

    std::tuple<name, name, name> key() const
    {
        return std::make_tuple(code, scope, table);
    }

    friend bool operator<(const filter_entry &a, const filter_entry &b)
    {
        return a.key() < b.key();
    }
};

class statetrack_plugin_impl
{
  public:
    statetrack_plugin_impl()
    {
        blacklist_actions.emplace(std::make_pair(config::system_account_name,
                                                 std::set<name>{N(onblock)}));
        blacklist_actions.emplace(std::make_pair(N(blocktwitter),
                                                 std::set<name>{N(tweet)}));
    }

    void send_zmq_msg(const fc::string content);
    bool filter(const account_name &code, const fc::string &scope, const table_name &table);

    const table_id_object *get_kvo_tio(const database &db, const key_value_object &kvo);
    const account_object &get_tio_co(const database &db, const table_id_object &tio);
    const abi_def &get_co_abi(const account_object &co);
    fc::variant get_kvo_row(const database &db, const table_id_object &tio, const key_value_object &kvo, bool json = true);

    db_op get_db_op(const database &db, const account_object &co, op_type_enum op_type);
    db_op get_db_op(const database &db, const permission_object &po, op_type_enum op_type);
    db_op get_db_op(const database &db, const permission_link_object &plo, op_type_enum op_type);
    db_op get_db_op(const database &db, const table_id_object &tio, const key_value_object &kvo, op_type_enum op_type, bool json = true);

    db_rev get_db_rev(const int64_t revision, op_type_enum op_type);

    //generic_index state tables
    void on_applied_table(const database &db, const table_id_object &tio, op_type_enum op_type);
    //generic_index op
    void on_applied_op(const database &db, const account_object &co, op_type_enum op_type);
    void on_applied_op(const database &db, const permission_object &po, op_type_enum op_type);
    void on_applied_op(const database &db, const permission_link_object &plo, op_type_enum op_type);
    void on_applied_op(const database &db, const key_value_object &kvo, op_type_enum op_type);
    //generic_index undo
    void on_applied_rev(const int64_t revision, op_type_enum op_type);
    //blocks and transactions
    void on_applied_transaction(const transaction_trace_ptr &ttp);
    void on_accepted_block(const block_state_ptr &bsp);
    void on_action_trace(const action_trace &at, const block_state_ptr &bsp);
    void on_irreversible_block(const block_state_ptr &bsp);

    static void copy_inline_row(const key_value_object &obj, vector<char> &data)
    {
        data.resize(obj.value.size());
        memcpy(data.data(), obj.value.data(), obj.value.size());
    }

    static fc::string scope_sym_to_string(scope_name sym_code)
    {
        fc::string scope = sym_code.to_string();
        if (scope.length() > 0 && scope[0] == '.')
        {
            uint64_t scope_int = sym_code;
            vector<char> v;
            for (int i = 0; i < 7; ++i)
            {
                char c = (char)(scope_int & 0xff);
                if (!c)
                    break;
                v.emplace_back(c);
                scope_int >>= 8;
            }
            return fc::string(v.begin(), v.end());
        }
        return scope;
    }

    chain_plugin *chain_plug = nullptr;
    fc::string socket_bind_str;
    zmq::context_t *context;
    zmq::socket_t *sender_socket;

    std::map<transaction_id_type, transaction_trace_ptr> cached_traces;
    std::map<name, std::set<name>> blacklist_actions;

    std::set<filter_entry> filter_on;
    std::set<filter_entry> filter_out;
    fc::microseconds abi_serializer_max_time;

    std::vector<fc::optional<connection>> connections;

  private:
    bool shorten_abi_errors = true;
};

} // namespace eosio

FC_REFLECT_ENUM(eosio::op_type_enum, (TABLE_REMOVE)(ROW_CREATE)(ROW_MODIFY)(ROW_REMOVE)(TRX_ACTION)(REV_UNDO)(REV_COMMIT))
FC_REFLECT(eosio::db_account, (name)(vm_type)(vm_version)(privileged)(last_code_update)(code_version)(creation_date))
FC_REFLECT(eosio::db_table, (op_type)(code)(scope)(table)(payer))
FC_REFLECT_DERIVED(eosio::db_op, (eosio::db_table), (id)(value))
FC_REFLECT(eosio::db_rev, (op_type)(revision))
FC_REFLECT(eosio::db_action, (block_num)(block_time)(action_trace)(last_irreversible_block))
