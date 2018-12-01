/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/statetrack_plugin/statetrack_plugin_impl.hpp>

namespace eosio
{

using namespace chain;
using namespace chainbase;

// statetrack plugin implementation

void statetrack_plugin_impl::send_zmq_msg(const fc::string content)
{
  if (sender_socket == nullptr)
    return;

  zmq::message_t message(content.length());
  memcpy(message.data(), content.c_str(), content.length());
  sender_socket->send(message);
}

bool statetrack_plugin_impl::filter(const account_name &code, const fc::string &scope, const table_name &table)
{
  if (filter_on.size() > 0)
  {
    bool pass_on = false;
    if (filter_on.find({code, 0, 0}) != filter_on.end())
    {
      pass_on = true;
    }
    if (filter_on.find({code, scope, 0}) != filter_on.end())
    {
      pass_on = true;
    }
    if (filter_on.find({code, scope, table}) != filter_on.end())
    {
      pass_on = true;
    }
    if (!pass_on)
    {
      return false;
    }
  }

  if (filter_out.size() > 0)
  {
    if (filter_out.find({code, 0, 0}) != filter_out.end())
    {
      return false;
    }
    if (filter_out.find({code, scope, 0}) != filter_out.end())
    {
      return false;
    }
    if (filter_out.find({code, scope, table}) != filter_out.end())
    {
      return false;
    }
  }

  return true;
}

void statetrack_plugin_impl::build_blacklist() {
  blacklist_actions.emplace(std::make_pair(config::system_account_name,
                                            std::set<name>{N(onblock)}));
  blacklist_actions.emplace(std::make_pair(N(blocktwitter),
                                              std::set<name>{N(tweet)}));
}

void statetrack_plugin_impl::init_current_block() {
  is_undo_state = false;
}

const table_id_object *statetrack_plugin_impl::get_kvo_tio(const database &db, const key_value_object &kvo)
{
  return db.find<table_id_object>(kvo.t_id);
}

const account_object &statetrack_plugin_impl::get_tio_co(const database &db, const table_id_object &tio)
{
  const account_object *co = db.find<account_object, by_name>(tio.code);
  EOS_ASSERT(co != nullptr, account_query_exception, "Fail to retrieve account for ${code}", ("code", tio.code));
  return *co;
}

const abi_def &statetrack_plugin_impl::get_co_abi(const account_object &co)
{
  abi_def *abi = new abi_def();
  abi_serializer::to_abi(co.abi, *abi);
  return *abi;
}

fc::variant statetrack_plugin_impl::get_kvo_row(const database &db, const table_id_object &tio, const key_value_object &kvo, bool json)
{
  vector<char> data;
  copy_inline_row(kvo, data);

  if (json)
  {
    const account_object &co = get_tio_co(db, tio);
    const abi_def &abi = get_co_abi(co);

    abi_serializer abis;
    abis.set_abi(abi, abi_serializer_max_time);

    return abis.binary_to_variant(abis.get_table_type(tio.table), data, abi_serializer_max_time, shorten_abi_errors);
  }
  else
  {
    return fc::variant(data);
  }
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const account_object &co, op_type_enum op_type)
{
  db_op op;

  name system = N(system);

  op.oid = co.id._id;
  op.op_type = op_type;
  op.actionid = current_action_index;

  op.code = system;
  op.scope = system.to_string();
  op.table = N(accounts);

  db_account account;
  account.name = co.name;
  account.vm_type = co.vm_type;
  account.vm_version = co.vm_version;
  account.privileged = co.privileged;
  account.last_code_update = co.last_code_update;
  account.code_version = co.code_version;
  account.creation_date = co.creation_date;

  op.value = fc::variant(account);

  return op;
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const permission_object &po, op_type_enum op_type)
{
  db_op op;

  name system = N(system);

  op.oid = po.id._id;
  op.op_type = op_type;
  op.actionid = current_action_index;

  op.code = system;
  op.scope = system.to_string();
  op.table = N(permissions);
  op.value = fc::variant(po);

  return op;
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const permission_link_object &plo, op_type_enum op_type)
{
  db_op op;

  name system = N(system);

  op.oid = plo.id._id;
  op.op_type = op_type;
  op.actionid = current_action_index;

  op.code = system;
  op.scope = system.to_string();
  op.table = N(permission_links);
  op.value = fc::variant(plo);

  return op;
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const table_id_object &tio, const key_value_object &kvo, op_type_enum op_type, bool json)
{
  db_op op;

  op.oid = kvo.id._id;
  op.id = kvo.primary_key;
  op.op_type = op_type;
  op.actionid = current_action_index;

  op.code = tio.code;
  op.scope = scope_sym_to_string(tio.scope);
  op.table = tio.table;

  if (op_type == op_type_enum::ROW_CREATE ||
      op_type == op_type_enum::ROW_MODIFY)
  {
    op.value = get_kvo_row(db, tio, kvo, json);
  }

  return op;
}

db_rev statetrack_plugin_impl::get_db_rev(const int64_t revision, op_type_enum op_type)
{
  db_rev rev = {op_type, revision};
  return rev;
}

void statetrack_plugin_impl::on_applied_table(const database &db, const table_id_object &tio, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  auto code = tio.code;
  auto scope = scope_sym_to_string(tio.scope);
  auto table = tio.table;

  if (filter(code, scope, table))
  {
    db_op op;

    op.oid = tio.id._id;
    op.op_type = op_type;
    op.actionid = current_action_index;

    op.code = tio.code;
    op.scope = scope_sym_to_string(tio.scope);
    op.table = tio.table;

    //TODO op optimizations

    current_action.ops.emplace_back(op);

    fc::string data = fc::json::to_string(op);
    //ilog("STATETRACK table_id_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const account_object &co, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  name system = N(system);
  auto code = system;
  auto scope = system.to_string();
  auto table = N(accounts);

  if (filter(code, scope, table))
  {
    auto op = get_db_op(db, co, op_type);

    //TODO op optimizations
    // switch(op_type) {
    //   case op_type_enum::ROW_REMOVE:
    //     if(co_newids.get<IndexByOId>.count(co.id._id)) {
    //       co_newids.get<IndexByOId>.erase(co.id._id);
    //       return;
    //     }
    //     else if(co_old_values.get<IndexByOId>.count(co.id._id)) {
    //       co_old_values.get<IndexByOId>.erase(co.id._id);
    //     }
    //     break;
    //   case op_type_enum::ROW_MODIFY:
    //     if(co_newids.get<IndexByOId>.count(co.id._id)) {
    //       op.op_type = op_type_enum::ROW_CREATE;
    //       co_newids.get<IndexByOId>.erase(co.id._id);
    //     } 
    //     else if(co_old_values.get<IndexByOId>.count(co.id._id)) {
    //       co_old_values.get<IndexByOId>.erase(co.id._id);
    //     }
    //     break;
    // }

    current_action.ops.emplace_back(op);

    fc::string data = fc::json::to_string(op);
    //ilog("STATETRACK account_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const permission_object &po, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  name system = N(system);
  auto code = system;
  auto scope = system.to_string();
  auto table = N(permission);

  if (filter(code, scope, table))
  {
    auto op = get_db_op(db, po, op_type);

    //TODO op optimizations

    current_action.ops.emplace_back(op);

    fc::string data = fc::json::to_string(op);
    //ilog("STATETRACK permission_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const permission_link_object &plo, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  name system = N(system);
  auto code = system;
  auto scope = system.to_string();
  auto table = N(permission_links);

  if (filter(code, scope, table))
  {
    auto op = get_db_op(db, plo, op_type);

    //TODO op optimizations

    current_action.ops.emplace_back(op);

    fc::string data = fc::json::to_string(op);
    //ilog("STATETRACK permission_link_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const key_value_object &kvo, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  const table_id_object *tio_ptr = get_kvo_tio(db, kvo);

  if (tio_ptr == nullptr)
    return;

  const table_id_object &tio = *tio_ptr;

  auto code = tio.code;
  auto scope = scope_sym_to_string(tio.scope);
  auto table = tio.table;

  if (filter(code, scope, table))
  {
    auto op = get_db_op(db, *tio_ptr, kvo, op_type);

    //TODO op optimizations

    current_action.ops.emplace_back(op);

    fc::string data = fc::json::to_string(op);
    //ilog("STATETRACK key_value_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
  }
}

void statetrack_plugin_impl::on_applied_rev(const int64_t revision, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  fc::string data = fc::json::to_string(get_db_rev(revision, op_type));
  ilog("STATETRACK ${op_type}: ${data}", ("op_type", op_type)("data", data));
  //send_zmq_msg(data);
}

void statetrack_plugin_impl::on_applied_undo(const int64_t revision)
{
  if (sender_socket == nullptr)
    return;

  is_undo_state = true;

  fc::string data = fc::json::to_string(get_db_rev(revision, op_type_enum::REV_UNDO));
  ilog("STATETRACK ${op_type}: ${data}", ("op_type", op_type_enum::REV_UNDO)("data", data));
  //send_zmq_msg(data);
}

void statetrack_plugin_impl::on_applied_transaction(const transaction_trace_ptr &ttp)
{
  if (ttp->receipt) {

    db_transaction trx;

    trx.trx_id = ttp->id;

    auto trx_action_range = reversible_actions.get<IndexByTrxId>().equal_range(trx.trx_id);
    for (auto& action : boost::make_iterator_range(trx_action_range)) {
      
      trx.actions.emplace_back(action);
      auto newact = trx.actions.back();

      auto action_op_range = reversible_ops.get<IndexByActionId>().equal_range(action.actionid);
      for (auto& op : boost::make_iterator_range(action_op_range)) {
        newact.ops.emplace_back(op);
      }
    }

    //TODO send zmq transaction
    //send_zmq_msg(data);

    ilog("STATETRACK applied transaction: ${trx}", ("trx", trx));
  }
  else {
    auto trx_action_range = reversible_actions.get<IndexByTrxId>().equal_range(ttp->id);
    for (auto& action : boost::make_iterator_range(trx_action_range)) {
      auto action_op_range = reversible_ops.get<IndexByActionId>().equal_range(action.actionid);
      reversible_ops.get<IndexByActionId>().erase(action_op_range.first, action_op_range.second);
    }
    reversible_actions.get<IndexByTrxId>().erase(trx_action_range.first, trx_action_range.second);
  }
}

void statetrack_plugin_impl::on_accepted_block(const block_state_ptr &bsp)
{

}

void statetrack_plugin_impl::on_irreversible_block(const block_state_ptr &bsp)
{
  if (sender_socket == nullptr)
    return;

  fc::string data = fc::json::to_string(get_db_rev(bsp->block_num, op_type_enum::REV_COMMIT));
  ilog("STATETRACK irreversible block: ${data}", ("data", data));

  //send_zmq_msg(data);
  
  auto block_action_range = reversible_actions.get<IndexByBlockNum>().equal_range(bsp->block_num);
  for (auto& action : boost::make_iterator_range(block_action_range)) {
    auto action_op_range = reversible_ops.get<IndexByActionId>().equal_range(action.actionid);
    reversible_ops.get<IndexByActionId>().erase(action_op_range.first, action_op_range.second);
  }
  reversible_actions.get<IndexByBlockNum>().erase(block_action_range.first, block_action_range.second);
}

void statetrack_plugin_impl::on_applied_action(action_trace& trace)
{
  current_action.trx_id = trace.trx_id;
  current_action.block_num = trace.block_num;
  current_action.trace = chain_plug->chain().to_variant_with_abi(trace, abi_serializer_max_time);
  current_action_index++;
  reversible_actions.emplace_back(current_action);
  current_action.ops.clear();

  //ilog("on_applied_action: ${da}", ("da", *current_action));
}

} // namespace eosio