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

#include <boost/multi_index/random_access_index.hpp>

namespace eosio
{

using namespace chain;
using namespace chainbase;
using namespace bmi;
using boost::signals2::connection;
using bmi::random_access;

typedef typename get_index_type<account_object>::type co_index_type;
typedef typename get_index_type<permission_object>::type po_index_type;
typedef typename get_index_type<permission_link_object>::type plo_index_type;
typedef typename get_index_type<table_id_object>::type ti_index_type;
typedef typename get_index_type<key_value_object>::type kv_index_type;

enum op_type_enum
{
    TABLE_REMOVE = 0,
    ROW_CREATE = 1,
    ROW_MODIFY = 2,
    ROW_REMOVE = 3
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

struct db_op
{
    int64_t        oid;
    uint64_t       id;
    block_num_type block_num;
    uint64_t       actionid;

    op_type_enum   op_type;
    account_name   code;
    fc::string     scope;
    table_name     table;
    account_name   payer;

    fc::variant    value;
};

struct db_action
{
    uint64_t            actionid;
    transaction_id_type trx_id;
    block_num_type      block_num;
    fc::variant         trace;
    std::vector<db_op>  ops;
};

struct db_transaction {
    transaction_id_type    trx_id;
    std::vector<db_action> actions;
};

struct db_block {
    block_num_type      block_num;
    block_id_type       block_id;
    bool                irreversible;
};

struct db_undo_block : db_block {
    std::vector<db_op>  ops;
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

struct IndexByOId {};
struct IndexByActionId {};
struct IndexByTrxId {};
struct IndexByBlockNum {};

typedef multi_index_container<
    db_op,         
    indexed_by<
        random_access<>,
        ordered_unique<boost::multi_index::tag<IndexByOId>, member<db_op, int64_t, 
                             &db_op::oid> >,
        ordered_non_unique<boost::multi_index::tag<IndexByActionId>, member<db_op, uint64_t, 
                             &db_op::actionid> >,
        ordered_non_unique<boost::multi_index::tag<IndexByBlockNum>, member<db_op, block_num_type, 
                           &db_op::block_num> >
    >
> db_op_container;

typedef multi_index_container<
    db_action,         
    indexed_by<
        random_access<>,
        ordered_non_unique<boost::multi_index::tag<IndexByTrxId>, member<db_action, transaction_id_type, 
                           &db_action::trx_id> >,
        ordered_non_unique<boost::multi_index::tag<IndexByBlockNum>, member<db_action, block_num_type, 
                           &db_action::block_num> >
    >
> db_action_container;

class statetrack_plugin_impl
{
  public:
    void send_zmq_msg(const fc::string content);
    bool filter(const account_name &code, const fc::string &scope, const table_name &table);

    void build_blacklist();

    void init_current_block();

    const table_id_object *get_kvo_tio(const database &db, const key_value_object &kvo);
    const account_object &get_tio_co(const database &db, const table_id_object &tio);
    const abi_def &get_co_abi(const account_object &co);
    fc::variant get_kvo_row(const database &db, const table_id_object &tio, const key_value_object &kvo, bool json = true);

    db_op get_db_op(const database &db, const account_object &co, op_type_enum op_type);
    db_op get_db_op(const database &db, const permission_object &po, op_type_enum op_type);
    db_op get_db_op(const database &db, const permission_link_object &plo, op_type_enum op_type);
    db_op get_db_op(const database &db, const table_id_object &tio, const key_value_object &kvo, op_type_enum op_type, bool json = true);

    //generic_index state tables
    void on_applied_table(const database &db, const table_id_object &tio, op_type_enum op_type);
    //generic_index op
    void on_applied_op(const database &db, const account_object &co, op_type_enum op_type);
    void on_applied_op(const database &db, const permission_object &po, op_type_enum op_type);
    void on_applied_op(const database &db, const permission_link_object &plo, op_type_enum op_type);
    void on_applied_op(const database &db, const key_value_object &kvo, op_type_enum op_type);
    //generic_index undo
    void on_applied_undo(const int64_t revision);
    //blocks and transactions
    void on_applied_transaction(const transaction_trace_ptr &ttp);
    void on_accepted_block(const block_state_ptr &bsp);
    void on_irreversible_block(const block_state_ptr &bsp);
    void on_applied_action(action_trace& trace);
    void on_pre_undo_block(const block_state_ptr& bsp);
    void on_post_undo_block(const block_state_ptr& bsp);

    template<typename MultiIndexType> 
    void create_index_events(const database &db) {
        auto &index = db.get_index<MultiIndexType>();

        connections.emplace_back(
        fc::optional<connection>(index.applied_emplace.connect([&](const typename MultiIndexType::value_type &v) {
            on_applied_op(db, v, op_type_enum::ROW_CREATE);
        })));

        connections.emplace_back(
            fc::optional<connection>(index.applied_modify.connect([&](const typename MultiIndexType::value_type &v) {
                on_applied_op(db, v, op_type_enum::ROW_MODIFY);
            })));

        connections.emplace_back(
            fc::optional<connection>(index.applied_remove.connect([&](const typename MultiIndexType::value_type &v) {
                on_applied_op(db, v, op_type_enum::ROW_REMOVE);
            })));

        connections.emplace_back(
            fc::optional<connection>(index.applied_undo.connect([&](const int64_t revision) {
                on_applied_undo(revision);
            })));
    }

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

    bool is_undo_state;

    uint64_t  current_action_index = 0;
    db_action current_action;

    block_num_type current_undo_block_num = 0;
    db_undo_block current_undo_block;

    db_action_container reversible_actions;
    db_op_container     reversible_ops;

    std::map<name, std::set<name>> blacklist_actions;

    std::set<filter_entry> filter_on;
    std::set<filter_entry> filter_out;
    fc::microseconds abi_serializer_max_time;

    std::vector<fc::optional<connection>> connections;

  private:
    bool shorten_abi_errors = true;
};

} // namespace eosio

FC_REFLECT_ENUM(eosio::op_type_enum, (TABLE_REMOVE)(ROW_CREATE)(ROW_MODIFY)(ROW_REMOVE))
FC_REFLECT(eosio::db_account, (name)(vm_type)(vm_version)(privileged)(last_code_update)(code_version)(creation_date))
FC_REFLECT(eosio::db_op, (oid)(id)(op_type)(code)(scope)(table)(payer)(block_num)(actionid)(value))
FC_REFLECT(eosio::db_action, (trx_id)(trace)(ops))
FC_REFLECT(eosio::db_transaction, (trx_id)(actions))
FC_REFLECT(eosio::db_block, (block_num)(block_id)(irreversible))
FC_REFLECT_DERIVED(eosio::db_undo_block, (eosio::db_block), (ops))
